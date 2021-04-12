// Copyright 2019 GurumNetworks, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <utility>
#include <string>
#include <limits>
#include <thread>
#include <chrono>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "rcutils/error_handling.h"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/guid.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"

#include "rmw_gurumdds_cpp/types.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"

#include "./type_support_common.hpp"

extern "C"
{
rmw_ret_t
rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_subscription_allocation_t * allocation)
{
  (void)type_support;
  (void)message_bounds;
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_init_subscription_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_fini_subscription_allocation(rmw_subscription_allocation_t * allocation)
{
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_fini_subscription_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }

  if (node->implementation_identifier != gurum_gurumdds_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return nullptr;
  }

  if (topic_name == nullptr || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("subscription topic is null or empty string");
    return nullptr;
  }

  if (qos_policies == nullptr) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return nullptr;
  }

  if (subscription_options == nullptr) {
    RMW_SET_ERROR_MSG("subscription_options is null");
    return nullptr;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info is null");
    return nullptr;
  }

  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return nullptr;
  }

  const rosidl_message_type_support_t * type_support =
    get_message_typesupport_handle(type_supports, rosidl_typesupport_introspection_c__identifier);
  if (type_support == nullptr) {
    rcutils_reset_error();
    type_support = get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (type_support == nullptr) {
      rcutils_reset_error();
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  rmw_subscription_t * subscription = nullptr;
  GurumddsSubscriberInfo * subscriber_info = nullptr;
  dds_Subscriber * dds_subscriber = nullptr;
  dds_SubscriberQos subscriber_qos;
  dds_DataReader * topic_reader = nullptr;
  dds_DataReaderQos datareader_qos;
  dds_DataReaderListener datareader_listener = {};
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
  dds_GuardCondition * queue_guard_condition = nullptr;
  dds_TypeSupport * dds_typesupport = nullptr;
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  rmw_ret_t rmw_ret = RMW_RET_OK;

  std::string type_name =
    create_type_name(type_support->data, type_support->typesupport_identifier);
  if (type_name.empty()) {
    // Error message is already set
    return nullptr;
  }

  std::string processed_topic_name;
  if (!qos_policies->avoid_ros_namespace_conventions) {
    processed_topic_name = std::string(ros_topic_prefix) + topic_name;
  } else {
    processed_topic_name = topic_name;
  }

  std::string metastring =
    create_metastring(type_support->data, type_support->typesupport_identifier);
  if (metastring.empty()) {
    // Error message is already set
    return nullptr;
  }

  dds_typesupport = dds_TypeSupport_create(metastring.c_str());
  if (dds_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("failed to create typesupport");
    goto fail;
  }

  ret = dds_TypeSupport_register_type(dds_typesupport, participant, type_name.c_str());
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register type to domain participant");
    goto fail;
  }

  ret = dds_DomainParticipant_get_default_subscriber_qos(participant, &subscriber_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default subscriber qos");
    goto fail;
  }

  dds_subscriber =
    dds_DomainParticipant_create_subscriber(participant, &subscriber_qos, nullptr, 0);
  if (dds_subscriber == nullptr) {
    RMW_SET_ERROR_MSG("failed to create subscriber");
    dds_SubscriberQos_finalize(&subscriber_qos);
    goto fail;
  }

  ret = dds_SubscriberQos_finalize(&subscriber_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize subscriber qos");
    goto fail;
  }

  // Look for message topic
  topic_desc =
    dds_DomainParticipant_lookup_topicdescription(participant, processed_topic_name.c_str());
  if (topic_desc == nullptr) {
    dds_TopicQos topic_qos;
    ret = dds_DomainParticipant_get_default_topic_qos(participant, &topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      goto fail;
    }

    topic = dds_DomainParticipant_create_topic(
      participant, processed_topic_name.c_str(), type_name.c_str(), &topic_qos, nullptr, 0);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      goto fail;
    }

    ret = dds_TopicQos_finalize(&topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to finalize topic qos");
      goto fail;
    }
  } else {
    dds_Duration_t timeout;
    timeout.sec = 0;
    timeout.nanosec = 1;
    topic = dds_DomainParticipant_find_topic(participant, processed_topic_name.c_str(), &timeout);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      goto fail;
    }
  }

  if (!get_datareader_qos(dds_subscriber, qos_policies, &datareader_qos)) {
    // Error message already set
    goto fail;
  }

  datareader_listener.on_data_available = reader_on_data_available<GurumddsSubscriberInfo>;

  topic_reader = dds_Subscriber_create_datareader(
    dds_subscriber, topic, &datareader_qos, &datareader_listener,
    dds_DATA_AVAILABLE_STATUS);
  if (topic_reader == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datareader");
    goto fail;
  }

  ret = dds_DataReaderQos_finalize(&datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datareader qos");
    goto fail;
  }

  queue_guard_condition = dds_GuardCondition_create();
  if (queue_guard_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create guard condition");
    goto fail;
  }

  subscriber_info = new(std::nothrow) GurumddsSubscriberInfo();
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate subscriber info handle");
    goto fail;
  }

  subscriber_info->implementation_identifier = gurum_gurumdds_identifier;
  subscriber_info->subscriber = dds_subscriber;
  subscriber_info->topic_reader = topic_reader;
  subscriber_info->queue_guard_condition = queue_guard_condition;
  subscriber_info->dds_typesupport = dds_typesupport;
  subscriber_info->rosidl_message_typesupport = type_support;

  dds_DataReader_set_listener_context(subscriber_info->topic_reader, subscriber_info);

  subscription = rmw_subscription_allocate();
  if (subscription == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    goto fail;
  }

  subscription->implementation_identifier = gurum_gurumdds_identifier;
  subscription->data = subscriber_info;
  subscription->topic_name = reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (subscription->topic_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for topic name");
    goto fail;
  }
  memcpy(const_cast<char *>(subscription->topic_name), topic_name, strlen(topic_name) + 1);
  subscription->options = *subscription_options;
  subscription->can_loan_messages = false;

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);
  if (rmw_ret != RMW_RET_OK) {
    // Error message already set
    goto fail;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_cpp",
    "Created subscription with topic '%s' on node '%s%s%s'",
    topic_name, node->namespace_,
    node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

  return subscription;

fail:
  if (subscription != nullptr) {
    if (subscription->topic_name != nullptr) {
      rmw_free(const_cast<char *>(subscription->topic_name));
    }
    rmw_subscription_free(subscription);
  }

  if (dds_subscriber != nullptr) {
    if (topic_reader != nullptr) {
      if (queue_guard_condition != nullptr) {
        dds_GuardCondition_delete(queue_guard_condition);
      }
      dds_Subscriber_delete_datareader(dds_subscriber, topic_reader);
    }
    dds_DomainParticipant_delete_subscriber(participant, dds_subscriber);
  }

  if (dds_typesupport != nullptr) {
    dds_TypeSupport_delete(dds_typesupport);
  }

  if (subscriber_info != nullptr) {
    delete subscriber_info;
  }

  return nullptr;
}

rmw_ret_t
rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  if (subscription == nullptr) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (publisher_count == nullptr) {
    RMW_SET_ERROR_MSG("publisher_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("subscriber internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_Subscriber * dds_subscriber = info->subscriber;
  if (dds_subscriber == nullptr) {
    RMW_SET_ERROR_MSG("dds subscriber is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader * topic_reader = info->topic_reader;
  if (topic_reader == nullptr) {
    RMW_SET_ERROR_MSG("topic reader is null");
    return RMW_RET_ERROR;
  }

  dds_InstanceHandleSeq * seq = dds_InstanceHandleSeq_create(4);
  if (dds_DataReader_get_matched_publications(topic_reader, seq) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get matched publications");
    dds_InstanceHandleSeq_delete(seq);
    return RMW_RET_ERROR;
  }

  *publisher_count = (size_t)dds_InstanceHandleSeq_length(seq);

  dds_InstanceHandleSeq_delete(seq);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("subscription internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataReader * data_reader = info->topic_reader;
  if (data_reader == nullptr) {
    RMW_SET_ERROR_MSG("subscription internal data writer is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataReaderQos dds_qos;
  dds_ReturnCode_t ret = dds_DataReader_get_qos(data_reader, &dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("subscription can't get data reader qos policies");
    return RMW_RET_ERROR;
  }

  switch (dds_qos.history.kind) {
    case dds_KEEP_LAST_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
      break;
    case dds_KEEP_ALL_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
      break;
    default:
      qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
      break;
  }

  switch (dds_qos.durability.kind) {
    case dds_TRANSIENT_LOCAL_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
      break;
    case dds_VOLATILE_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
      break;
    default:
      qos->durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
      break;
  }

  switch (dds_qos.reliability.kind) {
    case dds_BEST_EFFORT_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
      break;
    case dds_RELIABLE_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
      break;
    default:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
      break;
  }

  qos->depth = static_cast<size_t>(dds_qos.history.depth);

  if (dds_qos.deadline.period.sec == dds_DURATION_INFINITE_SEC) {
    qos->deadline.sec = std::numeric_limits<uint64_t>::max();
    qos->deadline.nsec = std::numeric_limits<uint64_t>::max();
  } else {
    qos->deadline.sec = static_cast<uint64_t>(dds_qos.deadline.period.sec);
    qos->deadline.nsec = static_cast<uint64_t>(dds_qos.deadline.period.nanosec);
  }

  switch (dds_qos.liveliness.kind) {
    case dds_AUTOMATIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
      break;
    case dds_MANUAL_BY_TOPIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
      break;
    default:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
      break;
  }

  if (dds_qos.liveliness.lease_duration.sec == dds_DURATION_INFINITE_SEC) {
    qos->liveliness_lease_duration.sec = std::numeric_limits<uint64_t>::max();
    qos->liveliness_lease_duration.nsec = std::numeric_limits<uint64_t>::max();
  } else {
    qos->liveliness_lease_duration.sec =
      static_cast<uint64_t>(dds_qos.liveliness.lease_duration.sec);
    qos->liveliness_lease_duration.nsec =
      static_cast<uint64_t>(dds_qos.liveliness.lease_duration.nanosec);
  }


  return RMW_RET_OK;
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_ERROR
  )

  if (subscription == nullptr) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle, subscription->implementation_identifier,
    gurum_gurumdds_identifier, return RMW_RET_ERROR)

  auto node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  auto participant = static_cast<dds_DomainParticipant *>(node_info->participant);
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return RMW_RET_ERROR;
  }

  rmw_ret_t rmw_ret = RMW_RET_OK;
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  GurumddsSubscriberInfo * subscriber_info =
    static_cast<GurumddsSubscriberInfo *>(subscription->data);
  if (subscriber_info != nullptr) {
    dds_Subscriber * dds_subscriber = subscriber_info->subscriber;
    if (dds_subscriber != nullptr) {
      dds_DataReader * topic_reader = subscriber_info->topic_reader;
      if (topic_reader != nullptr) {
        ret = dds_Subscriber_delete_datareader(dds_subscriber, topic_reader);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete datareader");
          rmw_ret = RMW_RET_ERROR;
        }
        subscriber_info->topic_reader = nullptr;
      }

      ret = dds_DomainParticipant_delete_subscriber(participant, dds_subscriber);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete subscriber");
        rmw_ret = RMW_RET_ERROR;
      }
    } else if (subscriber_info->topic_reader != nullptr) {
      RMW_SET_ERROR_MSG("cannot delte datareader because the subscriber is null");
      rmw_ret = RMW_RET_ERROR;
    }

    if (subscriber_info->queue_guard_condition != nullptr) {
      dds_GuardCondition_delete(subscriber_info->queue_guard_condition);
      subscriber_info->queue_guard_condition = nullptr;
    }

    while (!subscriber_info->message_queue.empty()) {
      auto msg = subscriber_info->message_queue.front();
      if (msg.sample != nullptr) {
        dds_free(msg.sample);
      }
      if (msg.info != nullptr) {
        dds_free(msg.info);
      }
      subscriber_info->message_queue.pop();
    }

    if (subscriber_info->dds_typesupport != nullptr) {
      dds_TypeSupport_delete(subscriber_info->dds_typesupport);
      subscriber_info->dds_typesupport = nullptr;
    }

    delete subscriber_info;
    subscription->data = nullptr;
    if (subscription->topic_name != nullptr) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_cpp",
        "Deleted subscription with topic '%s' on node '%s%s%s'",
        subscription->topic_name, node->namespace_,
        node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

      rmw_free(const_cast<char *>(subscription->topic_name));
    }
  }

  rmw_subscription_free(subscription);

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);

  return rmw_ret;
}

static rmw_ret_t
_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void)allocation;
  *taken = false;

  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("subscription handle not from this implementation");
    return RMW_RET_ERROR;
  }

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  if (info->message_queue.empty()) {
    return RMW_RET_OK;
  }

  info->queue_mutex.lock();
  auto msg = info->message_queue.front();
  info->message_queue.pop();
  if (info->message_queue.empty()) {
    dds_GuardCondition_set_trigger_value(info->queue_guard_condition, false);
  }
  info->queue_mutex.unlock();

  bool ignore_sample = false;

  if (!msg.info->valid_data) {
    ignore_sample = true;
  }

  if (!ignore_sample) {
    if (msg.sample == nullptr) {
      RMW_SET_ERROR_MSG("Received invalid message");
      dds_free(msg.info);
      return RMW_RET_ERROR;
    }
    bool result = deserialize_cdr_to_ros(
      info->rosidl_message_typesupport->data,
      info->rosidl_message_typesupport->typesupport_identifier,
      ros_message,
      msg.sample,
      static_cast<size_t>(msg.size)
    );
    if (!result) {
      RMW_SET_ERROR_MSG("Failed to deserialize message");
      dds_free(msg.sample);
      dds_free(msg.info);
      return RMW_RET_ERROR;
    }

    *taken = true;

    if (message_info != nullptr) {
      message_info->source_timestamp =
        msg.info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        msg.info->source_timestamp.nanosec;
      // TODO(clemjh): SampleInfo doesn't contain received_timestamp
      message_info->received_timestamp = 0;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader, msg.info->publication_handle, custom_gid->publication_handle);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED("rmw_gurumdds_cpp", "Failed to get publication handle");
        }
        memset(custom_gid->publication_handle, 0, sizeof(custom_gid->publication_handle));
      }
    }
  }

  if (msg.sample != nullptr) {
    dds_free(msg.sample);
  }
  if (msg.info != nullptr) {
    dds_free(msg.info);
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take(
    gurum_gurumdds_identifier, subscription, ros_message, taken, nullptr, allocation);
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_ERROR);

  return _take(
    gurum_gurumdds_identifier, subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t
rmw_take_sequence(
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  (void)allocation;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription handle is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_sequence, "message sequence is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info_sequence, "message info sequence is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "taken handle is null", return RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  if (message_sequence->capacity < count) {
    RMW_SET_ERROR_MSG("message sequence capacity is not sufficient");
    return RMW_RET_ERROR;
  }

  if (message_info_sequence->capacity < count) {
    RMW_SET_ERROR_MSG("message info sequence capacity is not sufficient");
    return RMW_RET_ERROR;
  }

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  if (info->message_queue.empty()) {
    return RMW_RET_OK;
  }

  info->queue_mutex.lock();
  while (!info->message_queue.empty()) {
    auto msg = info->message_queue.front();
    info->message_queue.pop();
    if (info->message_queue.empty()) {
      dds_GuardCondition_set_trigger_value(info->queue_guard_condition, false);
    }
    bool ignore_sample = false;

    if (!msg.info->valid_data) {
      ignore_sample = true;
    }

    if (!ignore_sample) {
      if (msg.sample == nullptr) {
        RMW_SET_ERROR_MSG("Received invalid message");
        dds_free(msg.info);
        return RMW_RET_ERROR;
      }
      bool result = deserialize_cdr_to_ros(
        info->rosidl_message_typesupport->data,
        info->rosidl_message_typesupport->typesupport_identifier,
        message_sequence->data[*taken],
        msg.sample,
        static_cast<size_t>(msg.size)
      );
      if (!result) {
        RMW_SET_ERROR_MSG("Failed to deserialize message");
        dds_free(msg.sample);
        dds_free(msg.info);
        return RMW_RET_ERROR;
      }

      auto message_info = &(message_info_sequence->data[*taken]);

      message_info->source_timestamp =
        msg.info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        msg.info->source_timestamp.nanosec;
      // TODO(clemjh): SampleInfo doesn't contain received_timestamp
      message_info->received_timestamp = 0;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = gurum_gurumdds_identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader, msg.info->publication_handle, custom_gid->publication_handle);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED("rmw_gurumdds_cpp", "Failed to get publication handle");
        }
        memset(custom_gid->publication_handle, 0, sizeof(custom_gid->publication_handle));
      }

      (*taken)++;
    }

    if (msg.sample != nullptr) {
      dds_free(msg.sample);
    }
    if (msg.info != nullptr) {
      dds_free(msg.info);
    }
  }
  info->queue_mutex.unlock();

  // =============================================================================================

  message_sequence->size = *taken;
  message_info_sequence->size = *taken;

  return RMW_RET_OK;
}

static rmw_ret_t
_take_serialized(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void)allocation;
  *taken = false;

  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("subscription handle not from this implementation");
    return RMW_RET_ERROR;
  }

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  if (info->message_queue.empty()) {
    return RMW_RET_OK;
  }

  info->queue_mutex.lock();
  auto msg = info->message_queue.front();
  info->message_queue.pop();
  if (info->message_queue.empty()) {
    dds_GuardCondition_set_trigger_value(info->queue_guard_condition, false);
  }
  info->queue_mutex.unlock();

  bool ignore_sample = false;

  if (!msg.info->valid_data) {
    ignore_sample = true;
  }

  if (!ignore_sample) {
    if (msg.sample == nullptr) {
      RMW_SET_ERROR_MSG("Received invalid message");
      dds_free(msg.info);
      return RMW_RET_ERROR;
    }

    serialized_message->buffer_length = msg.size;
    if (serialized_message->buffer_capacity < msg.size) {
      rmw_ret_t rmw_ret = rmw_serialized_message_resize(serialized_message, msg.size);
      if (rmw_ret != RMW_RET_OK) {
        // Error message already set
        dds_free(msg.sample);
        dds_free(msg.info);
        return rmw_ret;
      }
    }

    memcpy(serialized_message->buffer, msg.sample, msg.size);

    *taken = true;

    if (message_info != nullptr) {
      message_info->source_timestamp =
        msg.info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        msg.info->source_timestamp.nanosec;
      // TODO(clemjh): SampleInfo doesn't contain received_timestamp
      message_info->received_timestamp = 0;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader, msg.info->publication_handle, custom_gid->publication_handle);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED("rmw_gurumdds_cpp", "Failed to get publication handle");
        }
        memset(custom_gid->publication_handle, 0, sizeof(custom_gid->publication_handle));
      }
    }
  }

  if (msg.sample != nullptr) {
    dds_free(msg.sample);
  }
  if (msg.info != nullptr) {
    dds_free(msg.info);
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take_serialized(
    gurum_gurumdds_identifier, subscription,
    serialized_message, taken, nullptr, allocation);
}

rmw_ret_t
rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_ERROR);

  return _take_serialized(
    gurum_gurumdds_identifier, subscription,
    serialized_message, taken, message_info, allocation);
}

rmw_ret_t
rmw_take_loaned_message(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  (void)subscription;
  (void)loaned_message;
  (void)taken;
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_take_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_take_loaned_message_with_info(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void)subscription;
  (void)loaned_message;
  (void)taken;
  (void)message_info;
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_take_loaned_message_with_info is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_return_loaned_message_from_subscription(
  const rmw_subscription_t * subscription,
  void * loaned_message)
{
  (void)subscription;
  (void)loaned_message;

  RMW_SET_ERROR_MSG("rmw_return_loaned_message_from_subscription is not supported");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

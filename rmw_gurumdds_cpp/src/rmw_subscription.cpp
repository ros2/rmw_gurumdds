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

  if (subscription_options->require_unique_network_flow_endpoints ==
    RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED)
  {
    RMW_SET_ERROR_MSG(
      "Strict requirement on unique network flow endpoints for subscriptions not supported");
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
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
  dds_ReadCondition * read_condition = nullptr;
  dds_TypeSupport * dds_typesupport = nullptr;
  dds_ReturnCode_t ret;
  rmw_ret_t rmw_ret;

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
      dds_TopicQos_finalize(&topic_qos);
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

  topic_reader =
    dds_Subscriber_create_datareader(dds_subscriber, topic, &datareader_qos, nullptr, 0);
  if (topic_reader == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datareader");
    dds_DataReaderQos_finalize(&datareader_qos);
    goto fail;
  }

  ret = dds_DataReaderQos_finalize(&datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datareader qos");
    goto fail;
  }

  read_condition = dds_DataReader_create_readcondition(
    topic_reader, dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);
  if (read_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create read condition");
    goto fail;
  }

  node_info->sub_list.push_back(dds_subscriber);

  subscriber_info = new(std::nothrow) GurumddsSubscriberInfo();
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate subscriber info handle");
    goto fail;
  }

  subscriber_info->implementation_identifier = gurum_gurumdds_identifier;
  subscriber_info->subscriber = dds_subscriber;
  subscriber_info->topic_reader = topic_reader;
  subscriber_info->read_condition = read_condition;
  subscriber_info->rosidl_message_typesupport = type_support;

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

  dds_TypeSupport_delete(dds_typesupport);
  dds_typesupport = nullptr;

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

  if (topic != nullptr) {
    dds_DomainParticipant_delete_topic(participant, topic);
  }

  if (dds_subscriber != nullptr) {
    if (topic_reader != nullptr) {
      if (read_condition != nullptr) {
        dds_DataReader_delete_readcondition(topic_reader, read_condition);
      }
      dds_Subscriber_delete_datareader(dds_subscriber, topic_reader);
    }
    node_info->sub_list.remove(dds_subscriber);
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
    RMW_SET_ERROR_MSG("subscription internal data reader is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataReaderQos dds_qos;
  dds_ReturnCode_t ret = dds_DataReader_get_qos(data_reader, &dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("subscription can't get data reader qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability = convert_reliability(dds_qos.reliability);
  qos->durability = convert_durability(dds_qos.durability);
  qos->deadline = convert_deadline(dds_qos.deadline);
  qos->liveliness = convert_liveliness(dds_qos.liveliness);
  qos->liveliness_lease_duration = convert_liveliness_lease_duration(dds_qos.liveliness);
  qos->history = convert_history(dds_qos.history);
  qos->depth = static_cast<size_t>(dds_qos.history.depth);

  ret = dds_DataReaderQos_finalize(&dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datareader qos");
    return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION
  );

  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

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

  dds_ReturnCode_t ret;
  GurumddsSubscriberInfo * subscriber_info =
    static_cast<GurumddsSubscriberInfo *>(subscription->data);
  if (subscriber_info != nullptr) {
    dds_Subscriber * dds_subscriber = subscriber_info->subscriber;
    if (dds_subscriber != nullptr) {
      dds_DataReader * topic_reader = subscriber_info->topic_reader;
      if (topic_reader != nullptr) {
        dds_ReadCondition * read_condition = subscriber_info->read_condition;
        if (read_condition != nullptr) {
          ret = dds_DataReader_delete_readcondition(topic_reader, read_condition);
          if (ret != dds_RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to delete readcondition");
            return RMW_RET_ERROR;
          }
          subscriber_info->read_condition = nullptr;
        }

        ret = dds_Subscriber_delete_datareader(dds_subscriber, topic_reader);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete datareader");
          return RMW_RET_ERROR;
        }
        subscriber_info->topic_reader = nullptr;
      } else if (subscriber_info->read_condition != nullptr) {
        RMW_SET_ERROR_MSG("cannot delete readcondition because the datareader is null");
        return RMW_RET_ERROR;
      }

      node_info->sub_list.remove(dds_subscriber);
      ret = dds_DomainParticipant_delete_subscriber(participant, dds_subscriber);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete subscriber");
        return RMW_RET_ERROR;
      }
    } else if (subscriber_info->topic_reader != nullptr) {
      RMW_SET_ERROR_MSG("cannot delte datareader because the subscriber is null");
      return RMW_RET_ERROR;
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

  rmw_ret_t rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);

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

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  dds_DataSeq * data_values = dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoSeq * sample_infos = dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    dds_DataSeq_delete(data_values);
    return RMW_RET_ERROR;
  }

  dds_UnsignedLongSeq * sample_sizes = dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DataReader_raw_take(
    topic_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, 1,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  const char * topic_name =
    dds_TopicDescription_get_name(dds_DataReader_get_topicdescription(topic_reader));

  if (ret == dds_RETCODE_NO_DATA) {
    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_gurumdds_cpp", "No data on topic %s", topic_name);
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_cpp", "Received data on topic %s", topic_name);

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  if (sample_info->valid_data) {
    void * sample = dds_DataSeq_get(data_values, 0);
    if (sample == nullptr) {
      RMW_SET_ERROR_MSG("failed to get message");
      dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }
    uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);
    bool result = deserialize_cdr_to_ros(
      info->rosidl_message_typesupport->data,
      info->rosidl_message_typesupport->typesupport_identifier,
      ros_message,
      sample,
      static_cast<size_t>(sample_size)
    );
    if (!result) {
      RMW_SET_ERROR_MSG("failed to deserialize message");
      dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }

    *taken = true;

    if (message_info != nullptr) {
      message_info->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;
      // TODO(clemjh): SampleInfo doesn't contain received_timestamp
      message_info->received_timestamp = 0;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader, sample_info->publication_handle, custom_gid->publication_handle);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED("rmw_gurumdds_cpp", "Failed to get publication handle");
        }
        memset(custom_gid->publication_handle, 0, sizeof(custom_gid->publication_handle));
      }
    }
  }

  dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
  dds_DataSeq_delete(data_values);
  dds_SampleInfoSeq_delete(sample_infos);
  dds_UnsignedLongSeq_delete(sample_sizes);

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
    subscription, "subscription pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_INVALID_ARGUMENT);

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
    subscription, "subscription pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_INVALID_ARGUMENT);

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
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  if (0u == count) {
    RMW_SET_ERROR_MSG("count cannot be 0");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (message_sequence->capacity < count) {
    RMW_SET_ERROR_MSG("message sequence capacity is not sufficient");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (message_info_sequence->capacity < count) {
    RMW_SET_ERROR_MSG("message info sequence capacity is not sufficient");
    return RMW_RET_INVALID_ARGUMENT;
  }

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  dds_DataSeq * data_values = dds_DataSeq_create(count);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoSeq * sample_infos = dds_SampleInfoSeq_create(count);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    dds_DataSeq_delete(data_values);
    return RMW_RET_ERROR;
  }

  dds_UnsignedLongSeq * sample_sizes = dds_UnsignedLongSeq_create(count);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  *taken = 0;

  dds_ReturnCode_t ret = dds_DataReader_raw_take(
    topic_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, count,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  const char * topic_name =
    dds_TopicDescription_get_name(dds_DataReader_get_topicdescription(topic_reader));

  if (ret == dds_RETCODE_NO_DATA) {
    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_gurumdds_cpp", "No data on topic %s", topic_name);
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_cpp", "Received data on topic %s", topic_name);

  for (uint32_t i = 0; i < dds_SampleInfoSeq_length(sample_infos); i++) {
    dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, i);

    if (sample_info->valid_data) {
      void * sample = dds_DataSeq_get(data_values, i);
      if (sample == nullptr) {
        RMW_SET_ERROR_MSG("failed to get message");
        dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return RMW_RET_ERROR;
      }
      uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, i);
      bool result = deserialize_cdr_to_ros(
        info->rosidl_message_typesupport->data,
        info->rosidl_message_typesupport->typesupport_identifier,
        message_sequence->data[*taken],
        sample,
        static_cast<size_t>(sample_size)
      );
      if (!result) {
        RMW_SET_ERROR_MSG("failed to deserialize message");
        dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return RMW_RET_ERROR;
      }

      auto message_info = &(message_info_sequence->data[*taken]);

      message_info->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;
      // TODO(clemjh): SampleInfo doesn't contain received_timestamp
      message_info->received_timestamp = 0;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = gurum_gurumdds_identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader, sample_info->publication_handle, custom_gid->publication_handle);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED("rmw_gurumdds_cpp", "Failed to get publication handle");
        }
        memset(custom_gid->publication_handle, 0, sizeof(custom_gid->publication_handle));
      }

      (*taken)++;
    }
  }

  message_sequence->size = *taken;
  message_info_sequence->size = *taken;

  dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
  dds_DataSeq_delete(data_values);
  dds_SampleInfoSeq_delete(sample_infos);
  dds_UnsignedLongSeq_delete(sample_sizes);

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

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  GurumddsSubscriberInfo * info = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  dds_DataSeq * data_values = dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoSeq * sample_infos = dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    dds_DataSeq_delete(data_values);
    return RMW_RET_ERROR;
  }

  dds_UnsignedLongSeq * sample_sizes = dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DataReader_raw_take(
    topic_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, 1,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  const char * topic_name =
    dds_TopicDescription_get_name(dds_DataReader_get_topicdescription(topic_reader));

  if (ret == dds_RETCODE_NO_DATA) {
    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_gurumdds_cpp", "No data on topic %s", topic_name);
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_cpp", "Received data on topic %s", topic_name);

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  if (sample_info->valid_data) {
    void * sample = dds_DataSeq_get(data_values, 0);
    if (sample == nullptr) {
      RMW_SET_ERROR_MSG("failed to take data");
      dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }

    uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);
    serialized_message->buffer_length = sample_size;
    if (serialized_message->buffer_capacity < sample_size) {
      rmw_ret_t rmw_ret = rmw_serialized_message_resize(serialized_message, sample_size);
      if (rmw_ret != RMW_RET_OK) {
        // Error message already set
        dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return rmw_ret;
      }
    }

    memcpy(serialized_message->buffer, sample, sample_size);

    *taken = true;

    if (message_info != nullptr) {
      message_info->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;
      // TODO(clemjh): SampleInfo doesn't contain received_timestamp
      message_info->received_timestamp = 0;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader, sample_info->publication_handle, custom_gid->publication_handle);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED("rmw_gurumdds_cpp", "Failed to get publication handle");
        }
        memset(custom_gid->publication_handle, 0, sizeof(custom_gid->publication_handle));
      }
    }
  }

  dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
  dds_DataSeq_delete(data_values);
  dds_SampleInfoSeq_delete(sample_infos);
  dds_UnsignedLongSeq_delete(sample_sizes);

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
    subscription, "subscription pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_INVALID_ARGUMENT);

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
    subscription, "subscription pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_INVALID_ARGUMENT);

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

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
#include <chrono>
#include <thread>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/guid.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"

#include "rmw_gurumdds_static_cpp/types.hpp"
#include "rmw_gurumdds_static_cpp/identifier.hpp"

#include "./type_support_common.hpp"

extern "C"
{
rmw_ret_t
rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_bounds_t * message_bounds,
  rmw_subscription_allocation_t * allocation)
{
  // Unused in current implementation.
  (void)type_support;
  (void)message_bounds;
  (void)allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_fini_subscription_allocation(rmw_subscription_allocation_t * allocation)
{
  // Unused in current implementation.
  (void)allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_ERROR;
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies, bool ignore_local_publications)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }

  if (node->implementation_identifier != gurum_gurumdds_static_identifier) {
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
    get_message_typesupport_handle(type_supports, RMW_GURUMDDS_STATIC_CPP_TYPESUPPORT_C);
  if (type_support == nullptr) {
    type_support = get_message_typesupport_handle(type_supports, RMW_GURUMDDS_STATIC_CPP_TYPESUPPORT_CPP);
    if (type_support == nullptr) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  const message_type_support_callbacks_t * callbacks =
    static_cast<const message_type_support_callbacks_t *>(type_support->data);
  if (callbacks == nullptr) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return nullptr;
  }

  // Past this point, a failure results in unrolling code in the goto fail block.

  rmw_subscription_t * subscription = nullptr;
  GurumddsSubscriberInfo * subscriber_info = nullptr;
  dds_Subscriber * dds_subscriber = nullptr;
  dds_SubscriberQos subscriber_qos;
  dds_DataReader * topic_reader = nullptr;
  dds_DataReaderQos datareader_qos;
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
  dds_ReadCondition * read_condition = nullptr;
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  std::string type_name = _create_type_name(callbacks);
  rmw_ret_t rmw_ret = RMW_RET_OK;

  std::string processed_topic_name;
  if (!qos_policies->avoid_ros_namespace_conventions) {
    processed_topic_name = std::string(ros_topic_prefix) + topic_name;
  } else {
    processed_topic_name = topic_name;
  }

  callbacks->register_type(participant, type_name.c_str());

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
    goto fail;
  }

  read_condition = dds_DataReader_create_readcondition(
    topic_reader, dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);
  if (read_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create read condition");
    goto fail;
  }

  subscriber_info = new(std::nothrow) GurumddsSubscriberInfo();
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate subscriber info handle");
    goto fail;
  }

  subscriber_info->subscriber = dds_subscriber;
  subscriber_info->topic_reader = topic_reader;
  subscriber_info->read_condition = read_condition;
  subscriber_info->ignore_local_publications = ignore_local_publications;
  subscriber_info->callbacks = callbacks;

  subscription = rmw_subscription_allocate();
  if (subscription == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    goto fail;
  }

  subscription->implementation_identifier = gurum_gurumdds_static_identifier;
  subscription->data = subscriber_info;
  subscription->topic_name = reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (subscription->topic_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node name");
    goto fail;
  }
  memcpy(const_cast<char *>(subscription->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);
  if (rmw_ret != RMW_RET_OK) {
    // Error message already set
    goto fail;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

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
      if (read_condition != nullptr) {
        ret = dds_DataReader_delete_readcondition(topic_reader, read_condition);
        if (ret != dds_RETCODE_OK) {
          std::stringstream ss;
          ss << "leaking readcondition while handling failure at " <<
            __FILE__ << ":" << __LINE__ << '\n';
          (std::cerr << ss.str()).flush();
        }
      }
      ret = dds_Subscriber_delete_datareader(dds_subscriber, topic_reader);
      if (ret != dds_RETCODE_OK) {
        std::stringstream ss;
        ss << "leaking datareader while handling failure at " <<
          __FILE__ << ":" << __LINE__ << '\n';
        (std::cerr << ss.str()).flush();
      }
    }
    ret = dds_DomainParticipant_delete_subscriber(participant, dds_subscriber);
    if (ret != dds_RETCODE_OK) {
      std::stringstream ss;
      ss << "leaking subscriber while handling failure at " <<
        __FILE__ << ":" << __LINE__ << '\n';
      (std::cerr << ss.str()).flush();
    }
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
    RMW_SET_ERROR_MSG("failed to get matched subscriptions");
    dds_InstanceHandleSeq_delete(seq);
    return RMW_RET_ERROR;
  }
  *publisher_count = (size_t)dds_InstanceHandleSeq_length(seq);

  dds_InstanceHandleSeq_delete(seq);

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
    node handle, node->implementation_identifier, gurum_gurumdds_static_identifier, return RMW_RET_ERROR)

  if (subscription == nullptr) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle, subscription->implementation_identifier,
    gurum_gurumdds_static_identifier, return RMW_RET_ERROR)

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
        dds_ReadCondition * read_condition = subscriber_info->read_condition;
        if (read_condition != nullptr) {
          ret = dds_DataReader_delete_readcondition(topic_reader, read_condition);
          if (ret != dds_RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to delete readcondition");
            rmw_ret = RMW_RET_ERROR;
          }
          subscriber_info->read_condition = nullptr;
        }

        ret = dds_Subscriber_delete_datareader(dds_subscriber, topic_reader);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete datareader");
          rmw_ret = RMW_RET_ERROR;
        }
        subscriber_info->topic_reader = nullptr;
      } else if (subscriber_info->read_condition != nullptr) {
        RMW_SET_ERROR_MSG("cannot delete readcondition because the datareader is null");
        rmw_ret = RMW_RET_ERROR;
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

    delete subscriber_info;
    subscription->data = nullptr;
    if (subscription->topic_name != nullptr) {
      rmw_free(const_cast<char *>(subscription->topic_name));
    }
  }

  rmw_subscription_free(subscription);

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);

  return rmw_ret;
}

rmw_ret_t
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

  dds_ReturnCode_t ret = dds_DataReader_take(
    topic_reader, data_values, sample_infos, 1,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    dds_DataReader_return_loan(topic_reader, data_values, sample_infos);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    *taken = false;
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    dds_DataReader_return_loan(topic_reader, data_values, sample_infos);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  bool ignore_sample = false;

  if (!sample_info->valid_data) {
    ignore_sample = true;
  }

  if (!ignore_sample) {
    void * sample = dds_DataSeq_get(data_values, 0);
    // Some types such as string need to be converted
    if (!info->callbacks->convert_dds_to_ros(sample, ros_message)) {
      RMW_SET_ERROR_MSG("failed to convert message");
      dds_DataReader_return_loan(topic_reader, data_values, sample_infos);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      return RMW_RET_ERROR;
    }

    *taken = true;

    if (message_info != nullptr) {
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      custom_gid->publication_handle = sample_info->publication_handle;
    }
  }

  dds_DataReader_return_loan(topic_reader, data_values, sample_infos);
  dds_DataSeq_delete(data_values);
  dds_SampleInfoSeq_delete(sample_infos);

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

  return _take(gurum_gurumdds_static_identifier, subscription, ros_message, taken, nullptr, allocation);
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
    gurum_gurumdds_static_identifier, subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t
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
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    info, "custom subscriber info is null", return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    topic_reader, "topic reader is null", return RMW_RET_ERROR);

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
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DataReader_raw_take(
    topic_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, 1,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    *taken = false;
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

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  bool ignore_sample = false;

  if (!sample_info->valid_data) {
    ignore_sample = true;
  }

  if (!ignore_sample) {
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
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      auto custom_gid = reinterpret_cast<GurumddsPublisherGID *>(sender_gid->data);
      custom_gid->publication_handle = sample_info->publication_handle;
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
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take_serialized(
    gurum_gurumdds_static_identifier, subscription, serialized_message, taken, nullptr, allocation);
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
    gurum_gurumdds_static_identifier, subscription, serialized_message, taken, message_info, allocation);
}
}  // extern "C"

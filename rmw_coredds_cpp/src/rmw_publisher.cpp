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

#include <string>
#include <chrono>
#include <thread>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"
#include "rmw_coredds_shared_cpp/types.hpp"
#include "rmw_coredds_shared_cpp/qos.hpp"
#include "rmw_coredds_shared_cpp/namespace_prefix.hpp"
#include "rmw_coredds_cpp/identifier.hpp"
#include "rmw_coredds_cpp/types.hpp"

#include "rcutils/types.h"

#include "./type_support_common.hpp"

extern "C"
{
rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }

  if (node->implementation_identifier != gurum_coredds_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return nullptr;
  }

  if (topic_name == nullptr || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
    return nullptr;
  }

  if (qos_policies == nullptr) {
    RMW_SET_ERROR_MSG("qos profile is null");
    return nullptr;
  }

  CoreddsNodeInfo * node_info = static_cast<CoreddsNodeInfo *>(node->data);
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
    get_message_typesupport_handle(type_supports, RMW_COREDDS_CPP_TYPESUPPORT_C);
  if (type_support == nullptr) {
    type_support = get_message_typesupport_handle(type_supports, RMW_COREDDS_CPP_TYPESUPPORT_CPP);
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

  rmw_publisher_t * rmw_publisher = nullptr;
  CoreddsPublisherInfo * publisher_info = nullptr;
  dds_Publisher * dds_publisher = nullptr;
  dds_PublisherQos publisher_qos;
  dds_DataWriter * topic_writer = nullptr;
  dds_DataWriterQos datawriter_qos;
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  std::string type_name = _create_type_name(callbacks, "msg");
  rmw_ret_t rmw_ret = RMW_RET_OK;

  std::string processed_topic_name;
  if (!qos_policies->avoid_ros_namespace_conventions) {
    processed_topic_name = std::string(ros_topic_prefix) + topic_name;
  } else {
    processed_topic_name = topic_name;
  }

  callbacks->register_type(participant, type_name.c_str());

  ret = dds_DomainParticipant_get_default_publisher_qos(participant, &publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default publisher qos");
    goto fail;
  }

  dds_publisher = dds_DomainParticipant_create_publisher(participant, &publisher_qos, nullptr, 0);
  if (dds_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to create publisher");
    goto fail;
  }

  // Look for message topic
  topic_desc = dds_DomainParticipant_lookup_topicdescription(
    participant, processed_topic_name.c_str());
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

  if (!get_datawriter_qos(dds_publisher, qos_policies, &datawriter_qos)) {
    // Error message already set
    goto fail;
  }

  topic_writer = dds_Publisher_create_datawriter(dds_publisher, topic, &datawriter_qos, nullptr, 0);
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    goto fail;
  }

  publisher_info = new(std::nothrow) CoreddsPublisherInfo();
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate CoreddsPublisherInfo");
    return nullptr;
  }

  publisher_info->implementation_identifier = gurum_coredds_identifier;
  publisher_info->publisher = dds_publisher;
  publisher_info->topic_writer = topic_writer;
  publisher_info->callbacks = callbacks;
  publisher_info->publisher_gid.implementation_identifier = gurum_coredds_identifier;

  static_assert(sizeof(CoreddsPublisherGID) <= RMW_GID_STORAGE_SIZE,
    "RMW_GID_STORAGE_SIZE insufficient to store the rmw_coredds_cpp GID implementation.");
  memset(publisher_info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
  {
    auto publisher_gid =
      reinterpret_cast<CoreddsPublisherGID *>(publisher_info->publisher_gid.data);
    publisher_gid->publication_handle = dds_DataWriter_get_instance_handle(topic_writer);
  }

  rmw_publisher = rmw_publisher_allocate();
  if (rmw_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    goto fail;
  }

  rmw_publisher->implementation_identifier = gurum_coredds_identifier;
  rmw_publisher->data = publisher_info;
  rmw_publisher->topic_name = reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (rmw_publisher->topic_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);
  if (rmw_ret != RMW_RET_OK) {
    // Error message already set
    goto fail;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  return rmw_publisher;

fail:
  if (rmw_publisher != nullptr) {
    rmw_publisher_free(rmw_publisher);
  }

  if (dds_publisher != nullptr) {
    if (topic_writer != nullptr) {
      if (dds_Publisher_delete_datawriter(dds_publisher, topic_writer) != dds_RETCODE_OK) {
        std::stringstream ss;
        ss << "leaking datawriter while handling failure at " <<
          __FILE__ << ":" << __LINE__ << '\n';
        (std::cerr << ss.str()).flush();
      }
    }
    if (dds_DomainParticipant_delete_publisher(participant, dds_publisher) != dds_RETCODE_OK) {
      std::stringstream ss;
      ss << "leaking publisher while handling failure at " <<
        __FILE__ << ":" << __LINE__ << '\n';
      (std::cerr << ss.str()).flush();
    }
  }

  if (publisher_info != nullptr) {
    delete publisher_info;
  }

  return nullptr;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  if (publisher == nullptr) {
    RMW_SET_ERROR_MSG("publisher handle is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (subscription_count == nullptr) {
    RMW_SET_ERROR_MSG("subscription_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto info = static_cast<CoreddsPublisherInfo *>(publisher->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_Publisher * dds_publisher = info->publisher;
  if (dds_publisher == nullptr) {
    RMW_SET_ERROR_MSG("dds publisher is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * topic_writer = info->topic_writer;
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("topic writer is null");
    return RMW_RET_ERROR;
  }

  dds_InstanceHandleSeq * seq = dds_InstanceHandleSeq_create(4);
  if (dds_DataWriter_get_matched_subscriptions(topic_writer, seq) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get matched subscriptions");
    dds_InstanceHandleSeq_delete(seq);
    return RMW_RET_ERROR;
  }
  *subscription_count = (size_t)dds_InstanceHandleSeq_length(seq);

  dds_InstanceHandleSeq_delete(seq);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, gurum_coredds_identifier,
    return RMW_RET_ERROR)

  if (publisher == nullptr) {
    RMW_SET_ERROR_MSG("publisher handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher handle,
    publisher->implementation_identifier, gurum_coredds_identifier,
    return RMW_RET_ERROR)

  auto node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  auto participant = static_cast<dds_DomainParticipant *>(node_info->participant);
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return RMW_RET_ERROR;
  }

  CoreddsPublisherInfo * publisher_info = static_cast<CoreddsPublisherInfo *>(publisher->data);
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  if (publisher_info) {
    dds_Publisher * dds_publisher = publisher_info->publisher;

    if (dds_publisher != nullptr) {
      if (publisher_info->topic_writer != nullptr) {
        ret = dds_Publisher_delete_datawriter(dds_publisher, publisher_info->topic_writer);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete datawriter");
          return RMW_RET_ERROR;
        }
        publisher_info->topic_writer = nullptr;
      }

      ret = dds_DomainParticipant_delete_publisher(participant, dds_publisher);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete publisher");
        return RMW_RET_ERROR;
      }
      publisher_info->publisher = nullptr;
    } else if (publisher_info->topic_writer != nullptr) {
      RMW_SET_ERROR_MSG("cannot delete datawriter because the publisher is null");
      return RMW_RET_ERROR;
    }

    delete publisher_info;
    publisher->data = nullptr;
    if (publisher->topic_name != nullptr) {
      rmw_free(const_cast<char *>(publisher->topic_name));
    }
  }

  rmw_publisher_free(publisher);

  rmw_ret_t rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);
  if (rmw_ret != RMW_RET_OK) {
    // Error message already set
    return rmw_ret;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish(
  const rmw_publisher_t * publisher,
  const void * ros_message)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(publisher, "publisher pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros_message pointer is null", return RMW_RET_ERROR);

  auto info = static_cast<CoreddsPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

  dds_DataWriter * topic_writer = info->topic_writer;

  const message_type_support_callbacks_t * callbacks = info->callbacks;
  if (callbacks == nullptr) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return RMW_RET_ERROR;
  }

  // Some types such as string need to be converted
  void * dds_message = callbacks->alloc();
  if (!callbacks->convert_ros_to_dds(ros_message, dds_message)) {
    RMW_SET_ERROR_MSG("failed to convert string");
    return RMW_RET_ERROR;
  }

  if (dds_DataWriter_write(topic_writer, dds_message, dds_HANDLE_NIL) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to publish data");
    callbacks->free(dds_message);
    return RMW_RET_ERROR;
  }

  callbacks->free(dds_message);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(publisher, "publisher pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null", return RMW_RET_ERROR);

  auto info = static_cast<CoreddsPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

  dds_DataWriter * topic_writer = info->topic_writer;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "topic writer is null", return RMW_RET_ERROR);

  if (dds_DataWriter_raw_write(
      topic_writer, serialized_message->buffer,
      static_cast<uint32_t>(serialized_message->buffer_length)) != dds_RETCODE_OK)
  {
    RMW_SET_ERROR_MSG("failed to publish data");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  if (publisher == nullptr) {
    RMW_SET_ERROR_MSG("publisher is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher handle,
    publisher->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_ERROR);

  if (gid == nullptr) {
    RMW_SET_ERROR_MSG("gid is null");
    return RMW_RET_ERROR;
  }

  const CoreddsPublisherInfo * info =
    static_cast<const CoreddsPublisherInfo *>(publisher->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("publisher info handle is null");
    return RMW_RET_ERROR;
  }

  *gid = info->publisher_gid;
  return RMW_RET_OK;
}
}  // extern "C"

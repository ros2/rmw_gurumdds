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
#include <sstream>
#include <limits>
#include <thread>
#include <chrono>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/types.hpp"

#include "rcutils/types.h"
#include "rcutils/error_handling.h"

#include "./type_support_common.hpp"

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_publisher_allocation_t * allocation)
{
  (void)type_support;
  (void)message_bounds;
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_init_publisher_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t * allocation)
{
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_fini_publisher_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options)
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
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
    return nullptr;
  }

  if (qos_policies == nullptr) {
    RMW_SET_ERROR_MSG("qos profile is null");
    return nullptr;
  }

  if (publisher_options == nullptr) {
    RMW_SET_ERROR_MSG("publisher_options is null");
    return nullptr;
  }

  if (publisher_options->require_unique_network_flow_endpoints ==
    RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED)
  {
    RMW_SET_ERROR_MSG(
      "Strict requirement on unique network flow endpoints for publishers not supported");
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

  rmw_publisher_t * rmw_publisher = nullptr;
  GurumddsPublisherInfo * publisher_info = nullptr;
  dds_Publisher * dds_publisher = nullptr;
  dds_PublisherQos publisher_qos;
  dds_DataWriter * topic_writer = nullptr;
  dds_DataWriterQos datawriter_qos;
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
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

  ret = dds_DomainParticipant_get_default_publisher_qos(participant, &publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default publisher qos");
    goto fail;
  }

  dds_publisher = dds_DomainParticipant_create_publisher(participant, &publisher_qos, nullptr, 0);
  if (dds_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to create publisher");
    dds_PublisherQos_finalize(&publisher_qos);
    goto fail;
  }

  ret = dds_PublisherQos_finalize(&publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize publisher qos");
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

  if (!get_datawriter_qos(dds_publisher, qos_policies, &datawriter_qos)) {
    // Error message already set
    goto fail;
  }

  topic_writer = dds_Publisher_create_datawriter(dds_publisher, topic, &datawriter_qos, nullptr, 0);
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    dds_DataWriterQos_finalize(&datawriter_qos);
    goto fail;
  }

  ret = dds_DataWriterQos_finalize(&datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datawriter qos");
    goto fail;
  }

  publisher_info = new(std::nothrow) GurumddsPublisherInfo();
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate GurumddsPublisherInfo");
    goto fail;
  }

  publisher_info->implementation_identifier = gurum_gurumdds_identifier;
  publisher_info->publisher = dds_publisher;
  publisher_info->topic_writer = topic_writer;
  publisher_info->rosidl_message_typesupport = type_support;
  publisher_info->publisher_gid.implementation_identifier = gurum_gurumdds_identifier;

  static_assert(
    sizeof(GurumddsPublisherGID) <= RMW_GID_STORAGE_SIZE,
    "RMW_GID_STORAGE_SIZE insufficient to store the rmw_gurumdds_cpp GID implementation.");
  memset(publisher_info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
  {
    auto publisher_gid =
      reinterpret_cast<GurumddsPublisherGID *>(publisher_info->publisher_gid.data);
    dds_DataWriter_get_guid(topic_writer, publisher_gid->publication_handle);
  }

  rmw_publisher = rmw_publisher_allocate();
  if (rmw_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    goto fail;
  }

  rmw_publisher->implementation_identifier = gurum_gurumdds_identifier;
  rmw_publisher->data = publisher_info;
  rmw_publisher->topic_name = reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (rmw_publisher->topic_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for topic name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);
  rmw_publisher->options = *publisher_options;
  rmw_publisher->can_loan_messages = false;

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
    "Created publisher with topic '%s' on node '%s%s%s'",
    topic_name, node->namespace_,
    node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

  return rmw_publisher;

fail:
  if (rmw_publisher != nullptr) {
    rmw_publisher_free(rmw_publisher);
  }

  if (topic != nullptr) {
    dds_DomainParticipant_delete_topic(participant, topic);
  }

  if (dds_publisher != nullptr) {
    if (topic_writer != nullptr) {
      dds_Publisher_delete_datawriter(dds_publisher, topic_writer);
    }
    dds_DomainParticipant_delete_publisher(participant, dds_publisher);
  }

  if (dds_typesupport != nullptr) {
    dds_TypeSupport_delete(dds_typesupport);
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

  auto info = static_cast<GurumddsPublisherInfo *>(publisher->data);
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
rmw_publisher_assert_liveliness(const rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);

  auto * info = static_cast<GurumddsPublisherInfo *>(publisher->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  if (info->topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal datawriter is invalid");
    return RMW_RET_ERROR;
  }

  if (dds_DataWriter_assert_liveliness(info->topic_writer) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to assert liveliness of datawriter");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher handle,
    publisher->implementation_identifier, gurum_gurumdds_identifier,
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

  GurumddsPublisherInfo * publisher_info = static_cast<GurumddsPublisherInfo *>(publisher->data);
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
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_cpp",
        "Deleted publisher with topic '%s' on node '%s%s%s'",
        publisher->topic_name, node->namespace_,
        node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

      rmw_free(const_cast<char *>(publisher->topic_name));
    }
  }

  rmw_publisher_free(publisher);

  rmw_ret_t rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);

  return rmw_ret;
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(gid, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher handle,
    publisher->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  const GurumddsPublisherInfo * info =
    static_cast<const GurumddsPublisherInfo *>(publisher->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("publisher info handle is null");
    return RMW_RET_ERROR;
  }

  *gid = info->publisher_gid;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  GurumddsPublisherInfo * info = static_cast<GurumddsPublisherInfo *>(publisher->data);
  if (info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * data_writer = info->topic_writer;
  if (data_writer == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data writer is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriterQos dds_qos;
  dds_ReturnCode_t ret = dds_DataWriter_get_qos(data_writer, &dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("publisher can't get data writer qos policies");
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

  if (dds_qos.lifespan.duration.sec == dds_DURATION_INFINITE_SEC) {
    qos->lifespan.sec = std::numeric_limits<uint64_t>::max();
    qos->lifespan.nsec = std::numeric_limits<uint64_t>::max();
  } else {
    qos->lifespan.sec = dds_qos.lifespan.duration.sec;
    qos->lifespan.nsec = dds_qos.lifespan.duration.nanosec;
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

  ret = dds_DataWriterQos_finalize(&dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datawriter qos");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish(
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  (void)allocation;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher pointer is null",
    return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<GurumddsPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

  dds_DataWriter * topic_writer = info->topic_writer;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "topic writer is null", return RMW_RET_ERROR);

  const rosidl_message_type_support_t * rosidl_typesupport = info->rosidl_message_typesupport;
  if (rosidl_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("rosidl typesupport handle is null");
    return RMW_RET_ERROR;
  }

  size_t size = 0;
  void * dds_message = allocate_message(
    rosidl_typesupport->data,
    rosidl_typesupport->typesupport_identifier,
    ros_message,
    &size,
    false
  );
  if (dds_message == nullptr) {
    // Error message already set
    return RMW_RET_ERROR;
  }

  bool result = serialize_ros_to_cdr(
    rosidl_typesupport->data,
    rosidl_typesupport->typesupport_identifier,
    ros_message,
    dds_message,
    size
  );
  if (!result) {
    RMW_SET_ERROR_MSG("failed to serialize message");
    free(dds_message);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DataWriter_raw_write(topic_writer, dds_message, size);
  const char * errstr;
  if (ret == dds_RETCODE_OK) {
    errstr = "dds_RETCODE_OK";
  } else if (ret == dds_RETCODE_TIMEOUT) {
    errstr = "dds_RETCODE_TIMEOUT";
  } else if (ret == dds_RETCODE_OUT_OF_RESOURCES) {
    errstr = "dds_RETCODE_OUT_OF_RESOURCES";
  } else {
    errstr = "dds_RETCODE_ERROR";
  }

  if (ret != dds_RETCODE_OK) {
    std::stringstream errmsg;
    errmsg << "failed to publish data: " << errstr << ", " << ret;
    RMW_SET_ERROR_MSG(errmsg.str().c_str());
    free(dds_message);
    return RMW_RET_ERROR;
  }
  const char * topic_name = dds_Topic_get_name(dds_DataWriter_get_topic(topic_writer));
  RCUTILS_LOG_DEBUG_NAMED("rmw_gurumdds_cpp", "Published data on topic %s", topic_name);

  free(dds_message);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  (void)allocation;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher pointer is null",
    return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized_message pointer is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<GurumddsPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

  dds_DataWriter * topic_writer = info->topic_writer;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "topic writer is null", return RMW_RET_ERROR);

  dds_ReturnCode_t ret = dds_DataWriter_raw_write(
    topic_writer,
    serialized_message->buffer,
    static_cast<uint32_t>(serialized_message->buffer_length)
  );
  const char * errstr;
  if (ret == dds_RETCODE_OK) {
    errstr = "dds_RETCODE_OK";
  } else if (ret == dds_RETCODE_TIMEOUT) {
    errstr = "dds_RETCODE_TIMEOUT";
  } else if (ret == dds_RETCODE_OUT_OF_RESOURCES) {
    errstr = "dds_RETCODE_OUT_OF_RESOURCES";
  } else {
    errstr = "dds_RETCODE_ERROR";
  }

  if (ret != dds_RETCODE_OK) {
    std::stringstream errmsg;
    errmsg << "failed to publish data: " << errstr << ", " << ret;
    RMW_SET_ERROR_MSG(errmsg.str().c_str());
    return RMW_RET_ERROR;
  }
  const char * topic_name = dds_Topic_get_name(dds_DataWriter_get_topic(topic_writer));
  RCUTILS_LOG_DEBUG_NAMED("rmw_gurumdds_cpp", "Published data on topic %s", topic_name);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish_loaned_message(
  const rmw_publisher_t * publisher,
  void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  (void)publisher;
  (void)ros_message;
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_publish_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_borrow_loaned_message(
  const rmw_publisher_t * publisher,
  const rosidl_message_type_support_t * type_support,
  void ** ros_message)
{
  (void)publisher;
  (void)type_support;
  (void)ros_message;

  RMW_SET_ERROR_MSG("rmw_borrow_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_return_loaned_message_from_publisher(
  const rmw_publisher_t * publisher,
  void * loaned_message)
{
  (void)publisher;
  (void)loaned_message;

  RMW_SET_ERROR_MSG("rmw_return_loaned_message_from_publisher is not supported");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

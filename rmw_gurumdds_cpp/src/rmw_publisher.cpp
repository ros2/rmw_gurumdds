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

#include "rcutils/error_handling.h"
#include "rcutils/types.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/qos.hpp"

#include "tracetools/tracetools.h"

#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/rmw_publisher.hpp"
#include "rmw_gurumdds_cpp/type_support.hpp"
#include "rmw_gurumdds_cpp/type_support_common.hpp"
#include "rmw_gurumdds_cpp/type_support_service.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"

namespace rmw_gurumdds_cpp
{
rmw_publisher_t *
create_publisher(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Publisher * const pub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options,
  const bool internal)
{
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

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
  PublisherInfo * publisher_info = nullptr;
  dds_DataWriter * topic_writer = nullptr;
  dds_DataWriterQos datawriter_qos{};
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
  dds_TypeSupport * dds_typesupport = nullptr;
  dds_ReturnCode_t ret;

  std::string type_name =
    create_type_name(type_support->data, type_support->typesupport_identifier);
  if (type_name.empty()) {
    // Error message is already set
    return nullptr;
  }

  std::string processed_topic_name = rmw_gurumdds_cpp::create_topic_name(
    rmw_gurumdds_cpp::ros_topic_prefix, topic_name, "", qos_policies);

  std::string metastring =
    create_metastring(type_support->data, type_support->typesupport_identifier);
  if (metastring.empty()) {
    // Error message is already set
    return nullptr;
  }

  dds_typesupport = create_type_support_and_register(participant, type_support, type_name, metastring);
  if (dds_typesupport == nullptr) {
    return nullptr;
  }

  topic_desc = dds_DomainParticipant_lookup_topicdescription(
    participant, processed_topic_name.c_str());
  if (topic_desc == nullptr) {
    dds_TopicQos topic_qos;
    ret = dds_DomainParticipant_get_default_topic_qos(participant, &topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      return nullptr;
    }

    topic = dds_DomainParticipant_create_topic(
      participant, processed_topic_name.c_str(), type_name.c_str(), &topic_qos, nullptr, 0);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      dds_TopicQos_finalize(&topic_qos);
      return nullptr;
    }

    ret = dds_TopicQos_finalize(&topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to finalize topic qos");
      return nullptr;
    }

    TopicEventListener::associate_listener(topic);
  } else {
    dds_Duration_t timeout;
    timeout.sec = 0;
    timeout.nanosec = 1;
    topic = dds_DomainParticipant_find_topic(participant, processed_topic_name.c_str(), &timeout);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      return nullptr;
    }
  }

  ret = dds_DomainParticipantFactory_get_datawriter_qos_from_profile(topic_name, &datawriter_qos);
  if(ret != dds_RETCODE_OK) {
    ret = dds_Publisher_get_default_datawriter_qos(pub, &datawriter_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default datawriter qos");
      return nullptr;
    }
  }

  const rosidl_type_hash_t& type_hash = *type_support->get_type_hash_func(type_support);
  if (!rmw_gurumdds_cpp::get_datawriter_qos(qos_policies, type_hash, &datawriter_qos)) {
    // Error message already set
    return nullptr;
  }

  topic_writer = dds_Publisher_create_datawriter(pub, topic, &datawriter_qos, nullptr, 0);
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    dds_DataWriterQos_finalize(&datawriter_qos);
    return nullptr;
  }

  ret = dds_DataWriterQos_finalize(&datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datawriter qos");
    return nullptr;
  }

  publisher_info = new(std::nothrow) PublisherInfo();
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate PublisherInfo");
    return nullptr;
  }

  dds_DataWriterListener listener{};
  listener.on_offered_deadline_missed = [](const dds_DataWriter * topic_writer,
                                           const dds_OfferedDeadlineMissedStatus * status) {
    dds_DataWriter * writer = const_cast<dds_DataWriter *>(topic_writer);
    PublisherInfo * info = static_cast<PublisherInfo*>(dds_DataWriter_get_listener_context(writer));
    info->on_offered_deadline_missed(*status);
  };

  listener.on_offered_incompatible_qos = [](const dds_DataWriter * topic_writer,
                                            const dds_OfferedIncompatibleQosStatus * status) {
    dds_DataWriter * writer = const_cast<dds_DataWriter *>(topic_writer);
    PublisherInfo * info = static_cast<PublisherInfo*>(dds_DataWriter_get_listener_context(writer));
    info->on_offered_incompatible_qos(*status);
  };

  listener.on_liveliness_lost = [](const dds_DataWriter * topic_writer, const dds_LivelinessLostStatus * status) {
    dds_DataWriter * writer = const_cast<dds_DataWriter *>(topic_writer);
    PublisherInfo * info = static_cast<PublisherInfo*>(dds_DataWriter_get_listener_context(writer));
    info->on_liveliness_lost(*status);
  };

  listener.on_publication_matched = [](const dds_DataWriter * topic_writer,
                                       const dds_PublicationMatchedStatus * status) {
    dds_DataWriter * writer = const_cast<dds_DataWriter *>(topic_writer);
    PublisherInfo * info = static_cast<PublisherInfo*>(dds_DataWriter_get_listener_context(writer));
    info->on_publication_matched(*status);
  };

  dds_DataWriter_set_listener_context(topic_writer, publisher_info);
  publisher_info->topic_writer = topic_writer;
  publisher_info->topic_listener = listener;
  publisher_info->rosidl_message_typesupport = type_support;
  publisher_info->implementation_identifier = RMW_GURUMDDS_ID;
  publisher_info->sequence_number = 0;
  publisher_info->ctx = ctx;
  publisher_info->event_guard_cond[RMW_EVENT_LIVELINESS_LOST] = dds_GuardCondition_create();
  publisher_info->event_guard_cond[RMW_EVENT_OFFERED_DEADLINE_MISSED] = dds_GuardCondition_create();
  publisher_info->event_guard_cond[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE] = dds_GuardCondition_create();
  publisher_info->event_guard_cond[RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE] = dds_GuardCondition_create();
  publisher_info->event_guard_cond[RMW_EVENT_PUBLICATION_MATCHED] = dds_GuardCondition_create();
  dds_TypeSupport* reader_dds_type = dds_DataWriter_get_typesupport(topic_writer);
  set_type_support_ops(reader_dds_type, type_support);

  TopicEventListener::add_event(topic, publisher_info);

  rmw_gurumdds_cpp::entity_get_gid(
    reinterpret_cast<dds_Entity *>(publisher_info->topic_writer),
    publisher_info->publisher_gid);

  rmw_publisher = rmw_publisher_allocate();
  if (rmw_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    return nullptr;
  }
  rmw_publisher->topic_name = nullptr;

  auto scope_exit_rmw_publisher_delete = rcpputils::make_scope_exit(
    [rmw_publisher]() {
      if (rmw_publisher->topic_name != nullptr) {
        rmw_free(const_cast<char *>(rmw_publisher->topic_name));
      }
      rmw_publisher_free(rmw_publisher);
    });

  rmw_publisher->implementation_identifier = RMW_GURUMDDS_ID;
  rmw_publisher->data = publisher_info;
  rmw_publisher->topic_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (rmw_publisher->topic_name == nullptr) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to allocate publisher's topic name");
    return nullptr;
  }
  std::memcpy(
    const_cast<char *>(rmw_publisher->topic_name),
    topic_name,
    strlen(topic_name) + 1);
  rmw_publisher->options = *publisher_options;
  rmw_publisher->can_loan_messages = false;

  if (!internal) {
    if (rmw_gurumdds_cpp::graph_cache::on_publisher_created(ctx, node, publisher_info) != RMW_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to update graph for publisher");
      return nullptr;
    }
  }

  dds_TypeSupport_delete(dds_typesupport);
  dds_typesupport = nullptr;

  scope_exit_rmw_publisher_delete.cancel();

  rmw_gid_t gid;
  ret = rmw_get_gid_for_publisher(rmw_publisher, &gid);
  TRACETOOLS_TRACEPOINT(rmw_publisher_init, static_cast<const void *>(rmw_publisher), gid.data);

  return rmw_publisher;
}

rmw_ret_t
destroy_publisher(
  rmw_context_impl_t * const ctx,
  rmw_publisher_t * const publisher)
{
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  auto publisher_info = static_cast<PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("invalid publisher data");
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret;
  if (publisher_info->topic_writer != nullptr) {
    dds_Topic * topic = dds_DataWriter_get_topic(publisher_info->topic_writer);
    ret = dds_Publisher_delete_datawriter(ctx->publisher, publisher_info->topic_writer);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete datawriter");
      return RMW_RET_ERROR;
    }
    publisher_info->topic_writer = nullptr;
    TopicEventListener::remove_event(topic, publisher_info);

    ret = dds_DomainParticipant_delete_topic(ctx->participant, topic);
    if (ret == dds_RETCODE_PRECONDITION_NOT_MET) {
      RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "The entity using the topic still exists.");
    } else if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete topic");
      return RMW_RET_ERROR;
    } else {
      TopicEventListener::disassociate_Listener(topic);
    }
  }

  for(auto condition: publisher_info->event_guard_cond) {
    if(nullptr != condition) {
      dds_GuardCondition_delete(condition);
    }
  }

  delete publisher_info;
  publisher->data = nullptr;

  return RMW_RET_OK;
}

rmw_ret_t publish(
  const char* identifier,
  const rmw_publisher_t* publisher,
  const void* ros_message,
  rmw_publisher_allocation_t* allocation) {
  RCUTILS_UNUSED(allocation);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);


  auto publisher_info = static_cast<PublisherInfo *>(publisher->data);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher_info, RMW_RET_ERROR);

  dds_DataWriter * topic_writer = publisher_info->topic_writer;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_writer, RMW_RET_ERROR);

  const rosidl_message_type_support_t * rosidl_typesupport =
    publisher_info->rosidl_message_typesupport;
  if (rosidl_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("rosidl typesupport handle is null");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoEx sampleinfo_ex;
  std::memset(&sampleinfo_ex, 0, sizeof(dds_SampleInfoEx));
  ros_sn_to_dds_sn(++publisher_info->sequence_number, &sampleinfo_ex.seq);
  rmw_gurumdds_cpp::ros_guid_to_dds_guid(
      reinterpret_cast<const uint8_t *>(publisher_info->publisher_gid.data),
      reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

  dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
  TRACETOOLS_TRACEPOINT(
    rmw_publish,
    static_cast<const void *>(publisher),
    ros_message,
    rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp)
  );

  dds_ReturnCode_t ret = dds_DataWriter_write_w_sampleinfoex(
      topic_writer, ros_message, &sampleinfo_ex);

  if (ret != dds_RETCODE_OK) {
    std::stringstream errmsg;
    errmsg << "failed to publish data: " << dds_ReturnCode_to_string(ret) << ", " << ret;
    RMW_SET_ERROR_MSG(errmsg.str().c_str());
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Published data on topic %s", publisher->topic_name);
  return RMW_RET_OK;
}
} // namespace rmw_gurumdds_cpp

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_publisher_allocation_t * allocation)
{
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_init_publisher_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t * allocation)
{
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_fini_publisher_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if (strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("topic_name argument is empty");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_options, nullptr);

  // Adapt any 'best available' QoS options
  rmw_qos_profile_t adapted_qos_policies = *qos_policies;
  rmw_ret_t ret = rmw_dds_common::qos_profile_get_best_available_for_topic_publisher(
    node, topic_name, &adapted_qos_policies, rmw_get_subscriptions_info_by_topic);
  if (ret != RMW_RET_OK) {
    return nullptr;
  }

  if (!adapted_qos_policies.avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    ret = rmw_validate_full_topic_name(topic_name, &validation_result, nullptr);
    if (ret != RMW_RET_OK) {
      return nullptr;
    }
    if (validation_result != RMW_TOPIC_VALID) {
      const char * reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "topic name is invalid: %s", reason);
      return nullptr;
    }
  }

  if (publisher_options->require_unique_network_flow_endpoints ==
    RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED)
  {
    RMW_SET_ERROR_MSG("Unique network flow endpoints not supported on publishers");
    return nullptr;
  }

  rmw_context_impl_t * ctx = node->context->impl;

  rmw_publisher_t * const rmw_pub =
    rmw_gurumdds_cpp::create_publisher(
    ctx,
    node,
    ctx->participant,
    ctx->publisher,
    type_supports,
    topic_name,
    &adapted_qos_policies,
    publisher_options,
    RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST == ctx->base->options.discovery_options.automatic_discovery_range);

  if (rmw_pub == nullptr) {
    RMW_SET_ERROR_MSG("failed to create RMW publisher");
    return nullptr;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "Created publisher with topic '%s' on node '%s%s%s'",
    topic_name, node->namespace_,
    node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

  return rmw_pub;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription_count, RMW_RET_INVALID_ARGUMENT);

  auto publisher_info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * topic_writer = publisher_info->topic_writer;
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
  *subscription_count = static_cast<size_t>(dds_InstanceHandleSeq_length(seq));

  dds_InstanceHandleSeq_delete(seq);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publisher_assert_liveliness(const rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);

  auto publisher_info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  if (publisher_info->topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal datawriter is invalid");
    return RMW_RET_ERROR;
  }

  if (dds_DataWriter_assert_liveliness(publisher_info->topic_writer) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to assert liveliness of datawriter");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publisher_wait_for_all_acked(const rmw_publisher_t * publisher, rmw_time_t wait_timeout)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto publisher_info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_Duration_t timeout = rmw_gurumdds_cpp::rmw_time_to_dds(wait_timeout);
  dds_ReturnCode_t ret = dds_DataWriter_wait_for_acknowledgments(
    publisher_info->topic_writer, &timeout);

  if (ret == dds_RETCODE_OK) {
    return RMW_RET_OK;
  } else if (ret == dds_RETCODE_TIMEOUT) {
    return RMW_RET_TIMEOUT;
  } else {
    return RMW_RET_ERROR;
  }
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_context_impl_t * ctx = node->context->impl;

  if (rmw_gurumdds_cpp::graph_cache::on_publisher_deleted(
      ctx, node, reinterpret_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data)))
  {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to update graph for publisher");
    return RMW_RET_ERROR;
  }

  rmw_ret_t ret = rmw_gurumdds_cpp::destroy_publisher(ctx, publisher);

  if (ret == RMW_RET_OK) {
    if (publisher->topic_name != nullptr) {
      RCUTILS_LOG_DEBUG_NAMED(
        RMW_GURUMDDS_ID,
        "Deleted publisher with topic '%s' on node '%s%s%s'",
        publisher->topic_name, node->namespace_,
        node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

      rmw_free(const_cast<char *>(publisher->topic_name));
    }
    rmw_publisher_free(publisher);
  }

  return ret;
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(gid, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto publisher_info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher info handle is null");
    return RMW_RET_ERROR;
  }

  *gid = publisher_info->publisher_gid;

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto publisher_info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * topic_writer = publisher_info->topic_writer;
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data writer is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriterQos dds_qos;
  dds_ReturnCode_t ret = dds_DataWriter_get_qos(topic_writer, &dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("publisher can't get data writer qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability = rmw_gurumdds_cpp::convert_reliability(&dds_qos.reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos.durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos.deadline);
  qos->lifespan = rmw_gurumdds_cpp::convert_lifespan(&dds_qos.lifespan);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos.liveliness);
  qos->liveliness_lease_duration = rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos.liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos.history);
  qos->depth = static_cast<size_t>(dds_qos.history.depth);

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
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  return rmw_gurumdds_cpp::publish(publisher->implementation_identifier, publisher, ros_message, allocation);
}

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  RCUTILS_UNUSED(allocation);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(serialized_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto publisher_info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher_info, RMW_RET_ERROR);

  dds_DataWriter * topic_writer = publisher_info->topic_writer;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_writer, RMW_RET_ERROR);

  dds_SampleInfoEx sampleinfo_ex{};
  rmw_gurumdds_cpp::ros_sn_to_dds_sn(++publisher_info->sequence_number, &sampleinfo_ex.seq);
  rmw_gurumdds_cpp::ros_guid_to_dds_guid(
    reinterpret_cast<const uint8_t *>(publisher_info->publisher_gid.data),
    reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

  dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
  TRACETOOLS_TRACEPOINT(
    rmw_publish,
    static_cast<const void *>(publisher),
    serialized_message,
    rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp)
  );

  dds_ReturnCode_t ret = dds_DataWriter_raw_write_w_sampleinfoex(
    topic_writer,
    serialized_message->buffer,
    static_cast<uint32_t>(serialized_message->buffer_length),
    &sampleinfo_ex
  );

  if (ret != dds_RETCODE_OK) {
    std::stringstream errmsg;
    errmsg << "failed to publish data: " << dds_ReturnCode_to_string(ret) << ", " << ret;
    RMW_SET_ERROR_MSG(errmsg.str().c_str());
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Published data on topic %s", publisher->topic_name);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish_loaned_message(
  const rmw_publisher_t * publisher,
  void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_publish_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_borrow_loaned_message(
  const rmw_publisher_t * publisher,
  const rosidl_message_type_support_t * type_support,
  void ** ros_message)
{
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(ros_message);

  RMW_SET_ERROR_MSG("rmw_borrow_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_return_loaned_message_from_publisher(
  const rmw_publisher_t * publisher,
  void * loaned_message)
{
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(loaned_message);

  RMW_SET_ERROR_MSG("rmw_return_loaned_message_from_publisher is not supported");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

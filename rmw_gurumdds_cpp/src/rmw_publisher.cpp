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

#include <sstream>
#include <string>

#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/types.h"

#include "rcpputils/find_and_replace.hpp"
#include "rcpputils/scope_exit.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/qos.hpp"

#include "tracetools/tracetools.h"

#include "rmw_gurumdds_cpp/backend_buffer.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/rmw_publisher.hpp"
#include "rmw_gurumdds_cpp/type_support.hpp"
#include "rmw_gurumdds_cpp/type_support_common.hpp"
#include "rmw_gurumdds_cpp/type_support_service.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rosidl_typesupport_fastrtps_cpp/service_type_support.h"

#include "fastcdr/cdr/fixed_size_string.hpp"
#include "rmw_gurumdds_cpp/fastrtps.hpp"

#include "rosidl_buffer_backend_registry/backend_utils.hpp"

namespace rmw_gurumdds_cpp
{
namespace
{
// backend buffer
void init_backend_buffer_callback(
  PublisherInfo *publisher_info,
  const rmw_node_t *node,
  const char *topic_name)
{
  // backend_buffer
  if (publisher_info->is_buffer_aware) {
    auto state = publisher_info->buffer_state;

    std::string base_topic = publisher_info->fastrtps_topic_name_mangled;

    auto *backend_context =
      static_cast<const rmw_gurumdds_cpp::BufferBackendContext *>(
      publisher_info->serialization_context);
    auto *buf_registry =
      static_cast<rmw_gurumdds_cpp::BufferEndpointRegistry *>(
      node->context->impl->buffer_endpoint_registry);

    auto callback = [state, base_topic, backend_context](
      const rmw_gurumdds_cpp::BufferEndpointInfo & sub_info) {
        if (!state->alive.load()) {
          return;
        }

      // Detect whether the discovered subscriber is CPU-only.
      // CPU-only subscribers advertise only {"cpu": ""} in their
      // backend_metadata.
        bool sub_is_cpu_only = (sub_info.backend_metadata.size() == 1 &&
          sub_info.backend_metadata.count("cpu") == 1) ||
          sub_info.backend_metadata.empty();

        {
          std::lock_guard<std::mutex> lock(state->mutex);
          if (!state->alive.load()) {
            return;
          }

        // Check for duplicate in existing p2p endpoints or CPU-only list.
          for (const auto & ep : state->endpoints) {
            bool equal = false;
            rmw_ret_t ret = rmw_compare_gids_equal(&ep->target_subscriber_gid,
                                                 &sub_info.gid, &equal);
            if (RMW_RET_OK != ret) {
              RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_cpp",
                                    "Buffer publisher: rmw_compare_gids_equal "
                                    "failed during duplicate check");
              continue;
            }
            if (equal) {
              return;
            }
          }
          for (const auto & g : state->cpu_only_subscribers) {
            bool equal = false;
            rmw_ret_t ret = rmw_compare_gids_equal(&g, &sub_info.gid, &equal);
            if (RMW_RET_OK != ret) {
              RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_cpp",
                                    "Buffer publisher: rmw_compare_gids_equal "
                                    "failed during duplicate check");
              continue;
            }
            if (equal) {
              return;
            }
          }

          if (sub_is_cpu_only) {
          // CPU-only subscriber: track its GID; the shared CPU channel
          // DataWriter will serve it -- no per-subscriber endpoint needed.
            state->cpu_only_subscribers.push_back(sub_info.gid);
            RCUTILS_LOG_DEBUG_NAMED(
              "rmw_gurumdds_cpp",
              "Buffer publisher: CPU-only subscriber discovered on '%s', "
              "served by shared CPU channel",
              base_topic.c_str());
            return;
          }

        // Non-CPU subscriber: create a peer-to-peer endpoint.
          for (const auto & p : state->pending) {
            bool equal = false;
            rmw_ret_t ret = rmw_compare_gids_equal(&p.target_subscriber_gid,
                                                 &sub_info.gid, &equal);
            if (RMW_RET_OK != ret) {
              RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_cpp",
                                    "Buffer publisher: rmw_compare_gids_equal "
                                    "failed during pending check");
              continue;
            }
            if (equal) {
              return;
            }
          }

          std::string sub_hex = gid_to_hex(sub_info.gid);
          std::string unique_topic = base_topic + "/_buf/" + sub_hex;

          rmw_topic_endpoint_info_t discovered_endpoint_info =
            rmw_get_zero_initialized_topic_endpoint_info();
          discovered_endpoint_info.endpoint_type = RMW_ENDPOINT_SUBSCRIPTION;
          std::memcpy(discovered_endpoint_info.endpoint_gid, sub_info.gid.data,
                    RMW_GID_STORAGE_SIZE);

          if (backend_context) {
            std::vector<rmw_topic_endpoint_info_t> existing_endpoints;
            existing_endpoints.reserve(state->endpoints.size() +
                                     state->pending.size());
            for (const auto & ep : state->endpoints) {
              existing_endpoints.push_back(ep->subscriber_endpoint_info);
            }
            for (const auto & p : state->pending) {
              existing_endpoints.push_back(p.subscriber_endpoint_info);
            }

            std::unordered_map<std::string, std::vector<std::set<uint32_t>>>
            backend_endpoint_groups;
            (void)rosidl_buffer_backend_registry::notify_endpoint_discovered(
              backend_context->backend_instances, discovered_endpoint_info,
              existing_endpoints, backend_endpoint_groups,
              sub_info.backend_metadata);
          }

          PendingBufferPublisher pending;
          pending.unique_topic = unique_topic;
          pending.target_subscriber_gid = sub_info.gid;
          pending.subscriber_endpoint_info = discovered_endpoint_info;
          pending.backend_metadata = sub_info.backend_metadata;
          state->pending.push_back(std::move(pending));

          RCUTILS_LOG_DEBUG_NAMED(
            "rmw_gurumdds_cpp",
            "Buffer publisher: non-CPU subscriber discovered, queued '%s'",
            unique_topic.c_str());
        }
      };

    if (buf_registry) {
      buf_registry->register_subscriber_discovery_callback(
          topic_name, publisher_info->publisher_gid, callback);
    }
  }
}
} // namespace

const message_type_support_callbacks_t *
PublisherInfo::get_fastrtps_type_support_callbacks() const
{
  if (fastrtps_message_typesupport == nullptr) {
    return nullptr;
  }
  return static_cast<const message_type_support_callbacks_t *>(
    fastrtps_message_typesupport->data);
}

rmw_publisher_t * create_publisher(
  rmw_context_impl_t *const ctx, const rmw_node_t *node,
  dds_DomainParticipant *const participant, dds_Publisher *const pub,
  const rosidl_message_type_support_t *type_supports, const char *topic_name,
  const rmw_qos_profile_t *qos_policies,
  const rmw_publisher_options_t *publisher_options, const bool internal)
{
  CHECK_ALL_PTRS_NULL(ctx, participant, pub, type_supports, qos_policies,
                      publisher_options);

  if (!is_valid_qos(qos_policies)) {
    return nullptr;
  }

  if (!internal) {
    RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  }

  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  // fastrtps_typesupport 초기화.
  const rosidl_message_type_support_t *fast_type_support =
    get_message_typesupport_handle(type_supports,
                                     RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!fast_type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();
    fast_type_support = get_message_typesupport_handle(
        type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!fast_type_support) {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Type support not from this implementation. Got:\n"
          "    %s\n"
          "    %s\n"
          "while fetching it",
          prev_error_string.str, error_string.str);
      return nullptr;
    }
  }

  // gurum_typesupport 초기화.
  const rosidl_message_type_support_t *gurum_type_support =
    get_message_typesupport_handle(
          type_supports, rosidl_typesupport_introspection_c__identifier);
  if (gurum_type_support == nullptr) {
    rcutils_reset_error();
    gurum_type_support = get_message_typesupport_handle(
        type_supports,
        rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (gurum_type_support == nullptr) {
      rcutils_reset_error();
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  auto callbacks = static_cast<const message_type_support_callbacks_t *>(
    fast_type_support->data);
  std::string fastrtps_type_name = _create_type_name(callbacks);
  auto fastrtps_topic_name_mangled =
    rmw_gurumdds_cpp::_create_topic_name(qos_policies, ros_topic_prefix,
                                           topic_name)
    .to_string();

  if (callbacks->key_callbacks != nullptr) {
    RCUTILS_LOG_WARN_ONCE_NAMED(
        "rmw_gurumdds_cpp", "This message type has @key fields, but Topic "
                            "Instance / keyed topic semantics "
                            "are not supported by rmw_gurumdds_cpp. The "
                            "message will be sent as a normal unkeyed topic.");
  }

  rmw_publisher_t *rmw_publisher = nullptr;
  PublisherInfo *publisher_info = nullptr;
  dds_DataWriter *topic_writer = nullptr;
  raii::dds_DataWriterQos datawriter_qos;
  dds_Topic *topic = nullptr;
  dds_TopicDescription *topic_desc = nullptr;
  raii::dds_TypeSupport dds_typesupport;
  dds_ReturnCode_t ret;

  std::string type_name = create_type_name(
      gurum_type_support->data, gurum_type_support->typesupport_identifier);
  if (type_name.empty()) {
    // Error message is already set
    return nullptr;
  }

  std::string processed_topic_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_topic_prefix, topic_name, "", qos_policies);

  std::string metastring = create_metastring(
      gurum_type_support->data, gurum_type_support->typesupport_identifier);
  if (metastring.empty()) {
    // Error message is already set
    return nullptr;
  }

  dds_typesupport = create_type_support_and_register(
      participant, gurum_type_support, type_name, metastring);
  if (dds_typesupport == nullptr) {
    return nullptr;
  }

  topic_desc = dds_DomainParticipant_lookup_topicdescription(
      participant, processed_topic_name.c_str());
  if (topic_desc == nullptr) {
    raii::dds_TopicQos topic_qos;
    ret = raii::dds_DomainParticipant_get_default_topic_qos(participant,
                                                            topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      return nullptr;
    }

    topic = dds_DomainParticipant_create_topic(
        participant, processed_topic_name.c_str(), type_name.c_str(), topic_qos,
        nullptr, 0);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      // dds_TopicQos_finalize(&topic_qos);
      return nullptr;
    }

    TopicEventListener::associate_listener(topic);
  } else {
    dds_Duration_t timeout;
    timeout.sec = 0;
    timeout.nanosec = 1;
    topic = dds_DomainParticipant_find_topic(
        participant, processed_topic_name.c_str(), &timeout);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      return nullptr;
    }
  }

  ret = raii::dds_Publisher_get_default_datawriter_qos(pub, datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datawriter qos");
    return nullptr;
  }

  // backend_buffer
  bool has_buffer_fields = callbacks->has_buffer_fields;
  std::unordered_map<std::string, std::string> backend_metadata;
  if (has_buffer_fields) {
    auto *backend_context =
      static_cast<BufferBackendContext *>(ctx->buffer_serialization_context);
    if (backend_context) {
      backend_metadata =
        rosidl_buffer_backend_registry::get_all_backend_metadata(
              backend_context->backend_instances);
    }
    if (backend_metadata.find("cpu") == backend_metadata.end()) {
      backend_metadata["cpu"] = "";
    }
  }

  const rosidl_type_hash_t & type_hash =
    *gurum_type_support->get_type_hash_func(gurum_type_support);

  if (has_buffer_fields) {
    if (!rmw_gurumdds_cpp::get_datawriter_qos(
            qos_policies, type_hash, datawriter_qos, backend_metadata))
    {
      // Error message already set
      return nullptr;
    }
  } else {
    if (!rmw_gurumdds_cpp::get_datawriter_qos(qos_policies, type_hash,
                                              datawriter_qos))
    {
      // Error message already set
      return nullptr;
    }
  }

  topic_writer =
    dds_Publisher_create_datawriter(pub, topic, datawriter_qos, nullptr, 0);
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    // dds_DataWriterQos_finalize(&datawriter_qos);
    return nullptr;
  }

  dds_DataWriterListener listener;
  listener.on_offered_deadline_missed =
    [](const dds_DataWriter *topic_writer,
    const dds_OfferedDeadlineMissedStatus *status) {
      dds_DataWriter *writer = const_cast<dds_DataWriter *>(topic_writer);
      PublisherInfo *info = static_cast<PublisherInfo *>(
        dds_DataWriter_get_listener_context(writer));
      if (info == nullptr) {
        return;
      }
      info->on_offered_deadline_missed(*status);
    };

  listener.on_offered_incompatible_qos =
    [](const dds_DataWriter *topic_writer,
    const dds_OfferedIncompatibleQosStatus *status) {
      auto *writer = const_cast<dds_DataWriter *>(topic_writer);
      auto *info = static_cast<PublisherInfo *>(
        dds_DataWriter_get_listener_context(writer));
      if (info == nullptr) {
        return;
      }
      info->on_offered_incompatible_qos(*status);
    };

  listener.on_liveliness_lost = [](const dds_DataWriter *topic_writer,
    const dds_LivelinessLostStatus *status) {
      auto *writer = const_cast<dds_DataWriter *>(topic_writer);
      auto *info = static_cast<PublisherInfo *>(
        dds_DataWriter_get_listener_context(writer));
      if (info == nullptr) {
        return;
      }
      info->on_liveliness_lost(*status);
    };

  listener.on_publication_matched =
    [](const dds_DataWriter *topic_writer,
    const dds_PublicationMatchedStatus *status) {
      auto *writer = const_cast<dds_DataWriter *>(topic_writer);
      auto *info = static_cast<PublisherInfo *>(
        dds_DataWriter_get_listener_context(writer));
      if (info == nullptr) {
        return;
      }
      info->on_publication_matched(*status);
    };

  publisher_info = new (std::nothrow) PublisherInfo();
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate PublisherInfo");
    return nullptr;
  }

  publisher_info->fastrtps_topic_name_mangled = fastrtps_topic_name_mangled;
  publisher_info->rosidl_message_typesupport = gurum_type_support;
  publisher_info->fastrtps_message_typesupport = fast_type_support;

  auto init_guard_cond = [&publisher_info](rmw_event_type_t type) {
      publisher_info->event_guard_cond[type] = dds_GuardCondition_create();
    };

  dds_DataWriter_set_listener_context(topic_writer, publisher_info);
  publisher_info->topic_writer = topic_writer;
  publisher_info->topic_listener = listener;
  publisher_info->implementation_identifier = RMW_GURUMDDS_ID;
  publisher_info->sequence_number = 0;
  publisher_info->ctx = ctx;
  init_guard_cond(RMW_EVENT_LIVELINESS_LOST);
  init_guard_cond(RMW_EVENT_OFFERED_DEADLINE_MISSED);
  init_guard_cond(RMW_EVENT_OFFERED_QOS_INCOMPATIBLE);
  init_guard_cond(RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE);
  init_guard_cond(RMW_EVENT_PUBLICATION_MATCHED);

  ret = dds_DataWriter_set_listener(
      topic_writer, &publisher_info->topic_listener,
      dds_PUBLICATION_MATCHED_STATUS | dds_OFFERED_DEADLINE_MISSED_STATUS |
          dds_OFFERED_INCOMPATIBLE_QOS_STATUS | dds_LIVELINESS_LOST_STATUS);

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to set datawriter listener");
    return nullptr;
  }

  TopicEventListener::add_event(topic, publisher_info);

  rmw_gurumdds_cpp::entity_get_gid(
      reinterpret_cast<dds_Entity *>(publisher_info->topic_writer),
      publisher_info->publisher_gid);

  if (!internal) {
    std::lock_guard<std::mutex> lock(ctx->local_pub_mutex);
    ctx->local_publishers.push_back(publisher_info->publisher_gid);
  }

  rmw_publisher = rmw_publisher_allocate();
  if (rmw_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    return nullptr;
  }
  rmw_publisher->topic_name = nullptr;

  auto scope_exit_rmw_publisher_delete = rcpputils::make_scope_exit([&]() {
        dds_Publisher_delete_contained_entities(pub);
        if (topic != nullptr) {
          dds_DomainParticipant_delete_topic(participant, topic);
        }

        if (rmw_publisher->topic_name != nullptr) {
          rmw_free(const_cast<char *>(rmw_publisher->topic_name));
          rmw_publisher->topic_name = nullptr;
        }
        rmw_publisher_free(rmw_publisher);
        rmw_publisher = nullptr;
  });

  rmw_publisher->implementation_identifier = RMW_GURUMDDS_ID;
  rmw_publisher->data = publisher_info;
  rmw_publisher->topic_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (rmw_publisher->topic_name == nullptr) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID,
                            "failed to allocate publisher's topic name");
    return nullptr;
  }
  std::memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name,
              strlen(topic_name) + 1);
  rmw_publisher->options = *publisher_options;
  rmw_publisher->can_loan_messages = false;

  if (!internal) {
    if (rmw_gurumdds_cpp::graph_cache::on_publisher_created(
            ctx, node, publisher_info) != RMW_RET_OK)
    {
      RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID,
                              "failed to update graph for publisher");
      return nullptr;
    }
  }

  // backend_buffer
  publisher_info->is_buffer_aware = has_buffer_fields;
  if (has_buffer_fields) {
    publisher_info->serialization_context = ctx->buffer_serialization_context;
    publisher_info->backend_metadata = backend_metadata;
    publisher_info->participant = participant;
    publisher_info->publisher = pub;

    publisher_info->local_endpoint_info =
      rmw_get_zero_initialized_topic_endpoint_info();
    publisher_info->local_endpoint_info.endpoint_type = RMW_ENDPOINT_PUBLISHER;
    std::memcpy(publisher_info->local_endpoint_info.endpoint_gid,
                publisher_info->publisher_gid.data, RMW_GID_STORAGE_SIZE);

    auto *backend_context =
      static_cast<const rmw_gurumdds_cpp::BufferBackendContext *>(
      publisher_info->serialization_context);
    if (backend_context) {
      rosidl_buffer_backend_registry::notify_endpoint_created(
          backend_context->backend_instances,
          publisher_info->local_endpoint_info);
    }

    // Create CPU-only shared channel DataWriter.
    // All CPU-only subscribers share this single channel instead of
    // individual peer-to-peer endpoints.
    std::string cpu_topic_name = fastrtps_topic_name_mangled + "/_buf_cpu";

    topic_desc = dds_DomainParticipant_lookup_topicdescription(
        participant, cpu_topic_name.c_str());
    if (topic_desc == nullptr) {
      raii::dds_TopicQos cpu_topic_qos;
      ret = raii::dds_DomainParticipant_get_default_topic_qos(participant,
                                                              cpu_topic_qos);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to get default topic qos");
        return nullptr;
      }

      if (!get_topic_qos(qos_policies, cpu_topic_qos)) {
        // dds_TopicQos_finalize(&cpu_topic_qos);
        RMW_SET_ERROR_MSG(
            "create_publisher() failed setting CPU channel topic QoS");
        return nullptr;
      }

      publisher_info->cpu_topic = dds_DomainParticipant_create_topic(
          participant, cpu_topic_name.c_str(), type_name.c_str(), cpu_topic_qos,
          nullptr, 0);
      if (publisher_info->cpu_topic == nullptr) {
        // dds_TopicQos_finalize(&cpu_topic_qos);
        RMW_SET_ERROR_MSG("failed to create topic");
        return nullptr;
      }

    } else {
      dds_Duration_t timeout;
      timeout.sec = 0;
      timeout.nanosec = 1;
      publisher_info->cpu_topic = dds_DomainParticipant_find_topic(
          participant, cpu_topic_name.c_str(), &timeout);
      if (publisher_info->cpu_topic == nullptr) {
        RMW_SET_ERROR_MSG("failed to find topic");
        return nullptr;
      }
    }

    raii::dds_DataWriterQos cpu_writer_qos;
    raii::dds_DataWriter_get_qos(topic_writer, cpu_writer_qos);
    publisher_info->cpu_data_writer = dds_Publisher_create_datawriter(
        pub, publisher_info->cpu_topic, cpu_writer_qos, nullptr, 0);
    if (!publisher_info->cpu_data_writer) {
      // dds_DataWriterQos_finalize(&cpu_writer_qos);
      dds_DomainParticipant_delete_topic(participant,
                                         publisher_info->cpu_topic);
      publisher_info->cpu_topic = nullptr;
      RMW_SET_ERROR_MSG(
          "create_publisher() failed to create CPU channel DataWriter");
      return nullptr;
    }

    publisher_info->cpu_status_condition =
      dds_DataWriter_get_statuscondition(publisher_info->cpu_data_writer);
    dds_StatusCondition_set_enabled_statuses(
        publisher_info->cpu_status_condition, 0);

    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID,
                            "Created buffer-aware publisher on '%s'",
                            cpu_topic_name.c_str());
  }

  ///////////////////////////////////////////////////////////////////
  init_backend_buffer_callback(publisher_info, node, topic_name);

  scope_exit_rmw_publisher_delete.cancel();

  rmw_gid_t gid;
  ret = rmw_get_gid_for_publisher(rmw_publisher, &gid);
  TRACETOOLS_TRACEPOINT(rmw_publisher_init,
                        static_cast<const void *>(rmw_publisher), gid.data);

  return rmw_publisher;
}

rmw_ret_t destroy_publisher(
  rmw_context_impl_t *const ctx,
  rmw_publisher_t *const publisher)
{
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  auto publisher_info = static_cast<PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("invalid publisher data");
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret;

  // backend_buffer
  if (publisher_info->is_buffer_aware) {
    auto & state = *publisher_info->buffer_state;
    std::vector<std::shared_ptr<BufferPublisherEndpoint>> endpoints_to_destroy;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      state.alive.store(false);
      endpoints_to_destroy = std::move(state.endpoints);
      state.endpoints.clear();
      state.pending.clear();
      state.cpu_only_subscribers.clear();
    }

    auto *buf_registry =
      static_cast<BufferEndpointRegistry *>(ctx->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->unregister_callbacks(publisher_info->publisher_gid);
    }

    for (auto & endpoint : endpoints_to_destroy) {
      if (endpoint->data_writer) {
        ret = dds_Publisher_delete_datawriter(ctx->publisher,
                                              endpoint->data_writer);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete datawriter");
          return RMW_RET_ERROR;
        }
      }
      if (endpoint->topic && endpoint->owns_topic) {
        ret = dds_DomainParticipant_delete_topic(ctx->participant,
                                                 endpoint->topic);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete topic");
          return RMW_RET_ERROR;
        }
      }
    }

    // Clean up the shared CPU channel DataWriter and Topic.
    if (publisher_info->cpu_data_writer) {
      ret = dds_Publisher_delete_datawriter(ctx->publisher,
                                            publisher_info->cpu_data_writer);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete datawriter");
        return RMW_RET_ERROR;
      }
      publisher_info->cpu_data_writer = nullptr;
    }
    if (publisher_info->cpu_topic) {
      ret = dds_DomainParticipant_delete_topic(ctx->participant,
                                               publisher_info->cpu_topic);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete topic");
        return RMW_RET_ERROR;
      }
      publisher_info->cpu_topic = nullptr;
    }
  }

  if (publisher_info->topic_writer != nullptr) {
    dds_DataWriter_set_listener(publisher_info->topic_writer, nullptr, 0);
    dds_DataWriter_set_listener_context(publisher_info->topic_writer, nullptr);

    dds_Topic *topic = dds_DataWriter_get_topic(publisher_info->topic_writer);
    ret = dds_Publisher_delete_datawriter(ctx->publisher,
                                          publisher_info->topic_writer);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete datawriter");
      return RMW_RET_ERROR;
    }
    publisher_info->topic_writer = nullptr;
    TopicEventListener::remove_event(topic, publisher_info);
    TopicEventListener::disassociate_Listener(topic);

    ret = dds_DomainParticipant_delete_topic(ctx->participant, topic);
    if (ret == dds_RETCODE_PRECONDITION_NOT_MET) {
      RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID,
                              "The entity using the topic still exists.");
    } else if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete topic");
      return RMW_RET_ERROR;
    }
  }

  for (auto condition : publisher_info->event_guard_cond) {
    if (nullptr != condition) {
      dds_GuardCondition_delete(condition);
    }
  }

  rmw_gid_t pub_gid;
  std::memcpy(&pub_gid, &publisher_info->publisher_gid, sizeof(rmw_gid_t));

  // remove local publisher gid
  auto remove_callback = [&pub_gid](const rmw_gid_t & gid) {
      return std::memcmp(gid.data, pub_gid.data, RMW_GID_STORAGE_SIZE) == 0;
    };
  {
    std::lock_guard<std::mutex> lock(ctx->local_pub_mutex);

    auto it = std::remove_if(ctx->local_publishers.begin(),
                             ctx->local_publishers.end(), remove_callback);

    ctx->local_publishers.erase(it, ctx->local_publishers.end());
  }

  delete publisher_info;
  publisher->data = nullptr;

  return RMW_RET_OK;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

rmw_ret_t publish(
  const rmw_publisher_t *publisher, const void *ros_message,
  rmw_publisher_allocation_t *allocation)
{
  CHECK_ALL_PTRS_CODE(publisher, ros_message);

  CHECK_ID_CODE(publisher);

  RCUTILS_UNUSED(allocation);

  auto publisher_info = static_cast<PublisherInfo *>(publisher->data);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher_info, RMW_RET_ERROR);

  dds_DataWriter *topic_writer = publisher_info->topic_writer;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_writer, RMW_RET_ERROR);

  auto callbacks = publisher_info->get_fastrtps_type_support_callbacks();
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(callbacks, RMW_RET_ERROR);

  const size_t ser_size = callbacks->get_serialized_size(ros_message);

  size_t buffer_size = ser_size + 4;

  std::vector<char> buffer(buffer_size);

  eprosima::fastcdr::FastBuffer fastbuffer(buffer.data(), buffer_size);

  eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::CdrVersion::XCDRv1);

  ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

  ser.serialize_encapsulation();

  if (!callbacks->cdr_serialize(ros_message, ser)) {
    RMW_SET_ERROR_MSG(
        "failed to serialize ROS message with FastRTPS typesupport");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoEx sampleinfo_ex{};
  rmw_gurumdds_cpp::ros_sn_to_dds_sn(++publisher_info->sequence_number,
                                     &sampleinfo_ex.seq);
  rmw_gurumdds_cpp::ros_guid_to_dds_guid(
      reinterpret_cast<const uint8_t *>(publisher_info->publisher_gid.data),
      reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

  dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
  TRACETOOLS_TRACEPOINT(
      rmw_publish, static_cast<const void *>(publisher), ros_message,
      rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp));

  dds_ReturnCode_t ret = dds_DataWriter_raw_write_w_sampleinfoex(
      topic_writer, buffer.data(), static_cast<uint32_t>(buffer_size),
      &sampleinfo_ex);

  if (ret != dds_RETCODE_OK) {
    std::stringstream errmsg;
    errmsg << "failed to publish data: " << dds_ReturnCode_to_string(ret)
           << ", " << ret;
    RMW_SET_ERROR_MSG(errmsg.str().c_str());
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Published data on topic %s",
                          publisher->topic_name);

  return RMW_RET_OK;
}

// backend_buffer / publish series
void create_pending_buffer_writers(PublisherInfo *info)
{
  auto & state = *info->buffer_state;
  std::vector<PendingBufferPublisher> pending;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.pending.empty()) {
      return;
    }
    pending = std::move(state.pending);
    state.pending.clear();
  }

  std::vector<std::shared_ptr<BufferPublisherEndpoint>> new_endpoints;
  for (auto & p : pending) {
    auto endpoint = std::make_shared<BufferPublisherEndpoint>();
    endpoint->key = p.unique_topic;
    endpoint->target_subscriber_gid = p.target_subscriber_gid;
    endpoint->subscriber_endpoint_info = p.subscriber_endpoint_info;
    endpoint->backend_metadata = std::move(p.backend_metadata);

    dds_Topic *topic = nullptr;

    auto *topic_desc = dds_DomainParticipant_lookup_topicdescription(
        info->participant, p.unique_topic.c_str());
    if (topic_desc == nullptr) {
      dds_Topic *info_topic = dds_DataWriter_get_topic(info->topic_writer);

      raii::dds_TopicQos topic_qos;

      raii::dds_Topic_get_qos(info_topic, topic_qos);

      std::string type_name = create_type_name(
          info->rosidl_message_typesupport->data,
          info->rosidl_message_typesupport->typesupport_identifier);

      topic = dds_DomainParticipant_create_topic(
          info->participant, p.unique_topic.c_str(), type_name.c_str(),
          topic_qos, nullptr, 0);
      if (topic == nullptr) {
        RMW_SET_ERROR_MSG("failed to create topic");
        // dds_TopicQos_finalize(&topic_qos);
        continue;
      }

    } else {
      dds_Duration_t timeout;
      timeout.sec = 0;
      timeout.nanosec = 1;
      topic = dds_DomainParticipant_find_topic(
          info->participant, p.unique_topic.c_str(), &timeout);
      if (topic == nullptr) {
        RMW_SET_ERROR_MSG("failed to find topic");
        continue;
      }

      endpoint->owns_topic = false;
    }

    endpoint->topic = topic;

    raii::dds_DataWriterQos writer_qos;
    raii::dds_DataWriter_get_qos(info->topic_writer, writer_qos);
    // fastrtps에서는 동기 publish 모드, PREALLOCATED_WITH_REALLOC_MEMORY_MODE,
    // 데이터 공유 off로 고정하여 사용함.
    {
      std::string main_gid_str =
        encode_endpoint_gid_for_user_data(info->publisher_gid, "PGID:");

      auto & user_data = writer_qos->user_data;

      const size_t current_size = user_data.size;
      const size_t append_size = main_gid_str.size();
      const size_t capacity = sizeof(user_data.value);

      // 버퍼 오버플로우 방지
      if (append_size > capacity || current_size > capacity ||
        current_size + append_size > capacity)
      {
        continue;
      }

      std::memcpy(user_data.value + current_size,
                  reinterpret_cast<const uint8_t *>(main_gid_str.data()),
                  append_size);

      user_data.size = static_cast<uint32_t>(current_size + append_size);
    }
    dds_DataWriter *data_writer = dds_Publisher_create_datawriter(
        info->publisher, topic, writer_qos, nullptr, 0);
    if (!data_writer) {
      if (endpoint->owns_topic) {
        dds_DomainParticipant_delete_topic(info->participant, topic);
      }
      RCUTILS_LOG_ERROR_NAMED(
          "rmw_gurumdds_cpp",
          "Failed to create per-subscriber DataWriter for '%s'",
          p.unique_topic.c_str());
      continue;
    }
    endpoint->data_writer = data_writer;

    new_endpoints.push_back(std::move(endpoint));
  }

  if (!new_endpoints.empty()) {
    std::lock_guard<std::mutex> lock(state.mutex);
    for (auto & ep : new_endpoints) {
      state.endpoints.push_back(std::move(ep));
    }
  }
}

rmw_ret_t publish_to_buffer_endpoint(
  const rmw_publisher_t *publisher,
  const void *ros_message,
  rmw_publisher_allocation_t *allocation)
{
  CHECK_ALL_PTRS_CODE(publisher, ros_message);

  RCUTILS_UNUSED(allocation);

  create_pending_buffer_writers(static_cast<PublisherInfo *>(publisher->data));

  PublisherInfo *info = static_cast<PublisherInfo *>(publisher->data);

  auto callbacks = info->get_fastrtps_type_support_callbacks();
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(callbacks, RMW_RET_ERROR);
  auto & state = *info->buffer_state;

  std::lock_guard<std::mutex> lock(state.mutex);

  // Publish to the shared CPU channel for all CPU-only subscribers.
  if (!state.cpu_only_subscribers.empty() && info->cpu_data_writer) {
    RCUTILS_LOG_DEBUG_NAMED(
        RMW_GURUMDDS_ID,
        "SKIP CPU-only fallback for buffer message. cpu_only_subscribers=%zu",
        state.cpu_only_subscribers.size());

    const size_t ser_size = callbacks->get_serialized_size(ros_message);

    size_t buffer_size = ser_size + 4;

    std::vector<char> buffer(buffer_size);

    eprosima::fastcdr::FastBuffer fastbuffer(buffer.data(), buffer_size);

    eprosima::fastcdr::Cdr ser(fastbuffer,
      eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::CdrVersion::XCDRv1);

    ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

    ser.serialize_encapsulation();

    // 응용 프로그램에서 backend 버퍼의 "cpu" fallback을 잘못 설정하면 여기서
    // 터질 수 있음. 해당 문제는 fastrtps에서도 동일하게 발생함.
    try {
      if (!callbacks->cdr_serialize(ros_message, ser)) {
        RMW_SET_ERROR_MSG(
            "failed to serialize ROS message with FastRTPS typesupport");
        return RMW_RET_ERROR;
      }
    } catch (std::runtime_error & e) {
      RCUTILS_LOG_ERROR_NAMED(
          RMW_GURUMDDS_ID,
          "Backend-buffer path mismatch detected: the publisher sent this "
          "sample through the "
          "backend-buffer path, but this subscription is currently receiving "
          "through the CPU path. "
          "Please check the publisher/subscription backend-buffer "
          "configuration and ensure that "
          "both endpoints are using compatible transport paths.");
      return RMW_RET_ERROR;
    }

    dds_SampleInfoEx sampleinfo_ex{};
    rmw_gurumdds_cpp::ros_sn_to_dds_sn(++info->sequence_number,
                                       &sampleinfo_ex.seq);
    rmw_gurumdds_cpp::ros_guid_to_dds_guid(
        reinterpret_cast<const uint8_t *>(info->publisher_gid.data),
        reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

    dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
    TRACETOOLS_TRACEPOINT(
        rmw_publish, static_cast<const void *>(publisher), ros_message,
        rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp));

    dds_ReturnCode_t ret = dds_DataWriter_raw_write_w_sampleinfoex(
        info->cpu_data_writer, buffer.data(),
        static_cast<uint32_t>(buffer_size), &sampleinfo_ex);

    if (ret != dds_RETCODE_OK) {
      std::stringstream errmsg;
      errmsg << "failed to publish data: " << dds_ReturnCode_to_string(ret)
             << ", " << ret;
      RMW_SET_ERROR_MSG(errmsg.str().c_str());
      return RMW_RET_ERROR;
    }

    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Published data on topic %s",
                            publisher->topic_name);
  }

  // Publish to per-subscriber peer-to-peer endpoints for non-CPU subscribers.
  for (const auto & endpoint : state.endpoints) {
    size_t ser_size = callbacks->get_serialized_size(ros_message);
    size_t buffer_size = ser_size + 4; // +4 for CDR encapsulation header
    std::vector<uint8_t> buffer_data(buffer_size);

    eprosima::fastcdr::FastBuffer fast_buffer(
      reinterpret_cast<char *>(buffer_data.data()), buffer_size);
    eprosima::fastcdr::Cdr ser(fast_buffer,
      eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::CdrVersion::XCDRv1);
    ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

    auto *backend_context =
      static_cast<const BufferBackendContext *>(info->serialization_context);
    if (!backend_context) {
      RCUTILS_LOG_ERROR_NAMED(
          "rmw_gurumdds_cpp",
          "Buffer-aware serialize missing buffer backend context");
      continue;
    }
    bool ok = false;

    ser.serialize_encapsulation();

    try {
      ok = callbacks->cdr_serialize_with_endpoint(
          ros_message, ser, endpoint->subscriber_endpoint_info,
          backend_context->serialization_context);
    } catch (const std::exception & e) {
      RCUTILS_LOG_ERROR_NAMED(
          "rmw_gurumdds_cpp",
          "Buffer-aware serialize threw for endpoint '%s': %s",
          endpoint->key.c_str(), e.what());
      continue;
    }

    if (!ok) {
      RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_cpp",
                              "Buffer-aware serialize failed for endpoint '%s'",
                              endpoint->key.c_str());
      continue;
    }

    dds_SampleInfoEx sampleinfo_ex{};
    rmw_gurumdds_cpp::ros_sn_to_dds_sn(++info->sequence_number,
                                       &sampleinfo_ex.seq);
    rmw_gurumdds_cpp::ros_guid_to_dds_guid(
        reinterpret_cast<const uint8_t *>(info->publisher_gid.data),
        reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

    dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
    TRACETOOLS_TRACEPOINT(
        rmw_publish, static_cast<const void *>(publisher), ros_message,
        rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp));

    dds_ReturnCode_t ret = dds_DataWriter_raw_write_w_sampleinfoex(
        endpoint->data_writer, buffer_data.data(),
        static_cast<uint32_t>(buffer_size), &sampleinfo_ex);

    if (ret != dds_RETCODE_OK) {
      std::stringstream errmsg;
      errmsg << "failed to publish data: " << dds_ReturnCode_to_string(ret)
             << ", " << ret;
      RMW_SET_ERROR_MSG(errmsg.str().c_str());
      return RMW_RET_ERROR;
    }

    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Published data on topic %s",
                            publisher->topic_name);
  }

  return RMW_RET_OK;
}
} // namespace rmw_gurumdds_cpp

extern "C" {
rmw_ret_t rmw_init_publisher_allocation(
  const rosidl_message_type_support_t *type_support,
  const rosidl_runtime_c__Sequence__bound *message_bounds,
  rmw_publisher_allocation_t *allocation)
{
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_init_publisher_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t *allocation)
{
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_fini_publisher_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_publisher_t * rmw_create_publisher(
  const rmw_node_t *node, const rosidl_message_type_support_t *type_supports,
  const char *topic_name, const rmw_qos_profile_t *qos_policies,
  const rmw_publisher_options_t *publisher_options)
{
  CHECK_ALL_PTRS_NULL(node, type_supports, topic_name, qos_policies,
                      publisher_options);
  CHECK_ALL_PTRS_NULL(node->context, node->context->impl);
  CHECK_ID_NULL(node);

  if (strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("topic_name argument is empty");
    return nullptr;
  }

  // Adapt any 'best available' QoS options
  rmw_qos_profile_t adapted_qos_policies = *qos_policies;
  rmw_ret_t ret =
    rmw_dds_common::qos_profile_get_best_available_for_topic_publisher(
          node, topic_name, &adapted_qos_policies,
          rmw_get_subscriptions_info_by_topic);
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
      const char *reason =
        rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("topic name is invalid: %s", reason);
      return nullptr;
    }
  }

  if (publisher_options->require_unique_network_flow_endpoints ==
    RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED)
  {
    RMW_SET_ERROR_MSG(
        "Unique network flow endpoints not supported on publishers");
    return nullptr;
  }

  rmw_context_impl_t *ctx = node->context->impl;

  bool internal =
    RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST ==
    ctx->base->options.discovery_options.automatic_discovery_range;
  rmw_publisher_t *const rmw_pub = rmw_gurumdds_cpp::create_publisher(
      ctx, node, ctx->participant, ctx->publisher, type_supports, topic_name,
      &adapted_qos_policies, publisher_options, internal);

  if (rmw_pub == nullptr) {
    RMW_SET_ERROR_MSG("failed to create RMW publisher");
    return nullptr;
  }

  RCUTILS_LOG_DEBUG_NAMED(
      RMW_GURUMDDS_ID, "Created publisher with topic '%s' on node '%s%s%s'",
      topic_name, node->namespace_,
      node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/",
      node->name);

  return rmw_pub;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t *publisher,
  size_t *subscription_count)
{
  CHECK_ALL_PTRS_CODE(publisher, subscription_count);
  CHECK_ID_CODE(publisher);

  auto publisher_info =
    static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriter *topic_writer = publisher_info->topic_writer;
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("topic writer is null");
    return RMW_RET_ERROR;
  }

  raii::dds_InstanceHandleSeq seq = raii::dds_InstanceHandleSeq_create(4);
  if (dds_DataWriter_get_matched_subscriptions(topic_writer, seq) !=
    dds_RETCODE_OK)
  {
    RMW_SET_ERROR_MSG("failed to get matched subscriptions");
    return RMW_RET_ERROR;
  }
  *subscription_count = static_cast<size_t>(dds_InstanceHandleSeq_length(seq));

  return RMW_RET_OK;
}

rmw_ret_t rmw_publisher_assert_liveliness(const rmw_publisher_t *publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(publisher);

  auto publisher_info =
    static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  if (publisher_info->topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal datawriter is invalid");
    return RMW_RET_ERROR;
  }

  if (dds_DataWriter_assert_liveliness(publisher_info->topic_writer) !=
    dds_RETCODE_OK)
  {
    RMW_SET_ERROR_MSG("failed to assert liveliness of datawriter");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_publisher_wait_for_all_acked(
  const rmw_publisher_t *publisher,
  rmw_time_t wait_timeout)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(publisher);

  auto publisher_info =
    static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
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

rmw_ret_t rmw_destroy_publisher(rmw_node_t *node, rmw_publisher_t *publisher)
{
  CHECK_ALL_PTRS_CODE(node, publisher);

  CHECK_ID_CODE(node);
  CHECK_ID_CODE(publisher);

  rmw_context_impl_t *ctx = node->context->impl;

  if (rmw_gurumdds_cpp::graph_cache::on_publisher_deleted(
          ctx, node,
          reinterpret_cast<rmw_gurumdds_cpp::PublisherInfo *>(
        publisher->data)))
  {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID,
                            "failed to update graph for publisher");
    return RMW_RET_ERROR;
  }

  rmw_ret_t ret = rmw_gurumdds_cpp::destroy_publisher(ctx, publisher);

  if (ret == RMW_RET_OK) {
    if (publisher->topic_name != nullptr) {
      RCUTILS_LOG_DEBUG_NAMED(
          RMW_GURUMDDS_ID, "Deleted publisher with topic '%s' on node '%s%s%s'",
          publisher->topic_name, node->namespace_,
          node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/",
          node->name);

      rmw_free(const_cast<char *>(publisher->topic_name));
    }
    rmw_publisher_free(publisher);
  }

  return ret;
}

rmw_ret_t rmw_get_gid_for_publisher(
  const rmw_publisher_t *publisher,
  rmw_gid_t *gid)
{
  CHECK_ALL_PTRS_CODE(publisher, gid);

  CHECK_ID_CODE(publisher);

  auto publisher_info =
    static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher info handle is null");
    return RMW_RET_ERROR;
  }

  *gid = publisher_info->publisher_gid;

  return RMW_RET_OK;
}

rmw_ret_t rmw_publisher_get_actual_qos(
  const rmw_publisher_t *publisher,
  rmw_qos_profile_t *qos)
{
  CHECK_ALL_PTRS_CODE(publisher, qos);

  CHECK_ID_CODE(publisher);

  auto publisher_info =
    static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  if (publisher_info == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataWriter *topic_writer = publisher_info->topic_writer;
  if (topic_writer == nullptr) {
    RMW_SET_ERROR_MSG("publisher internal data writer is invalid");
    return RMW_RET_ERROR;
  }

  raii::dds_DataWriterQos dds_qos;
  dds_ReturnCode_t ret = raii::dds_DataWriter_get_qos(topic_writer, dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("publisher can't get data writer qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability =
    rmw_gurumdds_cpp::convert_reliability(&dds_qos->reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos->durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos->deadline);
  qos->lifespan = rmw_gurumdds_cpp::convert_lifespan(&dds_qos->lifespan);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos->liveliness);
  qos->liveliness_lease_duration =
    rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos->liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos->history);
  qos->depth = static_cast<size_t>(dds_qos->history.depth);

  return RMW_RET_OK;
}

rmw_ret_t rmw_publish(
  const rmw_publisher_t *publisher, const void *ros_message,
  rmw_publisher_allocation_t *allocation)
{
  CHECK_ALL_PTRS_CODE(publisher, ros_message);

  // backend_buffer
  auto info = static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  RMW_CHECK_ARGUMENT_FOR_NULL(info, RMW_RET_INVALID_ARGUMENT);

  if (info->is_buffer_aware) {
    size_t total_matched{};
    {
      std::lock_guard<std::mutex> lock(info->mutex_event);
      total_matched = info->publication_matched_status.current_count;
    }
    size_t buffer_aware_count;
    {
      std::lock_guard<std::mutex> lock(info->buffer_state->mutex);
      auto & st = *info->buffer_state;
      buffer_aware_count = st.endpoints.size() + st.pending.size() +
        st.cpu_only_subscribers.size();
    }

    if (total_matched <= buffer_aware_count) {
      return rmw_gurumdds_cpp::publish_to_buffer_endpoint(
          publisher, ros_message, allocation);
    }

    // fallback
    return rmw_gurumdds_cpp::publish(publisher, ros_message, allocation);
  }

  return rmw_gurumdds_cpp::publish(publisher, ros_message, allocation);
}

rmw_ret_t rmw_publish_serialized_message(
  const rmw_publisher_t *publisher,
  const rmw_serialized_message_t *serialized_message,
  rmw_publisher_allocation_t *allocation)
{
  CHECK_ALL_PTRS_CODE(publisher, serialized_message);

  CHECK_ID_CODE(publisher);

  RCUTILS_UNUSED(allocation);

  auto publisher_info =
    static_cast<rmw_gurumdds_cpp::PublisherInfo *>(publisher->data);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(publisher_info, RMW_RET_ERROR);

  dds_DataWriter *topic_writer = publisher_info->topic_writer;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_writer, RMW_RET_ERROR);

  dds_SampleInfoEx sampleinfo_ex{};
  rmw_gurumdds_cpp::ros_sn_to_dds_sn(++publisher_info->sequence_number,
                                     &sampleinfo_ex.seq);
  rmw_gurumdds_cpp::ros_guid_to_dds_guid(
      reinterpret_cast<const uint8_t *>(publisher_info->publisher_gid.data),
      reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

  dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
  TRACETOOLS_TRACEPOINT(
      rmw_publish, static_cast<const void *>(publisher), serialized_message,
      rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp));

  dds_ReturnCode_t ret = dds_DataWriter_raw_write_w_sampleinfoex(
      topic_writer, serialized_message->buffer,
      static_cast<uint32_t>(serialized_message->buffer_length), &sampleinfo_ex);

  if (ret != dds_RETCODE_OK) {
    std::stringstream errmsg;
    errmsg << "failed to publish data: " << dds_ReturnCode_to_string(ret)
           << ", " << ret;
    RMW_SET_ERROR_MSG(errmsg.str().c_str());
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Published data on topic %s",
                          publisher->topic_name);

  return RMW_RET_OK;
}

rmw_ret_t rmw_publish_loaned_message(
  const rmw_publisher_t *publisher,
  void *ros_message,
  rmw_publisher_allocation_t *allocation)
{
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(ros_message);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_publish_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_borrow_loaned_message(
  const rmw_publisher_t *publisher,
  const rosidl_message_type_support_t *type_support,
  void **ros_message)
{
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(ros_message);

  RMW_SET_ERROR_MSG("rmw_borrow_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_return_loaned_message_from_publisher(
  const rmw_publisher_t *publisher,
  void *loaned_message)
{
  RCUTILS_UNUSED(publisher);
  RCUTILS_UNUSED(loaned_message);

  RMW_SET_ERROR_MSG(
      "rmw_return_loaned_message_from_publisher is not supported");
  return RMW_RET_UNSUPPORTED;
}
} // extern "C"

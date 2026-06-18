// Copyright 2026 GurumNetworks, Inc.
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

#include "rmw_gurumdds_cpp/rmw_subscription.hpp"

#include <mutex>
#include <string>

#include "rcpputils/scope_exit.hpp"
#include "rcpputils/split.hpp"
#include "rcutils/error_handling.h"
#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/serialized_message.h"
#include "rmw/subscription_content_filter_options.h"
#include "rmw/types.h"
#include "rmw/validate_full_topic_name.h"
#include "rmw_dds_common/qos.hpp"
#include "rmw/event.h"
#include "rmw/event_callback_type.h"
#include "rmw_gurumdds_cpp/event_converter.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/fastrtps.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/rmw_subscription.hpp"
#include "rmw_gurumdds_cpp/type_support.hpp"
#include "rmw_gurumdds_cpp/type_support_common.hpp"
#include "rmw_gurumdds_cpp/type_support_service.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"
#include "rosidl_buffer_backend_registry/backend_utils.hpp"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "tracetools/tracetools.h"

namespace
{
void on_data_available_for_cpu(const dds_DataReader * r)
{
  if (r == nullptr) {
    return;
  }
  auto * reader = const_cast<dds_DataReader *>(r);

  auto * info =
    static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(dds_DataReader_get_listener_context(reader));

  if (info != nullptr) {
    info->on_cpu_channel_data_available();
  }
}

void on_data_available_for_accel(const dds_DataReader * r)
{
  if (r == nullptr) {
    return;
  }
  auto * reader = const_cast<dds_DataReader *>(r);

  auto * info =
    static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(dds_DataReader_get_listener_context(reader));

  if (info != nullptr) {
    info->on_accel_data_available();
  }
}

bool init_seqs(
  raii::dds_DataSeq & data_seq,
  raii::dds_SampleInfoSeq & info_seq,
  raii::dds_UnsignedLongSeq & raw_data_sizes)
{
  data_seq = raii::dds_DataSeq_create(1);
  if (nullptr == data_seq) {
    RMW_SET_ERROR_MSG("failed to allocate data_seq");
    return false;
  }
  info_seq = raii::dds_SampleInfoSeq_create(1);
  if (nullptr == info_seq) {
    dds_DataSeq_delete(data_seq);
    RMW_SET_ERROR_MSG("failed to allocate info_seq");
    return false;
  }
  raw_data_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (nullptr == raw_data_sizes) {
    raii::dds_DataSeq_delete(data_seq);
    raii::dds_SampleInfoSeq_delete(info_seq);
    RMW_SET_ERROR_MSG("failed to allocate raw_data_sizes");
    return false;
  }

  return true;
}

inline bool is_from_local_publication(
  rmw_context_impl_t * const ctx,
  rmw_gurumdds_cpp::SubscriberInfo * subscriber_info,
  dds_DataReader * topic_reader,
  dds_SampleInfoEx * sample_info_ex)
{
  RCUTILS_UNUSED(topic_reader);

  if (subscriber_info == nullptr || !subscriber_info->ignore_local_publications) {
    return false;
  }

  if (ctx == nullptr || sample_info_ex == nullptr) {
    return false;
  }

  rmw_gid_t sender_gid{};
  sender_gid.implementation_identifier = RMW_GURUMDDS_ID;
  std::memset(sender_gid.data, 0, RMW_GID_STORAGE_SIZE);

  rmw_gurumdds_cpp::dds_guid_to_ros_guid(
    reinterpret_cast<const int8_t *>(&sample_info_ex->src_guid),
    reinterpret_cast<int8_t *>(sender_gid.data));

  std::lock_guard<std::mutex> lock(ctx->local_pub_mutex);

  for (const auto & publisher_gid : ctx->local_publishers) {
    if (std::memcmp(sender_gid.data, publisher_gid.data, RMW_GID_STORAGE_SIZE) == 0) {
      return true;
    }
  }

  return false;
}

inline bool check_message_seq(
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence)
{
  if (count == 0) {
    RMW_SET_ERROR_MSG("count cannot be 0");
    return false;
  }

  if (message_sequence->data == nullptr) {
    RMW_SET_ERROR_MSG("message sequence data is null");
    return false;
  }

  if (message_info_sequence->data == nullptr) {
    RMW_SET_ERROR_MSG("message info sequence data is null");
    return false;
  }

  if (message_sequence->capacity < count) {
    RMW_SET_ERROR_MSG("message sequence capacity is not sufficient");
    return false;
  }

  if (message_info_sequence->capacity < count) {
    RMW_SET_ERROR_MSG("message info sequence capacity is not sufficient");
    return false;
  }

  // test_rmw_impl에서 size 관련 검사 테스트가 있음.
  if (message_sequence->size != 0) {
    RMW_SET_ERROR_MSG("message sequence size is not zero");
    return false;
  }

  if (message_info_sequence->size != 0) {
    RMW_SET_ERROR_MSG("message info sequence size is not zero");
    return false;
  }

  if (message_sequence->size > message_sequence->capacity) {
    RMW_SET_ERROR_MSG("message sequence size exceeds capacity");
    return false;
  }

  if (message_info_sequence->size > message_info_sequence->capacity) {
    RMW_SET_ERROR_MSG("message info sequence size exceeds capacity");
    return false;
  }

  return true;
}
}  // namespace

namespace rmw_gurumdds_cpp
{
const message_type_support_callbacks_t * SubscriberInfo::get_fastrtps_type_support_callbacks() const
{
  if (fastrtps_message_typesupport == nullptr) {
    return nullptr;
  }
  return static_cast<const message_type_support_callbacks_t *>(fastrtps_message_typesupport->data);
}

void SubscriberInfo::on_cpu_channel_data_available()
{
  if (buffer_data_guard != nullptr) {
    dds_GuardCondition_set_trigger_value(buffer_data_guard, true);
  }

  SeqStruct & seqs = cpu_seq;
  if (seqs.data_seq == nullptr || seqs.info_seq == nullptr || seqs.raw_data_sizes == nullptr) {
    return;
  }

  size_t unread = rmw_gurumdds_cpp::count_unread(
    cpu_channel_reader,
    seqs.data_seq,
    seqs.info_seq,
    seqs.raw_data_sizes);

  {
    std::lock_guard<std::mutex> lock(buffer_event_callback_data.mutex);
    if (buffer_event_callback_data.callback != nullptr) {
      buffer_event_callback_data.callback(
        buffer_event_callback_data.user_data,
        unread > 0 ? unread : 1);
    }
  }
}

void SubscriberInfo::on_accel_data_available()
{
  if (buffer_data_guard != nullptr) {
    dds_GuardCondition_set_trigger_value(buffer_data_guard, true);
  }

  SeqStruct & seqs = accel_seq;
  if (seqs.data_seq == nullptr || seqs.info_seq == nullptr || seqs.raw_data_sizes == nullptr) {
    return;
  }

  size_t unread = rmw_gurumdds_cpp::count_unread(
    accel_data_reader,
    seqs.data_seq,
    seqs.info_seq,
    seqs.raw_data_sizes);

  {
    std::lock_guard<std::mutex> lock(buffer_event_callback_data.mutex);
    if (buffer_event_callback_data.callback != nullptr) {
      buffer_event_callback_data.callback(
        buffer_event_callback_data.user_data,
        unread > 0 ? unread : 1);
    }
  }
}

rmw_subscription_t * create_subscription(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Subscriber * const sub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  const bool internal)
{
  CHECK_ALL_PTRS_NULL(ctx, participant, sub, type_supports, qos_policies, subscription_options);

  if (!is_valid_qos(qos_policies)) {
    return nullptr;
  }

  if (!internal) {
    RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  }

  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  // fastrtps_typesupport 초기화.
  const rosidl_message_type_support_t * fast_type_support =
    get_message_typesupport_handle(type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!fast_type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();
    fast_type_support =
      get_message_typesupport_handle(type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!fast_type_support) {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "    %s\n"
        "while fetching it",
        prev_error_string.str,
        error_string.str);
      return nullptr;
    }
  }

  // gurum_typesupport 초기화.
  const rosidl_message_type_support_t * gurum_type_support =
    get_message_typesupport_handle(type_supports, rosidl_typesupport_introspection_c__identifier);
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

  auto callbacks = static_cast<const message_type_support_callbacks_t *>(fast_type_support->data);
  std::string fastrtps_type_name = _create_type_name(callbacks);
  auto fastrtps_topic_name_mangled =
    rmw_gurumdds_cpp::_create_topic_name(qos_policies, ros_topic_prefix, topic_name).to_string();

  if (callbacks->key_callbacks != nullptr) {
    RCUTILS_LOG_WARN_ONCE_NAMED(
      "rmw_gurumdds_cpp",
      "This message type has @key fields, but Topic "
      "Instance / keyed topic semantics "
      "are not supported by rmw_gurumdds_cpp. The "
      "message will be sent as a normal unkeyed topic.");
  }

  rmw_subscription_t * rmw_subscription = nullptr;
  SubscriberInfo * subscriber_info = nullptr;
  dds_DataReader * topic_reader = nullptr;
  // dds_DataReaderQos datareader_qos{};
  raii::dds_DataReaderQos datareader_qos;
  dds_DataReaderListener topic_listener{};
  raii::dds_DataSeq data_seq;
  raii::dds_SampleInfoSeq info_seq;
  raii::dds_UnsignedLongSeq raw_data_sizes;
  dds_Topic * topic = nullptr;
  dds_TopicDescription * topic_desc = nullptr;
  dds_ReadCondition * read_condition = nullptr;
  raii::dds_TypeSupport dds_typesupport = nullptr;
  dds_ReturnCode_t ret;

  std::string type_name =
    create_type_name(gurum_type_support->data, gurum_type_support->typesupport_identifier);
  if (type_name.empty()) {
    return nullptr;
  }

  std::string processed_topic_name = rmw_gurumdds_cpp::create_topic_name(
    rmw_gurumdds_cpp::ros_topic_prefix,
    topic_name,
    "",
    qos_policies);

  std::string metastring =
    create_metastring(gurum_type_support->data, gurum_type_support->typesupport_identifier);
  if (metastring.empty()) {
    // Error message is already set
    return nullptr;
  }

  RCUTILS_LOG_ERROR_NAMED(
    RMW_GURUMDDS_ID,
    "metastring : %s",
    fastrtps_topic_name_mangled.c_str()
  );

  dds_typesupport =
    create_type_support_and_register(participant, gurum_type_support, type_name, metastring);
  if (dds_typesupport == nullptr) {
    return nullptr;
  }

  topic_desc =
    dds_DomainParticipant_lookup_topicdescription(participant, processed_topic_name.c_str());
  if (topic_desc == nullptr) {
    raii::dds_TopicQos topic_qos;
    ret = raii::dds_DomainParticipant_get_default_topic_qos(participant, topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      return nullptr;
    }

    topic = dds_DomainParticipant_create_topic(
      participant,
      processed_topic_name.c_str(),
      type_name.c_str(),
      topic_qos,
      nullptr,
      0);
    if (topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      // dds_TopicQos_finalize(&topic_qos);
      return nullptr;
    }

    // ret = dds_TopicQos_finalize(&topic_qos);
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

  ret = raii::dds_Subscriber_get_default_datareader_qos(sub, datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datareader qos");
    return nullptr;
  }

  // backend_buffer
  bool has_buffer_fields = callbacks->has_buffer_fields;
  std::unordered_map<std::string, std::string> filtered_backends;
  std::vector<std::string> my_backend_types;
  bool cpu_only = false;

  if (has_buffer_fields) {
    std::unordered_map<std::string, std::string> all_backends;
    auto * backend_context = static_cast<BufferBackendContext *>(ctx->buffer_serialization_context);
    if (backend_context) {
      all_backends = rosidl_buffer_backend_registry::get_all_backend_metadata(
        backend_context->backend_instances);
    }

    // Parse acceptable_buffer_backends option (comma-separated) to filter
    std::vector<std::string> requested_list;
    if (
      subscription_options->acceptable_buffer_backends &&
      strlen(subscription_options->acceptable_buffer_backends) > 0)
    {
      auto tokens = rcpputils::split(subscription_options->acceptable_buffer_backends, ',', true);
      for (auto & token : tokens) {
        auto begin_it = token.find_first_not_of(" \t");
        auto end_it = token.find_last_not_of(" \t");
        if (begin_it != std::string::npos) {
          requested_list.push_back(token.substr(begin_it, end_it - begin_it + 1));
        }
      }
    }

    // "any": accept all installed backends
    bool use_all = false;
    for (const auto & name : requested_list) {
      if (name == "any") {
        use_all = true;
        break;
      }
    }

    // NULL, empty, or only "cpu" entries: CPU-only (backward compat default)
    cpu_only = !use_all &&
      (requested_list.empty() ||
      std::all_of(requested_list.begin(), requested_list.end(), [](const std::string & n) {
        return n == "cpu";
                }));

    if (cpu_only) {
      // CPU-only: advertise "cpu" as the only supported backend so the
      // subscription stays on the buffer-aware per-endpoint route and
      // passes the backends_compatible check with CPU-only publishers.
      my_backend_types.push_back("cpu");
      filtered_backends["cpu"] = "";
    } else if (use_all) {
      filtered_backends = all_backends;
      for (const auto & [k, v] : all_backends) {
        my_backend_types.push_back(k);
      }
    } else {
      for (const auto & name : requested_list) {
        if (name == "cpu") {
          continue;
        }
        auto it = all_backends.find(name);
        if (it != all_backends.end()) {
          filtered_backends[it->first] = it->second;
          my_backend_types.push_back(it->first);
        }
      }
    }
  }

  const rosidl_type_hash_t & type_hash =
    *gurum_type_support->get_type_hash_func(gurum_type_support);
  if (has_buffer_fields) {
    if (!rmw_gurumdds_cpp::get_datareader_qos(
          qos_policies,
          type_hash,
          datareader_qos,
          filtered_backends))
    {
      // Error message already set
      return nullptr;
    }
  } else {
    if (!rmw_gurumdds_cpp::get_datareader_qos(qos_policies, type_hash, datareader_qos)) {
      // Error message already set
      return nullptr;
    }
  }

  topic_reader = dds_Subscriber_create_datareader(sub, topic, datareader_qos, nullptr, 0);
  if (topic_reader == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datareader");
    // dds_DataReaderQos_finalize(&datareader_qos);
    return nullptr;
  }

  read_condition = dds_DataReader_create_readcondition(
    topic_reader,
    dds_ANY_SAMPLE_STATE,
    dds_ANY_VIEW_STATE,
    dds_ANY_INSTANCE_STATE);
  if (read_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create read condition");
    return nullptr;
  }

  subscriber_info = new (std::nothrow) SubscriberInfo();
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate SubscriberInfo");
    return nullptr;
  }

  subscriber_info->fastrtps_topic_name_mangled = fastrtps_topic_name_mangled;
  subscriber_info->rosidl_message_typesupport = gurum_type_support;
  subscriber_info->fastrtps_message_typesupport = fast_type_support;

  if (!init_seqs(data_seq, info_seq, raw_data_sizes)) {
    RMW_SET_ERROR_MSG("failed to allocate sequence series");
    return nullptr;
  }

  dds_DataReader_set_listener_context(topic_reader, subscriber_info);
  topic_listener.on_requested_deadline_missed =
    [](const dds_DataReader * topic_reader, const dds_RequestedDeadlineMissedStatus * status) {
      auto * reader = const_cast<dds_DataReader *>(topic_reader);
      auto * info = static_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
      if (info == nullptr) {
        return;
      }
      info->on_requested_deadline_missed(*status);
    };

  topic_listener.on_requested_incompatible_qos =
    [](const dds_DataReader * topic_reader, const dds_RequestedIncompatibleQosStatus * status) {
      auto * reader = const_cast<dds_DataReader *>(topic_reader);
      auto * info = static_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
      if (info == nullptr) {
        return;
      }
      info->on_requested_incompatible_qos(*status);
    };

  topic_listener.on_data_available = [](const dds_DataReader * topic_reader) {
      auto * reader = const_cast<dds_DataReader *>(topic_reader);
      auto * info = static_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
      if (info == nullptr) {
        return;
      }
      info->on_data_available();
    };

  topic_listener.on_liveliness_changed =
    [](const dds_DataReader * topic_reader, const dds_LivelinessChangedStatus * status) {
      auto * reader = const_cast<dds_DataReader *>(topic_reader);
      auto * info = static_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
      if (info == nullptr) {
        return;
      }
      info->on_liveliness_changed(*status);
    };

  topic_listener.on_subscription_matched =
    [](const dds_DataReader * topic_reader, const dds_SubscriptionMatchedStatus * status) {
      auto * reader = const_cast<dds_DataReader *>(topic_reader);
      auto * info = static_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
      if (info == nullptr) {
        return;
      }
      info->on_subscription_matched(*status);
    };

  topic_listener.on_sample_lost =
    [](const dds_DataReader * topic_reader, const dds_SampleLostStatus * status) {
      auto * reader = const_cast<dds_DataReader *>(topic_reader);
      auto * info = static_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
      if (info == nullptr) {
        return;
      }
      info->on_sample_lost(*status);
    };

  auto init_guard_cond = [&subscriber_info](rmw_event_type_t type) {
      subscriber_info->event_guard_cond[type] = dds_GuardCondition_create();
    };

  subscriber_info->topic_reader = topic_reader;
  subscriber_info->read_condition = read_condition;
  subscriber_info->topic_listener = topic_listener;
  subscriber_info->data_seq = std::move(data_seq);
  subscriber_info->info_seq = std::move(info_seq);
  subscriber_info->raw_data_sizes = std::move(raw_data_sizes);
  subscriber_info->implementation_identifier = RMW_GURUMDDS_ID;
  subscriber_info->ctx = ctx;
  subscriber_info->ignore_local_publications = subscription_options->ignore_local_publications;

  dds_DataReader_set_listener(
    subscriber_info->topic_reader,
    &subscriber_info->topic_listener,
    dds_REQUESTED_DEADLINE_MISSED_STATUS | dds_REQUESTED_INCOMPATIBLE_QOS_STATUS |
      dds_DATA_AVAILABLE_STATUS | dds_LIVELINESS_CHANGED_STATUS | dds_SUBSCRIPTION_MATCHED_STATUS |
      dds_SAMPLE_LOST_STATUS);

  init_guard_cond(RMW_EVENT_LIVELINESS_CHANGED);
  init_guard_cond(RMW_EVENT_REQUESTED_DEADLINE_MISSED);
  init_guard_cond(RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE);
  init_guard_cond(RMW_EVENT_MESSAGE_LOST);
  init_guard_cond(RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE);
  init_guard_cond(RMW_EVENT_SUBSCRIPTION_MATCHED);

  TopicEventListener::add_event(topic, subscriber_info);

  rmw_gurumdds_cpp::entity_get_gid(
    reinterpret_cast<dds_Entity *>(subscriber_info->topic_reader),
    subscriber_info->subscriber_gid);

  rmw_subscription = rmw_subscription_allocate();
  if (rmw_subscription == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    return nullptr;
  }
  rmw_subscription->topic_name = nullptr;
  rmw_subscription->is_cft_enabled = false;
  rmw_subscription->is_cft_supported = false;

  auto scope_exit_rmw_subscription_delete = rcpputils::make_scope_exit([&]() {
        dds_Subscriber_delete_contained_entities(sub);
        if (topic != nullptr) {
          dds_DomainParticipant_delete_topic(participant, topic);
        }

        if (rmw_subscription->topic_name != nullptr) {
          rmw_free(const_cast<char *>(rmw_subscription->topic_name));
        }
        rmw_subscription_free(rmw_subscription);
  });

  rmw_subscription->implementation_identifier = RMW_GURUMDDS_ID;
  rmw_subscription->data = subscriber_info;
  rmw_subscription->topic_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (rmw_subscription->topic_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for topic name");
    return nullptr;
  }
  std::memcpy(const_cast<char *>(rmw_subscription->topic_name), topic_name, strlen(topic_name) + 1);
  rmw_subscription->options = *subscription_options;
  rmw_subscription->can_loan_messages = false;

  if (!internal) {
    if (
      rmw_gurumdds_cpp::graph_cache::on_subscriber_created(ctx, node, subscriber_info) !=
      RMW_RET_OK)
    {
      RMW_SET_ERROR_MSG("failed to update graph for subscriber");
      return nullptr;
    }
  }

  // backend_buffer
  //  Buffer-aware subscription setup
  SeqStruct & cpu_seqs = subscriber_info->cpu_seq;
  if (!init_seqs(cpu_seqs.data_seq, cpu_seqs.info_seq, cpu_seqs.raw_data_sizes)) {
    RMW_SET_ERROR_MSG("failed to allocate sequence series");
    return nullptr;
  }
  SeqStruct & accel_seqs = subscriber_info->accel_seq;
  if (!init_seqs(accel_seqs.data_seq, accel_seqs.info_seq, accel_seqs.raw_data_sizes)) {
    RMW_SET_ERROR_MSG("failed to allocate sequence series");
    return nullptr;
  }

  subscriber_info->is_buffer_aware = has_buffer_fields;
  subscriber_info->is_cpu_only = has_buffer_fields && cpu_only;
  if (has_buffer_fields) {
    subscriber_info->serialization_context = ctx->buffer_serialization_context;
    subscriber_info->my_backend_types = std::move(my_backend_types);
    subscriber_info->local_endpoint_info = rmw_get_zero_initialized_topic_endpoint_info();
    subscriber_info->local_endpoint_info.endpoint_type = RMW_ENDPOINT_SUBSCRIPTION;
    std::memcpy(
      subscriber_info->local_endpoint_info.endpoint_gid,
      subscriber_info->subscriber_gid.data,
      RMW_GID_STORAGE_SIZE);

    subscriber_info->buffer_data_guard = dds_GuardCondition_create();

    if (cpu_only) {
      // CPU-only: create a DataReader on the shared CPU channel.
      std::string cpu_topic_name = fastrtps_topic_name_mangled + "/_buf_cpu";

      raii::dds_TopicQos cpu_tqos;
      if (!get_topic_qos(qos_policies, cpu_tqos)) {
        RMW_SET_ERROR_MSG("create_subscription() failed setting CPU channel topic QoS");
        return nullptr;
      }

      topic_desc =
        dds_DomainParticipant_lookup_topicdescription(participant, cpu_topic_name.c_str());
      if (topic_desc == nullptr) {
        subscriber_info->cpu_topic = dds_DomainParticipant_create_topic(
          participant,
          cpu_topic_name.c_str(),
          type_name.c_str(),
          cpu_tqos,
          nullptr,
          0);

        if (subscriber_info->cpu_topic == nullptr) {
          RMW_SET_ERROR_MSG("failed to create topic");
          return nullptr;
        }

      } else {
        dds_Duration_t timeout;
        timeout.sec = 0;
        timeout.nanosec = 1;
        subscriber_info->cpu_topic =
          dds_DomainParticipant_find_topic(participant, cpu_topic_name.c_str(), &timeout);
        if (subscriber_info->cpu_topic == nullptr) {
          RMW_SET_ERROR_MSG("failed to find topic");
          return nullptr;
        }
      }

      raii::dds_DataReaderQos cpu_rqos;
      raii::dds_DataReader_get_qos(topic_reader, cpu_rqos);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to get default topic qos");
        dds_DomainParticipant_delete_topic(participant, subscriber_info->cpu_topic);
        // dds_DataReaderQos_finalize(&cpu_rqos);
        return nullptr;
      }

      subscriber_info->cpu_channel_listener.on_data_available = on_data_available_for_cpu;

      subscriber_info->cpu_channel_reader = dds_Subscriber_create_datareader(
        sub,
        subscriber_info->cpu_topic,
        cpu_rqos,
        &subscriber_info->cpu_channel_listener,
        dds_DATA_AVAILABLE_STATUS);

      // dds_DataReaderQos_finalize(&cpu_rqos);
      if (!subscriber_info->cpu_channel_reader) {
        dds_DomainParticipant_delete_topic(participant, subscriber_info->cpu_topic);
        subscriber_info->cpu_topic = nullptr;
        RMW_SET_ERROR_MSG("create_subscription() failed to create CPU channel DataReader");
        return nullptr;
      }

      dds_DataReader_set_listener_context(subscriber_info->cpu_channel_reader, subscriber_info);
    } else {
      // Accelerated: single shared DataReader for all buffer-aware publishers.
      std::string sub_hex = gid_to_hex(subscriber_info->subscriber_gid);
      std::string accel_topic_name = fastrtps_topic_name_mangled + "/_buf/" + sub_hex;

      raii::dds_TopicQos accel_tqos;
      raii::dds_Topic_get_qos(topic, accel_tqos);
      subscriber_info->accel_topic = dds_DomainParticipant_create_topic(
        participant,
        accel_topic_name.c_str(),
        type_name.c_str(),
        accel_tqos,
        nullptr,
        0);
      if (!subscriber_info->accel_topic) {
        RMW_SET_ERROR_MSG("create_subscription() failed to create accelerated channel topic");
        return nullptr;
      }
      raii::dds_DataReaderQos accel_rqos;
      raii::dds_DataReader_get_qos(topic_reader, accel_rqos);
      if (ret != dds_RETCODE_OK) {
        RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "fail_datareader_qos");
        // dds_DataReaderQos_finalize(&accel_rqos);
        return nullptr;
      }
      subscriber_info->accel_data_reader_listener.on_data_available = on_data_available_for_accel;

      subscriber_info->accel_data_reader = dds_Subscriber_create_datareader(
        sub,
        subscriber_info->accel_topic,
        accel_rqos,
        &subscriber_info->accel_data_reader_listener,
        dds_DATA_AVAILABLE_STATUS);
      if (!subscriber_info->accel_data_reader) {
        dds_DomainParticipant_delete_topic(participant, subscriber_info->accel_topic);
        subscriber_info->accel_topic = nullptr;

        RMW_SET_ERROR_MSG(
          "create_subscription() failed to create accelerated "
          "channel DataReader");
        return nullptr;
      }

      dds_DataReader_set_listener_context(subscriber_info->accel_data_reader, subscriber_info);
    }

    auto * backend_context =
      static_cast<const BufferBackendContext *>(subscriber_info->serialization_context);
    if (backend_context) {
      rosidl_buffer_backend_registry::notify_endpoint_created(
        backend_context->backend_instances,
        subscriber_info->local_endpoint_info);
    }

    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_gurumdds_cpp",
      "Created buffer-aware subscription on '%s' (mode: %s)",
      fastrtps_topic_name_mangled.c_str(),
      cpu_only ? "cpu-only" : "accelerated");
  }

  scope_exit_rmw_subscription_delete.cancel();

  TRACETOOLS_TRACEPOINT(
    rmw_subscription_init,
    rmw_subscription,
    subscriber_info->subscriber_gid.data);
  return rmw_subscription;
}

rmw_ret_t destroy_subscription(
  rmw_context_impl_t * const ctx,
  rmw_subscription_t * const subscription)
{
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  auto subscriber_info = static_cast<SubscriberInfo *>(subscription->data);
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("invalid subscriber data");
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret;

  // backend_buffer
  if (subscriber_info->is_buffer_aware) {
    {
      auto & state = *subscriber_info->buffer_state;
      std::lock_guard<std::mutex> lock(state.mutex);
      state.alive.store(false);
      state.publisher_metadata.clear();
    }

    auto * buf_registry = static_cast<BufferEndpointRegistry *>(ctx->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->unregister_callbacks(subscriber_info->subscriber_gid);
    }

    // cpu-channel
    if (subscriber_info->cpu_channel_reader != nullptr) {
      dds_DataReader_set_listener(subscriber_info->cpu_channel_reader, nullptr, 0);

      dds_DataReader_set_listener_context(subscriber_info->cpu_channel_reader, nullptr);

      ret = dds_Subscriber_delete_datareader(ctx->subscriber, subscriber_info->cpu_channel_reader);

      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete CPU channel datareader");
        return RMW_RET_ERROR;
      }

      subscriber_info->cpu_channel_reader = nullptr;
    }

    if (subscriber_info->cpu_topic != nullptr) {
      ret = dds_DomainParticipant_delete_topic(ctx->participant, subscriber_info->cpu_topic);

      if (ret == dds_RETCODE_PRECONDITION_NOT_MET) {
        RCUTILS_LOG_DEBUG_NAMED(
          RMW_GURUMDDS_ID,
          "The CPU channel topic is still used by another entity.");
      } else if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete CPU channel topic");
        return RMW_RET_ERROR;
      }

      subscriber_info->cpu_topic = nullptr;
    }
    // accel-channel
    if (subscriber_info->accel_data_reader != nullptr) {
      dds_DataReader_set_listener(subscriber_info->accel_data_reader, nullptr, 0);

      dds_DataReader_set_listener_context(subscriber_info->accel_data_reader, nullptr);

      ret = dds_Subscriber_delete_datareader(ctx->subscriber, subscriber_info->accel_data_reader);

      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete accelerated channel datareader");
        return RMW_RET_ERROR;
      }

      subscriber_info->accel_data_reader = nullptr;
    }

    if (subscriber_info->accel_topic != nullptr) {
      ret = dds_DomainParticipant_delete_topic(ctx->participant, subscriber_info->accel_topic);

      if (ret == dds_RETCODE_PRECONDITION_NOT_MET) {
        RCUTILS_LOG_DEBUG_NAMED(
          RMW_GURUMDDS_ID,
          "The accelerated channel topic is still used by another entity.");
      } else if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete accelerated channel topic");
        return RMW_RET_ERROR;
      }

      subscriber_info->accel_topic = nullptr;
    }
  }

  if (subscriber_info->topic_reader != nullptr) {
    dds_DataReader_set_listener(subscriber_info->topic_reader, nullptr, 0);
    dds_DataReader_set_listener_context(subscriber_info->topic_reader, nullptr);

    dds_Topic * topic = reinterpret_cast<dds_Topic *>(
      dds_DataReader_get_topicdescription(subscriber_info->topic_reader));

    ret = dds_DataReader_delete_readcondition(
      subscriber_info->topic_reader,
      subscriber_info->read_condition);
    if (dds_RETCODE_OK != ret) {
      RMW_SET_ERROR_MSG("failed to delete read condition");
      return RMW_RET_ERROR;
    }

    ret = dds_Subscriber_delete_datareader(ctx->subscriber, subscriber_info->topic_reader);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete datareader");
      return RMW_RET_ERROR;
    }

    TopicEventListener::remove_event(topic, subscriber_info);
    TopicEventListener::disassociate_Listener(topic);

    subscriber_info->topic_reader = nullptr;
    ret = dds_DomainParticipant_delete_topic(ctx->participant, topic);
    if (ret == dds_RETCODE_PRECONDITION_NOT_MET) {
      RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "The entity using the topic still exists.");
    } else if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete topic");
      return RMW_RET_ERROR;
    }
  }

  if (subscriber_info->buffer_data_guard != nullptr) {
    dds_GuardCondition_delete(subscriber_info->buffer_data_guard);
    subscriber_info->buffer_data_guard = nullptr;
  }

  for (auto & condition : subscriber_info->event_guard_cond) {
    if (condition != nullptr) {
      dds_GuardCondition_delete(condition);
      condition = nullptr;
    }
  }

  delete subscriber_info;
  subscription->data = nullptr;
  return RMW_RET_OK;
}

static rmw_ret_t take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, ros_message, taken);

  RCUTILS_UNUSED(allocation);
  *taken = false;

  CHECK_ID_CODE(subscription);

  auto subscriber_info = static_cast<SubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(subscriber_info, RMW_RET_ERROR);

  dds_DataReader * topic_reader = subscriber_info->topic_reader;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_reader, RMW_RET_ERROR);

  auto callbacks = subscriber_info->get_fastrtps_type_support_callbacks();
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(callbacks, RMW_RET_ERROR);

  auto * ctx = subscriber_info->ctx;

  raii::dds_DataSeq data_values = raii::dds_DataSeq_create(1);
  if (!data_values) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(1);
  if (!sample_infos) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (!sample_sizes) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    return RMW_RET_ERROR;
  }

  auto ret_loan = rcpputils::make_scope_exit([&]() {
        dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
  });

  dds_ReturnCode_t ret = dds_DataReader_raw_take_w_sampleinfoex(
    topic_reader,
    dds_HANDLE_NIL,
    data_values,
    sample_infos,
    sample_sizes,
    1,
    dds_ANY_SAMPLE_STATE,
    dds_ANY_VIEW_STATE,
    dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    return RMW_RET_ERROR;
  }

  // dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  dds_SampleInfoEx * sample_info_ex =
    reinterpret_cast<dds_SampleInfoEx *>(dds_SampleInfoSeq_get(sample_infos, 0));

  if (sample_info_ex == nullptr) {
    RMW_SET_ERROR_MSG("failed to get sample info");
    return RMW_RET_ERROR;
  }

  dds_SampleInfo * sample_info = &sample_info_ex->info;

  if (sample_info->valid_data) {
    if (
      is_from_local_publication(
        ctx,
        subscriber_info,
        topic_reader,
        sample_info_ex))
    {
      *taken = false;
      return RMW_RET_OK;
    }

    void * sample = dds_DataSeq_get(data_values, 0);
    if (sample == nullptr) {
      RMW_SET_ERROR_MSG("failed to take data");
      return RMW_RET_ERROR;
    }

    uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);

    eprosima::fastcdr::FastBuffer fastbuffer(
      static_cast<char *>(sample),
      static_cast<size_t>(sample_size));

    eprosima::fastcdr::Cdr deser(
      fastbuffer,
      eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::CdrVersion::XCDRv1);

    deser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

    deser.read_encapsulation();

    if (!callbacks->cdr_deserialize(deser, ros_message)) {
      RMW_SET_ERROR_MSG("failed to deserialize ROS message with FastRTPS typesupport");
      return RMW_RET_ERROR;
    }

    *taken = true;

    if (message_info != nullptr) {
      int64_t sequence_number = 0;
      // dds_SampleInfoEx * sampleinfo_ex =
      //   reinterpret_cast<dds_SampleInfoEx *>(sample_info);

      rmw_gurumdds_cpp::dds_sn_to_ros_sn(sample_info_ex->seq, &sequence_number);

      message_info->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;

      message_info->received_timestamp =
        sample_info_ex->reception_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info_ex->reception_timestamp.nanosec;

      message_info->publication_sequence_number = sequence_number;
      message_info->reception_sequence_number = RMW_MESSAGE_INFO_SEQUENCE_NUMBER_UNSUPPORTED;

      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = subscription->implementation_identifier;
      std::memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);

      dds_ReturnCode_t gid_ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader,
        sample_info->publication_handle,
        sender_gid->data);

      if (gid_ret != dds_RETCODE_OK) {
        if (gid_ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED(RMW_GURUMDDS_ID, "Failed to get publication handle");
        }
        std::memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      }
    }
  }

  TRACETOOLS_TRACEPOINT(
    rmw_take,
    static_cast<const void *>(subscription),
    static_cast<const void *>(ros_message),
    (message_info ? message_info->source_timestamp : 0LL),
    *taken);

  return RMW_RET_OK;
}

static rmw_ret_t take_serialized(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, serialized_message, taken);

  RCUTILS_UNUSED(allocation);
  *taken = false;

  CHECK_ID_CODE(subscription);

  auto subscriber_info = static_cast<SubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(subscriber_info, RMW_RET_ERROR);

  dds_DataReader * topic_reader = subscriber_info->topic_reader;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(topic_reader, RMW_RET_ERROR);

  auto * ctx = subscriber_info->ctx;

  raii::dds_DataSeq data_values = raii::dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    return RMW_RET_ERROR;
  }

  auto ret_loan = rcpputils::make_scope_exit([&]() {
        dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
  });

  dds_ReturnCode_t ret = dds_DataReader_raw_take_w_sampleinfoex(
    topic_reader,
    dds_HANDLE_NIL,
    data_values,
    sample_infos,
    sample_sizes,
    1,
    dds_ANY_SAMPLE_STATE,
    dds_ANY_VIEW_STATE,
    dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "No data on topic %s", subscription->topic_name);
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Received data on topic %s", subscription->topic_name);

  // dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  dds_SampleInfoEx * sample_info_ex =
    reinterpret_cast<dds_SampleInfoEx *>(dds_SampleInfoSeq_get(sample_infos, 0));

  if (sample_info_ex == nullptr) {
    RMW_SET_ERROR_MSG("failed to get sample info");
    return RMW_RET_ERROR;
  }

  dds_SampleInfo * sample_info = &sample_info_ex->info;

  if (sample_info->valid_data) {
    if (is_from_local_publication(ctx, subscriber_info, topic_reader, sample_info_ex)) {
      *taken = false;

      return RMW_RET_OK;
    }

    void * sample = dds_DataSeq_get(data_values, 0);
    if (sample == nullptr) {
      RMW_SET_ERROR_MSG("failed to take data");
      return RMW_RET_ERROR;
    }

    uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);
    serialized_message->buffer_length = sample_size;
    if (serialized_message->buffer_capacity < sample_size) {
      rmw_ret_t rmw_ret = rmw_serialized_message_resize(serialized_message, sample_size);
      if (rmw_ret != RMW_RET_OK) {
        return rmw_ret;
      }
    }

    std::memcpy(serialized_message->buffer, sample, sample_size);

    *taken = true;

    if (message_info != nullptr) {
      int64_t sequence_number = 0;
      dds_SampleInfoEx * sampleinfo_ex = reinterpret_cast<dds_SampleInfoEx *>(sample_info);
      rmw_gurumdds_cpp::dds_sn_to_ros_sn(sampleinfo_ex->seq, &sequence_number);
      message_info->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;
      message_info->received_timestamp =
        sampleinfo_ex->reception_timestamp.sec * static_cast<int64_t>(1000000000) +
        sampleinfo_ex->reception_timestamp.nanosec;
      message_info->publication_sequence_number = sequence_number;
      message_info->reception_sequence_number = RMW_MESSAGE_INFO_SEQUENCE_NUMBER_UNSUPPORTED;
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = RMW_GURUMDDS_ID;
      std::memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
        topic_reader,
        sample_info->publication_handle,
        sender_gid->data);
      if (ret != dds_RETCODE_OK) {
        if (ret == dds_RETCODE_ERROR) {
          RCUTILS_LOG_WARN_NAMED(RMW_GURUMDDS_ID, "Failed to get publication handle");
        }
        std::memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      }
    }
  }

  TRACETOOLS_TRACEPOINT(
    rmw_take,
    static_cast<const void *>(subscription),
    static_cast<const void *>(serialized_message),
    (message_info ? message_info->source_timestamp : 0LL),
    *taken);

  return RMW_RET_OK;
}

static rmw_ret_t take_sequence(
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, message_sequence, message_info_sequence, taken);

  CHECK_ID_CODE(subscription);

  RCUTILS_UNUSED(allocation);

  if (!check_message_seq(count, message_sequence, message_info_sequence)) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  *taken = 0;

  // Reset length of output sequences
  message_sequence->size = 0;
  message_info_sequence->size = 0;

  auto * info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  // fastrtps typesupport callbacks
  auto * callbacks = info->get_fastrtps_type_support_callbacks();
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    callbacks,
    "type support callbacks is null",
    return RMW_RET_ERROR);

  dds_DataReader * topic_reader = info->topic_reader;
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(topic_reader, "topic reader is null", return RMW_RET_ERROR);

  raii::dds_DataSeq data_values = raii::dds_DataSeq_create(count);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(count);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(count);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    return RMW_RET_ERROR;
  }

  auto ret_loan = rcpputils::make_scope_exit([&]() {
        dds_DataReader_raw_return_loan(topic_reader, data_values, sample_infos, sample_sizes);
  });

  while (*taken < count) {
    dds_ReturnCode_t ret = dds_DataReader_raw_take(
      topic_reader,
      dds_HANDLE_NIL,
      data_values,
      sample_infos,
      sample_sizes,
      count,
      dds_ANY_SAMPLE_STATE,
      dds_ANY_VIEW_STATE,
      dds_ANY_INSTANCE_STATE);

    if (ret == dds_RETCODE_NO_DATA) {
      RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "No data on topic %s", subscription->topic_name);
      break;
    }

    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to take data");
      return RMW_RET_ERROR;
    }

    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Received data on topic %s", subscription->topic_name);

    const uint32_t length = dds_SampleInfoSeq_length(sample_infos);
    for (uint32_t i = 0; i < length; i++) {
      dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, i);

      if (sample_info->valid_data) {
        void * sample = dds_DataSeq_get(data_values, i);
        if (sample == nullptr) {
          RMW_SET_ERROR_MSG("failed to get message");
          return RMW_RET_ERROR;
        }
        uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, i);

        // 역 직렬화
        eprosima::fastcdr::FastBuffer fastbuffer(
          static_cast<char *>(sample),
          static_cast<size_t>(sample_size));

        eprosima::fastcdr::Cdr deser(
          fastbuffer,
          eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
          eprosima::fastcdr::CdrVersion::XCDRv1);

        deser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

        deser.read_encapsulation();

        if (!callbacks->cdr_deserialize(deser, message_sequence->data[*taken])) {
          RMW_SET_ERROR_MSG("failed to deserialize message");
          return RMW_RET_ERROR;
        }

        auto message_info = &(message_info_sequence->data[*taken]);
        auto sampleinfo_ex = reinterpret_cast<dds_SampleInfoEx *>(sample_info);

        message_info->source_timestamp =
          sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
          sample_info->source_timestamp.nanosec;
        message_info->received_timestamp =
          sampleinfo_ex->reception_timestamp.sec * static_cast<int64_t>(1000000000) +
          sampleinfo_ex->reception_timestamp.nanosec;
        rmw_gid_t * sender_gid = &message_info->publisher_gid;
        sender_gid->implementation_identifier = RMW_GURUMDDS_ID;
        std::memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);

        dds_ReturnCode_t ret = dds_DataReader_get_guid_from_publication_handle(
          topic_reader,
          sample_info->publication_handle,
          sender_gid->data);
        if (ret != dds_RETCODE_OK) {
          if (ret == dds_RETCODE_ERROR) {
            RCUTILS_LOG_WARN_NAMED(RMW_GURUMDDS_ID, "Failed to get publication handle");
          }
          std::memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
        }

        (*taken)++;
      }
    }
  }

  message_sequence->size = *taken;
  message_info_sequence->size = *taken;

  return RMW_RET_OK;
}

// backend_buffer
static rmw_ret_t take_buffer_aware(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  CHECK_ID_CODE(subscription);

  *taken = false;

  auto info = static_cast<SubscriberInfo *>(subscription->data);
  auto callbacks = info->get_fastrtps_type_support_callbacks();
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(callbacks, RMW_RET_ERROR);

  // CPU-only path: take from the shared CPU channel DataReader.
  if (info->cpu_channel_reader) {
    raii::dds_DataSeq data_values = raii::dds_DataSeq_create(1);
    if (data_values == nullptr) {
      RMW_SET_ERROR_MSG("failed to create data sequence");
      return RMW_RET_ERROR;
    }

    raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(1);
    if (sample_infos == nullptr) {
      RMW_SET_ERROR_MSG("failed to create sample info sequence");
      dds_DataSeq_delete(data_values);
      return RMW_RET_ERROR;
    }

    raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(1);
    if (sample_sizes == nullptr) {
      RMW_SET_ERROR_MSG("failed to create sample size sequence");
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      return RMW_RET_ERROR;
    }

    auto ret_loan = rcpputils::make_scope_exit([&]() {
          dds_DataReader_raw_return_loan(
        info->cpu_channel_reader,
        data_values,
        sample_infos,
        sample_sizes);
    });

    dds_ReturnCode_t ret = dds_DataReader_raw_take_w_sampleinfoex(
      info->cpu_channel_reader,
      dds_HANDLE_NIL,
      data_values,
      sample_infos,
      sample_sizes,
      1,
      dds_ANY_SAMPLE_STATE,
      dds_ANY_VIEW_STATE,
      dds_ANY_INSTANCE_STATE);

    if (ret == dds_RETCODE_OK) {
      dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

      if (sample_info->valid_data) {
        void * sample = dds_DataSeq_get(data_values, 0);
        if (sample == nullptr) {
          RMW_SET_ERROR_MSG("failed to take data");
          return RMW_RET_ERROR;
        }

        uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);

        eprosima::fastcdr::FastBuffer fastbuffer(
          static_cast<char *>(sample),
          static_cast<size_t>(sample_size));

        eprosima::fastcdr::Cdr deser(
          fastbuffer,
          eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
          eprosima::fastcdr::CdrVersion::XCDRv1);

        deser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

        deser.read_encapsulation();

        if (!callbacks->cdr_deserialize(deser, ros_message)) {
          RMW_SET_ERROR_MSG("failed to deserialize ROS message with FastRTPS typesupport");
          return RMW_RET_ERROR;
        }

        if (
          count_unread(info->cpu_channel_reader, data_values, sample_infos, sample_sizes) > 0 &&
          info->buffer_data_guard)
        {
          dds_GuardCondition_set_trigger_value(info->buffer_data_guard, true);
        }
        *taken = true;

        TRACETOOLS_TRACEPOINT(
          rmw_take,
          static_cast<const void *>(subscription),
          static_cast<const void *>(ros_message),
          (message_info ? message_info->source_timestamp : 0LL),
          *taken);

        return RMW_RET_OK;
      }
    }
  }
  // Accelerated path: single shared DataReader for all buffer-aware publishers.
  if (!info->accel_data_reader) {
    return RMW_RET_OK;
  }

  raii::dds_DataSeq data_values = raii::dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    return RMW_RET_ERROR;
  }

  auto ret_loan = rcpputils::make_scope_exit([&]() {
        dds_DataReader_raw_return_loan(
      info->accel_data_reader,
      data_values,
      sample_infos,
      sample_sizes);
  });

  dds_ReturnCode_t ret = dds_DataReader_raw_take_w_sampleinfoex(
    info->accel_data_reader,
    dds_HANDLE_NIL,
    data_values,
    sample_infos,
    sample_sizes,
    1,
    dds_ANY_SAMPLE_STATE,
    dds_ANY_VIEW_STATE,
    dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    RMW_SET_ERROR_MSG("failed to no data");
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    return RMW_RET_ERROR;
  }

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  if (!sample_info->valid_data) {
    return RMW_RET_OK;
  }

  dds_PublicationBuiltinTopicData pub_data{};
  if (
    dds_DataReader_get_matched_publication_data(
      info->accel_data_reader,
      &pub_data,
      sample_info->publication_handle) != dds_RETCODE_OK)
  {
    RMW_SET_ERROR_MSG("failed to get matched publication data");
    return RMW_RET_ERROR;
  }

  rmw_gid_t writer_gid{};
  rmw_gurumdds_cpp::Guid_t writer_guid(pub_data);
  rmw_gurumdds_cpp::guid_to_gid(writer_guid, writer_gid);

  std::string writer_hex = gid_to_hex(writer_gid);

  rmw_topic_endpoint_info_t pub_endpoint_info = rmw_get_zero_initialized_topic_endpoint_info();
  pub_endpoint_info.endpoint_type = RMW_ENDPOINT_PUBLISHER;
  std::memcpy(pub_endpoint_info.endpoint_gid, writer_gid.data, RMW_GID_STORAGE_SIZE);

  {
    std::lock_guard<std::mutex> lock(info->buffer_state->mutex);
    auto it = info->buffer_state->publisher_metadata.find(writer_hex);
    if (it != info->buffer_state->publisher_metadata.end()) {
      pub_endpoint_info = it->second.publisher_endpoint_info;
    }
  }

  void * sample = dds_DataSeq_get(data_values, 0);
  if (sample == nullptr) {
    RMW_SET_ERROR_MSG("failed to take data");
    return RMW_RET_ERROR;
  }

  uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);

  eprosima::fastcdr::FastBuffer fastbuffer(
    static_cast<char *>(sample),
    static_cast<size_t>(sample_size));

  eprosima::fastcdr::Cdr deser(
    fastbuffer,
    eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::CdrVersion::XCDRv1);

  deser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

  auto * backend_context = static_cast<const BufferBackendContext *>(info->serialization_context);
  if (!backend_context) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_cpp",
      "Buffer-aware deserialize missing buffer backend context");
    return RMW_RET_OK;
  }

  deser.read_encapsulation();

  bool deser_ok = false;
  deser_ok = callbacks->cdr_deserialize_with_endpoint(
    deser,
    ros_message,
    pub_endpoint_info,
    backend_context->serialization_context);

  if (!deser_ok) {
    RMW_SET_ERROR_MSG("failed to deserialize ROS message with FastRTPS typesupport");
    return RMW_RET_ERROR;
  }

  *taken = true;

  if (
    count_unread(info->accel_data_reader, data_values, sample_infos, sample_sizes) > 0 &&
    info->buffer_data_guard)
  {
    dds_GuardCondition_set_trigger_value(info->buffer_data_guard, true);
  }

  TRACETOOLS_TRACEPOINT(
    rmw_take,
    static_cast<const void *>(subscription),
    static_cast<const void *>(ros_message),
    (message_info ? message_info->source_timestamp : 0LL),
    *taken);

  return RMW_RET_OK;
}

static rmw_ret_t take_buffer_aware_serialized(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  CHECK_ALL_PTRS_CODE(subscription, serialized_message);
  CHECK_ID_CODE(subscription);

  *taken = false;
  auto info = static_cast<SubscriberInfo *>(subscription->data);

  if (!info->cpu_channel_reader) {
    return RMW_RET_OK;
  }

  dds_DataReader * cpu_reader = info->cpu_channel_reader;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(cpu_reader, RMW_RET_ERROR);

  raii::dds_DataSeq data_values = raii::dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    return RMW_RET_ERROR;
  }

  auto ret_loan = rcpputils::make_scope_exit(
    [&]() {dds_DataReader_raw_return_loan(cpu_reader, data_values, sample_infos, sample_sizes);});

  dds_ReturnCode_t ret = dds_DataReader_raw_take_w_sampleinfoex(
    cpu_reader,
    dds_HANDLE_NIL,
    data_values,
    sample_infos,
    sample_sizes,
    1,
    dds_ANY_SAMPLE_STATE,
    dds_ANY_VIEW_STATE,
    dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "No data on topic %s", subscription->topic_name);
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "Received data on topic %s", subscription->topic_name);

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);

  if (sample_info->valid_data) {
    void * sample = dds_DataSeq_get(data_values, 0);
    if (sample == nullptr) {
      RMW_SET_ERROR_MSG("failed to take data");
      return RMW_RET_ERROR;
    }

    uint32_t sample_size = dds_UnsignedLongSeq_get(sample_sizes, 0);
    serialized_message->buffer_length = sample_size;
    if (serialized_message->buffer_capacity < sample_size) {
      rmw_ret_t rmw_ret = rmw_serialized_message_resize(serialized_message, sample_size);
      if (rmw_ret != RMW_RET_OK) {
        // Error message already set
        return rmw_ret;
      }
    }

    std::memcpy(serialized_message->buffer, sample, sample_size);

    *taken = true;
  }

  if (
    count_unread(info->cpu_channel_reader, data_values, sample_infos, sample_sizes) > 0 &&
    info->buffer_data_guard)
  {
    dds_GuardCondition_set_trigger_value(info->buffer_data_guard, true);
  }

  TRACETOOLS_TRACEPOINT(
    rmw_take,
    static_cast<const void *>(subscription),
    static_cast<const void *>(serialized_message),
    (message_info ? message_info->source_timestamp : 0LL),
    *taken);

  return RMW_RET_OK;
}
}  // namespace rmw_gurumdds_cpp

extern "C" {
rmw_ret_t rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_UNUSED(type_support);
  RCUTILS_UNUSED(message_bounds);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_init_subscription_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_fini_subscription_allocation(rmw_subscription_allocation_t * allocation)
{
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_fini_subscription_allocation is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_subscription_t * rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options)
{
  CHECK_ALL_PTRS_NULL(node, type_supports, topic_name, qos_policies, subscription_options);
  CHECK_ID_NULL(node);

  if (strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("topic_name argument is empty");
    return nullptr;
  }

  // Adapt any 'best available' QoS options
  rmw_qos_profile_t adapted_qos_policies = *qos_policies;
  rmw_ret_t ret = rmw_dds_common::qos_profile_get_best_available_for_topic_subscription(
    node,
    topic_name,
    &adapted_qos_policies,
    rmw_get_publishers_info_by_topic);
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
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("topic name is invalid: %s", reason);
      return nullptr;
    }
  }

  if (
    subscription_options->require_unique_network_flow_endpoints ==
    RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED)
  {
    RMW_SET_ERROR_MSG("Unique network flow endpoints not supported on subscriptions");
    return nullptr;
  }

  rmw_context_impl_t * ctx = node->context->impl;

  rmw_subscription_t * const rmw_sub = rmw_gurumdds_cpp::create_subscription(
    ctx,
    node,
    ctx->participant,
    ctx->subscriber,
    type_supports,
    topic_name,
    &adapted_qos_policies,
    subscription_options,
    RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST ==
      ctx->base->options.discovery_options.automatic_discovery_range);

  if (rmw_sub == nullptr) {
    RMW_SET_ERROR_MSG("failed to create RMW subscription");
    return nullptr;
  }

  auto * info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(rmw_sub->data);
  // backend_buffer
  //  Register buffer-aware publisher discovery callback.
  //  CPU-only subscriptions use the shared CPU channel and don't need
  //  per-publisher metadata tracking, so skip the callback for them.
  if (info->is_buffer_aware && !info->is_cpu_only) {
    auto state = info->buffer_state;
    auto * guard = info->buffer_data_guard;
    auto * backend_context =
      static_cast<const rmw_gurumdds_cpp::BufferBackendContext *>(info->serialization_context);
    auto * buf_registry = static_cast<rmw_gurumdds_cpp::BufferEndpointRegistry *>(
      node->context->impl->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->register_publisher_discovery_callback(
        rmw_sub->topic_name,
        info->subscriber_gid,
        [state, guard, backend_context](const rmw_gurumdds_cpp::BufferEndpointInfo & pub_info) {
          if (!state->alive.load()) {
            return;
          }

          std::string pub_hex = rmw_gurumdds_cpp::gid_to_hex(pub_info.gid);

          {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (!state->alive.load()) {
              return;
            }
            if (state->publisher_metadata.count(pub_hex) > 0) {
              return;
            }

            rmw_gurumdds_cpp::PublisherBufferMetadata meta;
            meta.publisher_gid = pub_info.gid;
            meta.publisher_endpoint_info = rmw_get_zero_initialized_topic_endpoint_info();
            meta.publisher_endpoint_info.endpoint_type = RMW_ENDPOINT_PUBLISHER;
            std::memcpy(
              meta.publisher_endpoint_info.endpoint_gid,
              pub_info.gid.data,
              RMW_GID_STORAGE_SIZE);
            meta.backend_metadata = pub_info.backend_metadata;

            if (backend_context) {
              std::vector<rmw_topic_endpoint_info_t> existing_endpoints;
              existing_endpoints.reserve(state->publisher_metadata.size());
              for (const auto & [hex, m] : state->publisher_metadata) {
                existing_endpoints.push_back(m.publisher_endpoint_info);
              }

              std::unordered_map<std::string, std::vector<std::set<uint32_t>>>
              backend_endpoint_groups;
              (void)rosidl_buffer_backend_registry::notify_endpoint_discovered(
                backend_context->backend_instances,
                meta.publisher_endpoint_info,
                existing_endpoints,
                backend_endpoint_groups,
                pub_info.backend_metadata);
            }

            state->publisher_metadata[pub_hex] = std::move(meta);

            if (guard) {
              dds_GuardCondition_set_trigger_value(guard, true);
            }
          }

          RCUTILS_LOG_DEBUG_NAMED(
            "rmw_gurumdds_cpp",
            "Buffer subscription: publisher discovered, recorded '%s'",
            pub_hex.c_str());
        });
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "Created subscription with topic '%s' on node '%s%s%s'",
    topic_name,
    node->namespace_,
    node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/",
    node->name);

  return rmw_sub;
}

rmw_ret_t rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  CHECK_ALL_PTRS_CODE(subscription, publisher_count);
  CHECK_ID_CODE(subscription);

  auto subscriber_info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("subscriber internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataReader * topic_reader = subscriber_info->topic_reader;
  if (topic_reader == nullptr) {
    RMW_SET_ERROR_MSG("topic reader is null");
    return RMW_RET_ERROR;
  }

  raii::dds_InstanceHandleSeq seq = raii::dds_InstanceHandleSeq_create(4);
  if (dds_DataReader_get_matched_publications(topic_reader, seq) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get matched publications");
    return RMW_RET_ERROR;
  }

  *publisher_count = static_cast<size_t>(dds_InstanceHandleSeq_length(seq));

  return RMW_RET_OK;
}

rmw_ret_t rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  CHECK_ALL_PTRS_CODE(subscription, qos);
  CHECK_ID_CODE(subscription);

  auto subscriber_info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("subscription internal data is invalid");
    return RMW_RET_ERROR;
  }

  dds_DataReader * topic_reader = subscriber_info->topic_reader;
  if (topic_reader == nullptr) {
    RMW_SET_ERROR_MSG("subscription internal data reader is invalid");
    return RMW_RET_ERROR;
  }

  raii::dds_DataReaderQos dds_qos;
  dds_ReturnCode_t ret = dds_DataReader_get_qos(topic_reader, dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("subscription can't get data reader qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability = rmw_gurumdds_cpp::convert_reliability(&dds_qos->reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos->durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos->deadline);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos->liveliness);
  qos->liveliness_lease_duration =
    rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos->liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos->history);
  qos->depth = static_cast<size_t>(dds_qos->history.depth);

  return RMW_RET_OK;
}

rmw_ret_t rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  CHECK_ALL_PTRS_CODE(node, subscription);
  CHECK_ID_CODE(node);
  CHECK_ID_CODE(subscription);

  rmw_context_impl_t * ctx = node->context->impl;

  if (
    rmw_gurumdds_cpp::graph_cache::on_subscriber_deleted(
      ctx,
      node,
      reinterpret_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data)))
  {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to update graph for subscriber");
    return RMW_RET_ERROR;
  }

  rmw_ret_t ret = rmw_gurumdds_cpp::destroy_subscription(ctx, subscription);

  if (ret == RMW_RET_OK) {
    if (subscription->topic_name != nullptr) {
      RCUTILS_LOG_DEBUG_NAMED(
        RMW_GURUMDDS_ID,
        "Deleted subscriber with topic '%s' on node '%s%s%s'",
        subscription->topic_name,
        node->namespace_,
        node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/",
        node->name);

      rmw_free(const_cast<char *>(subscription->topic_name));
    }

    rmw_subscription_free(subscription);
  }

  return ret;
}

rmw_ret_t rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, ros_message, taken);

  auto info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (info->is_buffer_aware) {
    rmw_ret_t ret = rmw_gurumdds_cpp::take_buffer_aware(subscription, ros_message, taken, nullptr);
    if (ret != RMW_RET_OK || *taken) {
      return ret;
    }
    // No data from buffer endpoints; fall back to the main DataReader for
    // messages published by non-buffer-aware publishers (e.g. cross-RMW).
    return rmw_gurumdds_cpp::take(subscription, ros_message, taken, nullptr, allocation);
  }

  return rmw_gurumdds_cpp::take(subscription, ros_message, taken, nullptr, allocation);
}

rmw_ret_t rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, ros_message, taken, message_info);

  auto info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (info->is_buffer_aware) {
    rmw_ret_t ret =
      rmw_gurumdds_cpp::take_buffer_aware(subscription, ros_message, taken, message_info);
    if (ret != RMW_RET_OK || *taken) {
      return ret;
    }

    return rmw_gurumdds_cpp::take(subscription, ros_message, taken, message_info, allocation);
  }

  return rmw_gurumdds_cpp::take(subscription, ros_message, taken, message_info, allocation);
}

// 수정 필요
rmw_ret_t rmw_take_sequence(
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, message_sequence, message_info_sequence, taken);

  RCUTILS_UNUSED(allocation);

  return rmw_gurumdds_cpp::take_sequence(
    subscription,
    count,
    message_sequence,
    message_info_sequence,
    taken,
    allocation);
}

rmw_ret_t rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, serialized_message, taken);

  auto info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (info->is_buffer_aware) {
    rmw_ret_t ret = rmw_gurumdds_cpp::take_buffer_aware_serialized(
      subscription,
      serialized_message,
      taken,
      nullptr);
    if (ret != RMW_RET_OK || *taken) {
      return ret;
    }

    return rmw_gurumdds_cpp::take_serialized(
      subscription,
      serialized_message,
      taken,
      nullptr,
      allocation);
  }

  return rmw_gurumdds_cpp::take_serialized(
    subscription,
    serialized_message,
    taken,
    nullptr,
    allocation);
}

rmw_ret_t rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  CHECK_ALL_PTRS_CODE(subscription, serialized_message, taken, message_info);

  auto info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (info->is_buffer_aware) {
    rmw_ret_t ret = rmw_gurumdds_cpp::take_buffer_aware_serialized(
      subscription,
      serialized_message,
      taken,
      nullptr);
    if (ret != RMW_RET_OK || *taken) {
      return ret;
    }

    return rmw_gurumdds_cpp::take_serialized(
      subscription,
      serialized_message,
      taken,
      nullptr,
      allocation);
  }

  return rmw_gurumdds_cpp::take_serialized(
    subscription,
    serialized_message,
    taken,
    message_info,
    allocation);
}

rmw_ret_t rmw_take_loaned_message(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_take_loaned_message is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_loaned_message_with_info(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(message_info);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_take_loaned_message_with_info is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_return_loaned_message_from_subscription(
  const rmw_subscription_t * subscription,
  void * loaned_message)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(loaned_message);

  RMW_SET_ERROR_MSG("rmw_return_loaned_message_from_subscription is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_set_on_new_message_callback(
  rmw_subscription_t * subscription,
  rmw_event_callback_t callback,
  const void * user_data)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(subscription);

  auto subscriber_info = static_cast<rmw_gurumdds_cpp::SubscriberInfo *>(subscription->data);
  if (subscriber_info == nullptr) {
    RMW_SET_ERROR_MSG("invalid subscription data");
    return RMW_RET_ERROR;
  }

  std::lock_guard<std::mutex> guard(subscriber_info->event_callback_data.mutex);
  dds_ReturnCode_t dds_rc = dds_RETCODE_ERROR;

  if (callback) {
    size_t unread_count = subscriber_info->count_unread();
    if (0 < unread_count) {
      callback(user_data, unread_count);
    }

    subscriber_info->mask |= dds_DATA_AVAILABLE_STATUS;
    subscriber_info->event_callback_data.callback = callback;
    subscriber_info->event_callback_data.user_data = user_data;
  } else {
    subscriber_info->event_callback_data.callback = nullptr;
    subscriber_info->event_callback_data.user_data = nullptr;
    subscriber_info->mask &= ~dds_DATA_AVAILABLE_STATUS;
  }

  dds_rc = dds_DataReader_set_listener(
    subscriber_info->topic_reader,
    &subscriber_info->topic_listener,
    subscriber_info->mask);

  return rmw_gurumdds_cpp::check_dds_ret_code(dds_rc);
}

rmw_ret_t rmw_subscription_set_content_filter(
  rmw_subscription_t * subscription,
  const rmw_subscription_content_filter_options_t * options)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(options);

  RMW_SET_ERROR_MSG("rmw_subscription_set_content_filter is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_subscription_get_content_filter(
  const rmw_subscription_t * subscription,
  rcutils_allocator_t * allocator,
  rmw_subscription_content_filter_options_t * options)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(options);

  RMW_SET_ERROR_MSG("rmw_subscription_get_content_filter is not supported");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_dynamic_message(
  const rmw_subscription_t * subscription,
  rosidl_dynamic_typesupport_dynamic_data_t * dynamic_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(dynamic_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_take_dynamic_message: unimplemented");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_take_dynamic_message_with_info(
  const rmw_subscription_t * subscription,
  rosidl_dynamic_typesupport_dynamic_data_t * dynamic_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_UNUSED(subscription);
  RCUTILS_UNUSED(dynamic_message);
  RCUTILS_UNUSED(taken);
  RCUTILS_UNUSED(message_info);
  RCUTILS_UNUSED(allocation);

  RMW_SET_ERROR_MSG("rmw_take_dynamic_message_with_info: unimplemented");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t rmw_serialization_support_init(
  const char * serialization_lib_name,
  rcutils_allocator_t * allocator,
  rosidl_dynamic_typesupport_serialization_support_t * serialization_support)
{
  RCUTILS_UNUSED(serialization_lib_name);
  RCUTILS_UNUSED(allocator);
  RCUTILS_UNUSED(serialization_support);

  RMW_SET_ERROR_MSG("rmw_serialization_support_init: unimplemented");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

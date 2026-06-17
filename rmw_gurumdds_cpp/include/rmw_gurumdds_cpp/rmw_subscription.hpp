// Copyright 2022 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_
#define RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_

#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>

#include "rmw/topic_endpoint_info.h"
#include "rmw/types.h"
#include "rmw_gurumdds_cpp/raii.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"

namespace rmw_gurumdds_cpp
{

// backend_buffer / subscription side
struct PublisherBufferMetadata
{
  rmw_gid_t publisher_gid{};
  rmw_topic_endpoint_info_t publisher_endpoint_info{};
  std::unordered_map<std::string, std::string> backend_metadata;
};

struct BufferSubscriptionState
{
  std::atomic<bool> alive{true};
  std::mutex mutex;
  /// GID hex string -> publisher metadata, populated by discovery callback.
  std::unordered_map<std::string, PublisherBufferMetadata> publisher_metadata;
};

struct SeqStruct
{
  // dds_DataSeq * data_seq{nullptr};
  // dds_SampleInfoSeq * info_seq{nullptr};
  // dds_UnsignedLongSeq * raw_data_sizes{nullptr};
  raii::dds_DataSeq data_seq;
  raii::dds_SampleInfoSeq info_seq;
  raii::dds_UnsignedLongSeq raw_data_sizes;
};

struct SubscriberInfo : EventInfo
{
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const rosidl_message_type_support_t * fastrtps_message_typesupport;

  const message_type_support_callbacks_t *
  get_fastrtps_type_support_callbacks() const;

  // backend_buffer
  bool is_buffer_aware{false};
  bool is_cpu_only{false};
  std::vector<std::string> my_backend_types;
  rmw_topic_endpoint_info_t local_endpoint_info{};
  const void * serialization_context{nullptr};
  std::shared_ptr<BufferSubscriptionState> buffer_state{
    std::make_shared<BufferSubscriptionState>()};
  /// Guard condition triggered when buffer channel DataReaders receive data.
  /// Used by rmw_wait to detect data on buffer-aware subscriptions.
  dds_GuardCondition * buffer_data_guard;

  // cpu channel
  dds_GuardCondition * cpu_guard;
  dds_DataReader * cpu_channel_reader = nullptr;
  dds_DataReaderListener cpu_channel_listener = {};
  dds_Topic * cpu_topic = nullptr;
  SeqStruct cpu_seq;

  // Accelerated shared channel reader (all buffer-aware publishers write here)
  dds_DataReader * accel_data_reader{nullptr};
  dds_Topic * accel_topic{nullptr};
  dds_DataReaderListener accel_data_reader_listener{};
  SeqStruct accel_seq;

  // cpu channel member function
  void on_cpu_channel_data_available();
  // accel channel member function
  void on_accel_data_available();

  event_callback_data_t buffer_event_callback_data;

  // fastrtps에서 토픽 이름을 캐싱해두는 것을 모방함.
  // 문자열에 ros prefix가 포함되어 있음에 주의.
  // 예시 : "/rr/foobar"
  std::string fastrtps_topic_name_mangled;

  bool ignore_local_publications = false;

  const char * implementation_identifier;
  rmw_context_impl_t * ctx;
  std::mutex mutex_event;
  rmw_event_callback_t on_new_event_cb[RMW_EVENT_TYPE_MAX] = {};
  const void * user_data_cb[RMW_EVENT_TYPE_MAX] = {};
  dds_GuardCondition * event_guard_cond[RMW_EVENT_TYPE_MAX] = {};
  dds_StatusMask mask = 0;
  bool requested_deadline_missed_changed = false;
  dds_RequestedDeadlineMissedStatus requested_deadline_missed_status = {};
  bool requested_incompatible_qos_changed = false;
  dds_RequestedIncompatibleQosStatus requested_incompatible_qos_status = {};
  bool inconsistent_topic_changed = false;
  dds_InconsistentTopicStatus inconsistent_topic_status = {};
  bool liveliness_changed = false;
  dds_LivelinessChangedStatus liveliness_changed_status = {};
  bool subscription_matched_changed = false;
  dds_SubscriptionMatchedStatus subscription_matched_status = {};
  bool sample_lost_changed = false;
  dds_SampleLostStatus sample_lost_status = {};

  rmw_gid_t subscriber_gid;
  dds_DataReader * topic_reader;
  dds_ReadCondition * read_condition;

  dds_DataReaderListener topic_listener;
  raii::dds_DataSeq data_seq;
  raii::dds_SampleInfoSeq info_seq;
  raii::dds_UnsignedLongSeq raw_data_sizes;
  event_callback_data_t event_callback_data;

  rmw_ret_t get_status(rmw_event_type_t event_type, void * event) override;

  dds_StatusCondition * get_status_condition() override;

  dds_GuardCondition * get_guard_condition(rmw_event_type_t event_type) override;

  bool is_status_changed(rmw_event_type_t event_type) override;

  bool has_callback(rmw_event_type_t event_type) override;

  bool has_callback_unsafe(rmw_event_type_t event_type) const;

  rmw_ret_t set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void * user_data,
    rmw_event_callback_t callback) override;

  void update_inconsistent_topic(int32_t total_count, int32_t total_count_change) override;

  void on_requested_deadline_missed(const dds_RequestedDeadlineMissedStatus & status);

  void on_requested_incompatible_qos(const dds_RequestedIncompatibleQosStatus & status);

  void on_data_available();

  void on_liveliness_changed(const dds_LivelinessChangedStatus & status);

  void on_subscription_matched(const dds_SubscriptionMatchedStatus & status);

  void on_sample_lost(const dds_SampleLostStatus & status);

  size_t count_unread();
};

rmw_subscription_t * create_subscription(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Subscriber * const sub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  const bool internal);

rmw_ret_t destroy_subscription(
  rmw_context_impl_t * const ctx,
  rmw_subscription_t * const subscription);
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_

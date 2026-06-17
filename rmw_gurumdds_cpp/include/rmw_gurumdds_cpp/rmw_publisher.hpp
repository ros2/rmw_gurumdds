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

#ifndef RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_
#define RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_

namespace rmw_gurumdds_cpp
{
// backend_buffer / publisher side
struct BufferPublisherEndpoint
{
  std::string key;
  dds_DataWriter *data_writer{nullptr};
  dds_Topic *topic{nullptr};
  bool owns_topic{true};
  rmw_gid_t target_subscriber_gid{};
  rmw_topic_endpoint_info_t subscriber_endpoint_info{};
  std::unordered_map<std::string, std::string> backend_metadata;
};

/// Metadata queued by the discovery callback for lazy DataWriter creation.
struct PendingBufferPublisher
{
  std::string unique_topic;
  rmw_gid_t target_subscriber_gid{};
  rmw_topic_endpoint_info_t subscriber_endpoint_info{};
  std::unordered_map<std::string, std::string> backend_metadata;
};

/// Mutable buffer state shared between the discovery callback and the
/// publish/destroy paths.  Managed via shared_ptr so the callback can
/// safely outlive the CustomPublisherInfo that created it.
struct BufferPublisherState
{
  std::atomic<bool> alive{true};
  std::mutex mutex;
  /// Per-subscriber (peer-to-peer) endpoints for non-CPU-only subscribers.
  std::vector<std::shared_ptr<BufferPublisherEndpoint>> endpoints;
  std::vector<PendingBufferPublisher> pending;
  /// GIDs of discovered CPU-only subscribers served by the shared CPU channel.
  std::vector<rmw_gid_t> cpu_only_subscribers;
};

struct PublisherInfo : EventInfo
{
  const rosidl_message_type_support_t *rosidl_message_typesupport;
  const rosidl_message_type_support_t *fastrtps_message_typesupport;

  const message_type_support_callbacks_t *
  get_fastrtps_type_support_callbacks() const;

  // fastrtps에서 토픽 이름을 캐싱해두는 것을 모방함.
  // 문자열에 ros prefix가 포함되어 있음에 주의.
  // 예시 : "/rr/foobar"
  std::string fastrtps_topic_name_mangled;

  // backend_buffer
  bool is_buffer_aware{false};
  std::unordered_map<std::string, std::string> backend_metadata;
  rmw_topic_endpoint_info_t local_endpoint_info{};
  const void *serialization_context{nullptr};
  std::shared_ptr<BufferPublisherState> buffer_state{
    std::make_shared<BufferPublisherState>()};

  // backend_buffer cpu-only shared channel
  dds_DataWriter *cpu_data_writer{nullptr};
  dds_Topic *cpu_topic{nullptr};
  dds_StatusCondition *cpu_status_condition{nullptr};

  dds_DomainParticipant *participant;
  dds_Publisher *publisher;

  const char *implementation_identifier;
  rmw_context_impl_t *ctx;
  int64_t sequence_number;

  rmw_gid_t publisher_gid;
  dds_DataWriter *topic_writer;
  std::mutex mutex_event;
  rmw_event_callback_t on_new_event_cb[RMW_EVENT_TYPE_MAX] = {};
  const void *user_data_cb[RMW_EVENT_TYPE_MAX] = {};
  dds_GuardCondition *event_guard_cond[RMW_EVENT_TYPE_MAX] = {};
  dds_StatusMask mask = 0;
  bool inconsistent_topic_changed = false;
  dds_InconsistentTopicStatus inconsistent_topic_status = {};
  bool offered_deadline_missed_changed = false;
  dds_OfferedDeadlineMissedStatus offered_deadline_missed_status = {};
  bool offered_incompatible_qos_changed = false;
  dds_OfferedIncompatibleQosStatus offered_incompatible_qos_status = {};
  bool liveliness_lost_changed = false;
  dds_LivelinessLostStatus liveliness_lost_status = {};
  bool publication_matched_changed = false;
  dds_PublicationMatchedStatus publication_matched_status = {};
  dds_DataWriterListener topic_listener = {};

  rmw_ret_t get_status(rmw_event_type_t event_type, void *event) override;

  dds_StatusCondition * get_status_condition() override;

  dds_GuardCondition * get_guard_condition(rmw_event_type_t event_type) override;

  bool is_status_changed(rmw_event_type_t event_type) override;

  bool has_callback(rmw_event_type_t event_type) override;

  bool has_callback_unsafe(rmw_event_type_t event_type) const;

  rmw_ret_t set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void *user_data,
    rmw_event_callback_t callback) override;

  void update_inconsistent_topic(
    int32_t total_count,
    int32_t total_count_change) override;

  void
  on_offered_deadline_missed(const dds_OfferedDeadlineMissedStatus & status);

  void
  on_offered_incompatible_qos(const dds_OfferedIncompatibleQosStatus & status);

  void on_liveliness_lost(const dds_LivelinessLostStatus & status);

  void on_publication_matched(const dds_PublicationMatchedStatus & status);
};

rmw_publisher_t * create_publisher(
  rmw_context_impl_t *const ctx, const rmw_node_t *node,
  dds_DomainParticipant *const participant, dds_Publisher *const pub,
  const rosidl_message_type_support_t *type_supports, const char *topic_name,
  const rmw_qos_profile_t *qos_policies,
  const rmw_publisher_options_t *publisher_options, const bool internal);

rmw_ret_t destroy_publisher(
  rmw_context_impl_t *const ctx,
  rmw_publisher_t *const publisher);

rmw_ret_t publish(
  const rmw_publisher_t *publisher, const void *ros_message,
  rmw_publisher_allocation_t *allocation);
} // namespace rmw_gurumdds_cpp

rmw_ret_t publish_to_buffer_endpoint(
  const rmw_publisher_t *publisher,
  const void *ros_message,
  rmw_publisher_allocation_t *allocation);

#endif // RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_

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

#ifndef RMW_GURUMDDS_CPP__EVENT_INFO_COMMON_HPP_
#define RMW_GURUMDDS_CPP__EVENT_INFO_COMMON_HPP_

#include <map>
#include <mutex>
#include <vector>

#include "rmw/ret_types.h"
#include "rmw/event_callback_type.h"

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"

namespace rmw_gurumdds_cpp
{
struct event_callback_data_t
{
  std::mutex mutex;
  rmw_event_callback_t callback {nullptr};
  const void * user_data {nullptr};
};

struct EventInfo
{
  virtual ~EventInfo() = default;

  virtual rmw_ret_t get_status(rmw_event_type_t event_type, void * event) = 0;

  virtual dds_StatusCondition * get_status_condition() = 0;

  virtual dds_GuardCondition * get_guard_condition(rmw_event_type_t event_type) = 0;

  virtual bool is_status_changed(rmw_event_type_t event_type) = 0;

  virtual bool has_callback(rmw_event_type_t event_type) = 0;

  virtual rmw_ret_t set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void * user_data,
    rmw_event_callback_t callback) = 0;

  virtual void update_inconsistent_topic(
    int32_t total_count,
    int32_t total_count_change
    ) = 0;
};

struct PublisherInfo : EventInfo
{
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const char * implementation_identifier;
  rmw_context_impl_t * ctx;
  int64_t sequence_number;

  rmw_gid_t publisher_gid;
  dds_DataWriter * topic_writer;
  std::mutex mutex_event;
  rmw_event_callback_t on_new_event_cb[RMW_EVENT_TYPE_MAX] = { };
  const void * user_data_cb[RMW_EVENT_TYPE_MAX] = { };
  dds_GuardCondition* event_guard_cond[RMW_EVENT_TYPE_MAX] = { };
  dds_StatusMask mask = 0;
  bool inconsistent_topic_changed = false;
  dds_InconsistentTopicStatus inconsistent_topic_status = { };
  bool offered_deadline_missed_changed = false;
  dds_OfferedDeadlineMissedStatus offered_deadline_missed_status = { };
  bool offered_incompatible_qos_changed = false;
  dds_OfferedIncompatibleQosStatus offered_incompatible_qos_status = { };
  bool liveliness_lost_changed = false;
  dds_LivelinessLostStatus liveliness_lost_status = { };
  bool publication_matched_changed = false;
  dds_PublicationMatchedStatus publication_matched_status = { };
  dds_DataWriterListener topic_listener = { };

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

  void update_inconsistent_topic(
    int32_t total_count,
    int32_t total_count_change) override;

  void on_offered_deadline_missed(const dds_OfferedDeadlineMissedStatus & status);

  void on_offered_incompatible_qos(const dds_OfferedIncompatibleQosStatus & status);

  void on_liveliness_lost(const dds_LivelinessLostStatus & status);

  void on_publication_matched(const dds_PublicationMatchedStatus & status);
};

size_t count_unread(
  dds_DataReader * reader,
  dds_DataSeq * data_seq,
  dds_SampleInfoSeq * info_seq,
  dds_UnsignedLongSeq * raw_data_sizes);

struct SubscriberInfo : EventInfo
{
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const char * implementation_identifier;
  rmw_context_impl_t * ctx;
  std::mutex mutex_event;
  rmw_event_callback_t on_new_event_cb[RMW_EVENT_TYPE_MAX] = { };
  const void * user_data_cb[RMW_EVENT_TYPE_MAX] = { };
  dds_GuardCondition* event_guard_cond[RMW_EVENT_TYPE_MAX] = { };
  dds_StatusMask mask = 0;
  bool requested_deadline_missed_changed = false;
  dds_RequestedDeadlineMissedStatus requested_deadline_missed_status = { };
  bool requested_incompatible_qos_changed = false;
  dds_RequestedIncompatibleQosStatus requested_incompatible_qos_status = { };
  bool inconsistent_topic_changed = false;
  dds_InconsistentTopicStatus inconsistent_topic_status = { };
  bool liveliness_changed = false;
  dds_LivelinessChangedStatus liveliness_changed_status = { };
  bool subscription_matched_changed = false;
  dds_SubscriptionMatchedStatus subscription_matched_status = { };
  bool sample_lost_changed = false;
  dds_SampleLostStatus sample_lost_status = { };

  rmw_gid_t subscriber_gid;
  dds_DataReader * topic_reader;
  dds_ReadCondition * read_condition;

  dds_DataReaderListener topic_listener;
  dds_DataSeq * data_seq;
  dds_SampleInfoSeq * info_seq;
  dds_UnsignedLongSeq * raw_data_sizes;
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

  void update_inconsistent_topic(
    int32_t total_count,
    int32_t total_count_change) override;

  void on_requested_deadline_missed(const dds_RequestedDeadlineMissedStatus& status);

  void on_requested_incompatible_qos(const dds_RequestedIncompatibleQosStatus& status);

  void on_data_available();

  void on_liveliness_changed(const dds_LivelinessChangedStatus& status);

  void on_subscription_matched(const dds_SubscriptionMatchedStatus& status);

  void on_sample_lost(const dds_SampleLostStatus& status);

  size_t count_unread();
};

class TopicEventListener {
public:
  static rmw_ret_t associate_listener(dds_Topic* topic);

  static rmw_ret_t disassociate_Listener(dds_Topic* topic);

  static void add_event(dds_Topic* topic, EventInfo* event_info);

  static void remove_event(dds_Topic* topic, EventInfo* event_info);

  void on_inconsistent_topic(const dds_InconsistentTopicStatus& status);

private:
  static void on_inconsistent_topic(const dds_Topic* the_topic,
                                    const dds_InconsistentTopicStatus* status);

private:
  static std::map<dds_Topic*, TopicEventListener*> table_;
  static std::mutex mutex_table_;

  std::recursive_mutex mutex_;
  std::vector<EventInfo*> event_list_;
};
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__EVENT_INFO_COMMON_HPP_

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

#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "rmw/event.h"
#include "rmw/event_callback_type.h"
#include "rmw/ret_types.h"

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "rmw_gurumdds_cpp/backend_buffer.hpp"
#include "rmw_gurumdds_cpp/dds_include.hpp"

namespace rmw_gurumdds_cpp
{
struct event_callback_data_t
{
  std::mutex mutex;
  rmw_event_callback_t callback{nullptr};
  const void *user_data{nullptr};
};

struct EventInfo
{
  virtual ~EventInfo() = default;

  virtual rmw_ret_t get_status(rmw_event_type_t event_type, void *event) = 0;

  virtual dds_StatusCondition * get_status_condition() = 0;

  virtual dds_GuardCondition *
  get_guard_condition(rmw_event_type_t event_type) = 0;

  virtual bool is_status_changed(rmw_event_type_t event_type) = 0;

  virtual bool has_callback(rmw_event_type_t event_type) = 0;

  virtual rmw_ret_t
  set_on_new_event_callback(
    rmw_event_type_t event_type, const void *user_data,
    rmw_event_callback_t callback) = 0;

  virtual void update_inconsistent_topic(
    int32_t total_count,
    int32_t total_count_change) = 0;
};

size_t count_unread(
  dds_DataReader *reader, dds_DataSeq *data_seq,
  dds_SampleInfoSeq *info_seq,
  dds_UnsignedLongSeq *raw_data_sizes);

class TopicEventListener {
public:
  static rmw_ret_t associate_listener(dds_Topic *topic);

  static rmw_ret_t disassociate_Listener(dds_Topic *topic);

  static void add_event(dds_Topic *topic, EventInfo *event_info);

  static void remove_event(dds_Topic *topic, EventInfo *event_info);

  void on_inconsistent_topic(const dds_InconsistentTopicStatus & status);

private:
  static void on_inconsistent_topic(
    const dds_Topic *the_topic,
    const dds_InconsistentTopicStatus *status);

private:
  static std::map<dds_Topic *, TopicEventListener *> table_;
  static std::mutex mutex_table_;

  std::recursive_mutex mutex_;
  std::vector<EventInfo *> event_list_;
};
} // namespace rmw_gurumdds_cpp

#endif // RMW_GURUMDDS_CPP__EVENT_INFO_COMMON_HPP_

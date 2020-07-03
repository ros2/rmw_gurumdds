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

#include "rmw_gurumdds_shared_cpp/event_converter.hpp"
#include "rmw_gurumdds_static_cpp/types.hpp"

rmw_ret_t GurumddsPublisherInfo::get_status(
  dds_StatusMask mask,
  void * event)
{
  if (mask == dds_LIVELINESS_LOST_STATUS) {
    dds_LivelinessLostStatus liveliness_lost;
    dds_ReturnCode_t dds_return_code =
      dds_DataWriter_get_liveliness_lost_status(topic_writer, &liveliness_lost);
    rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
    if (from_dds != RMW_RET_OK) {
      return from_dds;
    }

    auto rmw_liveliness_lost = static_cast<rmw_liveliness_lost_status_t *>(event);
    rmw_liveliness_lost->total_count = liveliness_lost.total_count;
    rmw_liveliness_lost->total_count_change = liveliness_lost.total_count_change;
  } else if (mask == dds_OFFERED_DEADLINE_MISSED_STATUS) {
    dds_OfferedDeadlineMissedStatus offered_deadline_missed;
    dds_ReturnCode_t dds_return_code =
      dds_DataWriter_get_offered_deadline_missed_status(topic_writer, &offered_deadline_missed);
    rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
    if (from_dds != RMW_RET_OK) {
      return from_dds;
    }

    auto rmw_offered_deadline_missed = static_cast<rmw_offered_deadline_missed_status_t *>(event);
    rmw_offered_deadline_missed->total_count = offered_deadline_missed.total_count;
    rmw_offered_deadline_missed->total_count_change = offered_deadline_missed.total_count_change;
  } else {
    return RMW_RET_UNSUPPORTED;
  }
  return RMW_RET_OK;
}

dds_StatusCondition * GurumddsPublisherInfo::get_statuscondition()
{
  return dds_DataWriter_get_statuscondition(topic_writer);
}

dds_StatusMask GurumddsPublisherInfo::get_status_changes()
{
  return dds_DataWriter_get_status_changes(topic_writer);
}

rmw_ret_t GurumddsSubscriberInfo::get_status(
  dds_StatusMask mask,
  void * event)
{
  if (mask == dds_LIVELINESS_CHANGED_STATUS) {
    dds_LivelinessChangedStatus liveliness_changed;
    dds_ReturnCode_t dds_return_code =
      dds_DataReader_get_liveliness_changed_status(topic_reader, &liveliness_changed);
    rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
    if (from_dds != RMW_RET_OK) {
      return from_dds;
    }

    auto rmw_liveliness_changed_status = static_cast<rmw_liveliness_changed_status_t *>(event);
    rmw_liveliness_changed_status->alive_count = liveliness_changed.alive_count;
    rmw_liveliness_changed_status->not_alive_count = liveliness_changed.not_alive_count;
    rmw_liveliness_changed_status->alive_count_change = liveliness_changed.alive_count_change;
    rmw_liveliness_changed_status->not_alive_count_change =
      liveliness_changed.not_alive_count_change;
  } else if (mask == dds_REQUESTED_DEADLINE_MISSED_STATUS) {
    dds_RequestedDeadlineMissedStatus requested_deadline_missed;
    dds_ReturnCode_t dds_return_code =
      dds_DataReader_get_requested_deadline_missed_status(topic_reader, &requested_deadline_missed);
    rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
    if (from_dds != RMW_RET_OK) {
      return from_dds;
    }

    auto rmw_requested_deadline_missed_status =
      static_cast<rmw_requested_deadline_missed_status_t *>(event);
    rmw_requested_deadline_missed_status->total_count = requested_deadline_missed.total_count;
    rmw_requested_deadline_missed_status->total_count_change =
      requested_deadline_missed.total_count_change;
  } else {
    return RMW_RET_UNSUPPORTED;
  }
  return RMW_RET_OK;
}

dds_StatusCondition * GurumddsSubscriberInfo::get_statuscondition()
{
  return dds_DataReader_get_statuscondition(topic_reader);
}

dds_StatusMask GurumddsSubscriberInfo::get_status_changes()
{
  return dds_DataReader_get_status_changes(topic_reader);
}

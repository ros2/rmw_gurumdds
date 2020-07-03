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
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_static_cpp/types.hpp"

rmw_ret_t GurumddsPublisherInfo::get_status(
  dds_StatusMask mask,
  void * event)
{
  if (mask == dds_LIVELINESS_LOST_STATUS) {
    dds_LivelinessLostStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataWriter_get_liveliness_lost_status(topic_writer, &status);
    rmw_ret_t rmw_ret = check_dds_ret_code(dds_ret);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto rmw_status = static_cast<rmw_liveliness_lost_status_t *>(event);
    rmw_status->total_count = status.total_count;
    rmw_status->total_count_change = status.total_count_change;
  } else if (mask == dds_OFFERED_DEADLINE_MISSED_STATUS) {
    dds_OfferedDeadlineMissedStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataWriter_get_offered_deadline_missed_status(topic_writer, &status);
    rmw_ret_t rmw_ret = check_dds_ret_code(dds_ret);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto rmw_status = static_cast<rmw_offered_deadline_missed_status_t *>(event);
    rmw_status->total_count = status.total_count;
    rmw_status->total_count_change = status.total_count_change;
  } else if (mask == dds_OFFERED_INCOMPATIBLE_QOS_STATUS) {
    dds_OfferedIncompatibleQosStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataWriter_get_offered_incompatible_qos_status(topic_writer, &status);
    rmw_ret_t rmw_ret = check_dds_ret_code(dds_ret);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto rmw_status = static_cast<rmw_offered_qos_incompatible_event_status_t *>(event);
    rmw_status->total_count = status.total_count;
    rmw_status->total_count_change = status.total_count_change;
    rmw_status->last_policy_kind = convert_qos_policy(status.last_policy_id);
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
    dds_LivelinessChangedStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataReader_get_liveliness_changed_status(topic_reader, &status);
    rmw_ret_t rmw_ret = check_dds_ret_code(dds_ret);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto rmw_status = static_cast<rmw_liveliness_changed_status_t *>(event);
    rmw_status->alive_count = status.alive_count;
    rmw_status->not_alive_count = status.not_alive_count;
    rmw_status->alive_count_change = status.alive_count_change;
    rmw_status->not_alive_count_change =
      status.not_alive_count_change;
  } else if (mask == dds_REQUESTED_DEADLINE_MISSED_STATUS) {
    dds_RequestedDeadlineMissedStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataReader_get_requested_deadline_missed_status(topic_reader, &status);
    rmw_ret_t rmw_ret = check_dds_ret_code(dds_ret);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto rmw_status =
      static_cast<rmw_requested_deadline_missed_status_t *>(event);
    rmw_status->total_count = status.total_count;
    rmw_status->total_count_change = status.total_count_change;
  } else if (mask == dds_REQUESTED_INCOMPATIBLE_QOS_STATUS) {
    dds_RequestedIncompatibleQosStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataReader_get_requested_incompatible_qos_status(topic_reader, &status);
    rmw_ret_t rmw_ret = check_dds_ret_code(dds_ret);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto rmw_status = static_cast<rmw_requested_qos_incompatible_event_status_t *>(event);
    rmw_status->total_count = status.total_count;
    rmw_status->total_count_change = status.total_count_change;
    rmw_status->last_policy_kind = convert_qos_policy(status.last_policy_id);
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

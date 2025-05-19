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

#include <cstdint>

#include "rmw_gurumdds_cpp/event_converter.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

namespace rmw_gurumdds_cpp
{
void PublisherInfo::update_inconsistent_topic(int32_t total_count, int32_t total_count_change)
{
  std::lock_guard guard_callback{mutex_event};
  inconsistent_topic_changed = true;
  inconsistent_topic_status.total_count_change += total_count_change;
  inconsistent_topic_status.total_count = total_count;

  auto callback = on_new_event_cb[RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE];
  auto user_data = user_data_cb[RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE];
  if(nullptr != callback) {
    callback(user_data, total_count_change);
  }

  auto dds_condition = event_guard_cond[RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE];
  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void PublisherInfo::on_offered_deadline_missed(const dds_OfferedDeadlineMissedStatus & status)
{
  std::lock_guard guard_callback{mutex_event};
  offered_deadline_missed_changed = true;
  liveliness_lost_status.total_count_change += status.total_count_change;
  liveliness_lost_status.total_count = status.total_count;

  auto callback = on_new_event_cb[RMW_EVENT_OFFERED_DEADLINE_MISSED];
  auto user_data = user_data_cb[RMW_EVENT_OFFERED_DEADLINE_MISSED];
  if(nullptr != callback) {
    callback(user_data, liveliness_lost_status.total_count_change);
  }

  auto dds_condition = event_guard_cond[RMW_EVENT_OFFERED_DEADLINE_MISSED];
  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void PublisherInfo::on_offered_incompatible_qos(const dds_OfferedIncompatibleQosStatus & status)
{
  std::lock_guard guard_callback{mutex_event};
  offered_incompatible_qos_changed = true;
  offered_incompatible_qos_status.total_count_change += status.total_count_change;
  offered_incompatible_qos_status.total_count = status.total_count;
  offered_incompatible_qos_status.last_policy_id = status.last_policy_id;

  auto callback = on_new_event_cb[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE];
  auto user_data = user_data_cb[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE];
  if(nullptr != callback) {
    callback(user_data, offered_incompatible_qos_status.total_count_change);
  }

  auto dds_condition = event_guard_cond[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE];
  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void PublisherInfo::on_liveliness_lost(const dds_LivelinessLostStatus & status) {
  std::lock_guard guard_callback{mutex_event};
  liveliness_lost_changed = true;
  liveliness_lost_status.total_count_change += status.total_count_change;
  liveliness_lost_status.total_count = status.total_count;

  auto callback = on_new_event_cb[RMW_EVENT_LIVELINESS_LOST];
  auto user_data = user_data_cb[RMW_EVENT_LIVELINESS_LOST];
  if(nullptr != callback) {
    callback(user_data, liveliness_lost_status.total_count_change);
  }

  auto dds_condition = event_guard_cond[RMW_EVENT_LIVELINESS_LOST];
  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void PublisherInfo::on_publication_matched(const dds_PublicationMatchedStatus & status) {
  std::lock_guard guard_callback{mutex_event};
  publication_matched_changed = true;
  publication_matched_status.total_count_change += status.total_count_change;
  publication_matched_status.total_count = status.total_count;
  publication_matched_status.current_count_change += status.current_count_change;
  publication_matched_status.current_count = status.current_count;

  auto callback = on_new_event_cb[RMW_EVENT_PUBLICATION_MATCHED];
  auto user_data = user_data_cb[RMW_EVENT_PUBLICATION_MATCHED];
  if(nullptr != callback) {
    callback(user_data, publication_matched_status.total_count_change);
  }

  auto dds_condition = event_guard_cond[RMW_EVENT_PUBLICATION_MATCHED];
  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

rmw_ret_t PublisherInfo::set_on_new_event_callback(
  rmw_event_type_t event_type,
  const void * user_data,
  rmw_event_callback_t callback) {
  std::lock_guard guard{mutex_event};
  dds_StatusMask event_status_type = rmw_gurumdds_cpp::get_status_kind_from_rmw(event_type);
  if(callback != nullptr) {
    int32_t changes;
    dds_Topic* topic;
    switch(event_type) {
      case RMW_EVENT_LIVELINESS_LOST:
        dds_DataWriter_get_liveliness_lost_status(topic_writer, &liveliness_lost_status);
        changes = liveliness_lost_status.total_count_change;
        liveliness_lost_status.total_count_change = 0;
        liveliness_lost_changed = false;
        break;
      case RMW_EVENT_OFFERED_DEADLINE_MISSED:
        dds_DataWriter_get_offered_deadline_missed_status(topic_writer,
                                                          &offered_deadline_missed_status);
        changes = offered_deadline_missed_status.total_count_change;
        offered_deadline_missed_status.total_count_change = 0;
        offered_deadline_missed_changed = false;
        break;
      case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
        dds_DataWriter_get_offered_incompatible_qos_status(topic_writer,
                                                           &offered_incompatible_qos_status);
        changes = offered_incompatible_qos_status.total_count_change;
        offered_incompatible_qos_status.total_count_change = 0;
        offered_incompatible_qos_changed = false;
        break;
      case RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE:
        topic = dds_DataWriter_get_topic(topic_writer);
        dds_Topic_get_inconsistent_topic_status(topic, &inconsistent_topic_status);
        changes = inconsistent_topic_status.total_count_change;
        inconsistent_topic_status.total_count_change = 0;
        inconsistent_topic_changed = false;
        break;
      case RMW_EVENT_PUBLICATION_MATCHED:
        dds_DataWriter_get_publication_matched_status(topic_writer,
                                                      &publication_matched_status);
        changes = publication_matched_status.total_count_change;
        publication_matched_status.total_count_change = 0;
        publication_matched_status.current_count_change = 0;
        publication_matched_changed = false;
        break;
      default:
          return RMW_RET_UNSUPPORTED;
    }

    if(changes > 0) {
      callback(user_data, changes);
    }

    mask |= event_status_type;
    on_new_event_cb[event_type] = callback;
    user_data_cb[event_type] = user_data;
  } else {
    mask &= ~event_status_type;
    on_new_event_cb[event_type] = nullptr;
    user_data_cb[event_type] = nullptr;
  }

  dds_DataWriter_set_listener(topic_writer, &topic_listener, mask);
  return RMW_RET_OK;
}

rmw_ret_t PublisherInfo::get_status(rmw_event_type_t event_type, void * event)
{
  std::lock_guard lock_mutex{mutex_event};
  if (event_type == RMW_EVENT_LIVELINESS_LOST) {
    if(liveliness_lost_changed) {
      liveliness_lost_changed = false;
    } else {
      dds_DataWriter_get_liveliness_lost_status(topic_writer, &liveliness_lost_status);
    }

    auto rmw_status = static_cast<rmw_liveliness_lost_status_t *>(event);
    rmw_status->total_count = liveliness_lost_status.total_count;
    rmw_status->total_count_change = liveliness_lost_status.total_count_change;
    liveliness_lost_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_OFFERED_DEADLINE_MISSED) {
    if(offered_deadline_missed_changed) {
      offered_deadline_missed_changed = false;
    } else {
      dds_DataWriter_get_offered_deadline_missed_status(topic_writer,
                                                        &offered_deadline_missed_status);
    }

    auto rmw_status = static_cast<rmw_offered_deadline_missed_status_t *>(event);
    rmw_status->total_count = offered_deadline_missed_status.total_count;
    rmw_status->total_count_change = offered_deadline_missed_status.total_count_change;
    offered_deadline_missed_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_OFFERED_QOS_INCOMPATIBLE) {
    if(offered_incompatible_qos_changed) {
      offered_incompatible_qos_changed = false;
    } else {
      dds_DataWriter_get_offered_incompatible_qos_status(topic_writer,
                                                         &offered_incompatible_qos_status);
    }

    auto rmw_status = static_cast<rmw_offered_qos_incompatible_event_status_t *>(event);
    auto last_policy_id = offered_incompatible_qos_status.last_policy_id;
    rmw_status->total_count = offered_incompatible_qos_status.total_count;
    rmw_status->total_count_change = offered_incompatible_qos_status.total_count_change;
    rmw_status->last_policy_kind = convert_qos_policy(last_policy_id);
    offered_incompatible_qos_status.total_count_change = 0;
  } else if(event_type == RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE) {
    if(inconsistent_topic_changed) {
      inconsistent_topic_changed = false;
    } else {
      dds_Topic* const topic = dds_DataWriter_get_topic(topic_writer);
      dds_Topic_get_inconsistent_topic_status(topic, &inconsistent_topic_status);
    }

    auto const rmw_status = static_cast<rmw_incompatible_type_status_t *>(event);
    rmw_status->total_count = inconsistent_topic_status.total_count;
    rmw_status->total_count_change = inconsistent_topic_status.total_count_change;
    inconsistent_topic_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_PUBLICATION_MATCHED) {
    if(publication_matched_changed) {
      publication_matched_changed = false;
    } else {
      dds_DataWriter_get_publication_matched_status(topic_writer,
                                                    &publication_matched_status);
    }

    auto const rmw_status = static_cast<rmw_matched_status_t *>(event);
    rmw_status->current_count = publication_matched_status.current_count;
    rmw_status->current_count_change = publication_matched_status.current_count_change;
    rmw_status->total_count = publication_matched_status.total_count;
    rmw_status->total_count_change = publication_matched_status.total_count_change;
    publication_matched_status.current_count_change = 0;
    publication_matched_status.total_count_change = 0;
  } else {
    return RMW_RET_UNSUPPORTED;
  }

  dds_GuardCondition_set_trigger_value(event_guard_cond[event_type], false);
  return RMW_RET_OK;
}

dds_StatusCondition * PublisherInfo::get_status_condition()
{
  return dds_DataWriter_get_statuscondition(topic_writer);
}

dds_GuardCondition * PublisherInfo::get_guard_condition(rmw_event_type_t event_type)
{
  return event_guard_cond[event_type];
}

bool PublisherInfo::is_status_changed(rmw_event_type_t event_type)
{
  std::lock_guard lock_guard{mutex_event};
  bool changed = false;
  if(has_callback_unsafe(event_type)) {
    switch(event_type) {
      case RMW_EVENT_LIVELINESS_LOST:
        changed = liveliness_lost_changed;
        break;
      case RMW_EVENT_OFFERED_DEADLINE_MISSED:
        changed = offered_deadline_missed_changed;
        break;
      case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
        changed = offered_incompatible_qos_changed;
        break;
      case RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE:
        changed = inconsistent_topic_changed;
        break;
      case RMW_EVENT_PUBLICATION_MATCHED:
        changed = publication_matched_changed;
        break;
      default:
        return false;
    }

    if(changed) {
      dds_GuardCondition_set_trigger_value(event_guard_cond[event_type], false);
    }
  }

  auto status_kind = rmw_gurumdds_cpp::get_status_kind_from_rmw(event_type);
  if((dds_DataWriter_get_status_changes(topic_writer) & status_kind) > 0) {
    changed = true;
  }

  return changed;
}

bool PublisherInfo::has_callback(rmw_event_type_t event_type)
{
  std::lock_guard lock_guard{mutex_event};
  return has_callback_unsafe(event_type);
}

bool PublisherInfo::has_callback_unsafe(rmw_event_type_t event_type) const
{
  // RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE is always used with a callback
  auto status_kind = rmw_gurumdds_cpp::get_status_kind_from_rmw(event_type);
  return ((mask | dds_INCONSISTENT_TOPIC_STATUS) & status_kind) > 0;
}

rmw_ret_t SubscriberInfo::set_on_new_event_callback(
  rmw_event_type_t event_type,
  const void * user_data,
  rmw_event_callback_t callback)
{
  std::lock_guard guard{mutex_event};
  dds_StatusMask event_status_type = rmw_gurumdds_cpp::get_status_kind_from_rmw(event_type);
  if(callback != nullptr) {
    int32_t changes;
    dds_Topic* topic;
    switch(event_type) {
      case RMW_EVENT_LIVELINESS_CHANGED:
        dds_DataReader_get_liveliness_changed_status(topic_reader,
                                                     &liveliness_changed_status);
        changes = liveliness_changed_status.alive_count_change;
        changes += liveliness_changed_status.not_alive_count_change;
        liveliness_changed_status.alive_count_change = 0;
        liveliness_changed_status.not_alive_count_change = 0;
        liveliness_changed = false;
        break;
      case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
        dds_DataReader_get_requested_deadline_missed_status(topic_reader,
                                                            &requested_deadline_missed_status);
        changes = requested_deadline_missed_status.total_count_change;
        requested_deadline_missed_status.total_count_change = 0;
        requested_deadline_missed_changed = false;
        break;
      case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
        dds_DataReader_get_requested_incompatible_qos_status(topic_reader,
                                                             &requested_incompatible_qos_status);
        changes = requested_incompatible_qos_status.total_count_change;
        requested_incompatible_qos_status.total_count_change = 0;
        requested_incompatible_qos_changed = false;
        break;
      case RMW_EVENT_MESSAGE_LOST:
        dds_DataReader_get_sample_lost_status(topic_reader, &sample_lost_status);
        changes = sample_lost_status.total_count_change;
        sample_lost_status.total_count_change = 0;
        sample_lost_changed = false;
        break;
      case RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE:
        topic = reinterpret_cast<dds_Topic*>(dds_DataReader_get_topicdescription(topic_reader));
        dds_Topic_get_inconsistent_topic_status(topic, &inconsistent_topic_status);
        changes = inconsistent_topic_status.total_count_change;
        inconsistent_topic_status.total_count_change = 0;
        inconsistent_topic_changed = false;
        break;
      case RMW_EVENT_SUBSCRIPTION_MATCHED:
        dds_DataReader_get_subscription_matched_status(topic_reader,
                                                       &subscription_matched_status);
        changes = subscription_matched_status.total_count_change;
        subscription_matched_status.total_count_change = 0;
        subscription_matched_status.current_count_change = 0;
        subscription_matched_changed = false;
        break;
      default:
        return RMW_RET_UNSUPPORTED;
    }

    if(changes > 0) {
      callback(user_data, changes);
    }

    mask |= event_status_type;
    on_new_event_cb[event_type] = callback;
    user_data_cb[event_type] = user_data;
  } else {
    mask &= ~event_status_type;
    on_new_event_cb[event_type] = nullptr;
    user_data_cb[event_type] = nullptr;
  }

  dds_DataReader_set_listener(topic_reader, &topic_listener, mask);
  return RMW_RET_OK;
}

rmw_ret_t SubscriberInfo::get_status(rmw_event_type_t event_type, void * event)
{
  std::lock_guard lock_guard{mutex_event};
  if (event_type == RMW_EVENT_LIVELINESS_CHANGED) {
    if(liveliness_changed) {
      liveliness_changed = false;
    } else {
      dds_DataReader_get_liveliness_changed_status(topic_reader,
                                                   &liveliness_changed_status);
    }

    auto rmw_status = static_cast<rmw_liveliness_changed_status_t *>(event);
    rmw_status->alive_count = liveliness_changed_status.alive_count;
    rmw_status->not_alive_count = liveliness_changed_status.not_alive_count;
    rmw_status->alive_count_change = liveliness_changed_status.alive_count_change;
    rmw_status->not_alive_count_change = liveliness_changed_status.not_alive_count_change;
    liveliness_changed_status.alive_count_change = 0;
    liveliness_changed_status.not_alive_count_change = 0;
  } else if (event_type == RMW_EVENT_REQUESTED_DEADLINE_MISSED) {
    if(requested_deadline_missed_changed) {
      requested_deadline_missed_changed = false;
    } else {
      dds_DataReader_get_requested_deadline_missed_status(topic_reader,
                                                          &requested_deadline_missed_status);
    }

    auto rmw_status = static_cast<rmw_requested_deadline_missed_status_t *>(event);
    rmw_status->total_count = requested_deadline_missed_status.total_count;
    rmw_status->total_count_change = requested_deadline_missed_status.total_count_change;
    requested_deadline_missed_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE) {
    if(requested_incompatible_qos_changed) {
      requested_incompatible_qos_changed = false;
    } else {
      dds_DataReader_get_requested_incompatible_qos_status(topic_reader,
                                                           &requested_incompatible_qos_status);
    }

    auto rmw_status = static_cast<rmw_requested_qos_incompatible_event_status_t *>(event);
    auto last_policy_id = requested_incompatible_qos_status.last_policy_id;
    rmw_status->total_count = requested_incompatible_qos_status.total_count;
    rmw_status->total_count_change = requested_incompatible_qos_status.total_count_change;
    rmw_status->last_policy_kind = rmw_gurumdds_cpp::convert_qos_policy(last_policy_id);
    requested_incompatible_qos_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_MESSAGE_LOST) {
    if(sample_lost_changed) {
      sample_lost_changed = false;
    } else {
      dds_DataReader_get_sample_lost_status(topic_reader, &sample_lost_status);
    }

    auto rmw_status = static_cast<rmw_message_lost_status_t *>(event);
    rmw_status->total_count = sample_lost_status.total_count;
    rmw_status->total_count_change = sample_lost_status.total_count_change;
    sample_lost_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE) {
    if(inconsistent_topic_changed) {
      inconsistent_topic_changed = false;
    } else {
      auto topic =
          reinterpret_cast<dds_Topic*>(dds_DataReader_get_topicdescription(this->topic_reader));
      dds_Topic_get_inconsistent_topic_status(topic, &inconsistent_topic_status);
    }

    auto const rmw_status = static_cast<rmw_incompatible_type_status_t *>(event);
    rmw_status->total_count = inconsistent_topic_status.total_count;
    rmw_status->total_count_change = inconsistent_topic_status.total_count_change;
    inconsistent_topic_status.total_count_change = 0;
  } else if (event_type == RMW_EVENT_SUBSCRIPTION_MATCHED) {
    if(subscription_matched_changed) {
      subscription_matched_changed = false;
    } else {
      dds_DataReader_get_subscription_matched_status(topic_reader,
                                                     &subscription_matched_status);
    }

    auto const rmw_status = static_cast<rmw_matched_status_t *>(event);
    rmw_status->current_count = subscription_matched_status.current_count;
    rmw_status->current_count_change = subscription_matched_status.current_count_change;
    rmw_status->total_count = subscription_matched_status.total_count;
    rmw_status->total_count_change = subscription_matched_status.total_count_change;
    subscription_matched_status.current_count_change = 0;
    subscription_matched_status.total_count_change = 0;
  } else {
    return RMW_RET_UNSUPPORTED;
  }

  dds_GuardCondition_set_trigger_value(event_guard_cond[event_type], false);
  return RMW_RET_OK;
}

void SubscriberInfo::update_inconsistent_topic(int32_t total_count, int32_t total_count_change) {
  inconsistent_topic_status.total_count_change += total_count_change;
  inconsistent_topic_status.total_count = total_count;
  inconsistent_topic_changed = true;

  auto callback = on_new_event_cb[RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE];
  auto user_data = user_data_cb[RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE];
  auto dds_condition = event_guard_cond[RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE];
  if(nullptr != callback) {
    callback(user_data, total_count_change);
  }

  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

dds_StatusCondition * SubscriberInfo::get_status_condition()
{
  return dds_DataReader_get_statuscondition(topic_reader);
}

dds_GuardCondition * SubscriberInfo::get_guard_condition(rmw_event_type_t event_type)
{
  return event_guard_cond[event_type];
}

bool SubscriberInfo::is_status_changed(rmw_event_type_t event_type)
{
  std::lock_guard lock_guard{mutex_event};
  bool changed = false;
  if(has_callback_unsafe(event_type)) {
    switch(event_type) {
      case RMW_EVENT_LIVELINESS_CHANGED:
        changed = liveliness_changed;
        break;
      case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
        changed = requested_deadline_missed_changed;
        break;
      case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
        changed = requested_incompatible_qos_changed;
        break;
      case RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE:
        changed = inconsistent_topic_changed;
        break;
      case RMW_EVENT_SUBSCRIPTION_MATCHED:
        changed = subscription_matched_changed;
        break;
      default:
        return false;
    }

    if(changed) {
      dds_GuardCondition_set_trigger_value(event_guard_cond[event_type], false);
    }
  }

  auto status_kind = rmw_gurumdds_cpp::get_status_kind_from_rmw(event_type);
  if((dds_DataReader_get_status_changes(topic_reader) & status_kind) > 0) {
    changed = true;
  }

  return changed;
}

size_t count_unread(
  dds_DataReader * reader,
  dds_DataSeq * data_seq,
  dds_SampleInfoSeq * info_seq,
  dds_UnsignedLongSeq * raw_data_sizes)
{
  dds_ReturnCode_t rc = dds_DataReader_raw_read(
    reader,
    dds_HANDLE_NIL,
    data_seq,
    info_seq,
    raw_data_sizes,
    dds_LENGTH_UNLIMITED,
    dds_NOT_READ_SAMPLE_STATE,
    dds_ANY_VIEW_STATE,
    dds_ANY_INSTANCE_STATE
  );

  size_t count = 0;

  if (dds_RETCODE_OK != rc && dds_RETCODE_NO_DATA != rc) {
    RMW_SET_ERROR_MSG("failed to read raw data from DDS reader");
    return count;
  } else if(dds_RETCODE_OK == rc) {
    count = dds_SampleInfoSeq_length(info_seq);
  }

  rc = dds_DataReader_raw_return_loan(reader, data_seq, info_seq,
                                      raw_data_sizes);
  if (dds_RETCODE_OK != rc && dds_RETCODE_NO_DATA != rc) {
    RMW_SET_ERROR_MSG("failed to read raw data from DDS reader");
    return count;
  }

  return count;
}

size_t SubscriberInfo::count_unread()
{
  return rmw_gurumdds_cpp::count_unread(topic_reader, data_seq, info_seq, raw_data_sizes);
}

void SubscriberInfo::on_requested_deadline_missed(const dds_RequestedDeadlineMissedStatus & status)
{
  std::lock_guard guard(mutex_event);
  requested_deadline_missed_changed = true;
  requested_deadline_missed_status.total_count_change += status.total_count_change;
  requested_deadline_missed_status.total_count = status.total_count;
  auto callback = on_new_event_cb[RMW_EVENT_REQUESTED_DEADLINE_MISSED];
  auto user_data = user_data_cb[RMW_EVENT_REQUESTED_DEADLINE_MISSED];
  auto dds_condition = event_guard_cond[RMW_EVENT_REQUESTED_DEADLINE_MISSED];
  if(nullptr != callback) {
    callback(user_data, requested_deadline_missed_status.total_count_change);
  }

  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void SubscriberInfo::on_requested_incompatible_qos(
    const dds_RequestedIncompatibleQosStatus & status)
{
  std::lock_guard guard(mutex_event);
  requested_incompatible_qos_changed = true;
  requested_incompatible_qos_status.total_count_change += status.total_count_change;
  requested_incompatible_qos_status.total_count = status.total_count;
  requested_incompatible_qos_status.last_policy_id = status.last_policy_id;
  auto callback = on_new_event_cb[RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE];
  auto user_data = user_data_cb[RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE];
  auto dds_condition = event_guard_cond[RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE];
  if(nullptr != callback) {
    callback(user_data, requested_incompatible_qos_status.total_count_change);
  }

  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void SubscriberInfo::on_data_available() {
  std::lock_guard<std::mutex> guard(event_callback_data.mutex);
  if(event_callback_data.callback) {
    event_callback_data.callback(event_callback_data.user_data, count_unread());
  }
}

void SubscriberInfo::on_liveliness_changed(const dds_LivelinessChangedStatus & status)
{
  std::lock_guard guard(mutex_event);
  liveliness_changed = true;
  liveliness_changed_status.alive_count_change += status.alive_count_change;
  liveliness_changed_status.not_alive_count_change += status.not_alive_count_change;
  liveliness_changed_status.alive_count = status.alive_count;
  liveliness_changed_status.not_alive_count = status.not_alive_count;
  auto callback = on_new_event_cb[RMW_EVENT_LIVELINESS_CHANGED];
  auto user_data = user_data_cb[RMW_EVENT_LIVELINESS_CHANGED];
  auto dds_condition = event_guard_cond[RMW_EVENT_LIVELINESS_CHANGED];
  if(nullptr != callback) {
    callback(user_data, liveliness_changed_status.alive_count_change);
  }

  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void SubscriberInfo::on_subscription_matched(const dds_SubscriptionMatchedStatus & status)
{
  std::lock_guard guard(mutex_event);
  subscription_matched_changed = true;
  subscription_matched_status.total_count_change += status.total_count_change;
  subscription_matched_status.current_count_change += status.current_count_change;
  subscription_matched_status.current_count = status.current_count;
  subscription_matched_status.total_count = status.total_count;
  auto callback = on_new_event_cb[RMW_EVENT_SUBSCRIPTION_MATCHED];
  auto user_data = user_data_cb[RMW_EVENT_SUBSCRIPTION_MATCHED];
  auto dds_condition = event_guard_cond[RMW_EVENT_SUBSCRIPTION_MATCHED];
  if(nullptr != callback) {
    callback(user_data, subscription_matched_status.total_count_change);
  }

  dds_GuardCondition_set_trigger_value(dds_condition, true);
}

void SubscriberInfo::on_sample_lost(const dds_SampleLostStatus & status) {
  std::lock_guard guard(mutex_event);
  sample_lost_changed = true;
  sample_lost_status.total_count_change += status.total_count_change;
  sample_lost_status.total_count = status.total_count;
  auto callback = on_new_event_cb[RMW_EVENT_MESSAGE_LOST];
  auto user_data = user_data_cb[RMW_EVENT_MESSAGE_LOST];
  if(nullptr != callback) {
    callback(user_data, sample_lost_status.total_count_change);
  }

  dds_GuardCondition_set_trigger_value(event_guard_cond[RMW_EVENT_MESSAGE_LOST], true);
}

bool SubscriberInfo::has_callback(rmw_event_type_t event_type)
{
  std::lock_guard guard(mutex_event);
  return has_callback_unsafe(event_type);
}

bool SubscriberInfo::has_callback_unsafe(rmw_event_type_t event_type) const
{
  // RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE is always used with a callback
  auto status_kind = rmw_gurumdds_cpp::get_status_kind_from_rmw(event_type);
  return ((mask | dds_INCONSISTENT_TOPIC_STATUS) & status_kind) > 0;
}

std::mutex TopicEventListener::mutex_table_;
std::map<dds_Topic*, TopicEventListener*> TopicEventListener::table_;

rmw_ret_t TopicEventListener::associate_listener(dds_Topic * topic) {
  auto event_listener = new(std::nothrow) TopicEventListener{};
  if(nullptr == event_listener) {
    return RMW_RET_ERROR;
  }

  std::lock_guard guard{mutex_table_};
  if(!table_.emplace(topic, event_listener).second) {
    return RMW_RET_ERROR;
  }

  dds_TopicListener listener{};
  listener.on_inconsistent_topic = &TopicEventListener::on_inconsistent_topic;
  dds_Topic_set_listener_context(topic, event_listener);
  dds_Topic_set_listener(topic, &listener, dds_INCONSISTENT_TOPIC_STATUS);
  return RMW_RET_OK;
}

rmw_ret_t TopicEventListener::disassociate_Listener(dds_Topic * topic) {
  std::lock_guard guard{mutex_table_};
  auto it = table_.find(topic);
  if(table_.end() == it) {
    return RMW_RET_ERROR;
  }

  auto listener = it->second;
  std::unique_lock listener_guard{listener->mutex_};
  dds_Topic_set_listener_context(topic, nullptr);
  listener_guard.unlock();
  table_.erase(it);
  delete listener;
  return RMW_RET_OK;
}

void TopicEventListener::on_inconsistent_topic(const dds_Topic* the_topic,
                                               const dds_InconsistentTopicStatus* status) {
  auto topic = const_cast<dds_Topic*>(the_topic);
  auto listener = static_cast<TopicEventListener*>(dds_Topic_get_listener_context(topic));
  if(nullptr == listener) {
    return;
  }

  listener->on_inconsistent_topic(*status);
}

void TopicEventListener::on_inconsistent_topic(const dds_InconsistentTopicStatus& status) {
  std::lock_guard guard{mutex_};
  for(auto it : event_list_) {
    it->update_inconsistent_topic(status.total_count, status.total_count_change);
  }
}

void TopicEventListener::add_event(dds_Topic * topic, EventInfo * event_info) {
  std::lock_guard guard{mutex_table_};
  auto it = table_.find(topic);
  if(table_.end() == it) {
    return;
  }

  auto listener = it->second;
  std::unique_lock listener_guard{listener->mutex_};
  auto list_it = std::find(listener->event_list_.begin(), listener->event_list_.end(),
                           event_info);
  if(listener->event_list_.end() == list_it) {
    return;
  }

  listener->event_list_.emplace_back(event_info);
}

void TopicEventListener::remove_event(dds_Topic * topic, EventInfo * event_info) {
  std::lock_guard guard{mutex_table_};
  auto it = table_.find(topic);
  if(table_.end() == it) {
    return;
  }

    auto listener = it->second;
    std::unique_lock listener_guard{listener->mutex_};
    auto list_it = std::find(listener->event_list_.begin(), listener->event_list_.end(),
                             event_info);
    if(listener->event_list_.end() == list_it) {
      return;
    }

    listener->event_list_.erase(list_it);
}
}  // namespace rmw_gurumdds_cpp

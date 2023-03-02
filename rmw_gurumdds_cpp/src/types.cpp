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

#include <map>
#include <string>
#include <vector>

#include "rmw/impl/cpp/key_value.hpp"

#include "rmw_gurumdds_cpp/event_converter.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/guid.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/types.hpp"

#define ENTITYID_PARTICIPANT 0x000001C1

rmw_ret_t GurumddsPublisherInfo::get_status(
  dds_StatusMask mask,
  void * event)
{
  if (mask == dds_LIVELINESS_LOST_STATUS) {
    dds_LivelinessLostStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataWriter_get_liveliness_lost_status(this->topic_writer, &status);
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
      dds_DataWriter_get_offered_deadline_missed_status(this->topic_writer, &status);
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
      dds_DataWriter_get_offered_incompatible_qos_status(this->topic_writer, &status);
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
  return dds_DataWriter_get_statuscondition(this->topic_writer);
}

dds_StatusMask GurumddsPublisherInfo::get_status_changes()
{
  return dds_DataWriter_get_status_changes(this->topic_writer);
}

rmw_ret_t GurumddsSubscriberInfo::get_status(
  dds_StatusMask mask,
  void * event)
{
  if (mask == dds_LIVELINESS_CHANGED_STATUS) {
    dds_LivelinessChangedStatus status;
    dds_ReturnCode_t dds_ret =
      dds_DataReader_get_liveliness_changed_status(this->topic_reader, &status);
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
      dds_DataReader_get_requested_deadline_missed_status(this->topic_reader, &status);
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
      dds_DataReader_get_requested_incompatible_qos_status(this->topic_reader, &status);
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
  return dds_DataReader_get_statuscondition(this->topic_reader);
}

dds_StatusMask GurumddsSubscriberInfo::get_status_changes()
{
  return dds_DataReader_get_status_changes(this->topic_reader);
}

static std::map<std::string, std::vector<uint8_t>>
__parse_map(uint8_t * const data, const uint32_t data_len)
{
  std::vector<uint8_t> data_vec(data, data + data_len);
  std::map<std::string, std::vector<uint8_t>> map =
    rmw::impl::cpp::parse_key_value(data_vec);

  return map;
}

static rmw_ret_t
__get_user_data_key(
  dds_ParticipantBuiltinTopicData * data,
  const std::string key,
  std::string & value,
  bool & found)
{
  found = false;
  uint8_t * user_data =
    static_cast<uint8_t *>(data->user_data.value);
  const uint32_t user_data_len = data->user_data.size;
  if (nullptr == user_data || user_data_len == 0) {
    return RMW_RET_OK;
  }

  auto map = __parse_map(user_data, user_data_len);
  auto name_found = map.find(key);
  if (name_found != map.end()) {
    value = std::string(name_found->second.begin(), name_found->second.end());
    found = true;
  }

  return RMW_RET_OK;
}

void on_participant_changed(
  const dds_DomainParticipant * a_participant,
  const dds_ParticipantBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  dds_DomainParticipant * participant = const_cast<dds_DomainParticipant *>(a_participant);
  rmw_context_impl_t * ctx =
    reinterpret_cast<rmw_context_impl_t *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0));

  if (ctx == nullptr) {
    return;
  }

  dds_GUID_t dp_guid;
  GuidPrefix_t dp_guid_prefix;
  dds_BuiltinTopicKey_to_GUID(&dp_guid_prefix, data->key);
  memcpy(dp_guid.prefix, dp_guid_prefix.value, sizeof(dp_guid.prefix));
  dp_guid.entityId = ENTITYID_PARTICIPANT;

  if (reinterpret_cast<void *>(handle) == NULL) {
    graph_remove_participant(ctx, &dp_guid);
  } else {
    std::string enclave_str;
    bool enclave_found;
    dds_ReturnCode_t rc =
      __get_user_data_key(
      const_cast<dds_ParticipantBuiltinTopicData *>(data),
      "securitycontext", enclave_str, enclave_found);
    if (RMW_RET_OK != rc) {
      RMW_SET_ERROR_MSG("failed to parse user data for enclave");
    }

    const char * enclave = nullptr;
    if (enclave_found) {
      enclave = enclave_str.c_str();
    }

    if (RMW_RET_OK != graph_add_participant(ctx, &dp_guid, enclave)) {
      RMW_SET_ERROR_MSG("failed to assert remote participant in graph");
    }
  }
}

void on_publication_changed(
  const dds_DomainParticipant * a_participant,
  const dds_PublicationBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  dds_DomainParticipant * participant = const_cast<dds_DomainParticipant *>(a_participant);
  rmw_context_impl_t * ctx =
    reinterpret_cast<rmw_context_impl_t *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0));

  if (ctx == nullptr) {
    return;
  }

  dds_GUID_t endp_guid;
  GuidPrefix_t dp_guid_prefix, endp_guid_prefix;
  dds_BuiltinTopicKey_to_GUID(&dp_guid_prefix, data->participant_key);
  memcpy(endp_guid.prefix, dp_guid_prefix.value, sizeof(endp_guid.prefix));
  dds_BuiltinTopicKey_to_GUID(&endp_guid_prefix, data->key);
  memcpy(&endp_guid.entityId, endp_guid_prefix.value, sizeof(endp_guid.entityId));

  if (reinterpret_cast<void *>(handle) == NULL) {
    RCUTILS_LOG_DEBUG_NAMED(
      "pub on data available",
      "[ud] endp_gid=0x%08X.0x%08X.0x%08X.0x%08X ",
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[0],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[1],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[2],
      endp_guid.entityId);
    graph_remove_entity(ctx, &endp_guid, false);
  } else {
    dds_GUID_t dp_guid;
    memcpy(dp_guid.prefix, dp_guid_prefix.value, sizeof(dp_guid.prefix));
    dp_guid.entityId = ENTITYID_PARTICIPANT;

    graph_add_remote_entity(
      ctx,
      &endp_guid,
      &dp_guid,
      data->topic_name,
      data->type_name,
      &data->reliability,
      &data->durability,
      &data->deadline,
      &data->liveliness,
      &data->lifespan,
      false);

    RCUTILS_LOG_DEBUG_NAMED(
      "pub on data available",
      "dp_gid=0x%08X.0x%08X.0x%08X.0x%08X, "
      "gid=0x%08X.0x%08X.0x%08X.0x%08X, ",
      reinterpret_cast<const uint32_t *>(dp_guid.prefix)[0],
      reinterpret_cast<const uint32_t *>(dp_guid.prefix)[1],
      reinterpret_cast<const uint32_t *>(dp_guid.prefix)[2],
      dp_guid.entityId,
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[0],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[1],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[2],
      endp_guid.entityId);
  }
}

void on_subscription_changed(
  const dds_DomainParticipant * a_participant,
  const dds_SubscriptionBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  dds_DomainParticipant * participant = const_cast<dds_DomainParticipant *>(a_participant);
  rmw_context_impl_t * ctx =
    reinterpret_cast<rmw_context_impl_t *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0));

  if (ctx == nullptr) {
    return;
  }

  dds_GUID_t endp_guid;
  GuidPrefix_t dp_guid_prefix, endp_guid_prefix;
  dds_BuiltinTopicKey_to_GUID(&dp_guid_prefix, data->participant_key);
  memcpy(endp_guid.prefix, dp_guid_prefix.value, sizeof(endp_guid.prefix));
  dds_BuiltinTopicKey_to_GUID(&endp_guid_prefix, data->key);
  memcpy(&endp_guid.entityId, endp_guid_prefix.value, sizeof(endp_guid.entityId));

  if (reinterpret_cast<void *>(handle) == NULL) {
    RCUTILS_LOG_DEBUG_NAMED(
      "sub on data available",
      "[ud] endp_gid=0x%08X.0x%08X.0x%08X.0x%08X ",
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[0],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[1],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[2],
      endp_guid.entityId);
    graph_remove_entity(ctx, &endp_guid, false);
  } else {
    dds_GUID_t dp_guid;
    memcpy(dp_guid.prefix, dp_guid_prefix.value, sizeof(dp_guid.prefix));
    dp_guid.entityId = ENTITYID_PARTICIPANT;

    graph_add_remote_entity(
      ctx,
      &endp_guid,
      &dp_guid,
      data->topic_name,
      data->type_name,
      &data->reliability,
      &data->durability,
      &data->deadline,
      &data->liveliness,
      nullptr,
      true);

    RCUTILS_LOG_DEBUG_NAMED(
      "sub on data available",
      "dp_gid=0x%08X.0x%08X.0x%08X.0x%08X, "
      "gid=0x%08X.0x%08X.0x%08X.0x%08X, ",
      reinterpret_cast<const uint32_t *>(dp_guid.prefix)[0],
      reinterpret_cast<const uint32_t *>(dp_guid.prefix)[1],
      reinterpret_cast<const uint32_t *>(dp_guid.prefix)[2],
      dp_guid.entityId,
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[0],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[1],
      reinterpret_cast<const uint32_t *>(endp_guid.prefix)[2],
      endp_guid.entityId);
  }
}

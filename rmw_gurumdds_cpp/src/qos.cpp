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

#include <cstring>
#include <sstream>
#include <string>

#include "rosidl_runtime_c/type_hash.h"

#include "rmw_dds_common/qos.hpp"
#include "rmw_dds_common/time_utils.hpp"

#include "rmw/impl/cpp/key_value.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include <iomanip>

static inline bool is_time_unspecified(const rmw_time_t & time)
{
  return rmw_time_equal(time, RMW_DURATION_UNSPECIFIED);
}

namespace rmw_gurumdds_cpp
{
dds_Duration_t rmw_time_to_dds(const rmw_time_t & time)
{
  if (rmw_time_equal(time, RMW_DURATION_INFINITE)) {
    dds_Duration_t duration;
    duration.sec = dds_DURATION_INFINITE_SEC;
    duration.nanosec = dds_DURATION_INFINITE_NSEC;
    return duration;
  }
  rmw_time_t clamped_time = rmw_dds_common::clamp_rmw_time_to_dds_time(time);
  dds_Duration_t duration;
  duration.sec = static_cast<int32_t>(clamped_time.sec);
  duration.nanosec = static_cast<uint32_t>(clamped_time.nsec);
  return duration;
}

rmw_time_t dds_duration_to_rmw(const dds_Duration_t & duration)
{
  if (duration.sec == dds_DURATION_INFINITE_SEC &&
    duration.nanosec == dds_DURATION_INFINITE_NSEC)
  {
    return RMW_DURATION_INFINITE;
  }
  rmw_time_t time;
  time.sec = static_cast<uint64_t>(duration.sec);
  time.nsec = static_cast<uint64_t>(duration.nanosec);
  return time;
}

int64_t dds_time_to_i64(const dds_Time_t & t)
{
  return (static_cast<int64_t>(t.sec) * static_cast<int64_t>(1000000000ULL)) +
         static_cast<int64_t>(t.nanosec);
}

template<typename dds_EntityQos>
bool set_entity_qos_from_profile_generic(
  const rmw_qos_profile_t *qos_profile,
  dds_EntityQos *entity_qos)
{
  switch (qos_profile->history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      entity_qos->history.kind = dds_KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      entity_qos->history.kind = dds_KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("unknown qos history policy");
      return false;
  }

  switch (qos_profile->reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      entity_qos->reliability.kind = dds_BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      entity_qos->reliability.kind = dds_RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("unknown qos reliability policy");
      return false;
  }

  switch (qos_profile->durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      entity_qos->durability.kind = dds_TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      entity_qos->durability.kind = dds_VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("unknown qos durability policy");
      return false;
  }

  if (qos_profile->depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    entity_qos->history.depth = static_cast<int32_t>(qos_profile->depth);

    entity_qos->resource_limits.max_samples = entity_qos->history.depth;
    entity_qos->resource_limits.max_instances = 1;
    entity_qos->resource_limits.max_samples_per_instance =
      entity_qos->history.depth;
  } else if (qos_profile->history == RMW_QOS_POLICY_HISTORY_KEEP_ALL) {
    // NOTE: These values might be changed after further insepction
    entity_qos->resource_limits.max_samples = 4096;
    entity_qos->resource_limits.max_instances = 1;
    entity_qos->resource_limits.max_samples_per_instance = 4096;
  }

  if (!is_time_unspecified(qos_profile->deadline)) {
    entity_qos->deadline.period = rmw_time_to_dds(qos_profile->deadline);
  }

  switch (qos_profile->liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      entity_qos->liveliness.kind = dds_AUTOMATIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      entity_qos->liveliness.kind = dds_MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("unknown qos liveliness policy");
      return false;
  }

  if (!is_time_unspecified(qos_profile->liveliness_lease_duration)) {
    entity_qos->liveliness.lease_duration =
      rmw_time_to_dds(qos_profile->liveliness_lease_duration);
  }

  return true;
}

bool get_topic_qos(
  const rmw_qos_profile_t *qos_policies,
  dds_TopicQos *topic_qos)
{
  return set_entity_qos_from_profile_generic(qos_policies, topic_qos);
}

bool get_datawriter_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataWriterQos *datawriter_qos)
{

  if (!is_time_unspecified(qos_profile->lifespan)) {
    datawriter_qos->lifespan.duration = rmw_time_to_dds(qos_profile->lifespan);
  }

  set_entity_qos_from_profile_generic(qos_profile, datawriter_qos);

  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(
                        type_hash, user_data_str))
  {
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }

  size_t user_data_size = user_data_str.size();
  if (user_data_size > sizeof(dds_UserDataQosPolicy::value)) {
    user_data_size = sizeof(dds_UserDataQosPolicy::value);
  }

  std::memset(datawriter_qos->user_data.value, 0, user_data_size);
  std::memcpy(datawriter_qos->user_data.value, user_data_str.data(),
              user_data_size);
  datawriter_qos->user_data.size = user_data_size;

  return true;
}

bool get_datawriter_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataWriterQos *datawriter_qos,
  const rosidl_type_hash_t & service_type_hash)
{

  if (!is_time_unspecified(qos_profile->lifespan)) {
    datawriter_qos->lifespan.duration = rmw_time_to_dds(qos_profile->lifespan);
  }

  set_entity_qos_from_profile_generic(qos_profile, datawriter_qos);

  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(
                        type_hash, user_data_str))
  {
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }

  std::string service_type_hash_str;
  if (RMW_RET_OK == rmw_dds_common::encode_sertype_hash_for_user_data_qos(
                        service_type_hash, service_type_hash_str))
  {
    user_data_str += service_type_hash_str;
  } else {
    rmw_reset_error();
  }

  size_t user_data_size = user_data_str.size();
  if (user_data_size > sizeof(dds_UserDataQosPolicy::value)) {
    user_data_size = sizeof(dds_UserDataQosPolicy::value);
  }

  std::memset(datawriter_qos->user_data.value, 0, user_data_size);
  std::memcpy(datawriter_qos->user_data.value, user_data_str.data(),
              user_data_size);
  datawriter_qos->user_data.size = user_data_size;

  return true;
}

bool get_datareader_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataReaderQos *datareader_qos)
{
  set_entity_qos_from_profile_generic(qos_profile, datareader_qos);

  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(
                        type_hash, user_data_str))
  {
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }

  size_t user_data_size = user_data_str.size();
  if (user_data_size > sizeof(dds_UserDataQosPolicy::value)) {
    user_data_size = sizeof(dds_UserDataQosPolicy::value);
  }

  std::memset(datareader_qos->user_data.value, 0, user_data_size);
  std::memcpy(datareader_qos->user_data.value, user_data_str.data(),
              user_data_size);
  datareader_qos->user_data.size = user_data_size;

  return true;
}

bool get_datareader_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataReaderQos *datareader_qos,
  const rosidl_type_hash_t & service_type_hash)
{
  set_entity_qos_from_profile_generic(qos_profile, datareader_qos);

  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(
                        type_hash, user_data_str))
  {
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }

  std::string service_type_hash_str;
  if (RMW_RET_OK == rmw_dds_common::encode_sertype_hash_for_user_data_qos(
                        service_type_hash, service_type_hash_str))
  {
    user_data_str += service_type_hash_str;
  } else {
    rmw_reset_error();
  }

  size_t user_data_size = user_data_str.size();
  if (user_data_size > sizeof(dds_UserDataQosPolicy::value)) {
    user_data_size = sizeof(dds_UserDataQosPolicy::value);
  }

  std::memset(datareader_qos->user_data.value, 0, user_data_size);
  std::memcpy(datareader_qos->user_data.value, user_data_str.data(),
              user_data_size);
  datareader_qos->user_data.size = user_data_size;

  return true;
}

constexpr const char *BUFFER_BACKEND_KEY = "bufbe";

std::string encode_buffer_backends_for_user_data(
  const std::unordered_map<std::string, std::string> & backends)
{
  if (backends.empty()) {
    return {};
  }
  std::ostringstream ss;
  ss << BUFFER_BACKEND_KEY << '=';
  bool first = true;
  for (const auto &[name, aux] : backends) {
    if (!first) {
      ss << ',';
    }
    ss << name << ':' << aux;
    first = false;
  }
  ss << ';';
  return ss.str();
}

// backend_buffer 전용
bool get_datawriter_qos(
  const rmw_qos_profile_t *qos_profile, const rosidl_type_hash_t & type_hash,
  dds_DataWriterQos *datawriter_qos,
  std::unordered_map<std::string, std::string> & backend_metadata)
{

  if (!is_time_unspecified(qos_profile->lifespan)) {
    datawriter_qos->lifespan.duration = rmw_time_to_dds(qos_profile->lifespan);
  }

  set_entity_qos_from_profile_generic(qos_profile, datawriter_qos);

  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(
                        type_hash, user_data_str))
  {
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }

  if (!backend_metadata.empty()) {
    user_data_str += encode_buffer_backends_for_user_data(backend_metadata);
  }

  size_t user_data_size = user_data_str.size();
  if (user_data_size > sizeof(dds_UserDataQosPolicy::value)) {
    user_data_size = sizeof(dds_UserDataQosPolicy::value);
  }

  std::memset(datawriter_qos->user_data.value, 0, user_data_size);
  std::memcpy(datawriter_qos->user_data.value, user_data_str.data(),
              user_data_size);
  datawriter_qos->user_data.size = user_data_size;

  return true;
}

std::unordered_map<std::string, std::string>
parse_buffer_backends_from_user_data(const uint8_t *data, size_t size)
{
  std::unordered_map<std::string, std::string> result;
  if (!data || size == 0) {
    return result;
  }
  std::vector<uint8_t> udvec(data, data + size);
  auto kv = rmw::impl::cpp::parse_key_value(udvec);
  auto it = kv.find(BUFFER_BACKEND_KEY);
  if (it == kv.end()) {
    return result;
  }
  std::string backends_str(it->second.begin(), it->second.end());
  std::istringstream ss(backends_str);
  std::string entry;
  while (std::getline(ss, entry, ',')) {
    if (entry.empty()) {
      continue;
    }
    size_t colon = entry.find(':');
    std::string name = entry.substr(0, colon);
    std::string aux =
      (colon == std::string::npos) ? "" : entry.substr(colon + 1);
    if (!name.empty()) {
      result[name] = aux;
    }
  }
  return result;
}

inline bool parse_endpoint_gid_from_user_data(
  const uint8_t *data, size_t size,
  const char *tag, rmw_gid_t & gid)
{
  size_t tag_len = strlen(tag);
  if (!data || size < tag_len) {
    return false;
  }
  std::string str(reinterpret_cast<const char *>(data), size);
  auto pos = str.find(tag);
  if (pos == std::string::npos) {
    return false;
  }
  std::string hex_str = str.substr(pos + tag_len, RMW_GID_STORAGE_SIZE * 2);
  if (hex_str.size() != RMW_GID_STORAGE_SIZE * 2) {
    return false;
  }
  for (size_t i = 0; i < RMW_GID_STORAGE_SIZE; ++i) {
    unsigned val = 0;
    std::istringstream iss(hex_str.substr(i * 2, 2));
    iss >> std::hex >> val;
    gid.data[i] = static_cast<uint8_t>(val);
  }
  return true;
}

// backend_buffer
bool get_datareader_qos(
  const rmw_qos_profile_t *qos_profile, const rosidl_type_hash_t & type_hash,
  dds_DataReaderQos *datareader_qos,
  std::unordered_map<std::string, std::string> & backend_metadata)
{
  set_entity_qos_from_profile_generic(qos_profile, datareader_qos);

  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(
                        type_hash, user_data_str))
  {
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }

  if (!backend_metadata.empty()) {
    user_data_str += encode_buffer_backends_for_user_data(backend_metadata);
  }

  size_t user_data_size = user_data_str.size();
  if (user_data_size > sizeof(dds_UserDataQosPolicy::value)) {
    user_data_size = sizeof(dds_UserDataQosPolicy::value);
  }

  std::memset(datareader_qos->user_data.value, 0, user_data_size);
  std::memcpy(datareader_qos->user_data.value, user_data_str.data(),
              user_data_size);
  datareader_qos->user_data.size = user_data_size;

  return true;
}

rmw_qos_history_policy_t
convert_history(const dds_HistoryQosPolicy *const policy)
{
  switch (policy->kind) {
    case dds_KEEP_LAST_HISTORY_QOS:
      return RMW_QOS_POLICY_HISTORY_KEEP_LAST;
    case dds_KEEP_ALL_HISTORY_QOS:
      return RMW_QOS_POLICY_HISTORY_KEEP_ALL;
    default:
      return RMW_QOS_POLICY_HISTORY_UNKNOWN;
  }
}

rmw_qos_reliability_policy_t
convert_reliability(const dds_ReliabilityQosPolicy *const policy)
{
  switch (policy->kind) {
    case dds_BEST_EFFORT_RELIABILITY_QOS:
      return RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
    case dds_RELIABLE_RELIABILITY_QOS:
      return RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    default:
      return RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
  }
}

rmw_qos_durability_policy_t
convert_durability(const dds_DurabilityQosPolicy *const policy)
{
  switch (policy->kind) {
    case dds_VOLATILE_DURABILITY_QOS:
      return RMW_QOS_POLICY_DURABILITY_VOLATILE;
    case dds_TRANSIENT_LOCAL_DURABILITY_QOS:
      return RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
    default:
      return RMW_QOS_POLICY_DURABILITY_UNKNOWN;
  }
}

rmw_time_t convert_deadline(const dds_DeadlineQosPolicy *const policy)
{
  return dds_duration_to_rmw(policy->period);
}

rmw_time_t convert_lifespan(const dds_LifespanQosPolicy *const policy)
{
  rmw_time_t time = RMW_DURATION_INFINITE;
  return policy == nullptr ? time : dds_duration_to_rmw(policy->duration);
}

rmw_qos_liveliness_policy_t
convert_liveliness(const dds_LivelinessQosPolicy *const policy)
{
  switch (policy->kind) {
    case dds_AUTOMATIC_LIVELINESS_QOS:
      return RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
    case dds_MANUAL_BY_TOPIC_LIVELINESS_QOS:
      return RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
    default:
      return RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
  }
}

rmw_time_t
convert_liveliness_lease_duration(const dds_LivelinessQosPolicy *const policy)
{
  return dds_duration_to_rmw(policy->lease_duration);
}

rmw_qos_policy_kind_t convert_qos_policy(const dds_QosPolicyId_t policy_id)
{
  switch (policy_id) {
    case dds_HISTORY_QOS_POLICY_ID:
      return RMW_QOS_POLICY_HISTORY;
    case dds_RELIABILITY_QOS_POLICY_ID:
      return RMW_QOS_POLICY_RELIABILITY;
    case dds_DURABILITY_QOS_POLICY_ID:
      return RMW_QOS_POLICY_DURABILITY;
    case dds_DEADLINE_QOS_POLICY_ID:
      return RMW_QOS_POLICY_DEADLINE;
    case dds_LIFESPAN_QOS_POLICY_ID:
      return RMW_QOS_POLICY_LIFESPAN;
    case dds_LIVELINESS_QOS_POLICY_ID:
      return RMW_QOS_POLICY_LIVELINESS;
    default:
      return RMW_QOS_POLICY_INVALID;
  }
}

// backend_buffer
std::string encode_endpoint_gid_for_user_data(
  const rmw_gid_t & gid,
  const char *tag)
{
  std::ostringstream ss;
  ss << tag;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < RMW_GID_STORAGE_SIZE; ++i) {
    ss << std::setw(2) << static_cast<unsigned>(gid.data[i]);
  }
  return ss.str();
}

bool is_valid_qos(const rmw_qos_profile_t *qos)
{
  if (qos == nullptr) {
    RMW_SET_ERROR_MSG("qos_policies argument is null");
    return false;
  }

  if (qos->history == RMW_QOS_POLICY_HISTORY_UNKNOWN) {
    RMW_SET_ERROR_MSG("publisher qos history is unknown");
    return false;
  }

  if (qos->reliability == RMW_QOS_POLICY_RELIABILITY_UNKNOWN) {
    RMW_SET_ERROR_MSG("publisher qos reliability is unknown");
    return false;
  }

  if (qos->durability == RMW_QOS_POLICY_DURABILITY_UNKNOWN) {
    RMW_SET_ERROR_MSG("publisher qos durability is unknown");
    return false;
  }

  if (qos->liveliness == RMW_QOS_POLICY_LIVELINESS_UNKNOWN) {
    RMW_SET_ERROR_MSG("publisher qos liveliness is unknown");
    return false;
  }

  if (qos->history == RMW_QOS_POLICY_HISTORY_KEEP_LAST && qos->depth == 0) {
    RMW_SET_ERROR_MSG("publisher qos depth is zero for KEEP_LAST history");
    return false;
  }

  return true;
}
} // namespace rmw_gurumdds_cpp

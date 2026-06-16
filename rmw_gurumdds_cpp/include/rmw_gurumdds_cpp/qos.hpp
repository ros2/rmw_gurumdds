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

#ifndef RMW_GURUMDDS_CPP__QOS_HPP_
#define RMW_GURUMDDS_CPP__QOS_HPP_

#include <cassert>
#include <limits>

#include "rmw/error_handling.h"
#include "rmw/incompatible_qos_events_statuses.h"
#include "rmw/types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"

namespace rmw_gurumdds_cpp
{
dds_Duration_t rmw_time_to_dds(const rmw_time_t & time);

rmw_time_t dds_duration_to_rmw(const dds_Duration_t & duration);

int64_t dds_time_to_i64(const dds_Time_t & t);

bool get_topic_qos(
  const rmw_qos_profile_t *qos_policies,
  dds_TopicQos *topic_qos);

bool get_datareader_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataReaderQos *datareader_qos);

bool get_datawriter_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataWriterQos *datawriter_qos);

bool get_datareader_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataReaderQos *datareader_qos,
  const rosidl_type_hash_t & service_type_hash);

bool get_datawriter_qos(
  const rmw_qos_profile_t *qos_profile,
  const rosidl_type_hash_t & type_hash,
  dds_DataWriterQos *datawriter_qos,
  const rosidl_type_hash_t & service_type_hash);

// backend_buffer 전용
bool get_datawriter_qos(
  const rmw_qos_profile_t *qos_profile, const rosidl_type_hash_t & type_hash,
  dds_DataWriterQos *datawriter_qos,
  std::unordered_map<std::string, std::string> & backend_metadata);

// backend_buffer 전용
bool get_datareader_qos(
  const rmw_qos_profile_t *qos_profile, const rosidl_type_hash_t & type_hash,
  dds_DataReaderQos *datareader_qos,
  std::unordered_map<std::string, std::string> & backend_metadata);

rmw_qos_history_policy_t
convert_history(const dds_HistoryQosPolicy *const policy);

rmw_qos_reliability_policy_t
convert_reliability(const dds_ReliabilityQosPolicy *const policy);

rmw_qos_durability_policy_t
convert_durability(const dds_DurabilityQosPolicy *const policy);

rmw_time_t convert_deadline(const dds_DeadlineQosPolicy *const policy);

rmw_time_t convert_lifespan(const dds_LifespanQosPolicy *const policy);

rmw_qos_liveliness_policy_t
convert_liveliness(const dds_LivelinessQosPolicy *const policy);

rmw_time_t
convert_liveliness_lease_duration(const dds_LivelinessQosPolicy *const policy);

rmw_qos_policy_kind_t convert_qos_policy(const dds_QosPolicyId_t policy_id);

std::string encode_buffer_backends_for_user_data(
  const std::unordered_map<std::string, std::string> & backends);

std::unordered_map<std::string, std::string>
parse_buffer_backends_from_user_data(const uint8_t *data, size_t size);

std::string encode_endpoint_gid_for_user_data(
  const rmw_gid_t & gid,
  const char *tag);

bool parse_endpoint_gid_from_user_data(
  const uint8_t *data, size_t size,
  const char *tag, rmw_gid_t & gid);

bool is_valid_qos(const rmw_qos_profile_t *qos);

} // namespace rmw_gurumdds_cpp

#endif // RMW_GURUMDDS_CPP__QOS_HPP_

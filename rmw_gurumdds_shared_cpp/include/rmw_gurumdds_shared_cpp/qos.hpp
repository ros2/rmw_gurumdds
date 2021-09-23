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

#ifndef RMW_GURUMDDS_SHARED_CPP__QOS_HPP_
#define RMW_GURUMDDS_SHARED_CPP__QOS_HPP_

#include <cassert>
#include <limits>

#include "rmw/error_handling.h"
#include "rmw/types.h"
#include "rmw/incompatible_qos_events_statuses.h"

#include "rmw_gurumdds_shared_cpp/visibility_control.h"

#include "./dds_include.hpp"

RMW_GURUMDDS_SHARED_CPP_PUBLIC
bool
get_datawriter_qos(
  dds_Publisher * publisher,
  const rmw_qos_profile_t * qos_profile,
  dds_DataWriterQos * datawriter_qos);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
bool
get_datareader_qos(
  dds_Subscriber * subscriber,
  const rmw_qos_profile_t * qos_profile,
  dds_DataReaderQos * datareader_qos);

rmw_qos_reliability_policy_t
convert_reliability(
  dds_ReliabilityQosPolicy policy);

rmw_qos_durability_policy_t
convert_durability(
  dds_DurabilityQosPolicy policy);

rmw_time_t
convert_deadline(
  dds_DeadlineQosPolicy policy);

rmw_time_t
convert_lifespan(
  dds_LifespanQosPolicy policy);

rmw_qos_liveliness_policy_t
convert_liveliness(
  dds_LivelinessQosPolicy policy);

rmw_time_t
convert_liveliness_lease_duration(
  dds_LivelinessQosPolicy policy);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_qos_policy_kind_t
convert_qos_policy(
  dds_QosPolicyId_t policy_id);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
dds_Duration_t rmw_time_to_dds(
  const rmw_time_t & time);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_time_t dds_duration_to_rmw(
  const dds_Duration_t & duration);
#endif  // RMW_GURUMDDS_SHARED_CPP__QOS_HPP_

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
#include "rmw_gurumdds_cpp/visibility_control.h"

RMW_GURUMDDS_CPP_PUBLIC
bool
get_datawriter_qos(
  dds_Publisher * publisher,
  const rmw_qos_profile_t * qos_profile,
  dds_DataWriterQos * datawriter_qos);

RMW_GURUMDDS_CPP_PUBLIC
bool
get_datareader_qos(
  dds_Subscriber * subscriber,
  const rmw_qos_profile_t * qos_profile,
  dds_DataReaderQos * datareader_qos);

enum rmw_qos_history_policy_t
convert_history(
  const dds_HistoryQosPolicy * const policy);

enum rmw_qos_reliability_policy_t
convert_reliability(
  const dds_ReliabilityQosPolicy * const policy);

enum rmw_qos_durability_policy_t
convert_durability(
  const dds_DurabilityQosPolicy * const policy);

struct rmw_time_t
convert_deadline(
  const dds_DeadlineQosPolicy * const policy);

struct rmw_time_t
convert_lifespan(
  const dds_LifespanQosPolicy * const policy);

enum rmw_qos_liveliness_policy_t
convert_liveliness(
  const dds_LivelinessQosPolicy * const policy);

struct rmw_time_t
convert_liveliness_lease_duration(
  const dds_LivelinessQosPolicy * const policy);

RMW_GURUMDDS_CPP_PUBLIC
rmw_qos_policy_kind_t
convert_qos_policy(
  dds_QosPolicyId_t policy_id);

#endif  // RMW_GURUMDDS_CPP__QOS_HPP_

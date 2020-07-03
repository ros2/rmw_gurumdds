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

#include <limits>
#include "rmw_gurumdds_shared_cpp/qos.hpp"

bool is_time_default(const rmw_time_t & time)
{
  return time.sec == 0 && time.nsec == 0;
}

dds_Duration_t
rmw_time_to_dds(const rmw_time_t & time)
{
  dds_Duration_t duration;
  duration.sec = static_cast<int32_t>(time.sec);
  duration.nanosec = static_cast<uint32_t>(time.nsec);
  return duration;
}

template<typename dds_EntityQos>
bool
set_entity_qos_from_profile_generic(
  const rmw_qos_profile_t * qos_profile,
  dds_EntityQos * entity_qos)
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
    entity_qos->resource_limits.max_samples_per_instance = entity_qos->history.depth;
  } else if (qos_profile->history == RMW_QOS_POLICY_HISTORY_KEEP_ALL) {
    // NOTE: These values might be changed after further insepction
    entity_qos->resource_limits.max_samples = 4096;
    entity_qos->resource_limits.max_instances = 1;
    entity_qos->resource_limits.max_samples_per_instance = 4096;
  }

  if (!is_time_default(qos_profile->deadline)) {
    entity_qos->deadline.period = rmw_time_to_dds(qos_profile->deadline);
  }

  switch (qos_profile->liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      entity_qos->liveliness.kind = dds_AUTOMATIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_NODE:
      entity_qos->liveliness.kind = dds_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
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

  if (!is_time_default(qos_profile->liveliness_lease_duration)) {
    entity_qos->liveliness.lease_duration = rmw_time_to_dds(qos_profile->liveliness_lease_duration);
  }

  return true;
}

bool
get_datawriter_qos(
  dds_Publisher * publisher,
  const rmw_qos_profile_t * qos_profile,
  dds_DataWriterQos * datawriter_qos)
{
  dds_ReturnCode_t ret = dds_Publisher_get_default_datawriter_qos(publisher, datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datawriter qos");
    return false;
  }

  if (!is_time_default(qos_profile->lifespan)) {
    datawriter_qos->lifespan.duration = rmw_time_to_dds(qos_profile->lifespan);
  }

  set_entity_qos_from_profile_generic(qos_profile, datawriter_qos);

  return true;
}

bool get_datareader_qos(
  dds_Subscriber * subscriber,
  const rmw_qos_profile_t * qos_profile,
  dds_DataReaderQos * datareader_qos)
{
  dds_ReturnCode_t ret = dds_Subscriber_get_default_datareader_qos(subscriber, datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datareader qos");
    return false;
  }

  set_entity_qos_from_profile_generic(qos_profile, datareader_qos);

  return true;
}

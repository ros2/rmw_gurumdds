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

#ifndef RMW_GURUMDDS_CPP__GID_HPP_
#define RMW_GURUMDDS_CPP__GID_HPP_

#include "rmw/types.h"

#include "rmw_dds_common/gid_utils.hpp"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"

namespace rmw_gurumdds_cpp
{
void ros_guid_to_dds_guid(const uint8_t * guid_ros, uint8_t * guid_dds);

void dds_guid_to_ros_guid(const int8_t * guid_dds, int8_t * guid_ros);

void guid_to_gid(const dds_GUID_t & guid, rmw_gid_t & gid);

void entity_get_gid(dds_Entity * const entity, rmw_gid_t & gid);
/**
 * Structure to hold GUID information for DDS instances.
 */
struct Guid_t: public dds_GUID_t
{
  static constexpr uint32_t ENTITYID_PARTICIPANT = 0x000001C1;

  Guid_t();

  explicit Guid_t(const dds_GUID_t & other);

  explicit Guid_t(const dds_ParticipantBuiltinTopicData& builtin_topic_data);

  template<typename TBuiltinTopicData>
  explicit Guid_t(const TBuiltinTopicData& builtin_topic_data);

  template<typename TBuiltinTopicData>
  static Guid_t for_participant(const TBuiltinTopicData& builtin_topic_data);

  bool operator==(const dds_GUID_t & other) const;

  bool operator!=(const Guid_t & other) const;

  bool operator<(const Guid_t & other) const;
};  // struct Guid_t
}  // namespace rmw_gurumdds_cpp

#include "rmw_gurumdds_cpp/gid.inl"

#endif  // RMW_GURUMDDS_CPP__GID_HPP_

// Copyright 2024 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__GID_INL_
#define RMW_GURUMDDS_CPP__GID_INL_

#include <cstring>

namespace rmw_gurumdds_cpp
{
inline void ros_guid_to_dds_guid(const uint8_t * guid_ros, uint8_t * guid_dds)
{
  std::memcpy(guid_dds, guid_ros, 12);
  std::memcpy(&guid_dds[12], &guid_ros[12], 4);
}

inline void dds_guid_to_ros_guid(const int8_t * guid_dds, int8_t * guid_ros)
{
  std::memcpy(guid_ros, guid_dds, 12);
  std::memcpy(&guid_ros[12], &guid_dds[12], 4);
}

inline void guid_to_gid(const dds_GUID_t & guid, rmw_gid_t & gid)
{
  static_assert(
    RMW_GID_STORAGE_SIZE >= sizeof(guid),
    "rmw_gid_t type too small for an dds GUID");
  std::memset(&gid, 0, sizeof(gid));
  std::memcpy(gid.data, reinterpret_cast<const void *>(&guid), sizeof(guid));
  gid.implementation_identifier = RMW_GURUMDDS_ID;
}

inline void entity_get_gid(dds_Entity * const entity, rmw_gid_t & gid)
{
  dds_GUID_t dds_guid;
  if (dds_Entity_get_guid(entity, &dds_guid) == dds_RETCODE_OK) {
    guid_to_gid(dds_guid, gid);
  }
}

template<typename TBuiltinTopicData>
Guid_t::Guid_t(const TBuiltinTopicData& builtin_topic_data) {
  static_assert(std::is_same_v<TBuiltinTopicData, dds_PublicationBuiltinTopicData>
    || std::is_same_v<TBuiltinTopicData, dds_SubscriptionBuiltinTopicData>);
  std::memcpy(prefix, &builtin_topic_data.participant_key, sizeof(prefix));
  std::memcpy(&entityId, &builtin_topic_data.key, sizeof(entityId));
}

template<typename TBuiltinTopicData>
Guid_t Guid_t::for_participant(const TBuiltinTopicData& builtin_topic_data) {
  static_assert(std::is_same_v<TBuiltinTopicData, dds_PublicationBuiltinTopicData>
    || std::is_same_v<TBuiltinTopicData, dds_SubscriptionBuiltinTopicData>);
  Guid_t guid;
  std::memcpy(guid.prefix, &builtin_topic_data.participant_key, sizeof(guid.prefix));
  guid.entityId = rmw_gurumdds_cpp::Guid_t::ENTITYID_PARTICIPANT;
  return guid;
}
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__GID_INL_

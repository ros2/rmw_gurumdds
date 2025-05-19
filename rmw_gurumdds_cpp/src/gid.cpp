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
#include <type_traits>

#include "rmw/types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"

namespace rmw_gurumdds_cpp
{
Guid_t::Guid_t() : dds_GUID_t{} {}

Guid_t::Guid_t(const dds_GUID_t & other) : dds_GUID_t{other} {}

Guid_t::Guid_t(const dds_ParticipantBuiltinTopicData& builtin_topic_data) {
  std::memcpy(prefix, &builtin_topic_data.key, sizeof(prefix));
  entityId = ENTITYID_PARTICIPANT;
}

bool Guid_t::operator==(const dds_GUID_t & other) const
{
  return std::memcmp(this, &other, sizeof(dds_GUID_t)) == 0;
}

bool Guid_t::operator!=(const Guid_t & other) const
{
  return std::memcmp(this, &other, sizeof(dds_GUID_t)) != 0;
}

bool Guid_t::operator<(const Guid_t & other) const
{
  return std::memcmp(this, &other, sizeof(dds_GUID_t)) < 0;
}
}  // namespace rmw_gurumdds_cpp

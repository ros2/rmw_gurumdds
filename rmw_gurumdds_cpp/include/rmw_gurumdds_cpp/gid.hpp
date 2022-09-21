// Copyright 2022 GurumNetworks, Inc.
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

#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "rmw/types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"

inline
void guid_to_gid(const dds_GUID_t & guid, rmw_gid_t & gid)
{
  static_assert(
    RMW_GID_STORAGE_SIZE >= sizeof(guid),
    "rmw_gid_t type too small for an dds GUID");
  memset(&gid, 0, sizeof(gid));
  memcpy(gid.data, reinterpret_cast<const void *>(&guid), sizeof(guid));
  gid.implementation_identifier = RMW_GURUMDDS_ID;
}

inline
void entity_get_gid(dds_Entity * const entity, rmw_gid_t & gid)
{
  dds_GUID_t dds_guid;
  if (dds_Entity_get_guid(entity, &dds_guid) == dds_RETCODE_OK) {
    guid_to_gid(dds_guid, gid);
  }
}

#endif  // RMW_GURUMDDS_CPP__GID_HPP_

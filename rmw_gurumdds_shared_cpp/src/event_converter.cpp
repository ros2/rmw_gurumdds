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

#include <unordered_map>
#include <utility>

#include "rmw_gurumdds_shared_cpp/event_converter.hpp"

/// mapping of RMW_EVENT to the corresponding dds_StatusKind.
static const std::unordered_map<rmw_event_type_t, dds_StatusKind, std::hash<int>> mask_map = {
  {RMW_EVENT_LIVELINESS_CHANGED, dds_LIVELINESS_CHANGED_STATUS},
  {RMW_EVENT_REQUESTED_DEADLINE_MISSED, dds_REQUESTED_DEADLINE_MISSED_STATUS},
  {RMW_EVENT_LIVELINESS_LOST, dds_LIVELINESS_LOST_STATUS},
  {RMW_EVENT_OFFERED_DEADLINE_MISSED, dds_OFFERED_DEADLINE_MISSED_STATUS},
};

dds_StatusKind get_status_kind_from_rmw(const rmw_event_type_t event_t)
{
  return mask_map.at(event_t);
}

bool is_event_supported(const rmw_event_type_t event_t)
{
  return mask_map.count(event_t) > 0;
}

rmw_ret_t check_dds_ret_code(const dds_ReturnCode_t dds_return_code)
{
  if (dds_return_code == dds_RETCODE_OK) {
    return RMW_RET_OK;
  } else if (dds_return_code == dds_RETCODE_ERROR) {
    return RMW_RET_ERROR;
  } else if (dds_return_code == dds_RETCODE_TIMEOUT) {
    return RMW_RET_TIMEOUT;
  } else {
    return RMW_RET_ERROR;
  }
}

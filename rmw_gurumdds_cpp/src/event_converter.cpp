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

#include <utility>

#include "rmw_gurumdds_cpp/event_converter.hpp"

/// mapping of RMW_EVENT to the corresponding dds_StatusKind.
static const dds_StatusKind g_mask_map[] {
  dds_LIVELINESS_CHANGED_STATUS,  // RMW_EVENT_LIVELINESS_CHANGED
  dds_REQUESTED_DEADLINE_MISSED_STATUS,  // RMW_EVENT_REQUESTED_DEADLINE_MISSED
  dds_REQUESTED_INCOMPATIBLE_QOS_STATUS,  // RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE
  dds_SAMPLE_LOST_STATUS,  // RMW_EVENT_MESSAGE_LOST
  dds_LIVELINESS_LOST_STATUS,  // RMW_EVENT_LIVELINESS_LOST
  dds_OFFERED_DEADLINE_MISSED_STATUS,  // RMW_EVENT_OFFERED_DEADLINE_MISSED
  dds_OFFERED_INCOMPATIBLE_QOS_STATUS  // RMW_EVENT_OFFERED_QOS_INCOMPATIBLE
};

dds_StatusKind get_status_kind_from_rmw(const rmw_event_type_t event_t)
{
  if (!is_event_supported(event_t)) {
    return 0;
  }

  return g_mask_map[static_cast<int>(event_t)];
}

bool is_event_supported(const rmw_event_type_t event_t)
{
  return 0 <= event_t && event_t < RMW_EVENT_INVALID;
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

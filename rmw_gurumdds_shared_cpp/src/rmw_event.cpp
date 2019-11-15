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

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/event_converter.hpp"

rmw_ret_t
shared__rmw_take_event(
  const char * implementation_identifier,
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(event_handle, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(event_info, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    event handle,
    event_handle->implementation_identifier,
    implementation_identifier,
    return RMW_RET_ERROR);

  rmw_ret_t ret_code = RMW_RET_UNSUPPORTED;

  if (is_event_supported(event_handle->event_type)) {
    dds_StatusKind status_kind = get_status_kind_from_rmw(event_handle->event_type);
    auto custom_event_info = static_cast<GurumddsEventInfo *>(event_handle->data);
    ret_code = custom_event_info->get_status(status_kind, event_info);
  } else {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("event %d not supported", event_handle->event_type);
  }

  *taken = (ret_code == RMW_RET_OK);
  return ret_code;
}

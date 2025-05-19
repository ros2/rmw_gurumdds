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

#ifndef RMW_GURUMDDS_CPP__EVENT_CONVERTER_HPP_
#define RMW_GURUMDDS_CPP__EVENT_CONVERTER_HPP_

#include "rmw/event.h"
#include "rmw/ret_types.h"
#include "rmw/types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"

namespace rmw_gurumdds_cpp
{
/// Return the corresponding DDS_StatusKind to the input RMW_EVENT.
/**
 * \param event_t to translate to status kind
 * \return the StatusKind corresponding to the rmw_event_type_t
 */
dds_StatusKind get_status_kind_from_rmw(rmw_event_type_t event_t);

/// Return true if the input RMW event has a corresponding DDS_StatusKind.
/**
 * \param event_t input rmw event to check
 * \return true if there is an RMW to DDS_StatusKind mapping, false otherwise
 */
bool is_event_supported(rmw_event_type_t event_t);

/// Assign the input DDS return code to its corresponding RMW return code.
/**
  * \param dds_return_code input DDS return code
  * \return to_return the corresponding rmw_ret_t that maps to the input DDS_ReturnCode_t. By
  * default RMW_RET_ERROR is returned if no corresponding rmw_ret_t is not defined.
  */
rmw_ret_t check_dds_ret_code(dds_ReturnCode_t dds_return_code);
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__EVENT_CONVERTER_HPP_

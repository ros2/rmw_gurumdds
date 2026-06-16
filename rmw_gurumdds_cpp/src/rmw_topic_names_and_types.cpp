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

#include <map>
#include <set>

#include "rcutils/allocator.h"
#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/fastrtps.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

extern "C" {
rmw_ret_t rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator,
    "allocator argument is invalid",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  if (rmw_names_and_types_check_zero(topic_names_and_types) != RMW_RET_OK) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  rmw_gurumdds_cpp::DemangleFunction demangle_topic =
    rmw_gurumdds_cpp::demangle_ros_topic_from_topic;
  rmw_gurumdds_cpp::DemangleFunction demangle_type = rmw_gurumdds_cpp::demangle_if_ros_type;

  if (no_demangle) {
    demangle_topic = rmw_gurumdds_cpp::identity_demangle;
    demangle_type = rmw_gurumdds_cpp::identity_demangle;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  return common_ctx->graph_cache
         .get_names_and_types(demangle_topic, demangle_type, allocator, topic_names_and_types);
}
}  // extern "C"

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

#include <mutex>
#include <string>
#include <vector>

#include "rmw/get_topic_endpoint_info.h"
#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/topic_endpoint_info.h"
#include "rmw/types.h"

#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/guid.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

extern "C"
{
rmw_ret_t
rmw_get_publishers_info_by_topic(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * publishers_info)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator is null", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(publishers_info, RMW_RET_INVALID_ARGUMENT);

  if (rmw_topic_endpoint_info_array_check_zero(publishers_info) != RMW_RET_OK) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  std::string mangled_topic_name = topic_name;
  DemangleFunction demangle_type = _identity_demangle;
  if (!no_mangle) {
    mangled_topic_name = create_topic_name(ros_topic_prefix, topic_name, "", false);
    demangle_type = _demangle_if_ros_type;
  }

  return common_ctx->graph_cache.get_writers_info_by_topic(
    mangled_topic_name,
    demangle_type,
    allocator,
    publishers_info);
}

rmw_ret_t
rmw_get_subscriptions_info_by_topic(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * subscriptions_info)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator is null", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscriptions_info, RMW_RET_INVALID_ARGUMENT);

  if (rmw_topic_endpoint_info_array_check_zero(subscriptions_info) != RMW_RET_OK) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  std::string mangled_topic_name = topic_name;
  DemangleFunction demangle_type = _identity_demangle;
  if (!no_mangle) {
    mangled_topic_name = create_topic_name(ros_topic_prefix, topic_name, "", false);
    demangle_type = _demangle_if_ros_type;
  }

  return common_ctx->graph_cache.get_readers_info_by_topic(
    mangled_topic_name,
    demangle_type,
    allocator,
    subscriptions_info);
}
}  // extern "C"

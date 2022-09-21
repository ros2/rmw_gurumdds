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

#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/guid.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

using GetNamesAndTypesByNodeFunction = rmw_ret_t (*)(
  rmw_dds_common::Context *,
  const std::string &,
  const std::string &,
  DemangleFunction,
  DemangleFunction,
  rcutils_allocator_t *,
  rmw_names_and_types_t *);

rmw_ret_t
validate_names_and_namespace(
  const char * node_name,
  const char * node_namespace)
{
  if (node_name == nullptr) {
    RMW_SET_ERROR_MSG("node name is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (node_namespace == nullptr) {
    RMW_SET_ERROR_MSG("node namespace is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  return RMW_RET_OK;
}

static rmw_ret_t
__get_topic_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  DemangleFunction demangle_topic,
  DemangleFunction demangle_type,
  bool no_demangle,
  GetNamesAndTypesByNodeFunction get_names_and_types_by_node,
  rmw_names_and_types_t * topic_names_and_types)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  ret = validate_names_and_namespace(node_name, node_namespace);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  auto common_ctx = &node->context->impl->common_ctx;

  if (no_demangle) {
    demangle_topic = _identity_demangle;
    demangle_type = _identity_demangle;
  }

  return get_names_and_types_by_node(
    common_ctx,
    node_name,
    node_namespace,
    demangle_topic,
    demangle_type,
    allocator,
    topic_names_and_types);
}

static rmw_ret_t
__get_reader_names_and_types_by_node(
  rmw_dds_common::Context * common_context,
  const std::string & node_name,
  const std::string & node_namespace,
  DemangleFunction demangle_topic,
  DemangleFunction demangle_type,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * topic_names_and_types)
{
  return common_context->graph_cache.get_reader_names_and_types_by_node(
    node_name,
    node_namespace,
    demangle_topic,
    demangle_type,
    allocator,
    topic_names_and_types);
}

static rmw_ret_t
__get_writer_names_and_types_by_node(
  rmw_dds_common::Context * common_context,
  const std::string & node_name,
  const std::string & node_namespace,
  DemangleFunction demangle_topic,
  DemangleFunction demangle_type,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * topic_names_and_types)
{
  return common_context->graph_cache.get_writer_names_and_types_by_node(
    node_name,
    node_namespace,
    demangle_topic,
    demangle_type,
    allocator,
    topic_names_and_types);
}

extern "C"
{
rmw_ret_t
rmw_get_subscriber_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_subscriber_names_and_types_by_node: "
    "node=%s%s, demangle=%s",
    node_namespace, node_name, no_demangle == true ? "false" : "true");
  return __get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    _demangle_ros_topic_from_topic,
    _demangle_if_ros_type,
    no_demangle,
    __get_reader_names_and_types_by_node,
    topic_names_and_types);
}

rmw_ret_t
rmw_get_publisher_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_publisher_names_and_types_by_node: "
    "node=%s%s, demangle=%s",
    node_namespace, node_name, no_demangle == true ? "false" : "true");
  return __get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    _demangle_ros_topic_from_topic,
    _demangle_if_ros_type,
    no_demangle,
    __get_writer_names_and_types_by_node,
    topic_names_and_types);
}

rmw_ret_t
rmw_get_service_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_service_names_and_types_by_node: "
    "node=%s%s", node_namespace, node_name);
  return __get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    _demangle_service_request_from_topic,
    _demangle_service_type_only,
    false,
    __get_reader_names_and_types_by_node,
    service_names_and_types);
}

rmw_ret_t
rmw_get_client_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_client_names_and_types_by_node: "
    "node=%s%s", node_namespace, node_name);
  return __get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    _demangle_service_reply_from_topic,
    _demangle_service_type_only,
    false,
    __get_reader_names_and_types_by_node,
    service_names_and_types);
}
}  // extern "C"t

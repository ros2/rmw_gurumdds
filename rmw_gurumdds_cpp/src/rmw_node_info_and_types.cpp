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
#include <set>
#include <string>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/fastrtps.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"
using GetNamesAndTypesByNodeFunction = rmw_ret_t (*)(
  rmw_dds_common::Context *,
  const std::string &,
  const std::string &,
  rmw_gurumdds_cpp::DemangleFunction,
  rmw_gurumdds_cpp::DemangleFunction,
  rcutils_allocator_t *,
  rmw_names_and_types_t *);

static inline rmw_ret_t get_topic_names_and_types_by_node(
  const rmw_node_t *node,
  rcutils_allocator_t *allocator,
  const char *node_name,
  const char *node_namespace,
  rmw_gurumdds_cpp::DemangleFunction demangle_topic,
  rmw_gurumdds_cpp::DemangleFunction demangle_type,
  bool no_demangle,
  GetNamesAndTypesByNodeFunction get_names_and_types_by_node,
  rmw_names_and_types_t *topic_names_and_types)
{
  CHECK_ALL_PTRS_CODE(node);
  CHECK_ID_CODE(node);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator,
    "allocator argument is invalid",
    return RMW_RET_INVALID_ARGUMENT);

  int validation_result = RMW_NODE_NAME_VALID;
  rmw_ret_t ret =
    rmw_validate_node_name(node_name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return ret;
  }
  if (RMW_NODE_NAME_VALID != validation_result) {
    const char *reason =
      rmw_node_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "node_name argument is invalid: %s", reason);
    return RMW_RET_INVALID_ARGUMENT;
  }
  validation_result = RMW_NAMESPACE_VALID;
  ret = rmw_validate_namespace(node_namespace, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return ret;
  }
  if (RMW_NAMESPACE_VALID != validation_result) {
    const char *reason =
      rmw_namespace_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "node_namespace argument is invalid: %s", reason);
    return RMW_RET_INVALID_ARGUMENT;
  }
  ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (RMW_RET_OK != ret) {
    return ret;
  }
  auto common_ctx = &node->context->impl->common_ctx;
  if (no_demangle) {
    demangle_topic = rmw_gurumdds_cpp::identity_demangle;
    demangle_type = rmw_gurumdds_cpp::identity_demangle;
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

static inline rmw_ret_t get_reader_names_and_types_by_node(
  rmw_dds_common::Context *common_context,
  const std::string & node_name,
  const std::string & node_namespace,
  rmw_gurumdds_cpp::DemangleFunction demangle_topic,
  rmw_gurumdds_cpp::DemangleFunction demangle_type,
  rcutils_allocator_t *allocator,
  rmw_names_and_types_t *topic_names_and_types)
{
  return common_context->graph_cache.get_reader_names_and_types_by_node(
    node_name,
    node_namespace,
    demangle_topic,
    demangle_type,
    allocator,
    topic_names_and_types);
}

static inline rmw_ret_t get_writer_names_and_types_by_node(
  rmw_dds_common::Context *common_context,
  const std::string & node_name,
  const std::string & node_namespace,
  rmw_gurumdds_cpp::DemangleFunction demangle_topic,
  rmw_gurumdds_cpp::DemangleFunction demangle_type,
  rcutils_allocator_t *allocator,
  rmw_names_and_types_t *topic_names_and_types)
{
  return common_context->graph_cache.get_writer_names_and_types_by_node(
    node_name,
    node_namespace,
    demangle_topic,
    demangle_type,
    allocator,
    topic_names_and_types);
}

extern "C" {
rmw_ret_t rmw_get_subscriber_names_and_types_by_node(
  const rmw_node_t *node,
  rcutils_allocator_t *allocator,
  const char *node_name,
  const char *node_namespace,
  bool no_demangle,
  rmw_names_and_types_t *topic_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_subscriber_names_and_types_by_node: "
    "node=%s%s, demangle=%s",
    node_namespace,
    node_name,
    no_demangle == true ? "false" : "true");
  return get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    rmw_gurumdds_cpp::demangle_ros_topic_from_topic,
    rmw_gurumdds_cpp::demangle_if_ros_type,
    no_demangle,
    get_reader_names_and_types_by_node,
    topic_names_and_types);
}

rmw_ret_t rmw_get_publisher_names_and_types_by_node(
  const rmw_node_t *node,
  rcutils_allocator_t *allocator,
  const char *node_name,
  const char *node_namespace,
  bool no_demangle,
  rmw_names_and_types_t *topic_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_publisher_names_and_types_by_node: "
    "node=%s%s, demangle=%s",
    node_namespace,
    node_name,
    no_demangle == true ? "false" : "true");
  return get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    rmw_gurumdds_cpp::demangle_ros_topic_from_topic,
    rmw_gurumdds_cpp::demangle_if_ros_type,
    no_demangle,
    get_writer_names_and_types_by_node,
    topic_names_and_types);
}

rmw_ret_t rmw_get_service_names_and_types_by_node(
  const rmw_node_t *node,
  rcutils_allocator_t *allocator,
  const char *node_name,
  const char *node_namespace,
  rmw_names_and_types_t *service_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_service_names_and_types_by_node: "
    "node=%s%s",
    node_namespace,
    node_name);
  return get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    rmw_gurumdds_cpp::demangle_service_request_from_topic,
    rmw_gurumdds_cpp::demangle_service_type_only,
    false,
    get_reader_names_and_types_by_node,
    service_names_and_types);
}

rmw_ret_t rmw_get_client_names_and_types_by_node(
  const rmw_node_t *node,
  rcutils_allocator_t *allocator,
  const char *node_name,
  const char *node_namespace,
  rmw_names_and_types_t *service_names_and_types)
{
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "rmw_get_client_names_and_types_by_node: "
    "node=%s%s",
    node_namespace,
    node_name);
  return get_topic_names_and_types_by_node(
    node,
    allocator,
    node_name,
    node_namespace,
    rmw_gurumdds_cpp::demangle_service_reply_from_topic,
    rmw_gurumdds_cpp::demangle_service_type_only,
    false,
    get_reader_names_and_types_by_node,
    service_names_and_types);
}
} // extern "C"

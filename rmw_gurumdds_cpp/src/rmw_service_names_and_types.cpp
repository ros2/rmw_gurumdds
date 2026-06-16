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

#include "rmw/get_service_endpoint_info.h"

#include <map>
#include <set>

#include "rcutils/allocator.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"
#include "rmw/service_endpoint_info.h"
#include "rmw/service_endpoint_info_array.h"
#include "rmw/topic_endpoint_info.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/fastrtps.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

extern "C" {
rmw_ret_t rmw_get_service_names_and_types(
  const rmw_node_t *node, rcutils_allocator_t *allocator,
  rmw_names_and_types_t *service_names_and_types)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator argument is invalid",
                                   return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
                                   RMW_GURUMDDS_ID,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  if (rmw_names_and_types_check_zero(service_names_and_types) != RMW_RET_OK) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  return common_ctx->graph_cache.get_names_and_types(
      rmw_gurumdds_cpp::demangle_service_from_topic,
      rmw_gurumdds_cpp::demangle_service_type_only, allocator,
      service_names_and_types);
}

rmw_ret_t rmw_get_clients_info_by_service(
  const rmw_node_t *node, rcutils_allocator_t *allocator,
  const char *service_name, bool no_mangle,
  rmw_service_endpoint_info_array_t *clients_info)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
                                   RMW_GURUMDDS_ID,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator argument is invalid",
                                   return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(service_name, RMW_RET_INVALID_ARGUMENT);
  if (RMW_RET_OK != rmw_service_endpoint_info_array_check_zero(clients_info)) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (no_mangle) {
    // Services in DDS require mangled topic names
    // because they internally use separate readers and writers.
    // Therefore, this function cannot support the 'no_mangle' option.
    // If user need to query raw topic information without mangling,
    // use`rmw_get_publishers_info_by_topic` or
    // `rmw_get_subscriptions_info_by_topic` instead.
    RMW_SET_ERROR_MSG("'no_mangle' is not supported for services"
                      " because they rely on internally mangled topic names.\n"
                      "Use 'rmw_get_publishers_info_by_topic' or "
                      "'rmw_get_subscriptions_info_by_topic'"
                      " instead to access unmangled topic information.");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto *common_context = &node->context->impl->common_ctx;
  std::string mangled_rq_topic_name =
    rmw_gurumdds_cpp::_mangle_topic_name(
          rmw_gurumdds_cpp::ros_service_requester_prefix, service_name,
          "Request")
    .to_string();
  std::string mangled_rp_topic_name =
    rmw_gurumdds_cpp::_mangle_topic_name(
          rmw_gurumdds_cpp::ros_service_response_prefix, service_name, "Reply")
    .to_string();
  rmw_gurumdds_cpp::DemangleFunction demangle_type =
    rmw_gurumdds_cpp::_demangle_service_type_only;

  rmw_topic_endpoint_info_array_t subscriptions_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  subscriptions_info_delete(
    &subscriptions_info, [allocator](rmw_topic_endpoint_info_array_t *p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(p, allocator);
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG(
                  "Failed to destroy subscriptions_info when function ended.");
      }
    });
  rmw_ret_t ret = common_context->graph_cache.get_readers_info_by_topic(
      mangled_rp_topic_name, demangle_type, allocator, &subscriptions_info);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  rmw_topic_endpoint_info_array_t publishers_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  publishers_info_delete(
    &publishers_info, [allocator](rmw_topic_endpoint_info_array_t *p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(p, allocator);
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG(
                  "Failed to destroy publishers_info when function ended.");
      }
    });
  ret = common_context->graph_cache.get_writers_info_by_topic(
      mangled_rq_topic_name, demangle_type, allocator, &publishers_info);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  return common_context->graph_cache.get_clients_info_by_service(
      &subscriptions_info, &publishers_info, allocator, clients_info);
}

rmw_ret_t rmw_get_servers_info_by_service(
  const rmw_node_t *node, rcutils_allocator_t *allocator,
  const char *service_name, bool no_mangle,
  rmw_service_endpoint_info_array_t *servers_info)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
                                   RMW_GURUMDDS_ID,
                                   return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator argument is invalid",
                                   return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(service_name, RMW_RET_INVALID_ARGUMENT);
  if (RMW_RET_OK != rmw_service_endpoint_info_array_check_zero(servers_info)) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (no_mangle) {
    // Services in DDS require mangled topic names
    // because they internally use separate readers and writers.
    // Therefore, this function cannot support the 'no_mangle' option.
    // If user need to query raw topic information without mangling,
    // use`rmw_get_publishers_info_by_topic` or
    // `rmw_get_subscriptions_info_by_topic` instead.
    RMW_SET_ERROR_MSG("'no_mangle' is not supported for services"
                      " because they rely on internally mangled topic names.\n"
                      "Use 'rmw_get_publishers_info_by_topic' or "
                      "'rmw_get_subscriptions_info_by_topic'"
                      " instead to access unmangled topic information.");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto common_context = &node->context->impl->common_ctx;
  std::string mangled_rq_topic_name =
    rmw_gurumdds_cpp::_mangle_topic_name(
          rmw_gurumdds_cpp::ros_service_requester_prefix, service_name,
          "Request")
    .to_string();
  std::string mangled_rp_topic_name =
    rmw_gurumdds_cpp::_mangle_topic_name(
          rmw_gurumdds_cpp::ros_service_response_prefix, service_name, "Reply")
    .to_string();
  rmw_gurumdds_cpp::DemangleFunction demangle_type =
    rmw_gurumdds_cpp::_demangle_service_type_only;

  rmw_topic_endpoint_info_array_t subscriptions_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  subscriptions_info_delete(
    &subscriptions_info, [allocator](rmw_topic_endpoint_info_array_t *p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(p, allocator);
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG(
                  "Failed to destroy subscriptions_info when function failed.");
      }
    });
  rmw_ret_t ret = common_context->graph_cache.get_readers_info_by_topic(
      mangled_rq_topic_name, demangle_type, allocator, &subscriptions_info);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  rmw_topic_endpoint_info_array_t publishers_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  publishers_info_delete(
    &publishers_info, [allocator](rmw_topic_endpoint_info_array_t *p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(p, allocator);
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG(
                  "Failed to destroy publishers_info when function failed.");
      }
    });
  ret = common_context->graph_cache.get_writers_info_by_topic(
      mangled_rp_topic_name, demangle_type, allocator, &publishers_info);
  if (RMW_RET_OK != ret) {
    return ret;
  }
  return common_context->graph_cache.get_servers_info_by_service(
      &subscriptions_info, &publishers_info, allocator, servers_info);
}
} // extern "C"

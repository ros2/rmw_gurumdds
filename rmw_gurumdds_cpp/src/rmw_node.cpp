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

#include <array>
#include <utility>
#include <set>
#include <string>
#include <vector>
#include <list>
#include <thread>
#include <chrono>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/sanity_checks.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

rmw_node_t *
__rmw_create_node(
  const char * implementation_identifier,
  rmw_context_t * context,
  const char * name,
  const char * namespace_)
{
  /* Validate node's name and namespace */
  int validation_result = RMW_NODE_NAME_VALID;
  rmw_ret_t ret = rmw_validate_node_name(name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return nullptr;
  }
  if (RMW_NODE_NAME_VALID != validation_result) {
    const char * reason = rmw_node_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid node name: %s", reason);
    return nullptr;
  }
  validation_result = RMW_NAMESPACE_VALID;
  ret = rmw_validate_namespace(namespace_, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return nullptr;
  }
  if (RMW_NAMESPACE_VALID != validation_result) {
    const char * reason = rmw_node_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid node namespace: %s", reason);
    return nullptr;
  }

  bool node_localhost_only =
    context->options.localhost_only == RMW_LOCALHOST_ONLY_ENABLED;

  rmw_context_impl_t * ctx = context->impl;
  std::lock_guard<std::mutex> guard(ctx->initialization_mutex);

  if (ctx->is_shutdown) {
    RMW_SET_ERROR_MSG("context is already shutdown");
    return nullptr;
  }

  ret = ctx->initialize_node(namespace_, name, node_localhost_only);
  if (ret != RMW_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to initialize node in context");
    return nullptr;
  }

  auto context_finalize = rcpputils::make_scope_exit(
    [ctx]()
    {
      if (RMW_RET_OK != ctx->finalize_node()) {
        RCUTILS_LOG_ERROR_NAMED(
          RMW_GURUMDDS_ID, "failed to finalize node in context");
      }
    });

  rmw_node_t * node_handle = rmw_node_allocate();
  if (node_handle == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node handle");
    return nullptr;
  }

  auto cleanup_node = rcpputils::make_scope_exit(
    [node_handle]() {
      if (node_handle->name != nullptr) {
        rmw_free(const_cast<char *>(node_handle->name));
      }
      if (node_handle->namespace_ != nullptr) {
        rmw_free(const_cast<char *>(node_handle->namespace_));
      }
      rmw_node_free(node_handle);
    });

  node_handle->name = static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
  if (node_handle->name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node name");
    return nullptr;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
  if (node_handle->namespace_ == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node namespace");
    return nullptr;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);

  node_handle->implementation_identifier = implementation_identifier;
  node_handle->data = nullptr;
  node_handle->context = context;

  if (graph_on_node_created(ctx, node_handle) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to create node");
    return nullptr;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "Created node '%s' in namespace '%s'", name, namespace_);

  context_finalize.cancel();
  cleanup_node.cancel();
  return node_handle;
}

rmw_ret_t
__rmw_destroy_node(const char * implementation_identifier, rmw_node_t * node)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier, implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_context_impl_t * ctx = node->context->impl;
  std::lock_guard<std::mutex> guard(ctx->initialization_mutex);

  if (graph_on_node_deleted(ctx, node) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to update for node delete");
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "Deleted node '%s' in namespace '%s'", node->name, node->namespace_);

  rmw_free(const_cast<char *>(node->name));
  rmw_free(const_cast<char *>(node->namespace_));
  rmw_node_free(node);

  if (ctx->finalize_node() != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to finalize node");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

const rmw_guard_condition_t *
__rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  auto common_ctx = &node->context->impl->common_ctx;
  if (!common_ctx) {
    RMW_SET_ERROR_MSG("common_context is nullptr");
    return nullptr;
  }
  return common_ctx->graph_guard_condition;
}

rmw_ret_t
_get_node_names(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces,
  rcutils_string_array_t * enclaves)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier, implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (rmw_check_zero_rmw_string_array(node_names) != RMW_RET_OK ||
    rmw_check_zero_rmw_string_array(node_namespaces) != RMW_RET_OK)
  {
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (enclaves != nullptr &&
    rmw_check_zero_rmw_string_array(enclaves) != RMW_RET_OK)
  {
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  return common_ctx->graph_cache.get_node_names(
    node_names,
    node_namespaces,
    enclaves,
    &allocator);
}

rmw_ret_t
__rmw_get_node_names(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  return _get_node_names(implementation_identifier, node, node_names, node_namespaces, nullptr);
}

rmw_ret_t
__rmw_get_node_names_with_enclaves(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces,
  rcutils_string_array_t * enclaves)
{
  return _get_node_names(
    implementation_identifier, node, node_names, node_namespaces, enclaves);
}

extern "C"
{
rmw_node_t *
rmw_create_node(
  rmw_context_t * context,
  const char * name,
  const char * namespace_)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    context->impl,
    "expected initialized context",
    return nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    RMW_GURUMDDS_ID,
    return nullptr);
  return __rmw_create_node(
    RMW_GURUMDDS_ID, context, name, namespace_);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  return __rmw_destroy_node(RMW_GURUMDDS_ID, node);
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  return __rmw_node_get_graph_guard_condition(node);
}

rmw_ret_t
rmw_get_node_names(
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  return __rmw_get_node_names(
    RMW_GURUMDDS_ID, node, node_names, node_namespaces);
}

rmw_ret_t
rmw_get_node_names_with_enclaves(
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces,
  rcutils_string_array_t * enclaves)
{
  return __rmw_get_node_names_with_enclaves(
    RMW_GURUMDDS_ID, node, node_names, node_namespaces, enclaves);
}
}  // extern "C"

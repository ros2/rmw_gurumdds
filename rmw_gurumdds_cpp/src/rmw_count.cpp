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
#include <string>

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_gurumdds_cpp/fastrtps.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"

extern "C" {
rmw_ret_t rmw_count_publishers(
  const rmw_node_t *node, const char *topic_name,
  size_t *count)
{
  CHECK_ALL_PTRS_CODE(node, topic_name, count);
  CHECK_ID_CODE(node);

  int validation_result = RMW_TOPIC_VALID;
  rmw_ret_t ret =
    rmw_validate_full_topic_name(topic_name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  if (RMW_TOPIC_VALID != validation_result) {
    const char *reason =
      rmw_full_topic_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("topic_name argument is invalid: %s",
                                         reason);
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  const std::string mangled_topic_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_topic_prefix, topic_name, "", false);

  return common_ctx->graph_cache.get_writer_count(mangled_topic_name, count);
}

rmw_ret_t rmw_count_subscribers(
  const rmw_node_t *node, const char *topic_name,
  size_t *count)
{
  CHECK_ALL_PTRS_CODE(node, topic_name, count);
  CHECK_ID_CODE(node);

  int validation_result = RMW_TOPIC_VALID;
  rmw_ret_t ret =
    rmw_validate_full_topic_name(topic_name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  if (RMW_TOPIC_VALID != validation_result) {
    const char *reason =
      rmw_full_topic_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("topic_name argument is invalid: %s",
                                         reason);
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  const std::string mangled_topic_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_topic_prefix, topic_name, "", false);

  return common_ctx->graph_cache.get_reader_count(mangled_topic_name, count);
}

rmw_ret_t rmw_count_clients(
  const rmw_node_t *node, const char *service_name,
  size_t *count)
{
  CHECK_ALL_PTRS_CODE(node, service_name, count);
  CHECK_ID_CODE(node);

  int validation_result = RMW_TOPIC_VALID;
  rmw_ret_t ret =
    rmw_validate_full_topic_name(service_name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  if (RMW_TOPIC_VALID != validation_result) {
    const char *reason =
      rmw_full_topic_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service_name argument is invalid: %s",
                                         reason);
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  const std::string mangled_service_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_service_response_prefix, service_name, "Reply",
      false);

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID,
                          "[rmw_count_clients] mangled_service_name : %s",
                          mangled_service_name.c_str());

  return common_ctx->graph_cache.get_reader_count(mangled_service_name, count);
}

rmw_ret_t rmw_count_services(
  const rmw_node_t *node, const char *service_name,
  size_t *count)
{
  CHECK_ALL_PTRS_CODE(node, service_name, count);
  CHECK_ID_CODE(node);

  int validation_result = RMW_TOPIC_VALID;

  rmw_ret_t ret =
    rmw_validate_full_topic_name(service_name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  if (RMW_TOPIC_VALID != validation_result) {
    const char *reason =
      rmw_full_topic_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service_name argument is invalid: %s",
                                         reason);
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_ctx = &node->context->impl->common_ctx;
  const std::string mangled_service_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_service_response_prefix, service_name, "Reply",
      false);

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID,
                          "[rmw_count_services] mangled_service_name : %s",
                          mangled_service_name.c_str());

  return common_ctx->graph_cache.get_writer_count(mangled_service_name, count);
}
} // extern "C"

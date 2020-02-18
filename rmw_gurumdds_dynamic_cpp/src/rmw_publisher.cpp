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

#include <string>
#include <sstream>
#include <limits>
#include <thread>
#include <chrono>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_dynamic_cpp/identifier.hpp"
#include "rmw_gurumdds_dynamic_cpp/types.hpp"

#include "rcutils/types.h"

#include "./type_support_common.hpp"

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_bounds_t * message_bounds,
  rmw_publisher_allocation_t * allocation)
{
  (void)type_support;
  (void)message_bounds;
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_init_publisher_allocation is not supported");
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t * allocation)
{
  (void)allocation;

  RMW_SET_ERROR_MSG("rmw_fini_publisher_allocation is not supported");
  return RMW_RET_ERROR;
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies)
{
  // TODO(clemjh): Implement this
  return nullptr;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_publisher_assert_liveliness(const rmw_publisher_t * publisher)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_publish(
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  // TODO(clemjh): Implement this
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

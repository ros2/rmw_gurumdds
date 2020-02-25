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
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"

#include "rmw_gurumdds_dynamic_cpp/types.hpp"
#include "rmw_gurumdds_dynamic_cpp/identifier.hpp"

#include "./type_support_common.hpp"

// TODO
extern "C"
{
rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  // TODO(clemjh): Implement this
  (void)node;
  (void)type_supports;
  (void)service_name;
  (void)qos_policies;
  return nullptr;
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  // TODO(clemjh): Implement this
  (void)node;
  (void)client;
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available)
{
  // TODO(clemjh): Implement this
  (void)node;
  (void)client;
  (void)is_available;
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

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

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw/get_service_names_and_types.h"
#include "rmw/names_and_types.h"
#include "rmw/allocators.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"

#include "rmw_gurumdds_dynamic_cpp/identifier.hpp"
#include "rmw_gurumdds_dynamic_cpp/types.hpp"

#include "./type_support_common.hpp"

// TODO
extern "C"
{
rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
{
  // TODO(clemjh): Implement this
  (void)node;
  (void)type_supports;
  (void)service_name;
  (void)qos_policies;
  return nullptr;
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  // TODO(clemjh): Implement this
  (void)node;
  (void)service;
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

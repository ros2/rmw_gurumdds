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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/rmw_wait.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/types.hpp"

extern "C"
{
rmw_wait_set_t *
rmw_create_wait_set(rmw_context_t * context, size_t max_conditions)
{
  return shared__rmw_create_wait_set(gurum_gurumdds_identifier, context, max_conditions);
}

rmw_ret_t
rmw_destroy_wait_set(rmw_wait_set_t * wait_set)
{
  return shared__rmw_destroy_wait_set(gurum_gurumdds_identifier, wait_set);
}

rmw_ret_t
rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t * events,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
  return shared__rmw_wait<GurumddsSubscriberInfo, GurumddsServiceInfo, GurumddsClientInfo>(
    gurum_gurumdds_identifier, subscriptions, guard_conditions,
    services, clients, events, wait_set, wait_timeout);
}
}  // extern "C"

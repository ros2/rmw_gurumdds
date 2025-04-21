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

#ifndef RMW_GURUMDDS__WAIT_HPP_
#define RMW_GURUMDDS__WAIT_HPP_

#include "rmw/rmw.h"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/event_info_service.hpp"
#include <vector>
#include <unordered_set>

namespace rmw_gurumdds_cpp
{
struct WaitSetInfo
{
  dds_WaitSet * wait_set;
  dds_ConditionSeq * active_conditions;
  dds_ConditionSeq * attached_conditions;
  std::vector<SubscriberInfo*> cached_subscriptions;
  std::vector<dds_GuardCondition*> cached_guard_conditions;
  std::vector<ServiceInfo*> cached_service_conditions;
  std::vector<ClientInfo*> cached_client_conditions;
  std::vector<EventInfo*> cached_event_conditions;
  std::unordered_set<dds_Condition*> cached_status_conditions;
  std::mutex lock;
};

rmw_ret_t
wait(
  const char * implementation_identifier,
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t * events,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout);
} // namespace rmw_gurumdds_cpp

#endif // RMW_GURUMDDS__WAIT_HPP_

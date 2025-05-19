// Copyright 2022 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_
#define RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_

namespace rmw_gurumdds_cpp
{
rmw_subscription_t *
create_subscription(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Subscriber * const sub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  const bool internal);

rmw_ret_t
destroy_subscription(
  rmw_context_impl_t * const ctx,
  rmw_subscription_t * const subscription);
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_

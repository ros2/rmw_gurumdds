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

#include "rmw/rmw.h"

#include "../../src/type_support_common.hpp"
#include "../../src/type_support_service.hpp"

rmw_subscription_t *
__rmw_create_subscription(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Subscriber * const sub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  const bool internal);

rmw_ret_t
__rmw_destroy_subscription(
  rmw_context_impl_t * const ctx,
  rmw_subscription_t * const subscription);

extern "C"
{
rmw_ret_t
rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_fini_subscription_allocation(rmw_subscription_allocation_t * allocation);

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options);

rmw_ret_t
rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count);

rmw_ret_t
rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos);

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription);

rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_take_sequence(
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_take_loaned_message(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_take_loaned_message_with_info(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation);

rmw_ret_t
rmw_return_loaned_message_from_subscription(
  const rmw_subscription_t * subscription,
  void * loaned_message);
}  // extern "C"

#endif  // RMW_GURUMDDS_CPP__RMW_SUBSCRIPTION_HPP_

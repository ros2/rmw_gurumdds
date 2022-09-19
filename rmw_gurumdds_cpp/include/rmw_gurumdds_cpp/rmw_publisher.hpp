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

#ifndef RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_
#define RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_

#include "rmw/rmw.h"

#include "../../src/type_support_common.hpp"
#include "../../src/type_support_service.hpp"

rmw_publisher_t *
__rmw_create_publisher(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Publisher * const pub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options,
  const bool internal);

rmw_ret_t
__rmw_destroy_publisher(
  rmw_context_impl_t * const ctx,
  rmw_publisher_t * const publisher);

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_publisher_allocation_t * allocation);

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t * allocation);

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options);

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count);

rmw_ret_t
rmw_publisher_assert_liveliness(const rmw_publisher_t * publisher);

rmw_ret_t
rmw_publisher_wait_for_all_acked(const rmw_publisher_t * publisher, rmw_time_t wait_timeout);

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher);

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid);

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos);

rmw_ret_t
rmw_publish(
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation);

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation);

rmw_ret_t
rmw_publish_loaned_message(
  const rmw_publisher_t * publisher,
  void * ros_message,
  rmw_publisher_allocation_t * allocation);

rmw_ret_t
rmw_borrow_loaned_message(
  const rmw_publisher_t * publisher,
  const rosidl_message_type_support_t * type_support,
  void ** ros_message);

rmw_ret_t
rmw_return_loaned_message_from_publisher(
  const rmw_publisher_t * publisher,
  void * loaned_message);
}  // extern "C"

#endif  // RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_

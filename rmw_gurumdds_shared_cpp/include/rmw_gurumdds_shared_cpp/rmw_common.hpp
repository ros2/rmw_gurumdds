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

#ifndef RMW_GURUMDDS_SHARED_CPP__RMW_COMMON_HPP_
#define RMW_GURUMDDS_SHARED_CPP__RMW_COMMON_HPP_

#include "./visibility_control.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/names_and_types.h"
#include "rmw/event.h"
#include "rmw/topic_endpoint_info_array.h"

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_guard_condition_t *
shared__rmw_create_guard_condition(const char * implementation_identifier);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_destroy_guard_condition(
  const char * implementation_identifier,
  rmw_guard_condition_t * guard_condition);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_trigger_guard_condition(
  const char * implementation_identifier,
  const rmw_guard_condition_t * guard_condition);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_node_t *
shared__rmw_create_node(
  const char * implementation_identifier,
  rmw_context_t * context,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  bool localhost_only);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_destroy_node(
  const char * implementation_identifier,
  rmw_node_t * node);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
const rmw_guard_condition_t *
shared__rmw_node_get_graph_guard_condition(const rmw_node_t * node);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_node_names(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_node_names_with_enclaves(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces,
  rcutils_string_array_t * enclaves);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_publish(
  const char * implementation_identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_publish_serialized_message(
  const char * implementation_identifier,
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_take(
  const char * implementation_identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_take_with_info(
  const char * implementation_identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_wait_set_t *
shared__rmw_create_wait_set(
  const char * implementation_identifier,
  rmw_context_t * context,
  size_t max_conditions);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_destroy_wait_set(
  const char * implementation_identifier,
  rmw_wait_set_t * wait_set);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_subscriber_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_publisher_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_service_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_client_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * client_names_and_types);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_count_publishers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_count_subscribers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_service_names_and_types(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_topic_names_and_types(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * service_names_and_types);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_set_log_severity(rmw_log_severity_t severity);

rmw_ret_t
shared__rmw_init_event(
  const char * identifier,
  rmw_event_t * rmw_event,
  const char * topic_endpoint_impl_identifier,
  void * data,
  rmw_event_type_t event_type);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_take_event(
  const char * implementation_identifier,
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_node_assert_liveliness(
  const char * implementation_identifier,
  const rmw_node_t * node);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_publishers_info_by_topic(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * publishers_info);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_subscriptions_info_by_topic(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * subscriptions_info);

#endif  // RMW_GURUMDDS_SHARED_CPP__RMW_COMMON_HPP_

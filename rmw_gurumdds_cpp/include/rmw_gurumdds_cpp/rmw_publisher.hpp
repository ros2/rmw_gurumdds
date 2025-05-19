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

namespace rmw_gurumdds_cpp
{
rmw_publisher_t *
create_publisher(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * node,
  dds_DomainParticipant * const participant,
  dds_Publisher * const pub,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options,
  const bool internal);

rmw_ret_t
destroy_publisher(
  rmw_context_impl_t * const ctx,
  rmw_publisher_t * const publisher);

rmw_ret_t
publish(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation);
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__RMW_PUBLISHER_HPP_

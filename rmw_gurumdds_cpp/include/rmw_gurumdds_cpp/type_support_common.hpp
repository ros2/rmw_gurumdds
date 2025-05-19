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

#ifndef RMW_GURUMDDS_CPP__TYPE_SUPPORT_COMMON_HPP_
#define RMW_GURUMDDS_CPP__TYPE_SUPPORT_COMMON_HPP_

#include <string>
#include <sstream>
#include <cstdint>

#include "rmw_gurumdds_cpp/message_converter.hpp"
#include "rmw_gurumdds_cpp/message_serializer.hpp"
#include "rmw_gurumdds_cpp/message_deserializer.hpp"

#include "rmw/error_handling.h"
#include "rmw/macros.h"

namespace rmw_gurumdds_cpp
{
template<typename MessageMembersT>
std::string
create_type_name(const void * untyped_members);

template<typename MessageMembersT>
void *
allocate_message(
  const void * untyped_members,
  const uint8_t * ros_message,
  size_t * size);

template<typename MessageMembersT>
std::string
create_metastring(const void * untyped_members);

template<typename MessageMembersT>
std::string
parse_struct(const MessageMembersT * members, const char * field_name);

std::string
create_type_name(const void * untyped_members, const char * identifier);

std::string
create_metastring(const void * untyped_members, const char * identifier);

void *
allocate_message(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message,
  size_t * size);

ssize_t
get_serialized_size(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message);

bool
serialize_ros_to_cdr(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message,
  void * dds_message,
  size_t size);

bool
deserialize_cdr_to_ros(
  const void * untyped_members,
  const char * identifier,
  void * ros_message,
  void * dds_message,
  size_t size);
}  // namespace rmw_gurumdds_cpp

#include "rmw_gurumdds_cpp/type_support_common.inl"

#endif  // RMW_GURUMDDS_CPP__TYPE_SUPPORT_COMMON_HPP_

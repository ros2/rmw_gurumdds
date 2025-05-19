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

#ifndef RMW_GURUMDDS_CPP__TYPE_SUPPORT_SERVICE_HPP_
#define RMW_GURUMDDS_CPP__TYPE_SUPPORT_SERVICE_HPP_

#include "rmw_gurumdds_cpp/type_support_common.hpp"

#include <string>
#include <utility>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "rmw/impl/cpp/macros.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include "rmw_gurumdds_cpp/message_deserializer.hpp"
#include "rmw_gurumdds_cpp/message_serializer.hpp"

#define GET_TYPENAME(T) \
  typename std::remove_pointer<typename std::remove_const<decltype(T)>::type>::type

namespace rmw_gurumdds_cpp
{
template<typename ServiceMembersT>
std::pair<std::string, std::string>
create_service_type_name(const void * untyped_members);

std::pair<std::string, std::string>
create_service_type_name(const void * untyped_members, const char * identifier);

template<typename ServiceMembersT>
std::pair<std::string, std::string>
create_service_metastring(const void * untyped_members);

std::pair<std::string, std::string>
create_service_metastring(const void * untyped_members, const char * identifier);

template<typename ServiceMembersT>
void *
allocate_request_basic(
  const void * untyped_members,
  const void * ros_request,
  size_t * size);

void *
allocate_request_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size);

template<typename ServiceMembersT>
void *
allocate_response_basic(
  const void * untyped_members,
  const void * ros_response,
  size_t * size);

void *
allocate_response_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size);

template<typename ServiceMembersT>
void *
allocate_request_enhanced(
  const void * untyped_members,
  const void * ros_request,
  size_t * size);

void *
allocate_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size);

template<typename ServiceMembersT>
void *
allocate_response_enhanced(
  const void * untyped_members,
  const void * ros_response,
  size_t * size);

void *
allocate_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size);

template<typename MessageMembersT>
bool
serialize_service_basic(
  const void * untyped_members,
  const uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid,
  bool is_request);

bool
serialize_service_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_service,
  void * dds_service,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid,
  bool is_request);

template<typename ServiceMembersT>
bool
serialize_request_basic(
  const void * untyped_members,
  const uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid);

bool
serialize_request_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  void * dds_request,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid);

template<typename ServiceMembersT>
bool
serialize_response_basic(
  const void * untyped_members,
  const uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid);

bool
serialize_response_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  void * dds_response,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid);

template<typename MessageMembersT>
bool
serialize_service_enhanced(
  const void * untyped_members,
  const uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size);

bool
serialize_service_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_service,
  void * dds_service,
  size_t size);

template<typename ServiceMembersT>
bool
serialize_request_enhanced(
  const void * untyped_members,
  const uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size);

bool
serialize_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  void * dds_request,
  size_t size);

template<typename ServiceMembersT>
bool
serialize_response_enhanced(
  const void * untyped_members,
  const uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size);

bool
serialize_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  void * dds_response,
  size_t size);

template<typename MessageMembersT>
bool
deserialize_service_basic(
  const void * untyped_members,
  uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid,
  bool is_request);

bool
deserialize_service_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid,
  bool is_request);

template<typename ServiceMembersT>
bool
deserialize_request_basic(
  const void * untyped_members,
  uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid);

bool
deserialize_request_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid);

template<typename ServiceMembersT>
bool
deserialize_response_basic(
  const void * untyped_members,
  uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid);

bool
deserialize_response_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_response,
  void * dds_response,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid);

template<typename MessageMembersT>
bool
deserialize_service_enhanced(
  const void * untyped_members,
  uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size);

bool
deserialize_service_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size);

template<typename ServiceMembersT>
bool
deserialize_request_enhanced(
  const void * untyped_members,
  uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size);

bool
deserialize_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size);

template<typename ServiceMembersT>
bool
deserialize_response_enhanced(
  const void * untyped_members,
  uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size);

bool
deserialize_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_response,
  void * dds_response,
  size_t size);

inline void
ros_sn_to_dds_sn(int64_t sn_ros, uint64_t * sn_dds);

inline void
dds_sn_to_ros_sn(uint64_t sn_dds, int64_t * sn_ros);
}  // namespace rmw_gurumdds_cpp

#include "rmw_gurumdds_cpp/type_support_service.inl"

#endif  // RMW_GURUMDDS_CPP__TYPE_SUPPORT_SERVICE_HPP_

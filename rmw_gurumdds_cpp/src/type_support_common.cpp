// Copyright 2024 GurumNetworks, Inc.
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

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include "rmw_gurumdds_cpp/message_converter.hpp"
#include "rmw_gurumdds_cpp/type_support_common.hpp"

namespace rmw_gurumdds_cpp
{
template<typename MessageMembersT>
bool
serialize_ros_to_cdr(
  const void * untyped_members,
  const uint8_t * ros_message,
  uint8_t * dds_message,
  const size_t size)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  try {
    rmw_gurumdds_cpp::CdrSerializationBuffer<true> buffer{dds_message, size};
    rmw_gurumdds_cpp::MessageSerializer<true, MessageMembersT> serializer{buffer};
    serializer.serialize(members, ros_message, true);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to serialize ros message: %s", e.what());
    return false;
  }

  return true;
}

std::string
create_type_name(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return create_type_name<rosidl_typesupport_introspection_c__MessageMembers>(untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return create_type_name<rosidl_typesupport_introspection_cpp::MessageMembers>(untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return {};
}

template<typename MessageMembersT>
ssize_t
get_serialized_size(
  const void * untyped_members,
  const uint8_t * ros_message)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return -1;
  }

  if (ros_message == nullptr) {
    RMW_SET_ERROR_MSG("ros message is null");
    return -1;
  }

  rmw_gurumdds_cpp::CdrSerializationBuffer<false> buffer{nullptr, 0};
  rmw_gurumdds_cpp::MessageSerializer<false, MessageMembersT> serializer{buffer};
  serializer.serialize(members, ros_message, true);

  return static_cast<ssize_t>(buffer.get_offset() + 4);
}

template<typename MessageMembersT>
bool
deserialize_cdr_to_ros(
  const void * untyped_members,
  uint8_t * ros_message,
  uint8_t * dds_message,
  const size_t size)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  try {
    rmw_gurumdds_cpp::CdrDeserializationBuffer buffer{dds_message, size};
    rmw_gurumdds_cpp::MessageDeserializer<MessageMembersT> deserializer{buffer};
    deserializer.deserialize(members, ros_message);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

std::string
create_metastring(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return create_metastring<rosidl_typesupport_introspection_c__MessageMembers>(untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return create_metastring<rosidl_typesupport_introspection_cpp::MessageMembers>(untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return {};
}

void *
allocate_message(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return allocate_message<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return allocate_message<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

ssize_t
get_serialized_size(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return get_serialized_size<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message)
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return get_serialized_size<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message)
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return -1;
}

bool
serialize_ros_to_cdr(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message,
  void * dds_message,
  const size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return serialize_ros_to_cdr<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_ros_to_cdr<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
deserialize_cdr_to_ros(
  const void * untyped_members,
  const char * identifier,
  void * ros_message,
  void * dds_message,
  const size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_cdr_to_ros<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return deserialize_cdr_to_ros<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

}  // namespace rmw_gurumdds_cpp

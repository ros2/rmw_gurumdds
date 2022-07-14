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

#ifndef TYPE_SUPPORT_COMMON_HPP_
#define TYPE_SUPPORT_COMMON_HPP_

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <sstream>
#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "./message_converter.hpp"

template<typename MessageMembersT>
std::string
_create_type_name(const void * untyped_members)
{
  auto members = static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return "";
  }

  std::ostringstream type_name;
  std::string message_namespace(members->message_namespace_);
  if (!message_namespace.empty()) {
    size_t pos = 0;
    while ((pos = message_namespace.find("__", pos)) != std::string::npos) {
      message_namespace.replace(pos, 2, "::");
    }
    type_name << message_namespace << "::";
  }
  type_name << "dds_::" << members->message_name_ << "_";
  return type_name.str();
}

inline std::string
create_type_name(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _create_type_name<rosidl_typesupport_introspection_c__MessageMembers>(untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _create_type_name<rosidl_typesupport_introspection_cpp::MessageMembers>(untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return std::string("");
}

template<typename MessageMembersT>
std::string
_parse_struct(const MessageMembersT * members, const char * field_name, bool is_service)
{
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return "";
  }

  std::ostringstream metastring;
  metastring <<
    "{(" <<
  (field_name ? std::string("name=") + field_name + "_," : "") <<
    "type=" <<
    _create_type_name<MessageMembersT>(members) <<
    ",member=" <<
    members->member_count_ + (is_service ? 3 : 0) <<
    ")";

  for (size_t i = 0; i < members->member_count_; i++) {
    auto member = members->members_ + i;
    if (member->is_array_) {
      if (member->array_size_ > 0) {
        if (!member->is_upper_bound_) {
          // Array
          metastring << "[(name=" << member->name_ << "_,dimension=" << member->array_size_ << ")";
        } else {
          // BoundedSequence
          metastring << "<(name=" << member->name_ << "_,maximum=" << member->array_size_ << ")";
        }
      } else {
        // UnboundedSequence
        metastring << "<(name=" << member->name_ << "_)";
      }
    }

    if (member->type_id_ == rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE) {
      if (member->members_ == nullptr || member->members_->data == nullptr) {
        RMW_SET_ERROR_MSG("Members handle is null");
        return "";
      }

      auto inner_struct = static_cast<const MessageMembersT *>(member->members_->data);
      std::string inner_metastring = _parse_struct<MessageMembersT>(
        inner_struct, member->is_array_ ? nullptr : member->name_, false);
      if (inner_metastring.empty()) {
        return "";
      }
      metastring << inner_metastring;
    } else {
      switch (member->type_id_) {
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT:
          metastring << "f";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_DOUBLE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_LONG_DOUBLE:
          metastring << "d";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_OCTET:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          metastring << "B";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_WCHAR:
          metastring << "w";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN:
          metastring << "z";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          metastring << "S";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          metastring << "s";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          metastring << "I";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          metastring << "i";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          metastring << "L";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          metastring << "l";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          metastring << "'";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_WSTRING:
          metastring << "W";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
          break;
        default:
          RMW_SET_ERROR_MSG("Unknown type");
          return "";
      }

      metastring <<
        "(" <<
      (member->is_array_ ? "" : std::string("name=") + member->name_ + "_") <<
        ")";
    }
  }

  return metastring.str();
}

template<typename MessageMembersT>
std::string
_create_metastring(const void * untyped_members, bool is_service)
{
  auto members = static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Null members handle is given");
    return "";
  }

  std::ostringstream metastring;
  metastring <<
    "!1" <<
    _parse_struct<MessageMembersT>(
    static_cast<const MessageMembersT *>(members), nullptr, is_service);

  if (is_service) {
    metastring <<
      "L(name=gurumdds__client_guid_0_)" <<
      "L(name=gurumdds__client_guid_1_)" <<
      "l(name=gurumdds__sequence_number_)";
  }

  return metastring.str();
}

inline std::string
create_metastring(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _create_metastring<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      false
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _create_metastring<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      false
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return std::string("");
}

template<typename MessageMembersT>
void *
_allocate_message(
  const void * untyped_members,
  const uint8_t * ros_message,
  size_t * size,
  bool is_service)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  if (ros_message == nullptr) {
    RMW_SET_ERROR_MSG("ros message is null");
    return nullptr;
  }

  if (size == nullptr) {
    RMW_SET_ERROR_MSG("size pointer is null");
    return nullptr;
  }

  auto buffer = CDRSerializationBuffer(nullptr, 0);
  auto serializer = MessageSerializer(buffer);
  serializer.serialize(members, ros_message, true);
  if (is_service) {
    uint64_t dummy = 0;
    buffer << dummy;  // client_guid_0
    buffer << dummy;  // client_guid_1
    buffer << dummy;  // sequence_number
    buffer << dummy;  // padding
  }

  *size = buffer.get_offset() + 4;
  void * message = calloc(1, *size);
  if (message == nullptr) {
    RMW_SET_ERROR_MSG("Failed to allocate memory for dds message");
    return nullptr;
  }

  return message;
}

inline void *
allocate_message(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message,
  size_t * size,
  bool is_service)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_message<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      size,
      is_service
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_message<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      size,
      is_service
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

template<typename MessageMembersT>
ssize_t
_get_serialized_size(
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

  auto buffer = CDRSerializationBuffer(nullptr, 0);
  auto serializer = MessageSerializer(buffer);
  serializer.serialize(members, ros_message, true);

  return static_cast<ssize_t>(buffer.get_offset() + 4);
}

inline ssize_t
get_serialized_size(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _get_serialized_size<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message)
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _get_serialized_size<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message)
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return -1;
}

template<typename MessageMembersT>
bool
_serialize_ros_to_cdr(
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
    auto buffer = CDRSerializationBuffer(dds_message, size);
    auto serializer = MessageSerializer(buffer);
    serializer.serialize(members, ros_message, true);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to serialize ros message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
serialize_ros_to_cdr(
  const void * untyped_members,
  const char * identifier,
  const void * ros_message,
  void * dds_message,
  const size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_ros_to_cdr<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_ros_to_cdr<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename MessageMembersT>
bool
_deserialize_cdr_to_ros(
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
    auto buffer = CDRDeserializationBuffer(dds_message, size);
    auto deserializer = MessageDeserializer(buffer);
    deserializer.deserialize(members, ros_message, true);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
deserialize_cdr_to_ros(
  const void * untyped_members,
  const char * identifier,
  void * ros_message,
  void * dds_message,
  const size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_cdr_to_ros<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_cdr_to_ros<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_message),
      reinterpret_cast<uint8_t *>(dds_message),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

#endif  // TYPE_SUPPORT_COMMON_HPP_

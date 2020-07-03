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

#ifndef MESSAGE_CONVERTER_HPP_
#define MESSAGE_CONVERTER_HPP_

#include "rosidl_runtime_cpp/bounded_vector.hpp"

#include "rosidl_runtime_c/primitives_sequence.h"
#include "rosidl_runtime_c/primitives_sequence_functions.h"
#include "rosidl_runtime_c/string.h"
#include "rosidl_runtime_c/string_functions.h"
#include "rosidl_runtime_c/u16string.h"
#include "rosidl_runtime_c/u16string_functions.h"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"

#include "rosidl_typesupport_introspection_c/message_introspection.h"

#include "./cdr_buffer.hpp"

class MessageSerializer
{
public:
  explicit MessageSerializer(CDRSerializationBuffer & a_buffer)
  : buffer(a_buffer) {}

  template<typename MessageMembersT>
  void serialize(const MessageMembersT * members, const uint8_t * input, bool roundup_)
  {
    for (uint32_t i = 0; i < members->member_count_; i++) {
      auto member = members->members_ + i;
      switch (member->type_id_) {
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN:
          serialize_boolean(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_OCTET:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          serialize_primitive<uint8_t>(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          serialize_primitive<uint16_t>(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          serialize_primitive<uint32_t>(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_DOUBLE:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_LONG_DOUBLE:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          serialize_primitive<uint64_t>(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_WCHAR:
          serialize_wchar(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          serialize_string(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
          serialize_wstring(member, input);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          serialize_struct_arr(member, input);
          break;
        default:
          throw std::logic_error("This should not be rechable");
          break;
      }
    }

    if (roundup_) {
      buffer.roundup(4);
    }
  }

private:
  template<typename MessageMemberT>
  void serialize_boolean(
    const MessageMemberT * member,
    const uint8_t * input);

  template<typename MessageMemberT>
  void serialize_wchar(
    const MessageMemberT * member,
    const uint8_t * input);

  template<typename MessageMemberT>
  void serialize_string(
    const MessageMemberT * member,
    const uint8_t * input);

  template<typename MessageMemberT>
  void serialize_wstring(
    const MessageMemberT * member,
    const uint8_t * input);

  template<typename PrimitiveT, typename MessageMemberT>
  void serialize_primitive(
    const MessageMemberT * member,
    const uint8_t * input);

  template<typename MessageMemberT>
  void serialize_struct_arr(
    const MessageMemberT * member,
    const uint8_t * input);

private:
  CDRSerializationBuffer & buffer;
};

class MessageDeserializer
{
public:
  explicit MessageDeserializer(CDRDeserializationBuffer & a_buffer)
  : buffer(a_buffer) {}

  template<typename MessageMembersT>
  void deserialize(const MessageMembersT * members, uint8_t * output, bool roundup_)
  {
    for (uint32_t i = 0; i < members->member_count_; i++) {
      auto member = members->members_ + i;
      switch (member->type_id_) {
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN:
          deserialize_boolean(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_OCTET:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          deserialize_primitive<uint8_t>(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          deserialize_primitive<uint16_t>(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          deserialize_primitive<uint32_t>(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_DOUBLE:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_LONG_DOUBLE:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          deserialize_primitive<uint64_t>(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_WCHAR:
          deserialize_wchar(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          deserialize_string(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
          deserialize_wstring(member, output);
          break;
        case rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          deserialize_struct_arr(member, output);
          break;
        default:
          break;
      }
    }

    if (roundup_) {
      buffer.roundup(4);
    }
  }

private:
  template<typename MessageMemberT>
  void deserialize_boolean(
    const MessageMemberT * member,
    uint8_t * output);

  template<typename MessageMemberT>
  void deserialize_wchar(
    const MessageMemberT * member,
    uint8_t * output);

  template<typename MessageMemberT>
  void deserialize_string(
    const MessageMemberT * member,
    uint8_t * output);

  template<typename MessageMemberT>
  void deserialize_wstring(
    const MessageMemberT * member,
    uint8_t * output);

  template<typename PrimitiveT, typename MessageMemberT>
  void deserialize_primitive(
    const MessageMemberT * member,
    uint8_t * output);

  template<typename MessageMemberT>
  void deserialize_struct_arr(
    const MessageMemberT * member,
    uint8_t * output);

private:
  CDRDeserializationBuffer & buffer;
};

#endif  // MESSAGE_CONVERTER_HPP_

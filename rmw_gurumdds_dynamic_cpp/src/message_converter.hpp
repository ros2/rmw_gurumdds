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

#include "rosidl_generator_cpp/bounded_vector.hpp"

#include "rosidl_generator_c/primitives_sequence.h"
#include "rosidl_generator_c/primitives_sequence_functions.h"
#include "rosidl_generator_c/string.h"
#include "rosidl_generator_c/string_functions.h"
#include "rosidl_generator_c/u16string.h"
#include "rosidl_generator_c/u16string_functions.h"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"

#include "rosidl_typesupport_introspection_c/message_introspection.h"

#include "./cdr_buffer.hpp"

class MessageSerializer
{
public:
  MessageSerializer(CDRSerializationBuffer & a_buffer) : buffer(a_buffer) {}

  template<typename MessageMembersT>
  void serialize(const MessageMembersT * members, const uint8_t * input) {
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
          if (member->is_array_) {
            if (!member->array_size_ || member->is_upper_bound_) {
              // Sequence
              buffer << static_cast<const uint32_t>(member->size_function(input + member->offset_));
            }

            for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
              serialize(
                reinterpret_cast<const MessageMembersT *>(member->members_->data),
                reinterpret_cast<const uint8_t *>(
                  member->get_const_function(input + member->offset_, i)
                )
              );
            }
          } else {
            serialize(
              reinterpret_cast<const MessageMembersT *>(member->members_->data),
              reinterpret_cast<const uint8_t *>(
                input + member->offset_
              )
            );
          }
          break;
        default:
          throw std::logic_error("This should not be rechable");
          break;
      }
    }

    buffer.roundup(4);
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
    const uint8_t * input)
  {
    if (member->is_array_) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer << static_cast<const uint32_t>(member->size_function(input + member->offset_));
      }

      buffer.copy_arr(
        reinterpret_cast<const PrimitiveT *>(
          member->get_const_function(input + member->offset_, 0)
        ),
        member->size_function(input + member->offset_)
      );
    } else {
      buffer << *(reinterpret_cast<const PrimitiveT *>(input + member->offset_));
    }
  }

private:
  CDRSerializationBuffer & buffer;
};

class MessageDeserializer
{
public:
  MessageDeserializer(CDRDeserializationBuffer & a_buffer) : buffer(a_buffer) {}

  template<typename MessageMembersT>
  void deserialize(const MessageMembersT * members, uint8_t * output) {
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
          if (member->is_array_) {
            if (!member->array_size_ || member->is_upper_bound_) {
              // Sequence
              uint32_t size = 0;
              buffer >> size;
              member->resize_function(output + member->offset_, static_cast<size_t>(size));
            }

            for (uint32_t i = 0; i < member->size_function(output + member->offset_); i++) {
              deserialize(
                reinterpret_cast<const MessageMembersT *>(member->members_->data),
                reinterpret_cast<uint8_t *>(
                  member->get_function(output + member->offset_, i)
                )
              );
            }
          } else {
            deserialize(
              reinterpret_cast<const MessageMembersT *>(member->members_->data),
              reinterpret_cast<uint8_t *>(
                output + member->offset_
              )
            );
          }
          break;
        default:
          break;
      }
    }

    buffer.roundup(4);
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
    uint8_t * output)
  {
    if (member->is_array_) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        uint32_t size = 0;
        buffer >> size;
        member->resize_function(output + member->offset_, static_cast<size_t>(size));
      }

      buffer.copy_arr(
        reinterpret_cast<PrimitiveT *>(
          member->get_function(output + member->offset_, 0)
        ),
        member->size_function(output + member->offset_)
      );
    } else {
      buffer >> *(reinterpret_cast<PrimitiveT *>(output + member->offset_));
    }
  }

private:
  CDRDeserializationBuffer & buffer;
};

#endif  // MESSAGE_CONVERTER_HPP_

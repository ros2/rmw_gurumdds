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

#ifndef RMW_GURUMDDS_CPP__MESSAGE_DESERIALIZER_HPP_
#define RMW_GURUMDDS_CPP__MESSAGE_DESERIALIZER_HPP_

#include <utility>
#include <vector>
#include <string>

#include "rosidl_runtime_c/primitives_sequence.h"
#include "rosidl_runtime_c/primitives_sequence_functions.h"
#include "rosidl_runtime_c/string.h"
#include "rosidl_runtime_c/string_functions.h"
#include "rosidl_runtime_c/u16string.h"
#include "rosidl_runtime_c/u16string_functions.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "cdr_buffer.hpp"
#include "rmw_gurumdds_cpp/message_converter.hpp"

namespace rmw_gurumdds_cpp
{
template<typename MessageMembersT>
class MessageDeserializer {
public:
  using MessageMemberT = MessageMemberType<MessageMembersT>;

  static constexpr LanguageKind LANGUAGE_KIND = get_language_kind<MessageMemberT>();

  explicit MessageDeserializer(CdrDeserializationBuffer & buffer);

  void deserialize(const MessageMembersT * members, uint8_t * output);

private:
  void read_boolean(
    const MessageMemberT * member,
    uint8_t * output);

  void read_wchar(
    const MessageMemberT * member,
    uint8_t * output);

  void read_string(
    const MessageMemberT * member,
    uint8_t * output);

  void read_wstring(
    const MessageMemberT * member,
    uint8_t * output);

  template<typename PrimitiveT>
  void read_primitive(
    const MessageMemberT * member,
    uint8_t * output);

  void read_struct_arr(
    const MessageMemberT * member,
    uint8_t * output);

  CdrDeserializationBuffer & buffer_;
};
}  // namespace rmw_gurumdds_cpp

#include "message_deserializer.inl"

#endif  // RMW_GURUMDDS_CPP__MESSAGE_DESERIALIZER_HPP_

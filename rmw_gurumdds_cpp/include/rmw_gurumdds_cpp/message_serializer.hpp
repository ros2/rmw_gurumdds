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

#ifndef RMW_GURUMDDS_CPP__MESSAGE_SERIALIZER_HPP_
#define RMW_GURUMDDS_CPP__MESSAGE_SERIALIZER_HPP_

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
#include "message_converter.hpp"

namespace rmw_gurumdds_cpp
{
template<bool SERIALIZE, typename MessageMembersT>
class MessageSerializer {
public:
  using MessageMemberT = MessageMemberType<MessageMembersT>;

  static constexpr LanguageKind LANGUAGE_KIND = get_language_kind<MessageMemberT>();

  explicit MessageSerializer(CdrSerializationBuffer<SERIALIZE> & buffer);

  void serialize(const MessageMembersT * members, const uint8_t * input, bool roundup_);

private:
  void serialize_boolean(
    const MessageMemberT * member,
    const uint8_t * input);

  void serialize_wchar(
    const MessageMemberT * member,
    const uint8_t * input);

  void serialize_string(
    const MessageMemberT * member,
    const uint8_t * input);

  void serialize_wstring(
    const MessageMemberT * member,
    const uint8_t * input);


  void serialize_struct_arr(
    const MessageMemberT * member,
    const uint8_t * input);

  template<typename PrimitiveT>
  void serialize_primitive(
    const MessageMemberT * member,
    const uint8_t * input);

private:
  CdrSerializationBuffer<SERIALIZE> & buffer;
};
}  // namespace rmw_gurumdds_cpp

#include "rmw_gurumdds_cpp/message_serializer.inl"

#endif  // RMW_GURUMDDS_CPP__MESSAGE_SERIALIZER_HPP_

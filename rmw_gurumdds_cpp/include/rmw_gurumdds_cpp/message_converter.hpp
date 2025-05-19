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

#ifndef RMW_GURUMDDS_CPP__MESSAGE_CONVERTER_HPP_
#define RMW_GURUMDDS_CPP__MESSAGE_CONVERTER_HPP_

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

#include "rmw_gurumdds_cpp/cdr_buffer.hpp"

namespace rmw_gurumdds_cpp
{
enum class LanguageKind { UNKNOWN, C, CXX };

template<typename T>
constexpr LanguageKind get_language_kind() {
  if constexpr (std::is_same_v<T, rosidl_typesupport_introspection_c__MessageMember>) {
    return LanguageKind::C;
  } else if constexpr (std::is_same_v<T, rosidl_typesupport_introspection_cpp::MessageMember>) {
    return LanguageKind::CXX;
  }

  return LanguageKind::UNKNOWN;
}

template<typename MessageMembersT>
using MessageMemberType =
    std::remove_cv_t<std::remove_pointer_t<decltype(std::declval<MessageMembersT>().members_)>>;

template<typename T>
struct rmw_seq_t {};

#define RMW_GURUMDDS_SEQ_HELPER(HelperItemType, SIZE)                                       \
template<> struct rmw_seq_t<HelperItemType>: rosidl_runtime_c__uint ## SIZE ## __Sequence { \
  bool init(size_t size)                                                                    \
  {                                                                                         \
    return rosidl_runtime_c__uint ## SIZE ## __Sequence__init(this, size);                  \
  }                                                                                         \
                                                                                            \
  void fini() { rosidl_runtime_c__uint ## SIZE ## __Sequence__fini(this); }                 \
};

RMW_GURUMDDS_SEQ_HELPER(uint8_t, 8);
RMW_GURUMDDS_SEQ_HELPER(uint16_t, 16);
RMW_GURUMDDS_SEQ_HELPER(uint32_t, 32);
RMW_GURUMDDS_SEQ_HELPER(uint64_t, 64);
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__MESSAGE_CONVERTER_HPP_

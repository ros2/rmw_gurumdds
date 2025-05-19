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

#ifndef RMW_GURUMDDS_CPP__MESSAGE_SERIALIZER_INL_
#define RMW_GURUMDDS_CPP__MESSAGE_SERIALIZER_INL_

namespace rmw_gurumdds_cpp
{
template<bool SERIALIZE, typename MessageMembersT>
inline MessageSerializer<SERIALIZE, MessageMembersT>::MessageSerializer(CdrSerializationBuffer<SERIALIZE> & buffer)
: buffer(buffer) {
    static_assert(LANGUAGE_KIND != LanguageKind::UNKNOWN);
}

template<bool SERIALIZE, typename MessageMembersT>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize(
  const MessageMembersT * members,
  const uint8_t * input,
  bool roundup_) {
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

template<bool SERIALIZE, typename MessageMembersT>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize_boolean(
  const MessageMemberT * member,
  const uint8_t * input) {
  if (member->is_array_) {
    if constexpr (LANGUAGE_KIND == LanguageKind::C) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        auto seq =
          *(reinterpret_cast<const rosidl_runtime_c__boolean__Sequence *>(input + member->offset_));
        buffer << static_cast<uint32_t>(seq.size);

        for (uint32_t i = 0; i < seq.size; i++) {
          buffer << static_cast<uint8_t>(seq.data[i] == true);
        }
      } else {
        // Array
        for (uint32_t i = 0; i < member->array_size_; i++) {
          buffer << static_cast<uint8_t>(
            (reinterpret_cast<const bool *>(input + member->offset_))[i] == true);
        }
      }
    }

    if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        auto vec =
          *(reinterpret_cast<const std::vector<bool> *>(input + member->offset_));
        buffer << static_cast<uint32_t>(vec.size());
        for (const auto & i : vec) {
          buffer << static_cast<uint8_t>(i == true);
        }
      } else {
        // Array
        const uint32_t size = member->size_function(input + member->offset_);
        for (uint32_t i = 0; i < size; i++) {
          buffer <<
            *(reinterpret_cast<const uint8_t *>(
              member->get_const_function(input + member->offset_, i)));
        }
      }
    }
  } else {
    buffer << static_cast<uint8_t>(*(reinterpret_cast<const bool *>(input + member->offset_)));
  }
}

template<bool SERIALIZE, typename MessageMembersT>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize_wchar(
  const MessageMemberT * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if constexpr (LANGUAGE_KIND == LanguageKind::C) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        auto seq =
          *(reinterpret_cast<const rosidl_runtime_c__wchar__Sequence *>(input + member->offset_));
        buffer << static_cast<uint32_t>(seq.size);

        for (uint32_t i = 0; i < seq.size; i++) {
          buffer << static_cast<uint32_t>(seq.data[i]);
        }
      }
    }

    if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
      const uint32_t size = static_cast<uint32_t>(member->size_function(input + member->offset_));
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer << size;
      }

      for (uint32_t i = 0; i < size; i++) {
        buffer << *(reinterpret_cast<const uint16_t *>(member->get_const_function(input + member->offset_, i)));
      }
    }
  } else {
    buffer << *(reinterpret_cast<const uint16_t *>(input + member->offset_));
  }
}

template<bool SERIALIZE, typename MessageMembersT>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize_string(
  const MessageMemberT * member,
  const uint8_t *input) {
  if constexpr (LANGUAGE_KIND == LanguageKind::C) {
    if (member->is_array_) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        auto seq =
          *(reinterpret_cast<const rosidl_runtime_c__String__Sequence *>(input + member->offset_));
        buffer << static_cast<uint32_t>(seq.size);

        for (uint32_t i = 0; i < seq.size; i++) {
          buffer << seq.data[i];
        }
      } else {
        // Array
        for (uint32_t i = 0; i < member->array_size_; i++) {
          buffer << *(reinterpret_cast<const rosidl_runtime_c__String *>(input + member->offset_) + i);
        }
      }
    } else {
      buffer << *(reinterpret_cast<const rosidl_runtime_c__String *>(input + member->offset_));
    }
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
    if (member->is_array_) {
      const uint32_t size = static_cast<uint32_t>(member->size_function(input + member->offset_));
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer << size;
      }

      for (uint32_t i = 0; i < size; i++) {
        buffer << *(reinterpret_cast<const std::string *>(member->get_const_function(input + member->offset_, i)));
      }
    } else {
      buffer << *(reinterpret_cast<const std::string *>(input + member->offset_));
    }
  }
}

template<bool SERIALIZE, typename MessageMembersT>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize_wstring(
  const MessageMemberT * member,
  const uint8_t *input) {
  if constexpr (LANGUAGE_KIND == LanguageKind::C) {
    if (member->is_array_) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        auto seq =
          *(reinterpret_cast<const rosidl_runtime_c__U16String__Sequence *>(input + member->offset_));
        buffer << static_cast<uint32_t>(seq.size);

        for (uint32_t i = 0; i < seq.size; i++) {
          buffer << seq.data[i];
        }
      } else {
        // Array
        for (uint32_t i = 0; i < member->array_size_; i++) {
          buffer <<
            *(reinterpret_cast<const rosidl_runtime_c__U16String *>(input + member->offset_) + i);
        }
      }
    } else {
      buffer << *(reinterpret_cast<const rosidl_runtime_c__U16String *>(input + member->offset_));
    }

    return;
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
    if (member->is_array_) {
      const uint32_t size = member->size_function(input + member->offset_);
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer << static_cast<uint32_t>(size);
      }

      for (uint32_t i = 0; i < size; i++) {
        buffer << *(reinterpret_cast<const std::u16string *>(member->get_const_function(input + member->offset_, i)));
      }
    } else {
      buffer << *(reinterpret_cast<const std::u16string *>(input + member->offset_));
    }
  }
}

template<bool SERIALIZE, typename MessageMembersT>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize_struct_arr(
  const MessageMemberT * member,
  const uint8_t * input) {
  if (member->is_array_) {
    const void* ptr = input + member->offset_;
    const uint32_t size = static_cast<uint32_t>(member->size_function(input + member->offset_));
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer << size;
    }

    for (uint32_t i = 0; i < size; i++) {
      serialize(reinterpret_cast<const MessageMembersT *>(member->members_->data),
                reinterpret_cast<const uint8_t*>(member->get_const_function(ptr, i)), false);
    }
  } else {
    serialize(reinterpret_cast<const MessageMembersT *>(member->members_->data),
              reinterpret_cast<const uint8_t*>(input + member->offset_), false);
  }
}

template<bool SERIALIZE, typename MessageMembersT>
template<typename T>
inline void MessageSerializer<SERIALIZE, MessageMembersT>::serialize_primitive(
  const MessageSerializer::MessageMemberT * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if constexpr (LANGUAGE_KIND == LanguageKind::C) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        auto seq = reinterpret_cast<const rmw_gurumdds_cpp::rmw_seq_t<T>*>(input + member->offset_);
        buffer << static_cast<uint32_t>(seq->size);
        buffer.copy_arr(seq->data, seq->size);
      } else {
        // Array
        buffer.copy_arr(reinterpret_cast<const T*>(input + member->offset_), member->array_size_);
      }
    }

    if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
      const uint32_t size = static_cast<uint32_t>(member->size_function(input + member->offset_));
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer << size;
      }

      buffer.copy_arr(reinterpret_cast<const T*>(member->get_const_function(input + member->offset_, 0)), size);
    }

  } else {
    buffer << *reinterpret_cast<const T *>(input + member->offset_);
  }
}
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__MESSAGE_SERIALIZER_INL_

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

#ifndef RMW_GURUMDDS_CPP__MESSAGE_DESERIALIZER_INL_
#define RMW_GURUMDDS_CPP__MESSAGE_DESERIALIZER_INL_

namespace rmw_gurumdds_cpp
{
template<typename MessageMembersT>
inline MessageDeserializer<MessageMembersT>::MessageDeserializer(CdrDeserializationBuffer & buffer)
  : buffer_(buffer) {
  static_assert(LANGUAGE_KIND != LanguageKind::UNKNOWN);
}

template<typename MessageMembersT>
inline void MessageDeserializer<MessageMembersT>::deserialize(const MessageMembersT *members, uint8_t *output) {
  for (uint32_t i = 0; i < members->member_count_; i++) {
    auto member = members->members_ + i;
    switch (member->type_id_) {
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN:
        read_boolean(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_OCTET:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        read_primitive<uint8_t>(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        read_primitive<uint16_t>(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        read_primitive<uint32_t>(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_DOUBLE:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        read_primitive<uint64_t>(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_WCHAR:
        read_wchar(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        read_string(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
        read_wstring(member, output);
        break;
      case rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        read_struct_arr(member, output);
        break;
      default:
        break;
    }
  }
}

template<typename MessageMembersT>
inline void MessageDeserializer<MessageMembersT>::read_boolean(const MessageMemberT * member, uint8_t * output) {
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer_ >> size;
      if constexpr (LANGUAGE_KIND == LanguageKind::C) {
        auto seq_ptr = (reinterpret_cast<rosidl_runtime_c__boolean__Sequence *>(output + member->offset_));
        if (seq_ptr->data) {
          rosidl_runtime_c__boolean__Sequence__fini(seq_ptr);
        }

        bool res = rosidl_runtime_c__boolean__Sequence__init(seq_ptr, size);
        if (!res) {
          throw std::runtime_error("Failed to initialize sequence");
        }

        for (uint32_t i = 0; i < size; i++) {
          uint8_t data = 0;
          buffer_ >> data;
          seq_ptr->data[i] = (data != 0);
        }
      }

      if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
        auto vec = reinterpret_cast<std::vector<bool> *>(output + member->offset_);
        vec->resize(static_cast<size_t>(size));
        for (uint32_t i = 0; i < size; i++) {
          uint8_t data = 0;
          buffer_ >> data;
          vec->at(i) = static_cast<bool>(data != 0);
        }
      }
    } else {
      // Array
      if constexpr (LANGUAGE_KIND == LanguageKind::C) {
        auto arr = reinterpret_cast<bool *>(output + member->offset_);
        for (uint32_t i = 0; i < member->array_size_; i++) {
          uint8_t data = 0;
          buffer_ >> data;
          arr[i] = (data != 0);
        }
      }

      if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
        for (uint32_t i = 0; i < member->array_size_; i++) {
          uint8_t data = 0;
          buffer_ >> data;
          *(reinterpret_cast<bool *>(member->get_function(output + member->offset_, i))) =
            static_cast<bool>(data != 0);
        }
      }
    }
  } else {
    auto dst = reinterpret_cast<bool *>(output + member->offset_);
    uint8_t data = 0;
    buffer_ >> data;
    *dst = (data != 0);
  }
}

template<typename MessageMembersT>
inline void MessageDeserializer<MessageMembersT>::read_wchar(const MessageMemberT * member, uint8_t * output) {
  if (!member->is_array_) {
    auto dst = reinterpret_cast<uint16_t *>(output + member->offset_);
    buffer_ >> *dst;
    return;
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::C) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer_ >> size;
      auto seq_ptr = reinterpret_cast<rosidl_runtime_c__wchar__Sequence *>(output + member->offset_);
      if (seq_ptr->data) {
        rosidl_runtime_c__wchar__Sequence__fini(seq_ptr);
      }

      bool res = rosidl_runtime_c__wchar__Sequence__init(seq_ptr, size);
      if (!res) {
        throw std::runtime_error("Failed to initialize sequence");
      }

      for (uint32_t i = 0; i < size; i++) {
        uint16_t data = 0;
        buffer_ >> data;
        seq_ptr->data[i] = data;
      }
    } else {
      auto arr = reinterpret_cast<uint16_t *>(output + member->offset_);
      for (uint32_t i = 0; i < member->array_size_; i++) {
        buffer_ >> arr[i];
      }
    }
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer_ >> size;
      member->resize_function(output + member->offset_, static_cast<size_t>(size));
    }

    for (uint32_t i = 0; i < member->size_function(output + member->offset_); i++) {
      uint16_t data = 0;
      buffer_ >> data;
      *(reinterpret_cast<uint16_t *>(member->get_function(output + member->offset_, i))) =
        static_cast<uint16_t>(data);
    }
  }
}

template<typename MessageMembersT>
inline void MessageDeserializer<MessageMembersT>::read_string(const MessageMemberT * member, uint8_t * output) {
  if constexpr (LANGUAGE_KIND == LanguageKind::C) {
    if (member->is_array_) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        uint32_t size = 0;
        buffer_ >> size;

        auto seq_ptr =
          (reinterpret_cast<rosidl_runtime_c__String__Sequence *>(output + member->offset_));
        if (seq_ptr->data) {
          rosidl_runtime_c__String__Sequence__fini(seq_ptr);
        }
        bool res = rosidl_runtime_c__String__Sequence__init(seq_ptr, size);
        if (!res) {
          throw std::runtime_error("Failed to initialize sequence");
        }

        for (uint32_t i = 0; i < size; i++) {
          if (seq_ptr->data[i].data == nullptr) {
            if (!rosidl_runtime_c__String__init(&seq_ptr->data[i])) {
              throw std::runtime_error("Failed to initialize string");
            }
          }
          buffer_ >> seq_ptr->data[i];
        }
      } else {
        auto arr = reinterpret_cast<rosidl_runtime_c__String *>(output + member->offset_);
        for (uint32_t i = 0; i < member->array_size_; i++) {
          if (arr == nullptr) {
            if (!rosidl_runtime_c__String__init(&arr[i])) {
              throw std::runtime_error("Failed to initialize string");
            }
          }
          buffer_ >> arr[i];
        }
      }
    } else {
      auto dst = reinterpret_cast<rosidl_runtime_c__String *>(output + member->offset_);
      if (dst->data == nullptr) {
        if (!rosidl_runtime_c__String__init(dst)) {
          throw std::runtime_error("Failed to initialize string");
        }
      }
      buffer_ >> *dst;
    }
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
    if (member->is_array_) {
      uint32_t size = 0;
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer_ >> size;
        member->resize_function(output + member->offset_, size);
      }

      size = member->size_function(output + member->offset_);
      for (uint32_t i = 0; i < size; i++) {
        buffer_ >> *(reinterpret_cast<std::string *>(member->get_function(output + member->offset_, i)));
      }
    } else {
      buffer_ >> *(reinterpret_cast<std::string *>(output + member->offset_));
    }
  }
}

template<typename MessageMembersT>
template<typename PrimitiveT>
inline void MessageDeserializer<MessageMembersT>::read_primitive(const MessageMemberT * member, uint8_t * output) {
  if (!member->is_array_) {
    buffer_ >> *(reinterpret_cast<PrimitiveT*>(output + member->offset_));
    return;
  }

  uint32_t size = 0;
  PrimitiveT* arr = nullptr;
  if constexpr (LANGUAGE_KIND == LanguageKind::C) {
    if (!member->array_size_ || member->is_upper_bound_) {
      buffer_ >> size;
      auto seq_ptr = reinterpret_cast<rmw_gurumdds_cpp::rmw_seq_t<PrimitiveT>*>(output + member->offset_);
      if(nullptr != seq_ptr->data) {
        seq_ptr->fini();
      }

      bool res = seq_ptr->init(size);
      if (!res) {
        throw std::runtime_error("Failed to initialize sequence");
      }

      arr = seq_ptr->data;
    } else {
      size = member->array_size_;
      arr = reinterpret_cast<PrimitiveT*>(output + member->offset_);
    }
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
    if (!member->array_size_ || member->is_upper_bound_) {
      buffer_ >> size;
      member->resize_function(output + member->offset_, static_cast<size_t>(size));
    } else {
      size = member->array_size_;
    }

    arr = reinterpret_cast<PrimitiveT*>(member->get_function(output + member->offset_, 0));
  }

  buffer_.copy_arr(arr, size);
}

template<typename MessageMembersT>
inline void MessageDeserializer<MessageMembersT>::read_wstring(const MessageMemberT * member, uint8_t * output) {
  if constexpr (LANGUAGE_KIND == LanguageKind::C) {
    if (member->is_array_) {
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        uint32_t size = 0;
        buffer_ >> size;

        auto seq_ptr =
          (reinterpret_cast<rosidl_runtime_c__U16String__Sequence *>(
            output + member->offset_));
        if (seq_ptr->data) {
          rosidl_runtime_c__U16String__Sequence__fini(seq_ptr);
        }
        bool res = rosidl_runtime_c__U16String__Sequence__init(seq_ptr, size);
        if (!res) {
          throw std::runtime_error("Failed to initialize sequence");
        }

        for (uint32_t i = 0; i < size; i++) {
          if (seq_ptr->data[i].data == nullptr) {
            if (!rosidl_runtime_c__U16String__init(&seq_ptr->data[i])) {
              throw std::runtime_error("Failed to initialize string");
            }
          }
          buffer_ >> seq_ptr->data[i];
        }
      } else {
        auto arr = reinterpret_cast<rosidl_runtime_c__U16String *>(output + member->offset_);
        for (uint32_t i = 0; i < member->array_size_; i++) {
          if (arr == nullptr) {
            if (!rosidl_runtime_c__U16String__init(&arr[i])) {
              throw std::runtime_error("Failed to initialize string");
            }
          }
          buffer_ >> arr[i];
        }
      }
    } else {
      auto dst = reinterpret_cast<rosidl_runtime_c__U16String *>(output + member->offset_);
      if (dst->data == nullptr) {
        if (!rosidl_runtime_c__U16String__init(dst)) {
          throw std::runtime_error("Failed to initialize string");
        }
      }
      buffer_ >> *dst;
    }
  }

  if constexpr (LANGUAGE_KIND == LanguageKind::CXX) {
    if (member->is_array_) {
      uint32_t size = 0;
      if (!member->array_size_ || member->is_upper_bound_) {
        // Sequence
        buffer_ >> size;
        member->resize_function(output + member->offset_, size);
      }

      size = member->size_function(output + member->offset_);
      for (uint32_t i = 0; i < size; i++) {
        buffer_ >>
          *(reinterpret_cast<std::u16string *>(member->get_function(output + member->offset_, i)));
      }
    } else {
      buffer_ >> *(reinterpret_cast<std::u16string *>(output + member->offset_));
    }
  }
}

template<typename MessageMembersT>
inline void MessageDeserializer<MessageMembersT>::read_struct_arr(const MessageMemberT * member, uint8_t * output) {
  if (member->is_array_) {
    uint32_t size = 0;
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer_ >> size;
      member->resize_function(output + member->offset_, static_cast<size_t>(size));
    }

    size = member->size_function(output + member->offset_);
    for (uint32_t j = 0; j < size; j++) {
      deserialize(
        reinterpret_cast<const MessageMembersT *>(member->members_->data),
        reinterpret_cast<uint8_t *>(member->get_function(output + member->offset_, j)));
    }
  } else {
    deserialize(
      reinterpret_cast<const MessageMembersT *>(
        member->members_->data
        ),
      reinterpret_cast<uint8_t *>(
        output + member->offset_
        )
    );
  }
}
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__MESSAGE_DESERIALIZER_INL_

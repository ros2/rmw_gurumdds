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

#include <string>
#include <vector>

#include "message_converter.hpp"

#define SERIALIZER_C_SERIALIZE_PRIMITIVE(SIZE) \
  template<> \
  void MessageSerializer::serialize_primitive<uint ## SIZE ## _t>( \
    const rosidl_typesupport_introspection_c__MessageMember * member, \
    const uint8_t * input) \
  { \
    if (member->is_array_) { \
      if (!member->array_size_ || member->is_upper_bound_) { \
        auto seq = \
          *(reinterpret_cast<const rosidl_runtime_c__uint ## SIZE ## __Sequence *>( \
            input + member->offset_)); \
        buffer << static_cast<uint32_t>(seq.size); \
 \
        buffer.copy_arr(seq.data, seq.size); \
      } else { \
        buffer.copy_arr( \
          reinterpret_cast<const uint ## SIZE ## _t *>(input + member->offset_), \
          member->array_size_ \
        ); \
      } \
    } else { \
      buffer << *(reinterpret_cast<const uint ## SIZE ## _t *>(input + member->offset_)); \
    } \
  } \

#define DESERIALIZER_C_DESERIALIZE_PRIMITIVE(SIZE) \
  template<> \
  void MessageDeserializer::deserialize_primitive<uint ## SIZE ## _t>( \
    const rosidl_typesupport_introspection_c__MessageMember * member, \
    uint8_t * output) \
  { \
    if (member->is_array_) { \
      if (!member->array_size_ || member->is_upper_bound_) { \
        uint32_t size = 0; \
        buffer >> size; \
 \
        auto seq_ptr = \
          (reinterpret_cast<rosidl_runtime_c__uint ## SIZE ## __Sequence *>( \
            output + member->offset_)); \
        if (seq_ptr->data) { \
          rosidl_runtime_c__uint ## SIZE ## __Sequence__fini(seq_ptr); \
        } \
        bool res = rosidl_runtime_c__uint ## SIZE ## __Sequence__init(seq_ptr, size); \
        if (!res) { \
          throw std::runtime_error("Failed to initialize sequence"); \
        } \
 \
        buffer.copy_arr(seq_ptr->data, seq_ptr->size); \
      } else { \
        buffer.copy_arr( \
          reinterpret_cast<uint ## SIZE ## _t *>(output + member->offset_), \
          member->array_size_ \
        ); \
      } \
    } else { \
      buffer >> *(reinterpret_cast<uint ## SIZE ## _t *>(output + member->offset_)); \
    } \
  } \

#define SERIALIZER_CPP_SERIALIZE_PRIMITIVE(SIZE) \
  template<> \
  void MessageSerializer::serialize_primitive<uint ## SIZE ## _t>( \
    const rosidl_typesupport_introspection_cpp::MessageMember * member, \
    const uint8_t * input) \
  { \
    if (member->is_array_) { \
      if (!member->array_size_ || member->is_upper_bound_) { \
        buffer << static_cast<uint32_t>(member->size_function(input + member->offset_)); \
      } \
 \
      buffer.copy_arr( \
        reinterpret_cast<const uint ## SIZE ## _t *>( \
          member->get_const_function(input + member->offset_, 0) \
        ), \
        member->size_function(input + member->offset_) \
      ); \
    } else { \
      buffer << *(reinterpret_cast<const uint ## SIZE ## _t *>(input + member->offset_)); \
    } \
  }

#define DESERIALIZER_CPP_DESERIALIZE_PRIMITIVE(SIZE) \
  template<> \
  void MessageDeserializer::deserialize_primitive<uint ## SIZE ## _t>( \
    const rosidl_typesupport_introspection_cpp::MessageMember * member, \
    uint8_t * output) \
  { \
    if (member->is_array_) { \
      if (!member->array_size_ || member->is_upper_bound_) { \
        uint32_t size = 0; \
        buffer >> size; \
        member->resize_function(output + member->offset_, static_cast<size_t>(size)); \
      } \
 \
      buffer.copy_arr( \
        reinterpret_cast<uint ## SIZE ## _t *>( \
          member->get_function(output + member->offset_, 0) \
        ), \
        member->size_function(output + member->offset_) \
      ); \
    } else { \
      buffer >> *(reinterpret_cast<uint ## SIZE ## _t *>(output + member->offset_)); \
    } \
  }

SERIALIZER_CPP_SERIALIZE_PRIMITIVE(8)
SERIALIZER_CPP_SERIALIZE_PRIMITIVE(16)
SERIALIZER_CPP_SERIALIZE_PRIMITIVE(32)
SERIALIZER_CPP_SERIALIZE_PRIMITIVE(64)

template<>
void MessageSerializer::serialize_boolean(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
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
      for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
        buffer <<
          *(reinterpret_cast<const uint8_t *>(
          member->get_const_function(input + member->offset_, i)));
      }
    }
  } else {
    buffer << static_cast<uint8_t>(*(reinterpret_cast<const bool *>(input + member->offset_)));
  }
}

template<>
void MessageSerializer::serialize_wchar(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer << static_cast<uint32_t>(member->size_function(input + member->offset_));
    }

    for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
      buffer <<
        static_cast<uint32_t>(
        *(reinterpret_cast<const uint16_t *>(
          member->get_const_function(input + member->offset_, i))));
    }
  } else {
    buffer <<
      static_cast<uint32_t>(*(reinterpret_cast<const uint16_t *>(input + member->offset_)));
  }
}

template<>
void MessageSerializer::serialize_string(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer << static_cast<uint32_t>(member->size_function(input + member->offset_));
    }

    for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
      buffer <<
        *(reinterpret_cast<const std::string *>(
        member->get_const_function(input + member->offset_, i)));
    }
  } else {
    buffer << *(reinterpret_cast<const std::string *>(input + member->offset_));
  }
}

template<>
void MessageSerializer::serialize_wstring(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer << static_cast<uint32_t>(member->size_function(input + member->offset_));
    }

    for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
      buffer <<
        *(reinterpret_cast<const std::u16string *>(
        member->get_const_function(input + member->offset_, i)));
    }
  } else {
    buffer << *(reinterpret_cast<const std::u16string *>(input + member->offset_));
  }
}

template<>
void MessageSerializer::serialize_struct_arr(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer << static_cast<uint32_t>(member->size_function(input + member->offset_));
    }
    for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
      serialize(
        reinterpret_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
          member->members_->data
        ),
        reinterpret_cast<const uint8_t *>(
          member->get_const_function(input + member->offset_, i)
        ),
        false
      );
    }
  } else {
    serialize(
      reinterpret_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
        member->members_->data
      ),
      reinterpret_cast<const uint8_t *>(
        input + member->offset_
      ),
      false
    );
  }
}

SERIALIZER_C_SERIALIZE_PRIMITIVE(8)
SERIALIZER_C_SERIALIZE_PRIMITIVE(16)
SERIALIZER_C_SERIALIZE_PRIMITIVE(32)
SERIALIZER_C_SERIALIZE_PRIMITIVE(64)

template<>
void MessageSerializer::serialize_boolean(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
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
  } else {
    buffer << static_cast<uint8_t>(
      *(reinterpret_cast<const bool *>(input + member->offset_)) == true);
  }
}

template<>
void MessageSerializer::serialize_wchar(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      auto seq =
        *(reinterpret_cast<const rosidl_runtime_c__wchar__Sequence *>(input + member->offset_));
      buffer << static_cast<uint32_t>(seq.size);

      for (uint32_t i = 0; i < seq.size; i++) {
        buffer << static_cast<uint32_t>(seq.data[i]);
      }
    }
  } else {
    buffer <<
      static_cast<uint32_t>(*(reinterpret_cast<const uint16_t *>(input + member->offset_)));
  }
}

template<>
void MessageSerializer::serialize_string(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  const uint8_t * input)
{
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
        buffer <<
          *(reinterpret_cast<const rosidl_runtime_c__String *>(input + member->offset_) + i);
      }
    }
  } else {
    buffer << *(reinterpret_cast<const rosidl_runtime_c__String *>(input + member->offset_));
  }
}

template<>
void MessageSerializer::serialize_wstring(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      auto seq =
        *(reinterpret_cast<const rosidl_runtime_c__U16String__Sequence *>(
          input + member->offset_));
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
}

template<>
void MessageSerializer::serialize_struct_arr(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  const uint8_t * input)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      buffer << static_cast<uint32_t>(member->size_function(input + member->offset_));
      for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
        serialize(
          reinterpret_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
            member->members_->data
          ),
          reinterpret_cast<const uint8_t *>(
            member->get_const_function(input + member->offset_, i)
          ),
          false
        );
      }
    } else {
      const void * tmp = input + member->offset_;
      for (uint32_t i = 0; i < member->size_function(input + member->offset_); i++) {
        serialize(
          reinterpret_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
            member->members_->data
          ),
          reinterpret_cast<const uint8_t *>(
            member->get_const_function(&tmp, i)
          ),
          false
        );
      }
    }
  } else {
    serialize(
      reinterpret_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
        member->members_->data
      ),
      reinterpret_cast<const uint8_t *>(
        input + member->offset_
      ),
      false
    );
  }
}


// ================================================================================================


DESERIALIZER_CPP_DESERIALIZE_PRIMITIVE(8)
DESERIALIZER_CPP_DESERIALIZE_PRIMITIVE(16)
DESERIALIZER_CPP_DESERIALIZE_PRIMITIVE(32)
DESERIALIZER_CPP_DESERIALIZE_PRIMITIVE(64)

template<>
void MessageDeserializer::deserialize_boolean(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      auto vec =
        (reinterpret_cast<std::vector<bool> *>(output + member->offset_));
      uint32_t size = 0;
      buffer >> size;
      vec->resize(static_cast<size_t>(size));
      for (uint32_t i = 0; i < size; i++) {
        uint8_t data = 0;
        buffer >> data;
        vec->at(i) = static_cast<bool>(data != 0);
      }
    } else {
      // Array
      for (uint32_t i = 0; i < member->array_size_; i++) {
        uint8_t data = 0;
        buffer >> data;
        *(reinterpret_cast<bool *>(member->get_function(output + member->offset_, i))) =
          static_cast<bool>(data != 0);
      }
    }
  } else {
    uint8_t data = 0;
    buffer >> data;
    *(reinterpret_cast<bool *>(output + member->offset_)) = static_cast<bool>(data != 0);
  }
}

template<>
void MessageDeserializer::deserialize_wchar(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;
      member->resize_function(output + member->offset_, static_cast<size_t>(size));
    }

    for (uint32_t i = 0; i < member->size_function(output + member->offset_); i++) {
      uint32_t data = 0;
      buffer >> data;
      *(reinterpret_cast<uint16_t *>(member->get_function(output + member->offset_, i))) =
        static_cast<uint16_t>(data);
    }
  } else {
    uint32_t data = 0;
    buffer >> data;
    *(reinterpret_cast<uint16_t *>(output + member->offset_)) = static_cast<uint16_t>(data);
  }
}

template<>
void MessageDeserializer::deserialize_string(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;
      member->resize_function(output + member->offset_, size);
    }

    for (uint32_t i = 0; i < member->size_function(output + member->offset_); i++) {
      buffer >>
      *(reinterpret_cast<std::string *>(member->get_function(output + member->offset_, i)));
    }
  } else {
    buffer >> *(reinterpret_cast<std::string *>(output + member->offset_));
  }
}

template<>
void MessageDeserializer::deserialize_wstring(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;
      member->resize_function(output + member->offset_, size);
    }

    for (uint32_t i = 0; i < member->size_function(output + member->offset_); i++) {
      buffer >>
      *(reinterpret_cast<std::u16string *>(member->get_function(output + member->offset_, i)));
    }
  } else {
    buffer >> *(reinterpret_cast<std::u16string *>(output + member->offset_));
  }
}

template<>
void MessageDeserializer::deserialize_struct_arr(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;
      member->resize_function(output + member->offset_, static_cast<size_t>(size));
    }
    for (uint32_t j = 0; j < member->size_function(output + member->offset_); j++) {
      deserialize(
        reinterpret_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
          member->members_->data
        ),
        reinterpret_cast<uint8_t *>(
          member->get_function(output + member->offset_, j)
        ),
        false
      );
    }
  } else {
    deserialize(
      reinterpret_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
        member->members_->data
      ),
      reinterpret_cast<uint8_t *>(
        output + member->offset_
      ),
      false
    );
  }
}

DESERIALIZER_C_DESERIALIZE_PRIMITIVE(8)
DESERIALIZER_C_DESERIALIZE_PRIMITIVE(16)
DESERIALIZER_C_DESERIALIZE_PRIMITIVE(32)
DESERIALIZER_C_DESERIALIZE_PRIMITIVE(64)

template<>
void MessageDeserializer::deserialize_boolean(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      uint32_t size = 0;
      buffer >> size;

      auto seq_ptr =
        (reinterpret_cast<rosidl_runtime_c__boolean__Sequence *>(
          output + member->offset_));
      if (seq_ptr->data) {
        rosidl_runtime_c__boolean__Sequence__fini(seq_ptr);
      }
      bool res = rosidl_runtime_c__boolean__Sequence__init(seq_ptr, size);
      if (!res) {
        throw std::runtime_error("Failed to initialize sequence");
      }

      for (uint32_t i = 0; i < size; i++) {
        uint8_t data = 0;
        buffer >> data;
        seq_ptr->data[i] = (data != 0);
      }
    } else {
      auto arr = reinterpret_cast<bool *>(output + member->offset_);
      for (uint32_t i = 0; i < member->array_size_; i++) {
        uint8_t data = 0;
        buffer >> data;
        arr[i] = (data != 0);
      }
    }
  } else {
    auto dst = reinterpret_cast<bool *>(output + member->offset_);
    uint8_t data = 0;
    buffer >> data;
    *dst = (data != 0);
  }
}

template<>
void MessageDeserializer::deserialize_wchar(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;

      auto seq_ptr =
        (reinterpret_cast<rosidl_runtime_c__wchar__Sequence *>(output + member->offset_));
      if (seq_ptr->data) {
        rosidl_runtime_c__wchar__Sequence__fini(seq_ptr);
      }
      bool res = rosidl_runtime_c__wchar__Sequence__init(seq_ptr, size);
      if (!res) {
        throw std::runtime_error("Failed to initialize sequence");
      }

      for (uint32_t i = 0; i < size; i++) {
        uint32_t data = 0;
        buffer >> data;
        seq_ptr->data[i] = static_cast<uint16_t>(data);
      }
    } else {
      auto arr = reinterpret_cast<uint16_t *>(output + member->offset_);
      for (uint32_t i = 0; i < member->array_size_; i++) {
        uint8_t data = 0;
        buffer >> data;
        arr[i] = static_cast<uint16_t>(data);
      }
    }
  } else {
    auto dst = reinterpret_cast<uint16_t *>(output + member->offset_);
    uint32_t data = 0;
    buffer >> data;
    *dst = static_cast<uint16_t>(data);
  }
}

template<>
void MessageDeserializer::deserialize_string(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;

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
        buffer >> seq_ptr->data[i];
      }
    } else {
      auto arr = reinterpret_cast<rosidl_runtime_c__String *>(output + member->offset_);
      for (uint32_t i = 0; i < member->array_size_; i++) {
        if (arr == nullptr) {
          if (!rosidl_runtime_c__String__init(&arr[i])) {
            throw std::runtime_error("Failed to initialize string");
          }
        }
        buffer >> arr[i];
      }
    }
  } else {
    auto dst = reinterpret_cast<rosidl_runtime_c__String *>(output + member->offset_);
    if (dst->data == nullptr) {
      if (!rosidl_runtime_c__String__init(dst)) {
        throw std::runtime_error("Failed to initialize string");
      }
    }
    buffer >> *dst;
  }
}

template<>
void MessageDeserializer::deserialize_wstring(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;

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
        buffer >> seq_ptr->data[i];
      }
    } else {
      auto arr = reinterpret_cast<rosidl_runtime_c__U16String *>(output + member->offset_);
      for (uint32_t i = 0; i < member->array_size_; i++) {
        if (arr == nullptr) {
          if (!rosidl_runtime_c__U16String__init(&arr[i])) {
            throw std::runtime_error("Failed to initialize string");
          }
        }
        buffer >> arr[i];
      }
    }
  } else {
    auto dst = reinterpret_cast<rosidl_runtime_c__U16String *>(output + member->offset_);
    if (dst->data == nullptr) {
      if (!rosidl_runtime_c__U16String__init(dst)) {
        throw std::runtime_error("Failed to initialize string");
      }
    }
    buffer >> *dst;
  }
}

template<>
void MessageDeserializer::deserialize_struct_arr(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  uint8_t * output)
{
  if (member->is_array_) {
    if (!member->array_size_ || member->is_upper_bound_) {
      // Sequence
      uint32_t size = 0;
      buffer >> size;
      member->resize_function(output + member->offset_, static_cast<size_t>(size));
      for (uint32_t j = 0; j < member->size_function(output + member->offset_); j++) {
        deserialize(
          reinterpret_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
            member->members_->data
          ),
          reinterpret_cast<uint8_t *>(
            member->get_function(output + member->offset_, j)
          ),
          false
        );
      }
    } else {
      void * tmp = output + member->offset_;
      for (uint32_t j = 0; j < member->size_function(output + member->offset_); j++) {
        deserialize(
          reinterpret_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
            member->members_->data
          ),
          reinterpret_cast<uint8_t *>(
            member->get_function(&tmp, j)
          ),
          false
        );
      }
    }
  } else {
    deserialize(
      reinterpret_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
        member->members_->data
      ),
      reinterpret_cast<uint8_t *>(
        output + member->offset_
      ),
      false
    );
  }
}

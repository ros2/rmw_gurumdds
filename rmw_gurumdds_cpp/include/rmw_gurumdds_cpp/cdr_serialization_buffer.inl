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

#ifndef RMW_GURUMDDS_CPP__CDR_SERIALIZATION_BUFFER_INL_
#define RMW_GURUMDDS_CPP__CDR_SERIALIZATION_BUFFER_INL_

namespace rmw_gurumdds_cpp
{
template<>
inline CdrSerializationBuffer<true>::CdrSerializationBuffer(uint8_t * buf, size_t size)
  : CdrBuffer{buf, size} {
  if (nullptr == buf) {
    throw std::runtime_error("Buf is null");
  }

  if (size < CDR_HEADER_SIZE) {
    throw std::runtime_error("Insufficient buffer size");
  }
  std::memset(buf, 0, CDR_HEADER_SIZE);
  buf[CDR_HEADER_ENDIAN_IDX] = CDR_SYSTEM_ENDIAN;
  buf_ = buf + CDR_HEADER_SIZE;
  size_ = size - CDR_HEADER_SIZE;
}

template<>
inline CdrSerializationBuffer<false>::CdrSerializationBuffer(uint8_t *, size_t)
  : CdrBuffer{nullptr, std::numeric_limits<size_t>::max()} {
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(uint8_t src) {
  roundup(sizeof(uint8_t));
  if constexpr (SERIALIZE) {
    if (offset_ + sizeof(uint8_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    *(reinterpret_cast<uint8_t *>(buf_ + offset_)) = src;
  }

  advance(sizeof(uint8_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(uint16_t src) {
  roundup(sizeof(uint16_t));
  if constexpr (SERIALIZE) {
    if (offset_ + sizeof(uint16_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    *(reinterpret_cast<uint16_t *>(buf_ + offset_)) = src;
  }

  advance(sizeof(uint16_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(uint32_t src) {
  roundup(sizeof(uint32_t));
  if constexpr (SERIALIZE) {
    if (offset_ + sizeof(uint32_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    *(reinterpret_cast<uint32_t *>(buf_ + offset_)) = src;
  }

  advance(sizeof(uint32_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(uint64_t src) {
  roundup(sizeof(uint64_t));
  if constexpr (SERIALIZE) {
    if (offset_ + sizeof(uint64_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    *(reinterpret_cast<uint64_t *>(buf_ + offset_)) = src;
  }

  advance(sizeof(uint64_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(const std::string & src)
{
  *this << static_cast<uint32_t>(src.size() + 1);
  roundup(sizeof(char));  // align of char
  if constexpr (SERIALIZE) {
    if (offset_ + src.size() + 1 > size_) {
      throw std::runtime_error("Out of buffer");
    }
    std::memcpy(buf_ + offset_, src.c_str(), src.size() + 1);
  }
  advance(src.size() + 1);
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(const std::u16string & src)
{
  *this << static_cast<uint32_t>(src.size());
  roundup(sizeof(char16_t));  // align of char16_t
  if constexpr (SERIALIZE) {
    if (offset_ + (src.size() * sizeof(char16_t)) > size_) {
      throw std::runtime_error("Out of buffer");
    }

    std::memcpy(buf_ + offset_, src.data(), sizeof(char16_t) * src.size());
  }
  advance(src.size() * sizeof(char16_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(const rosidl_runtime_c__String & src)
{
  *this << static_cast<uint32_t>(src.size + 1);
  roundup(sizeof(char));  // align of char
  if constexpr (SERIALIZE) {
    if (offset_ + src.size + 1 > size_) {
      throw std::runtime_error("Out of buffer");
    }
    std::memcpy(buf_ + offset_, src.data, src.size + 1);
  }
  advance(src.size + 1);
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::operator<<(const rosidl_runtime_c__U16String & src)
{
  *this << static_cast<uint32_t>(src.size);
  roundup(sizeof(char16_t));  // align of char16_t
  if constexpr (SERIALIZE) {
    if (offset_ + (src.size * sizeof(char16_t)) > size_) {
      throw std::runtime_error("Out of buffer");
    }

    std::memcpy(buf_ + offset_, src.data, sizeof(char16_t) * src.size);
  }
  advance(src.size * sizeof(char16_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::copy_arr(const uint8_t * arr, size_t cnt)
{
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint8_t));
  if constexpr (SERIALIZE) {
    if (offset_ + cnt * sizeof(uint8_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    std::memcpy(buf_ + offset_, arr, cnt * sizeof(uint8_t));
  }
  advance(cnt * sizeof(uint8_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::copy_arr(const uint16_t * arr, size_t cnt)
{
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint16_t));
  if constexpr (SERIALIZE) {
    if (offset_ + cnt * sizeof(uint16_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    std::memcpy(buf_ + offset_, arr, cnt * sizeof(uint16_t));
  }
  advance(cnt * sizeof(uint16_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::copy_arr(const uint32_t * arr, size_t cnt)
{
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint32_t));
  if constexpr (SERIALIZE) {
    if (offset_ + cnt * sizeof(uint32_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    std::memcpy(buf_ + offset_, arr, cnt * sizeof(uint32_t));
  }
  advance(cnt * sizeof(uint32_t));
}

template<bool SERIALIZE>
inline void CdrSerializationBuffer<SERIALIZE>::copy_arr(const uint64_t * arr, size_t cnt)
{
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint64_t));
  if constexpr (SERIALIZE) {
    if (offset_ + cnt * sizeof(uint64_t) > size_) {
      throw std::runtime_error("Out of buffer");
    }
    std::memcpy(buf_ + offset_, arr, cnt * sizeof(uint64_t));
  }
  advance(cnt * sizeof(uint64_t));
}
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__CDR_SERIALIZATION_BUFFER_INL_

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

#include "rmw_gurumdds_cpp/cdr_buffer.hpp"

namespace rmw_gurumdds_cpp
{
static uint16_t bswap16(uint16_t data)
{
  return (data >> 8) | (data << 8);
}

static uint32_t bswap32(uint32_t data)
{
  return (data >> 24) |
         ((data >> 8) & 0x0000ff00) |
         ((data << 8) & 0x00ff0000) |
         (data << 24);
}

static uint64_t bswap64(uint64_t data)
{
  return (data >> 56) |
         ((data >> 40) & 0x000000000000ff00ull) |
         ((data >> 24) & 0x0000000000ff0000ull) |
         ((data >> 8) & 0x00000000ff000000ull) |
         ((data << 8) & 0x000000ff00000000ull) |
         ((data << 24) & 0x0000ff0000000000ull) |
         ((data << 40) & 0x00ff000000000000ull) |
         (data << 56);
}

CdrDeserializationBuffer::CdrDeserializationBuffer(uint8_t * buf, size_t size)
  : CdrBuffer{buf, size} {
  if (size < CDR_HEADER_SIZE) {
    throw std::runtime_error("Insufficient buffer size");
  }
  swap_ = (buf[1] != CDR_SYSTEM_ENDIAN);
  buf_ = buf + CDR_HEADER_SIZE;
  size_ = size - CDR_HEADER_SIZE;
  offset_ = 0;
}

void CdrDeserializationBuffer::operator>>(uint8_t & dst) {
  roundup(sizeof(uint8_t));
  if (offset_ + sizeof(uint8_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }
  dst = *(reinterpret_cast<uint8_t *>(buf_ + offset_));
  advance(sizeof(uint8_t));
}

void CdrDeserializationBuffer::operator>>(uint16_t & dst) {
  roundup(sizeof(uint16_t));
  if (offset_ + sizeof(uint16_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }
  dst = *(reinterpret_cast<uint16_t *>(buf_ + offset_));
  if(swap_)
    dst = bswap16(dst);

  advance(sizeof(uint16_t));
}

void CdrDeserializationBuffer::operator>>(uint32_t & dst) {
  roundup(sizeof(uint32_t));
  if (offset_ + sizeof(uint32_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }
  dst = *(reinterpret_cast<uint32_t *>(buf_ + offset_));
  if(swap_)
    dst = bswap32(dst);

  advance(sizeof(uint32_t));
}

void CdrDeserializationBuffer::operator>>(uint64_t & dst) {
  roundup(sizeof(uint64_t));
  if (offset_ + sizeof(uint64_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }
  dst = *(reinterpret_cast<uint64_t *>(buf_ + offset_));
  if(swap_)
    dst = bswap64(dst);

  advance(sizeof(uint64_t));
}

void CdrDeserializationBuffer::operator>>(std::string & dst) {
    uint32_t str_size = 0;
    *this >> str_size;
    roundup(sizeof(char));  // align of char
    if (str_size == 0) {
      dst = std::string{};
      return;
    }
    if (offset_ + str_size > size_) {
      throw std::runtime_error("Out of buffer");
    }

    const char* str = reinterpret_cast<const char *>(buf_ + offset_);
    if (str[str_size - 1] != '\0') {
      throw std::runtime_error("String is not null terminated");
    }

    dst = std::string(str, str_size - 1);
    advance(str_size);
}

void CdrDeserializationBuffer::operator>>(std::u16string & dst) {
  uint32_t str_size = 0;
  *this >> str_size;
  roundup(sizeof(char16_t));  // align of wchar
  if (str_size == 0) {
    dst = std::u16string{};
    return;
  }
  if (offset_ + str_size * sizeof(char16_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }

  std::u16string temp(str_size, u'\0');
  const uint16_t* str = reinterpret_cast<const uint16_t *>(buf_ + offset_);
  for (uint32_t i = 0; i < str_size; i++) {
    auto ch = str[i];
    temp[i] = swap_ ? bswap16(ch) : ch;
  }

  dst = std::move(temp);
  advance(str_size * sizeof(char16_t));
}

void CdrDeserializationBuffer::operator>>(rosidl_runtime_c__String & dst) {
  uint32_t str_size = 0;
  *this >> str_size;
  roundup(sizeof(char));  // align of char
  if (str_size == 0) {
    dst.data[0] = '\0';
    dst.size = 0;
    dst.capacity = 1;
    return;
  }
  if (offset_ + str_size > size_) {
    throw std::runtime_error("Out of buffer");
  }

  rosidl_runtime_c__String__assignn(
    &dst,
    reinterpret_cast<const char *>(buf_ + offset_),
    str_size - 1
  );
  advance(str_size);
}

void CdrDeserializationBuffer::operator>>(rosidl_runtime_c__U16String & dst) {
  uint32_t str_size = 0;
  *this >> str_size;
  roundup(sizeof(char16_t));  // align of wchar
  if (str_size == 0) {
    dst.data[0] = u'\0';
    dst.size = 0;
    dst.capacity = 1;
    return;
  }
  if (offset_ + str_size * sizeof(char16_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }

  bool res = rosidl_runtime_c__U16String__resize(&dst, str_size);
  if (!res) {
    throw std::runtime_error("Failed to resize wstring");
  }

  const uint16_t* str = reinterpret_cast<const uint16_t *>(buf_ + offset_);
  for (uint32_t i = 0; i < str_size; i++) {
    auto ch = str[i];
    dst.data[i] = swap_ ? bswap16(ch) : ch;
  }

  advance(str_size * sizeof(char16_t));
}

void CdrDeserializationBuffer::copy_arr(uint8_t * arr, size_t cnt) {
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint8_t));
  if (offset_ + cnt > size_) {
    throw std::runtime_error("Out of buffer");
  }
  std::memcpy(arr, buf_ + offset_, cnt);
  advance(cnt);
}

void CdrDeserializationBuffer::copy_arr(uint16_t * arr, size_t cnt) {
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint16_t));
  if (offset_ + cnt * sizeof(uint16_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }

  uint16_t* src = reinterpret_cast<uint16_t*>(buf_ + offset_);
  if (swap_) {
    for (size_t i = 0; i < cnt; i++) {
      arr[i] = bswap16(src[i]);
    }
  } else {
    std::memcpy(arr, src, cnt * sizeof(uint16_t));
  }
  advance(cnt * sizeof(uint16_t));
}

void CdrDeserializationBuffer::copy_arr(uint32_t * arr, size_t cnt) {
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint32_t));
  if (offset_ + cnt * sizeof(uint32_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }

  uint32_t* src = reinterpret_cast<uint32_t*>(buf_ + offset_);
  if (swap_) {
    for (size_t i = 0; i < cnt; i++) {
      arr[i] = bswap32(src[i]);
    }
  } else {
    std::memcpy(arr, src, cnt * sizeof(uint32_t));
  }
  advance(cnt * sizeof(uint32_t));
}

void CdrDeserializationBuffer::copy_arr(uint64_t * arr, size_t cnt) {
  if (cnt == 0) {
    return;
  }

  roundup(sizeof(uint64_t));
  if (offset_ + cnt * sizeof(uint64_t) > size_) {
    throw std::runtime_error("Out of buffer");
  }

  uint64_t* src = reinterpret_cast<uint64_t*>(buf_ + offset_);
  if (swap_) {
    for (size_t i = 0; i < cnt; i++) {
      arr[i] = bswap32(src[i]);
    }
  } else {
    std::memcpy(arr, src, cnt * sizeof(uint64_t));
  }
  advance(cnt * sizeof(uint64_t));
}
}  // namespace rmw_gurumdds_cpp

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

#ifndef CDR_BUFFER_HPP_
#define CDR_BUFFER_HPP_

#include <cstring>
#include <string>
#include <stdexcept>

#include "rosidl_runtime_c/string_functions.h"
#include "rosidl_runtime_c/string.h"
#include "rosidl_runtime_c/u16string_functions.h"
#include "rosidl_runtime_c/u16string.h"

#define CDR_BIG_ENDIAN 0
#define CDR_LITTLE_ENDIAN 1

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define system_endian CDR_LITTLE_ENDIAN
#else
#define system_endian CDR_BIG_ENDIAN
#endif

#define CDR_HEADER_SIZE 4
#define CDR_HEADER_ENDIAN_IDX 1

class CDRBuffer
{
public:
  size_t get_offset()
  {
    return offset;
  }

  void roundup(uint32_t align_)
  {
    align(align_);
  }

protected:
  void align(size_t align_)
  {
    size_t cnt = align_ ? (-offset & (align_ - 1)) : 0;
    if (buf != nullptr && offset + cnt > size) {
      throw std::runtime_error("Out of buffer");
    }
    advance(cnt);
  }

  void advance(size_t cnt)
  {
    offset += cnt;
  }

  uint8_t * buf;
  size_t offset;
  size_t size;

  CDRBuffer() {}
};

class CDRSerializationBuffer : public CDRBuffer
{
public:
  CDRSerializationBuffer(uint8_t * a_buf, size_t a_size)
  {
    if (a_buf != nullptr) {
      if (a_size < CDR_HEADER_SIZE) {
        throw std::runtime_error("Insufficient buffer size");
      }
      memset(a_buf, 0, CDR_HEADER_SIZE);
      a_buf[CDR_HEADER_ENDIAN_IDX] = system_endian;
      buf = a_buf + CDR_HEADER_SIZE;
      size = a_size - CDR_HEADER_SIZE;
    } else {
      buf = nullptr;
      size = 0;
    }
    offset = 0;
  }

  void operator<<(uint8_t src)
  {
    align(1);
    if (buf != nullptr) {
      if (offset + 1 > size) {
        throw std::runtime_error("Out of buffer");
      }
      *(reinterpret_cast<uint8_t *>(buf + offset)) = src;
    }
    advance(1);
  }

  void operator<<(uint16_t src)
  {
    align(2);
    if (buf != nullptr) {
      if (offset + 2 > size) {
        throw std::runtime_error("Out of buffer");
      }
      *(reinterpret_cast<uint16_t *>(buf + offset)) = src;
    }
    advance(2);
  }

  void operator<<(uint32_t src)
  {
    align(4);
    if (buf != nullptr) {
      if (offset + 4 > size) {
        throw std::runtime_error("Out of buffer");
      }
      *(reinterpret_cast<uint32_t *>(buf + offset)) = src;
    }
    advance(4);
  }

  void operator<<(uint64_t src)
  {
    align(8);
    if (buf != nullptr) {
      if (offset + 8 > size) {
        throw std::runtime_error("Out of buffer");
      }
      *(reinterpret_cast<uint64_t *>(buf + offset)) = src;
    }
    advance(8);
  }

  void operator<<(const std::string & src)
  {
    *this << static_cast<uint32_t>(src.size() + 1);
    align(1);  // align of char
    if (buf != nullptr) {
      if (offset + src.size() + 1 > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(buf + offset, src.c_str(), src.size() + 1);
    }
    advance(src.size() + 1);
  }

  void operator<<(const std::u16string & src)
  {
    *this << static_cast<uint32_t>(src.size());
    align(2);  // align of wchar
    if (buf != nullptr) {
      if (offset + (src.size() * 2) > size) {
        throw std::runtime_error("Out of buffer");
      }
      auto dst = reinterpret_cast<uint16_t *>(buf + offset);
      for (uint32_t i = 0; i < src.size(); i++) {
        *(dst + i) = static_cast<uint16_t>(src[i]);
      }
    }
    advance(src.size() * 2);
  }

  void operator<<(const rosidl_runtime_c__String & src)
  {
    *this << static_cast<uint32_t>(src.size + 1);
    align(1);  // align of char
    if (buf != nullptr) {
      if (offset + src.size + 1 > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(buf + offset, src.data, src.size + 1);
    }
    advance(src.size + 1);
  }

  void operator<<(const rosidl_runtime_c__U16String & src)
  {
    *this << static_cast<uint32_t>(src.size);
    align(2);  // align of wchar
    if (buf != nullptr) {
      if (offset + src.size * 2 > size) {
        throw std::runtime_error("Out of buffer");
      }
      auto dst = reinterpret_cast<uint16_t *>(buf + offset);
      for (uint32_t i = 0; i < src.size; i++) {
        *(dst + i) = static_cast<uint16_t>(src.data[i]);
      }
    }
    advance(src.size * 2);
  }

  void copy_arr(const uint8_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(1);
    if (buf != nullptr) {
      if (offset + cnt > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(buf + offset, arr, cnt);
    }
    advance(cnt);
  }

  void copy_arr(const uint16_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(2);
    if (buf != nullptr) {
      if (offset + cnt * 2 > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(buf + offset, arr, cnt * 2);
    }
    advance(cnt * 2);
  }

  void copy_arr(const uint32_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(4);
    if (buf != nullptr) {
      if (offset + cnt * 4 > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(buf + offset, arr, cnt * 4);
    }
    advance(cnt * 4);
  }

  void copy_arr(const uint64_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(8);
    if (buf != nullptr) {
      if (offset + cnt * 8 > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(buf + offset, arr, cnt * 8);
    }
    advance(cnt * 8);
  }
};

// ================================================================================================

class CDRDeserializationBuffer : public CDRBuffer
{
public:
  CDRDeserializationBuffer(uint8_t * a_buf, size_t a_size)
  {
    if (a_size < CDR_HEADER_SIZE) {
      throw std::runtime_error("Insufficient buffer size");
    }
    swap = (a_buf[1] != system_endian);
    buf = a_buf + CDR_HEADER_SIZE;
    size = a_size - CDR_HEADER_SIZE;
    offset = 0;
  }

  void operator>>(uint8_t & dst)
  {
    align(1);
    if (offset + 1 > size) {
      throw std::runtime_error("Out of buffer");
    }
    dst = *(reinterpret_cast<uint8_t *>(buf + offset));
    advance(1);
  }

  void operator>>(uint16_t & dst)
  {
    align(2);
    if (offset + 2 > size) {
      throw std::runtime_error("Out of buffer");
    }
    auto data = *(reinterpret_cast<uint16_t *>(buf + offset));
    dst = swap ? bswap16(data) : data;
    advance(2);
  }

  void operator>>(uint32_t & dst)
  {
    align(4);
    if (offset + 4 > size) {
      throw std::runtime_error("Out of buffer");
    }
    auto data = *(reinterpret_cast<uint32_t *>(buf + offset));
    dst = swap ? bswap32(data) : data;
    advance(4);
  }

  void operator>>(uint64_t & dst)
  {
    align(8);
    if (offset + 8 > size) {
      throw std::runtime_error("Out of buffer");
    }
    auto data = *(reinterpret_cast<uint64_t *>(buf + offset));
    dst = swap ? bswap64(data) : data;
    advance(8);
  }

  void operator>>(std::string & dst)
  {
    uint32_t str_size = 0;
    *this >> str_size;
    align(1);  // align of char
    if (str_size == 0) {
      dst = std::string("");
      return;
    }
    if (offset + str_size > size) {
      throw std::runtime_error("Out of buffer");
    }
    if (*(reinterpret_cast<char *>(buf + offset) + (str_size - 1)) != '\0') {
      throw std::runtime_error("String is not null terminated");
    }
    dst = std::string(reinterpret_cast<char *>(buf + offset), str_size - 1);
    advance(str_size);
  }

  void operator>>(std::u16string & dst)
  {
    uint32_t str_size = 0;
    *this >> str_size;
    align(2);  // align of wchar
    if (str_size == 0) {
      dst = std::u16string(u"");
      return;
    }
    if (offset + str_size * 2 > size) {
      throw std::runtime_error("Out of buffer");
    }
    if (*(reinterpret_cast<uint16_t *>(buf + offset) + (str_size - 1)) != '\0') {
      throw std::runtime_error("Wstring is not null terminated");
    }
    dst.reserve(str_size + 1);
    for (uint32_t i = 0; i < str_size; i++) {
      auto data = *(reinterpret_cast<uint16_t *>(buf + offset) + i);
      data = swap ? bswap16(data) : data;
      dst.push_back(data);
    }
    advance(str_size * 2);
  }

  void operator>>(rosidl_runtime_c__String & dst)
  {
    uint32_t str_size = 0;
    *this >> str_size;
    align(1);  // align of char
    if (buf != nullptr) {
      if (str_size == 0) {
        dst.data[0] = '\0';
        dst.size = 0;
        dst.capacity = 1;
        return;
      }
      if (offset + str_size > size) {
        throw std::runtime_error("Out of buffer");
      }
      rosidl_runtime_c__String__assignn(
        &dst,
        reinterpret_cast<const char *>(buf + offset),
        str_size - 1
      );
    }
    advance(str_size);
  }

  void operator>>(rosidl_runtime_c__U16String & dst)
  {
    uint32_t str_size = 0;
    *this >> str_size;
    align(2);  // align of wchar
    if (buf != nullptr) {
      if (str_size == 0) {
        dst.data[0] = u'\0';
        dst.size = 0;
        dst.capacity = 1;
        return;
      }
      if (offset + str_size * 2 > size) {
        throw std::runtime_error("Out of buffer");
      }
      bool res = rosidl_runtime_c__U16String__resize(&dst, str_size + 1);
      if (!res) {
        throw std::runtime_error("Failed to resize wstring");
      }
      if (str_size >= 1) {
        for (uint32_t i = 0; i < str_size; i++) {
          auto data = *(reinterpret_cast<uint16_t *>(buf + offset) + i);
          data = swap ? bswap16(data) : data;
          dst.data[i] = static_cast<uint16_t>(data);
        }
      }
      dst.data[str_size] = u'\0';
    }
    advance(str_size * 2);
  }

  void copy_arr(uint8_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(1);
    if (buf != nullptr) {
      if (offset + cnt > size) {
        throw std::runtime_error("Out of buffer");
      }
      memcpy(arr, buf + offset, cnt);
    }
    advance(cnt);
  }

  void copy_arr(uint16_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(2);
    if (buf != nullptr) {
      if (offset + cnt * 2 > size) {
        throw std::runtime_error("Out of buffer");
      }
      if (swap) {
        for (size_t i = 0; i < cnt; i++) {
          arr[i] = bswap16(*(reinterpret_cast<uint16_t *>(buf + offset) + i));
        }
      } else {
        memcpy(arr, buf + offset, cnt * 2);
      }
    }
    advance(cnt * 2);
  }

  void copy_arr(uint32_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(4);
    if (buf != nullptr) {
      if (offset + cnt * 4 > size) {
        throw std::runtime_error("Out of buffer");
      }
      if (swap) {
        for (size_t i = 0; i < cnt; i++) {
          arr[i] = bswap32(*(reinterpret_cast<uint32_t *>(buf + offset) + i));
        }
      } else {
        memcpy(arr, buf + offset, cnt * 4);
      }
    }
    advance(cnt * 4);
  }

  void copy_arr(uint64_t * arr, size_t cnt)
  {
    if (cnt == 0) {
      return;
    }

    align(8);
    if (buf != nullptr) {
      if (offset + cnt * 8 > size) {
        throw std::runtime_error("Out of buffer");
      }
      if (swap) {
        for (size_t i = 0; i < cnt; i++) {
          arr[i] = bswap64(*(reinterpret_cast<uint64_t *>(buf + offset) + i));
        }
      } else {
        memcpy(arr, buf + offset, cnt * 8);
      }
    }
    advance(cnt * 8);
  }

private:
  bool swap;

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
};

#endif  // CDR_BUFFER_HPP_

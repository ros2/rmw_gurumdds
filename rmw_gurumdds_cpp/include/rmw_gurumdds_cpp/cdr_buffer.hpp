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

#ifndef RMW_GURUMDDS_CPP__CDR_BUFFER_HPP_
#define RMW_GURUMDDS_CPP__CDR_BUFFER_HPP_

#include <cstring>
#include <string>
#include <stdexcept>
#include <limits>

#include "rosidl_runtime_c/string.h"
#include "rosidl_runtime_c/string_functions.h"
#include "rosidl_runtime_c/u16string.h"
#include "rosidl_runtime_c/u16string_functions.h"

#define CDR_BIG_ENDIAN 0
#define CDR_LITTLE_ENDIAN 1

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define CDR_SYSTEM_ENDIAN CDR_LITTLE_ENDIAN
#else
#define CDR_SYSTEM_ENDIAN CDR_BIG_ENDIAN
#endif

#define CDR_HEADER_SIZE 4
#define CDR_HEADER_ENDIAN_IDX 1

namespace rmw_gurumdds_cpp
{
class CdrBuffer {
public:
  CdrBuffer(uint8_t * buf, size_t size);

  size_t get_offset() const;

  void roundup(uint32_t align);

protected:
  void advance(size_t cnt);

  uint8_t * buf_;
  size_t offset_;
  size_t size_;
};

template<bool SERIALIZE>
class CdrSerializationBuffer: public CdrBuffer {
public:
  CdrSerializationBuffer(uint8_t * buf, size_t size);

  void operator<<(uint8_t src);

  void operator<<(uint16_t src);

  void operator<<(uint32_t src);

  void operator<<(uint64_t src);

  void operator<<(const std::string & src);

  void operator<<(const std::u16string & src);

  void operator<<(const rosidl_runtime_c__String & src);

  void operator<<(const rosidl_runtime_c__U16String & src);

  void copy_arr(const uint8_t * arr, size_t cnt);

  void copy_arr(const uint16_t * arr, size_t cnt);

  void copy_arr(const uint32_t * arr, size_t cnt);

  void copy_arr(const uint64_t * arr, size_t cnt);
};

class CdrDeserializationBuffer: public CdrBuffer {
public:
  CdrDeserializationBuffer(uint8_t * buf, size_t size);

  void operator>>(uint8_t & dst);

  void operator>>(uint16_t & dst);

  void operator>>(uint32_t & dst);

  void operator>>(uint64_t & dst);

  void operator>>(std::string & dst);

  void operator>>(std::u16string & dst);

  void operator>>(rosidl_runtime_c__String & dst);

  void operator>>(rosidl_runtime_c__U16String & dst);

  void copy_arr(uint8_t * arr, size_t cnt);

  void copy_arr(uint16_t * arr, size_t cnt);

  void copy_arr(uint32_t * arr, size_t cnt);

  void copy_arr(uint64_t * arr, size_t cnt);

private:
  bool swap_;
};
}  // namespace rmw_gurumdds_cpp

#include "rmw_gurumdds_cpp/cdr_serialization_buffer.inl"

#endif  // RMW_GURUMDDS_CPP__CDR_BUFFER_HPP_

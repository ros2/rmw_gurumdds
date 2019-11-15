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

#ifndef RMW_GURUMDDS_SHARED_CPP__GUID_HPP_
#define RMW_GURUMDDS_SHARED_CPP__GUID_HPP_

#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "rmw_gurumdds_shared_cpp/dds_include.hpp"

typedef uint8_t octet;

/**
 * Structure to hold GUID information for DDS instances.
 * http://www.eprosima.com/docs/fast-rtps/1.6.0/html/_guid_8h_source.html
 *
 */

struct GuidPrefix_t
{
  static constexpr size_t kSize = 16;
  octet value[kSize];

  GuidPrefix_t()
  {
    memset(value, 0, kSize);
  }

  explicit GuidPrefix_t(octet guid[kSize])
  {
    memcpy(value, guid, kSize);
  }

  GuidPrefix_t(const GuidPrefix_t & g)
  {
    memcpy(value, g.value, kSize);
  }

  GuidPrefix_t(GuidPrefix_t && g)
  {
    memmove(value, g.value, kSize);
  }

  GuidPrefix_t & operator=(const GuidPrefix_t & guidpre)
  {
    memcpy(value, guidpre.value, kSize);
    return *this;
  }

  GuidPrefix_t & operator=(GuidPrefix_t && guidpre)
  {
    memmove(value, guidpre.value, kSize);
    return *this;
  }

#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

  bool operator==(const GuidPrefix_t & prefix) const
  {
    return memcmp(value, prefix.value, kSize) == 0;
  }

  bool operator!=(const GuidPrefix_t & prefix) const
  {
    return memcmp(value, prefix.value, kSize) != 0;
  }

#endif
};

inline bool operator<(const GuidPrefix_t & g1, const GuidPrefix_t & g2)
{
  for (uint8_t i = 0; i < GuidPrefix_t::kSize; ++i) {
    if (g1.value[i] < g2.value[i]) {
      return true;
    } else if (g1.value[i] > g2.value[i]) {
      return false;
    }
  }
  return false;
}

inline std::ostream & operator<<(std::ostream & output, const GuidPrefix_t & guiP)
{
  output << std::hex;
  for (uint8_t i = 0; i < GuidPrefix_t::kSize - 1; ++i) {
    output << static_cast<int>(guiP.value[i]) << ".";
  }
  output << static_cast<int>(guiP.value[GuidPrefix_t::kSize - 1]);
  return output << std::dec;
}

inline void dds_BuiltinTopicKey_to_GUID(
  struct GuidPrefix_t * guid,
  dds_BuiltinTopicKey_t btk)
{
  memset(guid->value, 0, GuidPrefix_t::kSize);
#if BIG_ENDIAN
  memcpy(guid->value, reinterpret_cast<octet *>(btk.value), GuidPrefix_t::kSize - 4);
#else
  octet const * keyBuffer = reinterpret_cast<octet *>(btk.value);
  for (uint8_t i = 0; i < 3; ++i) {
    octet * guidElement = &(guid->value[i * 4]);
    octet * const * keyBufferElement = keyBuffer + (i * 4);
    guidElement[0] = keyBufferElement[3];
    guidElement[1] = keyBufferElement[2];
    guidElement[2] = keyBufferElement[1];
    guidElement[3] = keyBufferElement[0];
  }
#endif
}

#endif  // RMW_GURUMDDS_SHARED_CPP__GUID_HPP_

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

#include <cassert>
#include "rmw_gurumdds_cpp/cdr_buffer.hpp"

namespace rmw_gurumdds_cpp
{
CdrBuffer::CdrBuffer(uint8_t * buf, size_t size)
  : buf_{buf}
  , offset_{}
  , size_{size} {};

size_t CdrBuffer::get_offset() const {
  return offset_;
}

void CdrBuffer::roundup(uint32_t align) {
  assert(align != 0);
  size_t count = -offset_ & (align - 1);
  if (offset_ + count > size_) {
    throw std::runtime_error("Out of buffer");
  }

  advance(count);
}

void CdrBuffer::advance(size_t cnt) {
  offset_ += cnt;
}
}  // namespace rmw_gurumdds_cpp

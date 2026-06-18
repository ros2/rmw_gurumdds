// Copyright 2026 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__UTILS_HPP_
#define RMW_GURUMDDS_CPP__UTILS_HPP_
#include "rmw/rmw.h"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw/impl/cpp/macros.hpp"

#include <type_traits>

namespace rmw_gurumdds_cpp
{

#define CHECK_PTRS ((ptrs != nullptr) && ...)

template<typename ... Ptrs> bool check_all_ptrs(Ptrs... ptrs)
{
  static_assert((std::is_pointer_v<Ptrs>&& ...));
  return CHECK_PTRS;
}

#undef CHECK_PTRS

#define CHECK_ALL_PTRS_NULL(...) \
  do { \
    if (!rmw_gurumdds_cpp::check_all_ptrs(__VA_ARGS__)) { \
      return nullptr; \
    } \
  } while (0)

#define CHECK_ALL_PTRS_CODE(...) \
  do { \
    if (!rmw_gurumdds_cpp::check_all_ptrs(__VA_ARGS__)) { \
      return RMW_RET_INVALID_ARGUMENT; \
    } \
  } while (0)

template<typename T> bool check_identifier(T *t, const char *name)
{
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    name, t->implementation_identifier, RMW_GURUMDDS_ID, return false);

  return true;
}

#define CHECK_ID_NULL(obj) \
  do{ \
    if(obj == nullptr) return nullptr; \
    RMW_CHECK_TYPE_IDENTIFIERS_MATCH( \
      obj, \
      obj->implementation_identifier, \
      RMW_GURUMDDS_ID, \
      return nullptr); \
  } while(0);

#define CHECK_ID_CODE(obj) \
  do{ \
    if(obj == nullptr) return RMW_RET_INVALID_ARGUMENT; \
    RMW_CHECK_TYPE_IDENTIFIERS_MATCH( \
      obj, \
      obj->implementation_identifier, \
      RMW_GURUMDDS_ID, \
      return RMW_RET_INCORRECT_RMW_IMPLEMENTATION); \
  } while(0);

} // namespace rmw_gurumdds_cpp

#endif

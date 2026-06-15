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

#ifndef RMW_GURUMDDS_CPP__ETC_HPP_
#define RMW_GURUMDDS_CPP__ETC_HPP_
#include <type_traits>
#include "rmw/rmw.h"
#include "rmw_gurumdds_cpp/identifier.hpp"

namespace rmw_gurumdds_cpp {

template <typename... Ptrs>
bool check_all_ptrs(Ptrs... ptrs){
    static_assert((std::is_pointer_v<Ptrs> && ...));
    return ((ptrs != nullptr) && ...);
}

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

//RMW_RMW_CHECK_TYPE_IDENTIFIERS_MATCH
template <typename T>
bool check_identifier(T* t, const char * name){
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    name,
    t->implementation_identifier,
    RMW_GURUMDDS_ID,
    return false);
  
  return true;
}

//obj가 nullptr이면 터짐.
//ID 체크 전에 반드시 인자 체크를 할 것.
#define CHECK_ID_NULL(obj) \
    RMW_CHECK_TYPE_IDENTIFIERS_MATCH( \
        obj,\
        obj->implementation_identifier,\
        RMW_GURUMDDS_ID,\
        return nullptr);

#define CHECK_ID_CODE(obj) \
    RMW_CHECK_TYPE_IDENTIFIERS_MATCH( \
        obj,\
        obj->implementation_identifier,\
        RMW_GURUMDDS_ID,\
        return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
        
}
#endif
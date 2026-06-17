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

/*
코드 정리용 utility 매크로, 함수 정의 모음
*/

namespace rmw_gurumdds_cpp
{

// ROS2 포맷터가 폴딩식 괄호를 자기 맘대로 삭제시켜서 매크로로 우회함.
#define CHECK_PTRS ((ptrs != nullptr) && ...)

//포인터를 가변 인자로 받아 nullptr인지 체크하는 헬퍼 함수.
template<typename ... Ptrs> bool check_all_ptrs(Ptrs... ptrs)
{
  static_assert((std::is_pointer_v<Ptrs>&& ...));
  return CHECK_PTRS;
}

#undef CHECK_PTRS

/*
함수 인자들을 가변 인자로 받아서 nullptr 체크를 수행하는 매크로

만약, 객체와 객체 내부의 멤버 변수를 모두 체크해야한다면 반드시 분리해서
비교해야함.

예시 :
CHECK_ALL_PTRS_NULL(foo, foo->bar);
ㄴ foo가 nullptr인 경우 null 참조로 런타임 에러 발생.

CHECK_ALL_PTRS_NULL(foo);
CHECK_ALL_PTRS_NULL(foo->bar);
ㄴ foo가 nullptr인 경우 맨 위 코드에서 반환하므로 안전함.
*/
// 반환값 : nullptr
#define CHECK_ALL_PTRS_NULL(...) \
  do { \
    if (!rmw_gurumdds_cpp::check_all_ptrs(__VA_ARGS__)) { \
      return nullptr; \
    } \
  } while (0)

// 반환값 : RMW_RET_INVALID_ARGUMENT
#define CHECK_ALL_PTRS_CODE(...) \
  do { \
    if (!rmw_gurumdds_cpp::check_all_ptrs(__VA_ARGS__)) { \
      return RMW_RET_INVALID_ARGUMENT; \
    } \
  } while (0)
// RMW_CHECK_TYPE_IDENTIFIERS_MATCH
template<typename T> bool check_identifier(T *t, const char *name)
{
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    name, t->implementation_identifier, RMW_GURUMDDS_ID, return false);

  return true;
}

/*
ID 체크를 수행하는 매크로

rmw_node_t, rmw_context_t, rmw_publisher_t, rmw_subscription_t, rmw_client_t,
rmw_service 등, rmw 내부 객체의 imple_mentation_identifier가 RMW_GURUMDDS_ID와
일치하는지 확인함.
*/
// 반환값 : nullptr
#define CHECK_ID_NULL(obj) \
  do{ \
    if(obj == nullptr) return nullptr; \
    RMW_CHECK_TYPE_IDENTIFIERS_MATCH( \
      obj, \
      obj->implementation_identifier, \
      RMW_GURUMDDS_ID, \
      return nullptr); \
  } while(0);

// 반환값 : RMW_RET_INCORRECT_RMW_IMPLEMENTATION
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

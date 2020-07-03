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

#ifndef TYPE_SUPPORT_COMMON_HPP_
#define TYPE_SUPPORT_COMMON_HPP_

#include <string>
#include <sstream>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"

#include "rmw/impl/cpp/macros.hpp"

#include "rosidl_typesupport_gurumdds_c/identifier.h"
#include "rosidl_typesupport_gurumdds_cpp/identifier.hpp"
#include "rosidl_typesupport_gurumdds_cpp/message_type_support.h"
#include "rosidl_typesupport_gurumdds_cpp/service_type_support.h"

#define RMW_GURUMDDS_STATIC_CPP_TYPESUPPORT_C rosidl_typesupport_gurumdds_c__identifier
#define RMW_GURUMDDS_STATIC_CPP_TYPESUPPORT_CPP rosidl_typesupport_gurumdds_cpp::typesupport_identifier

inline std::string
_create_type_name(const message_type_support_callbacks_t * callbacks)
{
  std::ostringstream ss;
  std::string message_namespace(callbacks->message_namespace);
  if (!message_namespace.empty()) {
    ss << message_namespace << "::";
  }
  ss << "dds_::" << callbacks->message_name << "_";
  return ss.str();
}

#endif  // TYPE_SUPPORT_COMMON_HPP_

// Copyright 2014-2017 Open Source Robotics Foundation, Inc.
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

#include "rosidl_typesupport_coredds_c/identifier.h"
#include "rosidl_typesupport_coredds_cpp/identifier.hpp"
#include "rosidl_typesupport_coredds_cpp/message_type_support.h"
#include "rosidl_typesupport_coredds_cpp/service_type_support.h"

#define RMW_COREDDS_CPP_TYPESUPPORT_C rosidl_typesupport_coredds_c__identifier
#define RMW_COREDDS_CPP_TYPESUPPORT_CPP rosidl_typesupport_coredds_cpp::typesupport_identifier

inline std::string
_create_type_name(
  const message_type_support_callbacks_t * callbacks,
  const std::string & sep)
{
  return std::string(callbacks->package_name) +
    "::" + sep + "::dds_::" + callbacks->message_name + "_";
}

#endif  // TYPE_SUPPORT_COMMON_HPP_

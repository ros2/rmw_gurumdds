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

#ifndef RMW_GURUMDDS_CPP__DEMANGLE_HPP_
#define RMW_GURUMDDS_CPP__DEMANGLE_HPP_

#include <string>

namespace rmw_gurumdds_cpp
{
std::string
demangle_if_ros_topic(const std::string & topic_name);

std::string
demangle_if_ros_type(const std::string & dds_type_string);

std::string
demangle_ros_topic_from_topic(const std::string & topic_name);

std::string
demangle_service_from_topic(const std::string & topic_name);

std::string
demangle_service_request_from_topic(const std::string & topic_name);

std::string
demangle_service_reply_from_topic(const std::string & topic_name);

std::string
demangle_service_type_only(const std::string & dds_type_name);

// Used when ros names are not mangled.
std::string
identity_demangle(const std::string & name);
}  // namespace rmw_gurumdds_cpp

using DemangleFunction = std::string (*)(const std::string &);
using MangleFunction = DemangleFunction;

#endif  // RMW_GURUMDDS_CPP__DEMANGLE_HPP_

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

#include <string>
#include <vector>

#include "rmw_gurumdds_cpp/namespace_prefix.hpp"

namespace rmw_gurumdds_cpp
{
const char * const ros_topic_prefix = "rt";
const char * const ros_service_requester_prefix = "rq";
const char * const ros_service_response_prefix = "rr";
const std::vector<std::string> ros_prefixes
  = {ros_topic_prefix, ros_service_requester_prefix, ros_service_response_prefix};

std::string
resolve_prefix(const std::string & name, const std::string & prefix)
{
  if (name.rfind(prefix + "/", 0) == 0) {
    return name.substr(prefix.length());
  }
  return "";
}

std::string
get_ros_prefix_if_exists(const std::string & topic_name)
{
  for (const auto & prefix : ros_prefixes) {
    if (topic_name.rfind(prefix, 0) == 0 && topic_name.at(prefix.length()) == '/') {
      return prefix;
    }
  }
  return "";
}

std::string
strip_ros_prefix_if_exists(const std::string & topic_name)
{
  for (const auto & prefix : ros_prefixes) {
    if (topic_name.rfind(prefix, 0) == 0 && topic_name.at(prefix.length()) == '/') {
      return topic_name.substr(prefix.length());
    }
  }
  return topic_name;
}

const std::vector<std::string> & get_all_ros_prefixes()
{
  return ros_prefixes;
}
}  // namespace rmw_gurumdds_cpp

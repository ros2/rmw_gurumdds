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

#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"

namespace rmw_gurumdds_cpp
{
std::string
demangle_if_ros_topic(const std::string & topic_name)
{
  return strip_ros_prefix_if_exists(topic_name);
}

std::string
demangle_if_ros_type(const std::string & dds_type_string)
{
  std::string substring = "dds_::";
  size_t substring_position = dds_type_string.find(substring);
  if (
    dds_type_string[dds_type_string.size() - 1] == '_' &&
    substring_position != std::string::npos)
  {
    std::string type_namespace = dds_type_string.substr(0, substring_position);
    type_namespace = std::regex_replace(type_namespace, std::regex("::"), "/");
    size_t start = substring_position + substring.size();
    std::string type_name = dds_type_string.substr(start, dds_type_string.length() - 1 - start);
    return type_namespace + type_name;
  }
  // not a ROS type
  return dds_type_string;
}

std::string
demangle_ros_topic_from_topic(const std::string & topic_name)
{
  return resolve_prefix(topic_name, ros_topic_prefix);
}

std::string
_demangle_service_from_topic(
  const std::string & prefix, const std::string & topic_name, std::string suffix)
{
  std::string service_name = resolve_prefix(topic_name, prefix);
  if (service_name.empty()) {
    return "";
  }

  size_t suffix_position = service_name.rfind(suffix);
  if (suffix_position != std::string::npos) {
    if (service_name.length() - suffix_position - suffix.length() != 0) {
      RCUTILS_LOG_WARN_NAMED(
        RMW_GURUMDDS_ID,
        "service topic has prefix and suffix,"
        "but not at the end : '%s'", topic_name.c_str());
      return "";
    }
  } else {
    RCUTILS_LOG_WARN_NAMED(
      RMW_GURUMDDS_ID,
      "service topic has prefix but no suffix: '%s'", topic_name.c_str());
    return "";
  }
  return service_name.substr(0, suffix_position);
}

std::string
demangle_service_from_topic(const std::string & topic_name)
{
  const std::string demangled_topic = demangle_service_reply_from_topic(topic_name);
  if (!demangled_topic.empty()) {
    return demangled_topic;
  }
  return demangle_service_request_from_topic(topic_name);
}

std::string
demangle_service_request_from_topic(const std::string & topic_name)
{
  return _demangle_service_from_topic(ros_service_requester_prefix, topic_name, "Request");
}

std::string
demangle_service_reply_from_topic(const std::string & topic_name)
{
  return _demangle_service_from_topic(ros_service_response_prefix, topic_name, "Reply");
}

std::string
demangle_service_type_only(const std::string & dds_type_name)
{
  std::string ns_substring = "dds_::";
  size_t ns_substring_position = dds_type_name.find(ns_substring);
  if (ns_substring_position == std::string::npos) {
    return "";
  }

  static const std::string suffixes[] = {
    std::string("_Response_"),
    std::string("_Request_"),
  };
  std::string found_suffix = "";
  size_t suffix_position = std::string::npos;
  for (const auto & suffix : suffixes) {
    suffix_position = dds_type_name.rfind(suffix);
    if (suffix_position != std::string::npos) {
      if (dds_type_name.length() - suffix_position - suffix.length() != 0) {
        RCUTILS_LOG_WARN_NAMED(
          RMW_GURUMDDS_ID,
          "service type contains 'dds_::' and a suffix, but not at the end: '%s'",
          dds_type_name.c_str());
        continue;
      }
      found_suffix = suffix;
      break;
    }
  }

  if (suffix_position == std::string::npos) {
    RCUTILS_LOG_WARN_NAMED(
      RMW_GURUMDDS_ID,
      "service type contains 'dds_::' but does not have a suffix: '%s'",
      dds_type_name.c_str());
    return "";
  }

  // everything checks out, reformat it from '<pkg>::srv::dds_::<type><suffix>'
  // to '<namespace>/<type>'
  std::string type_namespace = dds_type_name.substr(0, ns_substring_position);
  type_namespace = std::regex_replace(type_namespace, std::regex("::"), "/");
  size_t start = ns_substring_position + ns_substring.length();
  std::string type_name = dds_type_name.substr(start, suffix_position - start);
  return type_namespace + type_name;
}

std::string
identity_demangle(const std::string & name)
{
  return name;
}
}  // namespace rmw_gurumdds_cpp

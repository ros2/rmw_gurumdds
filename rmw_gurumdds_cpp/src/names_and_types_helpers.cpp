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

#include <map>
#include <set>
#include <string>

#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/demangle.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"

constexpr char SAMPLE_PREFIX[] = "/Sample_";

rmw_ret_t
copy_topics_names_and_types(
  const std::map<std::string, std::set<std::string>> & topics,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  if (topics.size() > 0) {
    rmw_ret_t rmw_ret = rmw_names_and_types_init(topic_names_and_types, topics.size(), allocator);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto fail_cleanup = [&topic_names_and_types]() {
        rmw_ret_t rmw_ret = rmw_names_and_types_fini(topic_names_and_types);
        if (rmw_ret != RMW_RET_OK) {
          RCUTILS_LOG_ERROR("error during report of error: %s", rmw_get_error_string().str);
        }
      };

    auto demangle_topic = _demangle_if_ros_topic;
    auto demangle_type = _demangle_if_ros_type;
    if (no_demangle) {
      auto noop = [](const std::string & in) {
          return in;
        };
      demangle_topic = noop;
      demangle_type = noop;
    }

    size_t index = 0;
    for (const auto & topic_n_types : topics) {
      char * topic_name = rcutils_strdup(demangle_topic(topic_n_types.first).c_str(), *allocator);
      if (topic_name == nullptr) {
        RMW_SET_ERROR_MSG("failed to allocate memory for topic name");
        fail_cleanup();
        return RMW_RET_BAD_ALLOC;
      }
      topic_names_and_types->names.data[index] = topic_name;

      {
        rcutils_ret_t rcutils_ret = rcutils_string_array_init(
          &topic_names_and_types->types[index],
          topic_n_types.second.size(),
          allocator);
        if (rcutils_ret != RCUTILS_RET_OK) {
          RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
      }

      size_t type_index = 0;
      for (const auto & type : topic_n_types.second) {
        char * type_name = rcutils_strdup(demangle_type(type).c_str(), *allocator);
        if (type_name == nullptr) {
          RMW_SET_ERROR_MSG("failed to allocate memory for type name");
          fail_cleanup();
          return RMW_RET_BAD_ALLOC;
        }
        topic_names_and_types->types[index].data[type_index] = type_name;
        ++type_index;
      }
      ++index;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
copy_services_to_names_and_types(
  const std::map<std::string, std::set<std::string>> & services,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  if (services.size() > 0) {
    rmw_ret_t rmw_ret =
      rmw_names_and_types_init(service_names_and_types, services.size(), allocator);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }

    auto fail_cleanup = [&service_names_and_types]() {
        rmw_ret_t rmw_ret = rmw_names_and_types_fini(service_names_and_types);
        if (rmw_ret != RMW_RET_OK) {
          RCUTILS_LOG_ERROR("error during report of error: %s", rmw_get_error_string().str);
        }
      };

    size_t index = 0;
    for (const auto & service_n_types : services) {
      char * service_name = rcutils_strdup(service_n_types.first.c_str(), *allocator);
      if (service_name == nullptr) {
        RMW_SET_ERROR_MSG("failed to allocate memory for service name");
        fail_cleanup();
        return RMW_RET_BAD_ALLOC;
      }
      service_names_and_types->names.data[index] = service_name;

      {
        rcutils_ret_t rcutils_ret = rcutils_string_array_init(
          &service_names_and_types->types[index],
          service_n_types.second.size(),
          allocator);
        if (rcutils_ret != RCUTILS_RET_OK) {
          RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
      }

      size_t type_index = 0;
      for (const auto & type : service_n_types.second) {
        size_t n = type.find(SAMPLE_PREFIX);
        std::string stripped_type = type;
        if (n != std::string::npos) {
          stripped_type = type.substr(0, n + 1) + type.substr(n + strlen(SAMPLE_PREFIX));
        }

        char * type_name = rcutils_strdup(stripped_type.c_str(), *allocator);
        if (type_name == nullptr) {
          RMW_SET_ERROR_MSG("failed to allocate memory for type name");
          fail_cleanup();
          return RMW_RET_BAD_ALLOC;
        }

        service_names_and_types->types[index].data[type_index] = type_name;
        ++type_index;
      }
      ++index;
    }
  }

  return RMW_RET_OK;
}

std::string
create_topic_name(
  const char * prefix,
  const char * topic_name,
  const char * suffix,
  bool avoid_ros_namespace_conventions)
{
  if (avoid_ros_namespace_conventions) {
    return std::string(topic_name) + std::string(suffix);
  } else {
    return std::string(prefix) +
           std::string(topic_name) +
           std::string(suffix);
  }
}

std::string
create_topic_name(
  const char * prefix,
  const char * topic_name,
  const char * suffix,
  const rmw_qos_profile_t * qos_policies)
{
  return create_topic_name(
    prefix,
    topic_name,
    suffix,
    qos_policies->avoid_ros_namespace_conventions);
}

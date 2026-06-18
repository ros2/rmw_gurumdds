#ifndef RMW_GURUMDDS_CPP__FASTRTPS_HPP_
#define RMW_GURUMDDS_CPP__FASTRTPS_HPP_

#include <sstream>
#include <string>

#include "fastcdr/cdr/fixed_size_string.hpp"
#include "rcpputils/find_and_replace.hpp"
#include "rmw/error_handling.h"

#include "rcutils/logging_macros.h"
#include "rcutils/types.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rosidl_typesupport_fastrtps_cpp/service_type_support.h"

#define RMW_FASTRTPS_CPP_TYPESUPPORT_C rosidl_typesupport_fastrtps_c__identifier
#define RMW_FASTRTPS_CPP_TYPESUPPORT_CPP \
  rosidl_typesupport_fastrtps_cpp::typesupport_identifier

// 헬퍼 함수 모음
namespace rmw_gurumdds_cpp
{
using DemangleFunction = std::string (*)(const std::string &);
using MangleFunction = DemangleFunction;

inline eprosima::fastcdr::string_255
_mangle_topic_name(
  const char *prefix, const char *base,
  const char *suffix = nullptr)
{
  std::ostringstream topicName;
  if (prefix) {
    topicName << prefix;
  }
  topicName << base;
  if (suffix) {
    topicName << suffix;
  }
  return topicName.str();
}

inline std::string
_demangle_service_type_only(const std::string & dds_type_name)
{
  std::string ns_substring = "dds_::";
  size_t ns_substring_position = dds_type_name.find(ns_substring);
  if (std::string::npos == ns_substring_position) {
    // not a ROS service type
    return "";
  }
  auto suffixes = {
    std::string("_Response_"),
    std::string("_Request_"),
  };
  std::string found_suffix = "";
  size_t suffix_position = 0;
  for (auto suffix : suffixes) {
    suffix_position = dds_type_name.rfind(suffix);
    if (suffix_position != std::string::npos) {
      if (dds_type_name.length() - suffix_position - suffix.length() != 0) {
        RCUTILS_LOG_WARN_NAMED(
            "rmw_gurumdds_cpp",
            "service type contains 'dds_::' and a suffix, but not at the end"
            ", report this: '%s'",
            dds_type_name.c_str());
        continue;
      }
      found_suffix = suffix;
      break;
    }
  }
  if (std::string::npos == suffix_position) {
    RCUTILS_LOG_WARN_NAMED(
        "rmw_gurumdds_cpp",
        "service type contains 'dds_::' but does not have a suffix"
        ", report this: '%s'",
        dds_type_name.c_str());
    return "";
  }
  // everything checks out, reformat it from
  // '[type_namespace::]dds_::<type><suffix>' to '[type_namespace/]<type>'
  std::string type_namespace = dds_type_name.substr(0, ns_substring_position);
  type_namespace = rcpputils::find_and_replace(type_namespace, "::", "/");
  size_t start = ns_substring_position + ns_substring.length();
  std::string type_name = dds_type_name.substr(start, suffix_position - start);
  return type_namespace + type_name;
}

inline std::string _create_type_name(
  std::string message_namespace,
  std::string message_name)
{
  std::ostringstream ss;
  if (!message_namespace.empty()) {
    // Find and replace C namespace separator with C++, in case this is using C
    // typesupport
    std::string message_namespace_new =
      rcpputils::find_and_replace(message_namespace, "__", "::");
    ss << message_namespace_new << "::";
  }
  ss << "dds_::" << message_name << "_";
  return ss.str();
}

inline std::string
_create_type_name(const message_type_support_callbacks_t *members)
{
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return "";
  }
  std::string message_namespace(members->message_namespace_);
  std::string message_name(members->message_name_);
  return _create_type_name(message_namespace, message_name);
}

inline eprosima::fastcdr::string_255
_create_topic_name(
  const rmw_qos_profile_t *qos_profile, const char *prefix,
  const char *base, const char *suffix = nullptr)
{
  assert(qos_profile);
  assert(base);
  if (qos_profile->avoid_ros_namespace_conventions) {
    prefix = nullptr;
  }
  return rmw_gurumdds_cpp::_mangle_topic_name(prefix, base, suffix);
}
} // namespace rmw_gurumdds_cpp

#endif // RMW_GURUMDDS_CPP__FASTRTPS_HPP_

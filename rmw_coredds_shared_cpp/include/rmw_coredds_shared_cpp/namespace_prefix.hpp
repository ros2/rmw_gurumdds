#ifndef RMW_COREDDS_SHARED_CPP__NAMESPACE_PREFIX_HPP_
#define RMW_COREDDS_SHARED_CPP__NAMESPACE_PREFIX_HPP_

#include <string>
#include <vector>

#include "rmw_coredds_shared_cpp/visibility_control.h"

RMW_COREDDS_SHARED_CPP_PUBLIC extern const char * const ros_topic_prefix;
RMW_COREDDS_SHARED_CPP_PUBLIC extern const char * const ros_service_requester_prefix;
RMW_COREDDS_SHARED_CPP_PUBLIC extern const char * const ros_service_response_prefix;

RMW_COREDDS_SHARED_CPP_PUBLIC extern std::vector<std::string> _ros_prefixes;

/// Return the ROS specific prefix if it exists, otherwise "".
std::string _get_ros_prefix_if_exists(const std::string & topic_name);

/// Returns the topic name stripped of and ROS specific prefix if exists.
std::string _strip_ros_prefix_if_exists(const std::string & topic_name);

/// Returns the list of ros prefixes
const std::vector<std::string> & _get_all_ros_prefixes();

#endif  // RMW_COREDDS_SHARED_CPP__NAMESPACE_PREFIX_HPP_

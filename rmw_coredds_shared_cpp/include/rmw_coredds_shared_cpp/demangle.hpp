#ifndef RMW_COREDDS_SHARED_CPP__DEMANGLE_HPP_
#define RMW_COREDDS_SHARED_CPP__DEMANGLE_HPP_

#include <string>

std::string
_demangle_if_ros_topic(const std::string & topic_name);

std::string
_demangle_if_ros_type(const std::string & dds_type_string);

std::string
_demangle_service_from_topic(const std::string & topic_name);

std::string
_demangle_service_type_only(const std::string & dds_type_name);

#endif  // RMW_COREDDS_SHARED_CPP__DEMANGLE_HPP_

#include <string>
#include <vector>

#include "rmw_coredds_shared_cpp/namespace_prefix.hpp"

const char * const ros_topic_prefix = "rt";
const char * const ros_service_requester_prefix = "rq";
const char * const ros_service_response_prefix = "rr";

std::vector<std::string> _ros_prefixes =
{ros_topic_prefix, ros_service_requester_prefix, ros_service_response_prefix};

std::string _get_ros_prefix_if_exists(const std::string & topic_name)
{
  for (const auto & prefix : _ros_prefixes) {
    if (topic_name.rfind(prefix, 0) == 0 && topic_name.at(prefix.length()) == '/') {
      return prefix;
    }
  }
  return "";
}

std::string _strip_ros_prefix_if_exists(const std::string & topic_name)
{
  for (const auto & prefix : _ros_prefixes) {
    if (topic_name.rfind(prefix, 0) == 0 && topic_name.at(prefix.length()) == '/') {
      return topic_name.substr(prefix.length());
    }
  }
  return topic_name;
}

const std::vector<std::string> & _get_all_ros_prefixes()
{
  return _ros_prefixes;
}

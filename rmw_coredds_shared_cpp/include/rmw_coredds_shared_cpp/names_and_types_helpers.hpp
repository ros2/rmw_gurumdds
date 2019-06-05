#ifndef RMW_COREDDS_SHARED_CPP__NAMES_AND_TYPES_HELPERS_HPP_
#define RMW_COREDDS_SHARED_CPP__NAMES_AND_TYPES_HELPERS_HPP_

#include <map>
#include <set>
#include <string>

#include "rcutils/allocator.h"

#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

rmw_ret_t
copy_services_to_names_and_types(
  const std::map<std::string, std::set<std::string>> & services,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types);

rmw_ret_t
copy_topics_names_and_types(
  const std::map<std::string, std::set<std::string>> & topics,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

#endif  // RMW_COREDDS_SHARED_CPP__NAMES_AND_TYPES_HELPERS_HPP_

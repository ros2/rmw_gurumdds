#include <map>
#include <set>
#include <string>

#include "rcutils/allocator.h"

#include "rmw/allocators.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/rmw.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  return shared__rmw_get_topic_names_and_types(
    gurum_coredds_identifier, node, allocator, no_demangle, topic_names_and_types);
}
}  // extern "C"

#include <map>
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  return shared__rmw_count_publishers(gurum_coredds_identifier, node, topic_name, count);
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  return shared__rmw_count_subscribers(gurum_coredds_identifier, node, topic_name, count);
}
}  // extern "C"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_get_subscriber_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  return shared__rmw_get_subscriber_names_and_types_by_node(
    gurum_coredds_identifier, node, allocator, node_name, node_namespace,
    no_demangle, topic_names_and_types);
}

rmw_ret_t
rmw_get_publisher_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  return shared__rmw_get_publisher_names_and_types_by_node(
    gurum_coredds_identifier, node, allocator, node_name, node_namespace,
    no_demangle, topic_names_and_types);
}

rmw_ret_t
rmw_get_service_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  return shared__rmw_get_service_names_and_types_by_node(
    gurum_coredds_identifier, node, allocator, node_name, node_namespace,
    service_names_and_types);
}
}  // extern "C"

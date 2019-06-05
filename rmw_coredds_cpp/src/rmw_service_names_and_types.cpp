#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_get_service_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  return shared__rmw_get_service_names_and_types(
    gurum_coredds_identifier, node, allocator, service_names_and_types);
}
}  // extern "C"

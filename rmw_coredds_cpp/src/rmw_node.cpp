#include <array>
#include <utility>
#include <set>
#include <string>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"
#include "rcutils/allocator.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/sanity_checks.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_node_t *
rmw_create_node(
  rmw_context_t * context,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_node_security_options_t * security_options)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    gurum_coredds_identifier,
    return nullptr);
  return shared__rmw_create_node(
    gurum_coredds_identifier, name, namespace_, domain_id, security_options);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  return shared__rmw_destroy_node(gurum_coredds_identifier, node);
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  return shared__rmw_node_get_graph_guard_condition(node);
}

rmw_ret_t
rmw_get_node_names(
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  return shared__rmw_get_node_names(gurum_coredds_identifier, node, node_names, node_namespaces);
}
}  // extern "C"

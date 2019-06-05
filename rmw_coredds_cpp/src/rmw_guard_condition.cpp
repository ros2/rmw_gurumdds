#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_guard_condition_t *
rmw_create_guard_condition(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    gurum_coredds_identifier,
    return nullptr);
  return shared__rmw_create_guard_condition(gurum_coredds_identifier);
}

rmw_ret_t
rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  return shared__rmw_destroy_guard_condition(gurum_coredds_identifier, guard_condition);
}

rmw_ret_t
rmw_trigger_guard_condition(const rmw_guard_condition_t * guard_condition)
{
  return shared__rmw_trigger_guard_condition(gurum_coredds_identifier, guard_condition);
}
}  // extern "C"

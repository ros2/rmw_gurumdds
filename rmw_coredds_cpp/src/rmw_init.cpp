#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rcutils/logging_macros.h"

#include "rmw_coredds_shared_cpp/dds_include.hpp"
#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_init_options_init(rmw_init_options_t * init_options, rcutils_allocator_t allocator)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  if (init_options->implementation_identifier != nullptr) {
    RMW_SET_ERROR_MSG("expected zero-initialized init_options");
    return RMW_RET_INVALID_ARGUMENT;
  }
  init_options->instance_id = 0;
  init_options->implementation_identifier = gurum_coredds_identifier;
  init_options->allocator = allocator;
  init_options->impl = nullptr;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_init_options_copy(const rmw_init_options_t * src, rmw_init_options_t * dst)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(src, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(dst, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    src,
    src->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (dst->implementation_identifier != nullptr) {
    RMW_SET_ERROR_MSG("expected zero-initialized dst");
    return RMW_RET_INVALID_ARGUMENT;
  }
  *dst = *src;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_init_options_fini(rmw_init_options_t * init_options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR(&(init_options->allocator), return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init_options,
    init_options->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  *init_options = rmw_get_zero_initialized_init_options();
  return RMW_RET_OK;
}

rmw_ret_t
rmw_init(const rmw_init_options_t * options, rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    options,
    options->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  context->instance_id = options->instance_id;
  context->implementation_identifier = gurum_coredds_identifier;
  context->impl = nullptr;

  dds_DomainParticipantFactory * dpf = dds_DomainParticipantFactory_get_instance();
  if (dpf == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
  }

  RCUTILS_LOG_INFO_NAMED("rmw_coredds_cpp", "RMW context successfully initialized");

  return RMW_RET_OK;
}

rmw_ret_t
rmw_shutdown(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  // Nothing to do here for now.
  // This is just the middleware's notification that shutdown was called.
  return RMW_RET_OK;
}

rmw_ret_t
rmw_context_fini(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  // context impl is explicitly supposed to be nullptr for now, see rmw_init's code
  // RCUTILS_CHECK_ARGUMENT_FOR_NULL(context->impl, RMW_RET_INVALID_ARGUMENT);
  *context = rmw_get_zero_initialized_context();
  return RMW_RET_OK;
}
}  // extern "C"

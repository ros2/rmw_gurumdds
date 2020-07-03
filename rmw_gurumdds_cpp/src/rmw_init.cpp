// Copyright 2019 GurumNetworks, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"

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
  init_options->implementation_identifier = gurum_gurumdds_identifier;
  init_options->domain_id = RMW_DEFAULT_DOMAIN_ID;
  init_options->security_options = rmw_get_zero_initialized_security_options();
  init_options->localhost_only = RMW_LOCALHOST_ONLY_DEFAULT;
  init_options->enclave = nullptr;
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
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (dst->implementation_identifier != nullptr) {
    RMW_SET_ERROR_MSG("expected zero-initialized dst");
    return RMW_RET_INVALID_ARGUMENT;
  }

  src->allocator.deallocate(dst->enclave, src->allocator.state);

  *dst = *src;
  dst->security_options = rmw_get_zero_initialized_security_options();
  dst->enclave = rcutils_strdup(src->enclave, src->allocator);
  if (dst->enclave == nullptr && src->enclave != nullptr) {
    return RMW_RET_BAD_ALLOC;
  }

  return rmw_security_options_copy(&src->security_options, &src->allocator, &dst->security_options);
}

rmw_ret_t
rmw_init_options_fini(rmw_init_options_t * init_options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR(&(init_options->allocator), return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init_options,
    init_options->implementation_identifier,
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  init_options->allocator.deallocate(init_options->enclave, init_options->allocator.state);
  rmw_security_options_fini(&init_options->security_options, &init_options->allocator);
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
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  context->instance_id = options->instance_id;
  context->implementation_identifier = gurum_gurumdds_identifier;
  context->impl = nullptr;

  rmw_ret_t ret = rmw_init_options_copy(options, &context->options);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  dds_DomainParticipantFactory * dpf = dds_DomainParticipantFactory_get_instance();
  if (dpf == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    return RMW_RET_ERROR;
  }

  const char * env_name = "RMW_GURUMDDS_INIT_LOG";
  char * env_value = nullptr;

  env_value = getenv(env_name);
  if (env_value != nullptr) {
    if (strcmp(env_value, "1") == 0) {
      RCUTILS_LOG_INFO_NAMED(
        "rmw_gurumdds_cpp",
        "RMW successfully initialized with GurumDDS");
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_shutdown(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    gurum_gurumdds_identifier,
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
    gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  *context = rmw_get_zero_initialized_context();
  return RMW_RET_OK;
}
}  // extern "C"

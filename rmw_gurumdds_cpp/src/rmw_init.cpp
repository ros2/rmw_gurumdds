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

#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/ret_types.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"

extern "C" {
rmw_ret_t rmw_init_options_init(
  rmw_init_options_t *init_options, rcutils_allocator_t allocator)
{
  CHECK_ALL_PTRS_CODE(init_options);
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  if (init_options->implementation_identifier != nullptr) {
    RMW_SET_ERROR_MSG("expected zero-initialized init_options");
    return RMW_RET_INVALID_ARGUMENT;
  }

  init_options->instance_id = 0;
  init_options->implementation_identifier = RMW_GURUMDDS_ID;
  init_options->domain_id = RMW_DEFAULT_DOMAIN_ID;
  init_options->security_options = rmw_get_zero_initialized_security_options();
  init_options->enclave = nullptr;
  init_options->allocator = allocator;
  init_options->impl = nullptr;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_init_options_copy(const rmw_init_options_t *src, rmw_init_options_t *dst)
{
  CHECK_ALL_PTRS_CODE(src, dst);
  CHECK_ALL_PTRS_CODE(src->implementation_identifier);
  CHECK_ID_CODE(src);
  RCUTILS_CHECK_ALLOCATOR(&src->allocator, return RMW_RET_INVALID_ARGUMENT);

  if (dst->implementation_identifier != nullptr) {
    RMW_SET_ERROR_MSG("destination init option is not zero-initialized");
    return RMW_RET_INVALID_ARGUMENT;
  }

  rcutils_allocator_t allocator = src->allocator;
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  rmw_init_options_t tmp = *src;
  rmw_ret_t ret;
  if (src->enclave != nullptr) {
    ret = rmw_enclave_options_copy(src->enclave, &allocator, &tmp.enclave);
    if (RMW_RET_OK != ret) {
      return ret;
    }
  }
  tmp.security_options = rmw_get_zero_initialized_security_options();
  ret = rmw_security_options_copy(
    &src->security_options, 
    &allocator, 
    &tmp.security_options);
  if (RMW_RET_OK != ret) {
    rmw_enclave_options_fini(tmp.enclave, &allocator);
    // Error already set
    return ret;
  }

  *dst = tmp;
  return RMW_RET_OK;
}

rmw_ret_t rmw_init_options_fini(rmw_init_options_t *init_options)
{
  CHECK_ALL_PTRS_CODE(init_options);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    init_options->implementation_identifier,
    "init option is not initialized",
    return RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(init_options);
  RCUTILS_CHECK_ALLOCATOR(
    &init_options->allocator, return RMW_RET_INVALID_ARGUMENT);

  init_options->allocator.deallocate(
    init_options->enclave, init_options->allocator.state);

  rmw_ret_t ret = rmw_security_options_fini(
    &init_options->security_options, &init_options->allocator);
  *init_options = rmw_get_zero_initialized_init_options();

  return ret;
}

rmw_ret_t rmw_init(const rmw_init_options_t *options, rmw_context_t *context)
{
  CHECK_ALL_PTRS_CODE(options, context);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    options->implementation_identifier,
    "init option is not initialized",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    options->enclave,
    "expected non-null enclave",
    return RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(options);

  if (context->implementation_identifier != nullptr) {
    RMW_SET_ERROR_MSG("context is not zero-initialized");
    return RMW_RET_INVALID_ARGUMENT;
  }

  rmw_ret_t ret = RMW_RET_OK;
  dds_DomainParticipantFactory *dpf = nullptr;
  const rmw_context_t zero_context = rmw_get_zero_initialized_context();
  const char *env_name = "RMW_GURUMDDS_INIT_LOG";
  char *env_value = nullptr;

  auto fail = [&]() {
      if (context->impl != nullptr) {
        context->impl->finalize();
        delete context->impl;
        context->impl = nullptr;
      }
      *context = zero_context;
      return ret;
    };

  const char *mapping_env = "RMW_GURUMDDS_REQUEST_REPLY_MAPPING";
  char *mapping_env_value = nullptr;
  bool service_mapping_basic = false;

  mapping_env_value = getenv(mapping_env);
  if (mapping_env_value != nullptr) {
    service_mapping_basic = (strcmp(mapping_env_value, "basic") == 0);
  }

  context->instance_id = options->instance_id;
  context->implementation_identifier = RMW_GURUMDDS_ID;
  context->actual_domain_id =
    RMW_DEFAULT_DOMAIN_ID != options->domain_id ? options->domain_id : 0u;
  context->impl = new (std::nothrow) rmw_context_impl_t(context);
  if (context->impl == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate rmw context impl");
    return fail();
  }
  context->impl->is_shutdown = false;
  context->impl->service_mapping_basic = service_mapping_basic;

  // backend_buffer
  context->impl->buffer_serialization_context = nullptr;
  context->impl->buffer_endpoint_registry = nullptr;

  ret = rmw_init_options_copy(options, &context->options);
  if (ret != RMW_RET_OK) {
    return fail();
  }

  dpf = dds_DomainParticipantFactory_get_instance();
  if (dpf == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    ret = rmw_init_options_fini(&context->options);
    if (ret != RMW_RET_OK) {
      RMW_SAFE_FWRITE_TO_STDERR("failed to fini rmw init options");
    }
    ret = RMW_RET_ERROR;
    return fail();
  }

  env_value = getenv(env_name);
  if (env_value != nullptr) {
    if (strcmp(env_value, "1") == 0) {
      RCUTILS_LOG_INFO_NAMED(
        RMW_GURUMDDS_ID, "RMW successfully initialized with GurumDDS");
    }
  }

  // backend_buffer
  auto buffer_context =
    new (std::nothrow) rmw_gurumdds_cpp::BufferBackendContext();
  if (nullptr == buffer_context) {
    RMW_SET_ERROR_MSG("failed to allocate buffer backend context");
    return RMW_RET_BAD_ALLOC;
  }
  context->impl->buffer_serialization_context = buffer_context;

  auto buffer_endpoint_registry =
    new (std::nothrow) rmw_gurumdds_cpp::BufferEndpointRegistry();
  if (nullptr == buffer_endpoint_registry) {
    delete buffer_context;
    context->impl->buffer_serialization_context = nullptr;
    RMW_SET_ERROR_MSG("failed to allocate buffer endpoint registry");
    return RMW_RET_BAD_ALLOC;
  }
  context->impl->buffer_endpoint_registry = buffer_endpoint_registry;

  try {
    rmw_gurumdds_cpp::initialize_buffer_backends(*buffer_context);
  } catch (const std::exception & e) {
    // Non-fatal: buffer backends are optional.
    RCUTILS_LOG_INFO_NAMED(
      "rmw_gurumdds_cpp", "Buffer backends not available: %s", e.what());
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_shutdown(rmw_context_t *context)
{
  CHECK_ALL_PTRS_CODE(context);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    context->impl,
    "context is not initialized",
    return RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(context);

  context->impl->is_shutdown = true;

  // backend_buffer
  auto *buffer_context = static_cast<rmw_gurumdds_cpp::BufferBackendContext *>(
    context->impl->buffer_serialization_context);
  if (buffer_context) {
    rmw_gurumdds_cpp::shutdown_buffer_backends(*buffer_context);
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_context_fini(rmw_context_t *context)
{
  CHECK_ALL_PTRS_CODE(context);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    context->impl,
    "context is not initialized",
    return RMW_RET_INVALID_ARGUMENT);
  CHECK_ID_CODE(context);

  if (!context->impl->is_shutdown) {
    RCUTILS_SET_ERROR_MSG("rmw context has not been shutdown");
    return RMW_RET_INVALID_ARGUMENT;
  }

  rmw_ret_t ret_exit = RMW_RET_OK;
  rmw_ret_t ret = context->impl->finalize();
  if (ret != RMW_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to finalize context impl");
    ret_exit = ret;
  }

  // backend_buffer
  auto *buffer_context = static_cast<rmw_gurumdds_cpp::BufferBackendContext *>(
    context->impl->buffer_serialization_context);
  if (buffer_context) {
    delete buffer_context;
    context->impl->buffer_serialization_context = nullptr;
  }

  auto *buffer_endpoint_registry =
    static_cast<rmw_gurumdds_cpp::BufferEndpointRegistry *>(
    context->impl->buffer_endpoint_registry);
  if (buffer_endpoint_registry) {
    delete buffer_endpoint_registry;
    context->impl->buffer_endpoint_registry = nullptr;
  }

  ret = rmw_init_options_fini(&context->options);
  if (ret != RMW_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      RMW_GURUMDDS_ID, "failed to finalize rmw context options");
    ret_exit = ret;
  }

  delete context->impl;
  *context = rmw_get_zero_initialized_context();

  dds_DomainParticipantFactory_shutdown();

  return ret_exit;
}
} // extern "C"

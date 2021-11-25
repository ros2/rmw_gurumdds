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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"

rmw_wait_set_t *
shared__rmw_create_wait_set(
  const char * implementation_identifier,
  rmw_context_t * context,
  size_t max_conditions)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    implementation_identifier,
    return nullptr);

  (void)max_conditions;
  rmw_wait_set_t * wait_set = rmw_wait_set_allocate();

  GurumddsWaitSetInfo * wait_set_info = nullptr;

  if (!wait_set) {
    RMW_SET_ERROR_MSG("failed to allocate wait set");
    goto fail;
  }

  wait_set->implementation_identifier = implementation_identifier;
  wait_set->data = rmw_allocate(sizeof(GurumddsWaitSetInfo));
  wait_set_info = static_cast<GurumddsWaitSetInfo *>(wait_set->data);

  if (!wait_set_info) {
    RMW_SET_ERROR_MSG("failed to allocate wait set");
    goto fail;
  }

  wait_set_info->wait_set = dds_WaitSet_create();
  if (wait_set_info->wait_set == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate wait set");
    goto fail;
  }

  wait_set_info->active_conditions = dds_ConditionSeq_create(4);
  if (wait_set_info->active_conditions == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate active_conditions sequence");
    goto fail;
  }

  wait_set_info->attached_conditions = dds_ConditionSeq_create(4);
  if (wait_set_info->attached_conditions == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate attached_conditions sequence");
    goto fail;
  }

  return wait_set;

fail:
  if (wait_set_info != nullptr) {
    if (wait_set_info->active_conditions != nullptr) {
      dds_ConditionSeq_delete(wait_set_info->active_conditions);
    }

    if (wait_set_info->attached_conditions != nullptr) {
      dds_ConditionSeq_delete(wait_set_info->attached_conditions);
    }

    if (wait_set_info->wait_set != nullptr) {
      dds_WaitSet_delete(wait_set_info->wait_set);
    }

    wait_set_info = nullptr;
  }

  if (wait_set != nullptr) {
    if (wait_set->data != nullptr) {
      rmw_free(wait_set->data);
    }

    rmw_wait_set_free(wait_set);
  }

  return nullptr;
}

rmw_ret_t
shared__rmw_destroy_wait_set(const char * implementation_identifier, rmw_wait_set_t * wait_set)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait_set handle, wait_set->implementation_identifier,
    implementation_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  GurumddsWaitSetInfo * wait_set_info = static_cast<GurumddsWaitSetInfo *>(wait_set->data);

  if (wait_set_info->active_conditions != nullptr) {
    dds_ConditionSeq_delete(wait_set_info->active_conditions);
  }

  if (wait_set_info->attached_conditions != nullptr) {
    dds_ConditionSeq_delete(wait_set_info->attached_conditions);
  }

  if (wait_set_info->wait_set != nullptr) {
    dds_WaitSet_delete(wait_set_info->wait_set);
  }

  wait_set_info = nullptr;

  if (wait_set->data != nullptr) {
    rmw_free(wait_set->data);
  }

  if (wait_set != nullptr) {
    rmw_wait_set_free(wait_set);
  }

  return RMW_RET_OK;
}

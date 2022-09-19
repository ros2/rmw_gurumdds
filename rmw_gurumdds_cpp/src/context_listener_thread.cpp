// Copyright 2022 GurumNetworks, Inc.
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

#include <atomic>
#include <cassert>
#include <cstring>
#include <thread>

#include "rcutils/macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/init.h"
#include "rmw/ret_types.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/gid_utils.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "rmw_gurumdds_cpp/context_listener_thread.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

static
dds_Condition *
rmw_attach_reader_to_waitset(
  dds_DataReader * const reader,
  dds_WaitSet * const waitset)
{
  dds_ReadCondition * const read_cond = dds_DataReader_create_readcondition(
    reader, dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  dds_Condition * const cond = reinterpret_cast<dds_Condition *>(read_cond);

  if (dds_RETCODE_OK != dds_WaitSet_attach_condition(waitset, cond)) {
    RMW_SET_ERROR_MSG("failed to attach read condition to waitset");
    return nullptr;
  }

  return cond;
}

static
void rmw_gurumdds_listener_thread(rmw_context_impl_t * ctx)
{
  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "[listener thread] starting up...");

  GurumddsSubscriberInfo * sub_partinfo =
    reinterpret_cast<GurumddsSubscriberInfo *>(ctx->common_ctx.sub->data);
  dds_ReturnCode_t ret = dds_RETCODE_ERROR;

  uint32_t active_len = 0;
  uint32_t attached_condition_count = 0;
  dds_Duration_t timeout = {dds_DURATION_INFINITE_SEC, dds_DURATION_INFINITE_NSEC};

  bool active = false;
  bool attached_exit = false;
  bool attached_partinfo = false;

  dds_Condition * cond_active = nullptr;
  dds_Condition * cond_part_info = nullptr;

  dds_GuardCondition * gcond_exit =
    reinterpret_cast<dds_GuardCondition *>(ctx->common_ctx.listener_thread_gc->data);

  GurumddsWaitSetInfo * waitset_info = new(std::nothrow) GurumddsWaitSetInfo();
  if (waitset_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate WaitSetInfo");
    goto cleanup;
  }

  waitset_info->wait_set = dds_WaitSet_create();
  if (waitset_info->wait_set == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate WaitSet");
    goto cleanup;
  }

  if (sub_partinfo->topic_reader != nullptr) {
    cond_part_info =
      rmw_attach_reader_to_waitset(sub_partinfo->topic_reader, waitset_info->wait_set);
    if (cond_part_info == nullptr) {
      goto cleanup;
    }
    attached_partinfo = true;
    attached_condition_count += 1;
  }

  if (RMW_RET_OK !=
    dds_WaitSet_attach_condition(
      waitset_info->wait_set,
      reinterpret_cast<dds_Condition *>(gcond_exit)))
  {
    RMW_SET_ERROR_MSG("failed to attach exit condition to listener thread waitset");
    goto cleanup;
  }
  attached_exit = true;
  attached_condition_count += 1;

  waitset_info->active_conditions = dds_ConditionSeq_create(attached_condition_count);
  if (waitset_info->active_conditions == nullptr) {
    RMW_SET_ERROR_MSG("failed to create condition sequence");
    goto cleanup;
  }

  active = ctx->common_ctx.thread_is_running.load();

  do {
    if (!active) {
      continue;
    }
    ret = dds_WaitSet_wait(waitset_info->wait_set, waitset_info->active_conditions, &timeout);

    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("wait failed for listener thread");
      goto cleanup;
    }

    active_len = dds_ConditionSeq_length(waitset_info->active_conditions);

    for (uint32_t i = 0; i < active_len && active; i++) {
      cond_active = dds_ConditionSeq_get(waitset_info->active_conditions, i);
      if (cond_active == reinterpret_cast<dds_Condition *>(gcond_exit)) {
        active = false;
        continue;
      }
    }

    for (uint32_t i = 0; i < active_len && active; i++) {
      cond_active = dds_ConditionSeq_get(waitset_info->active_conditions, i);
      if (nullptr != cond_part_info && cond_part_info == cond_active) {
        graph_on_participant_info(ctx);
      } else {
        RMW_SET_ERROR_MSG("unexpected active condition");
        goto cleanup;
      }
    }

    active = active && ctx->common_ctx.thread_is_running.load();
  } while (active);

cleanup:
  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "[listener thread] cleaning up...");
  if (waitset_info != nullptr) {
    if (waitset_info->active_conditions) {
      dds_ConditionSeq_delete(waitset_info->active_conditions);
    }
    if (waitset_info->wait_set != nullptr) {
      if (attached_exit) {
        if (dds_RETCODE_OK !=
          dds_WaitSet_detach_condition(
            waitset_info->wait_set,
            reinterpret_cast<dds_Condition *>(gcond_exit)))
        {
          RMW_SET_ERROR_MSG(
            "failed to detach graph condition from "
            "listener thread waitset");
          return;
        }
      }
      if (attached_partinfo) {
        if (dds_RETCODE_OK !=
          dds_WaitSet_detach_condition(
            waitset_info->wait_set,
            cond_part_info))
        {
          RMW_SET_ERROR_MSG(
            "failed to detach participant info condition from "
            "listener thread waitset");
          return;
        }
        dds_DataReader_delete_readcondition(
          sub_partinfo->topic_reader, reinterpret_cast<dds_ReadCondition *>(cond_part_info));
      }
      dds_WaitSet_delete(waitset_info->wait_set);
    }
    delete waitset_info;
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "[listener thread] done");
}

rmw_ret_t
run_listener_thread(rmw_context_t * ctx)
{
  rmw_dds_common::Context * const common_ctx = &ctx->impl->common_ctx;
  common_ctx->listener_thread_gc = rmw_create_guard_condition(ctx);

  if (common_ctx->listener_thread_gc == nullptr) {
    RMW_SET_ERROR_MSG("Failed to create listener thread guard condition");
    return RMW_RET_ERROR;
  }

  common_ctx->thread_is_running.store(true);

  try {
    common_ctx->listener_thread = std::thread(rmw_gurumdds_listener_thread, ctx->impl);
    RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "[listener thread] started");
    return RMW_RET_OK;
  } catch (const std::exception & exc) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to create std::thread: %s", exc.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("Failed to create std::thread");
  }

  // Handling when thread creation fails
  common_ctx->thread_is_running.store(false);
  if (rmw_destroy_guard_condition(common_ctx->listener_thread_gc) != RMW_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "Failed to destroy listener thread guard condition");
  }

  return RMW_RET_ERROR;
}


rmw_ret_t
stop_listener_thread(rmw_context_t * ctx)
{
  rmw_dds_common::Context * const common_ctx = &ctx->impl->common_ctx;

  if (common_ctx->thread_is_running.exchange(false)) {
    if (rmw_trigger_guard_condition(common_ctx->listener_thread_gc) != RMW_RET_OK) {
      return RMW_RET_ERROR;
    }

    try {
      common_ctx->listener_thread.join();
    } catch (const std::exception & exc) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to join std::thread: %s", exc.what());
      return RMW_RET_ERROR;
    } catch (...) {
      RMW_SET_ERROR_MSG("Failed to join std::thread");
      return RMW_RET_ERROR;
    }

    if (rmw_destroy_guard_condition(common_ctx->listener_thread_gc) != RMW_RET_OK) {
      return RMW_RET_ERROR;
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(RMW_GURUMDDS_ID, "[listener thread] stopped");
  return RMW_RET_OK;
}

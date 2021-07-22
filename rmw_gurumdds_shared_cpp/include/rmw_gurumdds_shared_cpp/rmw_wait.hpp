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

#ifndef RMW_GURUMDDS_SHARED_CPP__RMW_WAIT_HPP_
#define RMW_GURUMDDS_SHARED_CPP__RMW_WAIT_HPP_

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"
#include "rmw_gurumdds_shared_cpp/event_converter.hpp"

#define CHECK_ATTACH(ret) \
  if (ret == dds_RETCODE_OK) { \
    continue; \
  } else if (ret == dds_RETCODE_OUT_OF_RESOURCES) { \
    RMW_SET_ERROR_MSG("failed to attach condition to wait set: out of resources"); \
    return RMW_RET_ERROR; \
  } else if (ret == dds_RETCODE_BAD_PARAMETER) { \
    RMW_SET_ERROR_MSG("failed to attach condition to wait set: condition pointer was invalid"); \
    return RMW_RET_ERROR; \
  } else { \
    RMW_SET_ERROR_MSG("failed to attach condition to wait set"); \
    return RMW_RET_ERROR; \
  }

rmw_ret_t
__gather_event_conditions(
  rmw_events_t * events,
  std::unordered_set<dds_StatusCondition *> & status_conditions)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(events, RMW_RET_INVALID_ARGUMENT);
  std::unordered_map<dds_StatusCondition *, dds_StatusMask> status_map;

  for (size_t i = 0; i < events->event_count; i++) {
    auto now = static_cast<rmw_event_t *>(events->events[i]);
    RMW_CHECK_ARGUMENT_FOR_NULL(events, RMW_RET_INVALID_ARGUMENT);

    auto event_info = static_cast<GurumddsEventInfo *>(now->data);
    if (event_info == nullptr) {
      RMW_SET_ERROR_MSG("event handle is null");
      return RMW_RET_ERROR;
    }

    dds_StatusCondition * status_condition = event_info->get_statuscondition();
    if (status_condition == nullptr) {
      RMW_SET_ERROR_MSG("failed to get status condition");
      return RMW_RET_ERROR;
    }

    if (is_event_supported(now->event_type)) {
      auto map_pair = status_map.insert(std::make_pair(status_condition, 0));
      auto it = map_pair.first;
      status_map[status_condition] = get_status_kind_from_rmw(now->event_type) | it->second;
    } else {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("unsupported event: %d", now->event_type);
    }
  }

  for (auto & map_pair : status_map) {
    dds_StatusCondition_set_enabled_statuses(map_pair.first, map_pair.second);
    status_conditions.insert(map_pair.first);
  }

  return RMW_RET_OK;
}

rmw_ret_t
__handle_active_event_conditions(rmw_events_t * events)
{
  if (events == nullptr) {
    return RMW_RET_OK;
  }

  for (size_t i = 0; i < events->event_count; i++) {
    auto now = static_cast<rmw_event_t *>(events->events[i]);
    RMW_CHECK_ARGUMENT_FOR_NULL(events, RMW_RET_INVALID_ARGUMENT);

    auto event_info = static_cast<GurumddsEventInfo *>(now->data);
    if (event_info == nullptr) {
      RMW_SET_ERROR_MSG("event handle is null");
      return RMW_RET_ERROR;
    }

    dds_StatusMask mask = event_info->get_status_changes();
    bool is_active = false;

    if (is_event_supported(now->event_type)) {
      is_active = ((mask & get_status_kind_from_rmw(now->event_type)) != 0);
    }

    if (!is_active) {
      events->events[i] = nullptr;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t __detach_condition(
  dds_WaitSet * dds_wait_set,
  dds_Condition * condition)
{
  dds_ReturnCode_t dds_return_code = dds_WaitSet_detach_condition(dds_wait_set, condition);
  rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
  if (from_dds != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to detach condition from wait set");
    return from_dds;
  }

  return RMW_RET_OK;
}

template<typename SubscriberInfo, typename ServiceInfo, typename ClientInfo>
rmw_ret_t
shared__rmw_wait(
  const char * implementation_identifier,
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t * events,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
  (void)events;
  struct atexit_t
  {
    ~atexit_t()
    {
      if (wait_set == nullptr) {
        RMW_SET_ERROR_MSG("wait set handle is null");
        return;
      }

      RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
        wait set handle, wait_set->implementation_identifier,
        implementation_identifier, return )

      GurumddsWaitSetInfo * wait_set_info = static_cast<GurumddsWaitSetInfo *>(wait_set->data);
      if (wait_set_info == nullptr) {
        RMW_SET_ERROR_MSG("WaitSet implementation struct is null");
        return;
      }

      dds_WaitSet * dds_wait_set = static_cast<dds_WaitSet *>(wait_set_info->wait_set);
      if (dds_wait_set == nullptr) {
        RMW_SET_ERROR_MSG("DDS wait set handle is null");
        return;
      }

      dds_ConditionSeq * attached_conditions =
        static_cast<dds_ConditionSeq *>(wait_set_info->attached_conditions);
      if (attached_conditions == nullptr) {
        RMW_SET_ERROR_MSG("DDS condition sequence handle is null");
        return;
      }

      dds_ReturnCode_t ret = dds_WaitSet_get_conditions(dds_wait_set, attached_conditions);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to get attached conditions for wait set");
        return;
      }

      for (uint32_t i = 0; i < dds_ConditionSeq_length(attached_conditions); ++i) {
        ret = dds_WaitSet_detach_condition(
          dds_wait_set, dds_ConditionSeq_get(attached_conditions, i));
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to detach condition from wait set");
        }
      }

      while(dds_ConditionSeq_length(attached_conditions) > 0) {
        dds_ConditionSeq_remove(attached_conditions, 0);
      }
    }
    rmw_wait_set_t * wait_set = nullptr;
    const char * implementation_identifier = nullptr;
  } atexit;

  atexit.wait_set = wait_set;
  atexit.implementation_identifier = implementation_identifier;

  if (wait_set == nullptr) {
    RMW_SET_ERROR_MSG("wait set handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait set handle, wait_set->implementation_identifier,
    implementation_identifier, return RMW_RET_ERROR);

  GurumddsWaitSetInfo * wait_set_info = static_cast<GurumddsWaitSetInfo *>(wait_set->data);
  if (wait_set_info == nullptr) {
    RMW_SET_ERROR_MSG("WaitSet implementation struct is null");
    return RMW_RET_ERROR;
  }

  dds_WaitSet * dds_wait_set = static_cast<dds_WaitSet *>(wait_set_info->wait_set);
  if (dds_wait_set == nullptr) {
    RMW_SET_ERROR_MSG("DDS wait set handle is null");
    return RMW_RET_ERROR;
  }

  dds_ConditionSeq * active_conditions =
    static_cast<dds_ConditionSeq *>(wait_set_info->active_conditions);
  if (active_conditions == nullptr) {
    RMW_SET_ERROR_MSG("DDS condition sequence handle is null");
    return RMW_RET_ERROR;
  }

  if (subscriptions != nullptr) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      SubscriberInfo * subscriber_info =
        static_cast<SubscriberInfo *>(subscriptions->subscribers[i]);
      if (subscriber_info == nullptr) {
        RMW_SET_ERROR_MSG("subscriber info handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReadCondition * read_condition = subscriber_info->read_condition;
      if (read_condition == nullptr) {
        RMW_SET_ERROR_MSG("read condition handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReturnCode_t ret = dds_WaitSet_attach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(read_condition));
      CHECK_ATTACH(ret);
    }
  }

  std::unordered_set<dds_StatusCondition *> status_conditions;

  rmw_ret_t ret_code = __gather_event_conditions(events, status_conditions);
  if (ret_code != RMW_RET_OK) {
    return ret_code;
  }

  for (auto status_condition : status_conditions) {
    dds_ReturnCode_t ret = dds_WaitSet_attach_condition(
      dds_wait_set,
      reinterpret_cast<dds_Condition *>(status_condition));
    CHECK_ATTACH(ret);
  }

  if (guard_conditions != nullptr) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      dds_GuardCondition * guard_condition =
        static_cast<dds_GuardCondition *>(guard_conditions->guard_conditions[i]);
      if (guard_condition == nullptr) {
        RMW_SET_ERROR_MSG("guard condition handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReturnCode_t ret = dds_WaitSet_attach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(guard_condition));
      CHECK_ATTACH(ret);
    }
  }

  if (services != nullptr) {
    for (size_t i = 0; i < services->service_count; ++i) {
      ServiceInfo * service_info = static_cast<ServiceInfo *>(services->services[i]);
      if (service_info == nullptr) {
        RMW_SET_ERROR_MSG("service info handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReadCondition * read_condition = service_info->read_condition;
      if (read_condition == nullptr) {
        RMW_SET_ERROR_MSG("read condition handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReturnCode_t ret = dds_WaitSet_attach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(read_condition));
      CHECK_ATTACH(ret);
    }
  }

  if (clients != nullptr) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      ClientInfo * client_info = static_cast<ClientInfo *>(clients->clients[i]);
      if (client_info == nullptr) {
        RMW_SET_ERROR_MSG("client info handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReadCondition * read_condition = client_info->read_condition;
      if (read_condition == nullptr) {
        RMW_SET_ERROR_MSG("read condition handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReturnCode_t ret = dds_WaitSet_attach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(read_condition));
      CHECK_ATTACH(ret);
    }
  }

  rmw_ret_t rret = RMW_RET_OK;

  const char * env_name = "RMW_GURUMDDS_WAIT_USE_POLLING";
  char * env_value = nullptr;
  bool use_polling = false;

  env_value = getenv(env_name);
  if (env_value != nullptr) {
    use_polling = (strcmp(env_value, "1") == 0);
  }

  if (!use_polling) {  // Default: use dds_WaitSet_wait()
    dds_Duration_t timeout;
    if (wait_timeout == nullptr) {
      timeout.sec = dds_DURATION_INFINITE_SEC;
      timeout.nanosec = dds_DURATION_ZERO_NSEC;
    } else {
      timeout.sec = static_cast<int32_t>(wait_timeout->sec);
      timeout.nanosec = static_cast<uint32_t>(wait_timeout->nsec);
    }

    dds_ReturnCode_t status = dds_WaitSet_wait(dds_wait_set, active_conditions, &timeout);
    if (status != dds_RETCODE_OK && status != dds_RETCODE_TIMEOUT) {
      RMW_SET_ERROR_MSG("failed to wait on wait set");
      return RMW_RET_ERROR;
    }

    if (status == dds_RETCODE_TIMEOUT) {
      rret = RMW_RET_TIMEOUT;
    }
  } else {  // use polilng
    uint64_t sec, nsec;
    bool inf = false;
    if (wait_timeout != nullptr) {
      sec = wait_timeout->sec;
      nsec = wait_timeout->nsec;
      inf = false;
    } else {
      sec = 0;
      nsec = 0;
      inf = true;
    }
    auto t = std::chrono::steady_clock::now() +
      std::chrono::nanoseconds(sec * 1000000000ULL + nsec);
    bool triggered = false;

    while (dds_ConditionSeq_length(active_conditions) > 0) {
      dds_ConditionSeq_remove(active_conditions, 0);
    }

    dds_ConditionSeq * conds = dds_ConditionSeq_create(8);
    dds_WaitSet_get_conditions(dds_wait_set, conds);

    for (uint32_t i = 0; i < dds_ConditionSeq_length(conds); ++i) {
      dds_Condition * cond = dds_ConditionSeq_get(conds, i);
      if (cond == NULL) {
        continue;
      }

      if (dds_Condition_get_trigger_value(cond) == true) {
        dds_ConditionSeq_add(active_conditions, cond);
        triggered = true;
      }
    }

    for (uint32_t i = 0; (inf || std::chrono::steady_clock::now() <= t) && !triggered; ++i) {
      if (i >= dds_ConditionSeq_length(conds)) {
        i = 0;
      }

      dds_Condition * cond = dds_ConditionSeq_get(conds, i);
      if (cond == NULL) {
        continue;
      }

      if (dds_Condition_get_trigger_value(cond) == true) {
        dds_ConditionSeq_add(active_conditions, cond);
        triggered = true;
      }
    }
    dds_ConditionSeq_delete(conds);

    if (!triggered) {
      rret = RMW_RET_TIMEOUT;
    }
  }

  if (subscriptions != nullptr) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      SubscriberInfo * subscriber_info =
        static_cast<SubscriberInfo *>(subscriptions->subscribers[i]);
      if (subscriber_info == nullptr) {
        RMW_SET_ERROR_MSG("subscriber info handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReadCondition * read_condition = subscriber_info->read_condition;
      if (!read_condition) {
        RMW_SET_ERROR_MSG("read condition handle is null");
        return RMW_RET_ERROR;
      }

      uint32_t j = 0;
      for (; j < dds_ConditionSeq_length(active_conditions); ++j) {
        if (
          dds_ConditionSeq_get(active_conditions, j) ==
          reinterpret_cast<dds_Condition *>(read_condition))
        {
          break;
        }
      }

      if (j >= dds_ConditionSeq_length(active_conditions)) {
        subscriptions->subscribers[i] = 0;
      }

      rmw_ret_t rmw_ret_code = __detach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(read_condition));
      if (rmw_ret_code != RMW_RET_OK) {
        return rmw_ret_code;
      }
    }
  }

  if (guard_conditions != nullptr) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      dds_Condition * condition =
        static_cast<dds_Condition *>(guard_conditions->guard_conditions[i]);
      if (condition == nullptr) {
        RMW_SET_ERROR_MSG("condition handle is null");
        return RMW_RET_ERROR;
      }

      uint32_t j = 0;
      for (; j < dds_ConditionSeq_length(active_conditions); ++j) {
        if (dds_ConditionSeq_get(active_conditions, j) == condition) {
          dds_GuardCondition * guard = reinterpret_cast<dds_GuardCondition *>(condition);
          dds_ReturnCode_t ret = dds_GuardCondition_set_trigger_value(guard, false);
          if (ret != dds_RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to set trigger value");
            return RMW_RET_ERROR;
          }
          break;
        }
      }

      if (j >= dds_ConditionSeq_length(active_conditions)) {
        guard_conditions->guard_conditions[i] = 0;
      }

      rmw_ret_t rmw_ret_code = __detach_condition(dds_wait_set, condition);
      if (rmw_ret_code != RMW_RET_OK) {
        return rmw_ret_code;
      }
    }
  }

  if (services != nullptr) {
    for (size_t i = 0; i < services->service_count; ++i) {
      ServiceInfo * service_info = static_cast<ServiceInfo *>(services->services[i]);
      if (service_info == nullptr) {
        RMW_SET_ERROR_MSG("service info handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReadCondition * read_condition = service_info->read_condition;
      if (read_condition == nullptr) {
        RMW_SET_ERROR_MSG("read condition handle is null");
        return RMW_RET_ERROR;
      }

      uint32_t j = 0;
      for (; j < dds_ConditionSeq_length(active_conditions); ++j) {
        if (
          dds_ConditionSeq_get(active_conditions, j) ==
          reinterpret_cast<dds_Condition *>(read_condition))
        {
          break;
        }
      }

      if (j >= dds_ConditionSeq_length(active_conditions)) {
        services->services[i] = 0;
      }

      rmw_ret_t rmw_ret_code = __detach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(read_condition));
      if (rmw_ret_code != RMW_RET_OK) {
        return rmw_ret_code;
      }
    }
  }

  if (clients != nullptr) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      ClientInfo * client_info = static_cast<ClientInfo *>(clients->clients[i]);
      if (client_info == nullptr) {
        RMW_SET_ERROR_MSG("client info handle is null");
        return RMW_RET_ERROR;
      }

      dds_ReadCondition * read_condition = client_info->read_condition;
      if (read_condition == nullptr) {
        RMW_SET_ERROR_MSG("read condition handle is null");
        return RMW_RET_ERROR;
      }

      uint32_t j = 0;
      for (; j < dds_ConditionSeq_length(active_conditions); ++j) {
        if (
          dds_ConditionSeq_get(active_conditions, j) ==
          reinterpret_cast<dds_Condition *>(read_condition))
        {
          break;
        }
      }

      if (j >= dds_ConditionSeq_length(active_conditions)) {
        clients->clients[i] = 0;
      }

      rmw_ret_t rmw_ret_code = __detach_condition(
        dds_wait_set, reinterpret_cast<dds_Condition *>(read_condition));
      if (rmw_ret_code != RMW_RET_OK) {
        return rmw_ret_code;
      }
    }
  }

  {
    rmw_ret_t rmw_ret_code = __handle_active_event_conditions(events);
    if (rmw_ret_code != RMW_RET_OK) {
      return rmw_ret_code;
    }
  }

  return rret;
}

#endif  // RMW_GURUMDDS_SHARED_CPP__RMW_WAIT_HPP_

// Copyright 2024 GurumNetworks, Inc.
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

#include <chrono>
#include <unordered_map>
#include <unordered_set>

#include "rcpputils/scope_exit.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/event_converter.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/event_info_service.hpp"
#include "rmw_gurumdds_cpp/wait.hpp"

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

namespace rmw_gurumdds_cpp
{

template<typename T>
bool check_reattached(const std::vector<T*> & cached_subscription, void** array, size_t count) {
  if(array == nullptr || count == 0)
    return !cached_subscription.empty();

  if(cached_subscription.size() != count)
    return true;

  return memcmp(cached_subscription.data(), array, sizeof(void*) * count) != 0;
}

template<typename T>
void detach_conditions(dds_WaitSet* wait_set, const T & cached_conditions) {
  for(auto condition : cached_conditions) {
    dds_WaitSet_detach_condition(wait_set, reinterpret_cast<dds_Condition*>(condition));
  }
}

static rmw_ret_t
gather_event_conditions(
  rmw_events_t * events,
  std::unordered_set<dds_Condition*> & status_conditions)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(events, RMW_RET_INVALID_ARGUMENT);
  std::unordered_map<dds_StatusCondition *, dds_StatusMask> status_map;

  for (size_t i = 0; i < events->event_count; i++) {
    auto now = static_cast<rmw_event_t *>(events->events[i]);
    RMW_CHECK_ARGUMENT_FOR_NULL(events, RMW_RET_INVALID_ARGUMENT);

    auto event_info = static_cast<EventInfo *>(now->data);
    if (nullptr == event_info) {
      RMW_SET_ERROR_MSG("event handle is null");
      return RMW_RET_ERROR;
    }

    const auto event_type = now->event_type;
    if(!is_event_supported(event_type)) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("unsupported event: %d", now->event_type);
      continue;
    }

    if(event_info->has_callback(event_type)) {
      dds_GuardCondition * condition = event_info->get_guard_condition(event_type);
      if (nullptr == condition) {
        RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("unsupported event: %d", event_type);
        continue;
      }

      status_conditions.insert(reinterpret_cast<dds_Condition *>(condition));
    }

    auto& mask = status_map[event_info->get_status_condition()];
    mask |= get_status_kind_from_rmw(event_type);
  }

  for(auto & pair : status_map) {
    if(pair.second == 0) {
      continue;
    }

    dds_StatusCondition_set_enabled_statuses(pair.first, pair.second);
    status_conditions.insert(reinterpret_cast<dds_Condition *>(pair.first));
  }

  return RMW_RET_OK;
}

static rmw_ret_t handle_active_event_conditions(rmw_events_t * events)
{
  if (events == nullptr) {
    return RMW_RET_OK;
  }

  for (size_t i = 0; i < events->event_count; i++) {
    auto now = static_cast<rmw_event_t *>(events->events[i]);
    RMW_CHECK_ARGUMENT_FOR_NULL(events, RMW_RET_INVALID_ARGUMENT);

    auto event_info = static_cast<EventInfo *>(now->data);
    if (event_info == nullptr) {
      RMW_SET_ERROR_MSG("event handle is null");
      return RMW_RET_ERROR;
    }

    if (!event_info->is_status_changed(now->event_type)) {
      events->events[i] = nullptr;
    }
  }

  return RMW_RET_OK;
}

static rmw_ret_t
wait_w_polling(
  dds_WaitSet * dds_wait_set,
  dds_ConditionSeq * active_conditions,
  const rmw_time_t * wait_timeout
    ) {
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
    if (cond == nullptr) {
      continue;
    }

    if (dds_Condition_get_trigger_value(cond)) {
      dds_ConditionSeq_add(active_conditions, cond);
      triggered = true;
    }
  }

  for (uint32_t i = 0; (inf || std::chrono::steady_clock::now() <= t) && !triggered; ++i) {
    if (i >= dds_ConditionSeq_length(conds)) {
      i = 0;
    }

    dds_Condition * cond = dds_ConditionSeq_get(conds, i);
    if (cond == nullptr) {
      continue;
    }

    if (dds_Condition_get_trigger_value(cond)) {
      dds_ConditionSeq_add(active_conditions, cond);
      triggered = true;
    }
  }

  dds_ConditionSeq_delete(conds);
  return triggered ? RMW_RET_OK : RMW_RET_TIMEOUT;
}

rmw_ret_t
wait(
  const char * implementation_identifier,
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t * events,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  auto atexit = rcpputils::make_scope_exit([&]() {
    if (wait_set == nullptr) {
      RMW_SET_ERROR_MSG("wait set handle is null");
      return;
    }

    RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
      wait set handle, wait_set->implementation_identifier,
      implementation_identifier, return );

    WaitSetInfo * wait_set_info = static_cast<WaitSetInfo *>(wait_set->data);
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

    const uint32_t condition_seq_length = dds_ConditionSeq_length(attached_conditions);
    for (uint32_t i = 0; i < condition_seq_length; ++i) {
      ret = dds_WaitSet_detach_condition(
        dds_wait_set, dds_ConditionSeq_get(attached_conditions, i));
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to detach condition from wait set");
      }
    }

    while (dds_ConditionSeq_length(attached_conditions) > 0) {
      dds_ConditionSeq_remove(attached_conditions, 0);
    }

    wait_set_info->cached_subscriptions.clear();
    wait_set_info->cached_guard_conditions.clear();
    wait_set_info->cached_service_conditions.clear();
    wait_set_info->cached_client_conditions.clear();
    wait_set_info->cached_event_conditions.clear();
    wait_set_info->cached_status_conditions.clear();
    wait_set_info->cached_subscriptions.shrink_to_fit();
    wait_set_info->cached_guard_conditions.shrink_to_fit();
    wait_set_info->cached_service_conditions.shrink_to_fit();
    wait_set_info->cached_client_conditions.shrink_to_fit();
    wait_set_info->cached_event_conditions.shrink_to_fit();
  });

  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait set handle, wait_set->implementation_identifier,
    implementation_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  WaitSetInfo * wait_set_info = static_cast<WaitSetInfo *>(wait_set->data);
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

  bool need_reattached = false;
  need_reattached = check_reattached(wait_set_info->cached_subscriptions, (subscriptions != nullptr) ? subscriptions->subscribers : nullptr,
                                     subscriptions != nullptr ? subscriptions->subscriber_count : 0);

  if(!need_reattached)
    need_reattached = check_reattached(wait_set_info->cached_guard_conditions, (guard_conditions != nullptr) ? guard_conditions->guard_conditions : nullptr,
                                       guard_conditions != nullptr ? guard_conditions->guard_condition_count : 0);

  if(!need_reattached)
    need_reattached = check_reattached(wait_set_info->cached_service_conditions, (services != nullptr) ? services->services : nullptr,
                                       services != nullptr ? services->service_count : 0);

  if(!need_reattached)
    need_reattached = check_reattached(wait_set_info->cached_client_conditions, (clients != nullptr) ? clients->clients : nullptr,
                                       clients != nullptr ? clients->client_count : 0);

  if(!need_reattached)
    need_reattached = check_reattached(wait_set_info->cached_event_conditions, events != nullptr ? events->events : nullptr, events != nullptr ? events->event_count : 0);

  if(need_reattached) {
    dds_ConditionSeq * attached_conditions =
        static_cast<dds_ConditionSeq *>(wait_set_info->attached_conditions);
    dds_ReturnCode_t ret = dds_WaitSet_get_conditions(dds_wait_set, attached_conditions);
    uint32_t length = dds_ConditionSeq_length(attached_conditions);
    for (uint32_t i = 0; i < length; ++i) {
      ret = dds_WaitSet_detach_condition(
          dds_wait_set, dds_ConditionSeq_get(attached_conditions, i));
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to detach condition from wait set");
      }
    }

    if(length != 0) {
      do {
        length -= 1;
        dds_ConditionSeq_remove(attached_conditions, length);

      } while(length != 0);
    }

    wait_set_info->cached_event_conditions.clear();
    wait_set_info->cached_status_conditions.clear();
    wait_set_info->cached_client_conditions.clear();
    wait_set_info->cached_service_conditions.clear();
    wait_set_info->cached_guard_conditions.clear();
    wait_set_info->cached_subscriptions.clear();
    if(subscriptions != nullptr) {
      for(uint32_t i = 0; i < subscriptions->subscriber_count; ++i) {
        auto it = static_cast<SubscriberInfo*>(subscriptions->subscribers[i]);
        if(it != nullptr)
          dds_WaitSet_attach_condition(wait_set_info->wait_set, reinterpret_cast<dds_Condition*>(it->read_condition));

        wait_set_info->cached_subscriptions.push_back(it);
      }
    }

    if(guard_conditions != nullptr) {
      for(uint32_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
        auto it = static_cast<dds_GuardCondition*>(guard_conditions->guard_conditions[i]);
        if(it != nullptr)
          dds_WaitSet_attach_condition(wait_set_info->wait_set, reinterpret_cast<dds_Condition*>(it));

        wait_set_info->cached_guard_conditions.push_back(it);
      }
    }

    if(services != nullptr) {
      for(uint32_t i = 0; i < services->service_count; ++i) {
        auto it = static_cast<ServiceInfo*>(services->services[i]);
        if(it != nullptr)
          dds_WaitSet_attach_condition(wait_set_info->wait_set, reinterpret_cast<dds_Condition*>(it->read_condition));

        wait_set_info->cached_service_conditions.push_back(it);
      }
    }

    if(clients != nullptr) {
      for(uint32_t i = 0; i < clients->client_count; ++i) {
        auto it = static_cast<ClientInfo*>(clients->clients[i]);
        if(it != nullptr)
          dds_WaitSet_attach_condition(wait_set_info->wait_set, reinterpret_cast<dds_Condition*>(it->read_condition));

        wait_set_info->cached_client_conditions.push_back(it);
      }
    }

    if(events != nullptr) {
      for(uint32_t i = 0; i < events->event_count; ++i) {
        auto it = static_cast<EventInfo*>(events->events[i]);
        if(it != nullptr)
          wait_set_info->cached_event_conditions.push_back(it);
      }

      gather_event_conditions(events, wait_set_info->cached_status_conditions);
      for (auto status_condition : wait_set_info->cached_status_conditions) {
        dds_WaitSet_attach_condition(
            dds_wait_set,
            reinterpret_cast<dds_Condition *>(status_condition));
      }
    }

    wait_set_info->cached_subscriptions.shrink_to_fit();
    wait_set_info->cached_guard_conditions.shrink_to_fit();
    wait_set_info->cached_service_conditions.shrink_to_fit();
    wait_set_info->cached_client_conditions.shrink_to_fit();
    wait_set_info->cached_event_conditions.shrink_to_fit();
  }

  rmw_ret_t rret = RMW_RET_OK;
  static constexpr const char * env_name = "RMW_GURUMDDS_WAIT_USE_POLLING";
  static bool initialized_polling = false;
  static bool polling = false;
  if(!initialized_polling) {
    initialized_polling = true;
    char * env_value = nullptr;
    env_value = getenv(env_name);
    if (nullptr != env_value) {
      polling = (strcmp(env_value, "1") == 0);
    }
  }

  if (!polling) {  // Default: use dds_WaitSet_wait()
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
      if (subscriptions != nullptr) {
        memset(subscriptions->subscribers, 0, sizeof(void*) * subscriptions->subscriber_count);
      }

      if (guard_conditions != nullptr) {
        memset(guard_conditions->guard_conditions, 0, sizeof(void*) * guard_conditions->guard_condition_count);
      }

      if (services != nullptr) {
        memset(services->services, 0, sizeof(void*) * services->service_count);
      }

      if (clients != nullptr) {
        memset(clients->clients, 0, sizeof(void*) * clients->client_count);
      }

      if (events != nullptr) {
        memset(events->events, 0, sizeof(void*) * events->event_count);
      }

      atexit.cancel();
      return rret;
    }
  } else {  // use polilng
    rret = wait_w_polling(dds_wait_set, active_conditions, wait_timeout);
  }

  const uint32_t active_cond_length = dds_ConditionSeq_length(active_conditions);
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
      for (; j < active_cond_length; ++j) {
        if (
          dds_ConditionSeq_get(active_conditions, j) ==
          reinterpret_cast<dds_Condition *>(read_condition)) {
          break;
        }
      }

      if (j < active_cond_length) {
        continue;
      }

      subscriptions->subscribers[i] = nullptr;
    }
  }

  if (guard_conditions != nullptr) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      auto * condition =
        static_cast<dds_Condition *>(guard_conditions->guard_conditions[i]);
      if (condition == nullptr) {
        RMW_SET_ERROR_MSG("condition handle is null");
        return RMW_RET_ERROR;
      }

      uint32_t j = 0;
      for (; j < active_cond_length; ++j) {
        if (dds_ConditionSeq_get(active_conditions, j) == condition) {
          auto * guard = reinterpret_cast<dds_GuardCondition *>(condition);
          dds_ReturnCode_t ret = dds_GuardCondition_set_trigger_value(guard, false);
          if (ret != dds_RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to set trigger value");
            return RMW_RET_ERROR;
          }
          break;
        }
      }

      if (j < active_cond_length) {
        continue;
      }

      guard_conditions->guard_conditions[i] = nullptr;
    }
  }

  if (services != nullptr) {
    for (size_t i = 0; i < services->service_count; ++i) {
      auto * service_info = static_cast<ServiceInfo *>(services->services[i]);
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
      for (; j < active_cond_length; ++j) {
        if (
          dds_ConditionSeq_get(active_conditions, j) ==
          reinterpret_cast<dds_Condition *>(read_condition))
        {
          break;
        }
      }

      if (j < active_cond_length) {
        continue;
      }

      services->services[i] = nullptr;
    }
  }

  if (clients != nullptr) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      auto * client_info = static_cast<ClientInfo *>(clients->clients[i]);
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
      for (; j < active_cond_length; ++j) {
        if (
          dds_ConditionSeq_get(active_conditions, j) ==
          reinterpret_cast<dds_Condition *>(read_condition))
        {
          break;
        }
      }

      if (j < active_cond_length) {
        continue;
      }

      clients->clients[i] = nullptr;
    }
  }

  rmw_ret_t rmw_ret_code = handle_active_event_conditions(events);
  if (rmw_ret_code != RMW_RET_OK) {
    return rmw_ret_code;
  }

  atexit.cancel();
  return rret;
}
} // namespace rmw_gurumdds_cpp

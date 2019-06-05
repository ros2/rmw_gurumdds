#ifndef RMW_COREDDS_SHARED_CPP__RMW_WAIT_HPP_
#define RMW_COREDDS_SHARED_CPP__RMW_WAIT_HPP_

#include <chrono>
#include <utility>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"
#include "rmw_coredds_shared_cpp/types.hpp"
#include "rmw_coredds_shared_cpp/dds_include.hpp"

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

template<typename SubscriberInfo, typename ServiceInfo, typename ClientInfo>
rmw_ret_t
shared__rmw_wait(
  const char * implementation_identifier,
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
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

      CoreddsWaitSetInfo * wait_set_info = static_cast<CoreddsWaitSetInfo *>(wait_set->data);
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
        RMW_SET_ERROR_MSG("Failed to get attached conditions for wait set");
        return;
      }

      for (uint32_t i = 0; i < dds_ConditionSeq_length(attached_conditions); ++i) {
        ret = dds_WaitSet_detach_condition(
          dds_wait_set, dds_ConditionSeq_get(attached_conditions, i));
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to detach condition from wait set");
        }
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

  CoreddsWaitSetInfo * wait_set_info = static_cast<CoreddsWaitSetInfo *>(wait_set->data);
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

  const char * env_name = "RMW_COREDDS_WAIT_USE_POLLING";
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

      dds_ReturnCode_t ret = dds_WaitSet_detach_condition(
        dds_wait_set, (dds_Condition *)read_condition);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to detach condition from wait set");
        return RMW_RET_ERROR;
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

      dds_ReturnCode_t ret = dds_WaitSet_detach_condition(dds_wait_set, condition);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to detach condition from wait set");
        return RMW_RET_ERROR;
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

      dds_ReturnCode_t ret = dds_WaitSet_detach_condition(
        dds_wait_set, (dds_Condition *)read_condition);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to detach condition from wait set");
        return RMW_RET_ERROR;
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

      dds_ReturnCode_t ret = dds_WaitSet_detach_condition(
        dds_wait_set, (dds_Condition *)read_condition);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to detach condition from wait set");
        return RMW_RET_ERROR;
      }
    }
  }

  return rret;
}

#endif  // RMW_COREDDS_SHARED_CPP__RMW_WAIT_HPP_

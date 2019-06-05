#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"
#include "rmw_coredds_shared_cpp/rmw_wait.hpp"
#include "rmw_coredds_cpp/identifier.hpp"
#include "rmw_coredds_cpp/types.hpp"

extern "C"
{
rmw_wait_set_t *
rmw_create_wait_set(size_t max_conditions)
{
  return shared__rmw_create_wait_set(gurum_coredds_identifier, max_conditions);
}

rmw_ret_t
rmw_destroy_wait_set(rmw_wait_set_t * wait_set)
{
  return shared__rmw_destroy_wait_set(gurum_coredds_identifier, wait_set);
}

rmw_ret_t
rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
  return shared__rmw_wait<CoreddsSubscriberInfo, CoreddsServiceInfo, CoreddsClientInfo>(
    gurum_coredds_identifier, subscriptions, guard_conditions,
    services, clients, wait_set, wait_timeout);
}
}  // extern "C"

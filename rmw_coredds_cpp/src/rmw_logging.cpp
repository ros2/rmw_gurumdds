#include "rmw_coredds_shared_cpp/rmw_common.hpp"

extern "C"
{
rmw_ret_t
rmw_set_log_severity(rmw_log_severity_t severity)
{
  return shared__rmw_set_log_severity(severity);
}
}  // extern "C"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "rcutils/logging_macros.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"
#include "rmw_coredds_shared_cpp/dds_include.hpp"

rmw_ret_t
shared__rmw_set_log_severity(rmw_log_severity_t severity)
{
  switch (severity) {
    case RMW_LOG_SEVERITY_DEBUG:
      dds_DomainParticipantFactory_set_loglevel(1);
      break;
    case RMW_LOG_SEVERITY_INFO:
      dds_DomainParticipantFactory_set_loglevel(2);
      break;
    case RMW_LOG_SEVERITY_WARN:
      dds_DomainParticipantFactory_set_loglevel(3);
      break;
    case RMW_LOG_SEVERITY_ERROR:
      dds_DomainParticipantFactory_set_loglevel(4);
      break;
    case RMW_LOG_SEVERITY_FATAL:
      dds_DomainParticipantFactory_set_loglevel(5);
      break;
    default:
      RCUTILS_LOG_ERROR("Unknown logging severity type %d", severity);
      return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}

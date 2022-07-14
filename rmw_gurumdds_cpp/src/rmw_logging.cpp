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

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"

extern "C"
{
rmw_ret_t
rmw_set_log_severity(rmw_log_severity_t severity)
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
}  // extern "C"

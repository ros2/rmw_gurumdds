#ifndef RMW_COREDDS_SHARED_CPP__QOS_HPP_
#define RMW_COREDDS_SHARED_CPP__QOS_HPP_

#include <cassert>
#include <limits>

#include "rmw/error_handling.h"
#include "rmw/types.h"

#include "rmw_coredds_shared_cpp/visibility_control.h"

#include "./dds_include.hpp"

bool
get_datawriter_qos(
  dds_Publisher * publisher,
  const rmw_qos_profile_t * qos_profile,
  dds_DataWriterQos * datawriter_qos);

bool
get_datareader_qos(
  dds_Subscriber * subscriber,
  const rmw_qos_profile_t * qos_profile,
  dds_DataReaderQos * datareader_qos);

#endif  // RMW_COREDDS_SHARED_CPP__QOS_HPP_

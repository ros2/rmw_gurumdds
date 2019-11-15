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

#ifndef RMW_GURUMDDS_SHARED_CPP__QOS_HPP_
#define RMW_GURUMDDS_SHARED_CPP__QOS_HPP_

#include <cassert>
#include <limits>

#include "rmw/error_handling.h"
#include "rmw/types.h"

#include "rmw_gurumdds_shared_cpp/visibility_control.h"

#include "./dds_include.hpp"

RMW_GURUMDDS_SHARED_CPP_PUBLIC
bool
get_datawriter_qos(
  dds_Publisher * publisher,
  const rmw_qos_profile_t * qos_profile,
  dds_DataWriterQos * datawriter_qos);

RMW_GURUMDDS_SHARED_CPP_PUBLIC
bool
get_datareader_qos(
  dds_Subscriber * subscriber,
  const rmw_qos_profile_t * qos_profile,
  dds_DataReaderQos * datareader_qos);

#endif  // RMW_GURUMDDS_SHARED_CPP__QOS_HPP_

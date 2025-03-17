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

#ifndef RMW_GURUMDDS__TYPE_SUPPORT_HPP_
#define RMW_GURUMDDS__TYPE_SUPPORT_HPP_

#include <string>

#include "dds_include.hpp"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
namespace rmw_gurumdds_cpp
{
dds_TypeSupport*
create_type_support_and_register(
  dds_DomainParticipant * participant,
  const rosidl_message_type_support_t * type_support,
  const std::string & type_name,
  const std::string & metastring);

void set_type_support_ops(
  dds_TypeSupport* dds_type_support,
  const rosidl_message_type_support_t* type_support
  );

void set_service_typesupport(dds_DataWriter* writer, dds_DataReader* reader, const rosidl_service_type_support_t* rosidl_typesupport);

void set_client_typesupport(dds_DataWriter* writer, dds_DataReader* reader, const rosidl_service_type_support_t* rosidl_typesupport);

}

#endif  // RMW_GURUMDDS__TYPE_SUPPORT_HPP_

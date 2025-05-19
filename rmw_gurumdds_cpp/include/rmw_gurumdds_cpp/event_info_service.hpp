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

#ifndef RMW_GURUMDDS_CPP__EVENT_INFO_SERVICE_HPP_
#define RMW_GURUMDDS_CPP__EVENT_INFO_SERVICE_HPP_

#include "rmw/event_callback_type.h"
#include "rmw/types.h"

#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"

namespace rmw_gurumdds_cpp
{
struct ClientInfo
{
  const rosidl_service_type_support_t * service_typesupport;
  const char * implementation_identifier;
  rmw_context_impl_t * ctx;
  int64_t sequence_number;
  uint8_t writer_guid[16];

  rmw_gid_t publisher_gid;
  rmw_gid_t subscriber_gid;
  dds_DataWriter * request_writer;
  dds_DataReader * response_reader;
  dds_ReadCondition * read_condition;

  dds_DataReaderListener response_listener;
  dds_DataSeq * data_seq;
  dds_SampleInfoSeq * info_seq;
  dds_UnsignedLongSeq * raw_data_sizes;
  event_callback_data_t event_callback_data;

  size_t count_unread()
  {
    return rmw_gurumdds_cpp::count_unread(response_reader, data_seq, info_seq, raw_data_sizes);
  }
};

struct ServiceInfo {
  const rosidl_service_type_support_t * service_typesupport;
  const char * implementation_identifier;
  rmw_context_impl_t * ctx;

  rmw_gid_t publisher_gid;
  rmw_gid_t subscriber_gid;
  dds_DataWriter * response_writer;
  dds_DataReader * request_reader;
  dds_ReadCondition * read_condition;

  dds_DataReaderListener request_listener;
  dds_DataSeq * data_seq;
  dds_SampleInfoSeq * info_seq;
  dds_UnsignedLongSeq * raw_data_sizes;
  event_callback_data_t event_callback_data;

  size_t count_unread()
  {
    return rmw_gurumdds_cpp::count_unread(request_reader, data_seq, info_seq, raw_data_sizes);
  }
};
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__EVENT_INFO_SERVICE_HPP_

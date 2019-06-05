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

#ifndef RMW_COREDDS_CPP__TYPES_HPP_
#define RMW_COREDDS_CPP__TYPES_HPP_

#include "rmw/rmw.h"
#include "rosidl_typesupport_coredds_cpp/message_type_support.h"

typedef struct _CoreddsPublisherInfo
{
  dds_Publisher * publisher;
  rmw_gid_t publisher_gid;
  dds_DataWriter * topic_writer;
  const message_type_support_callbacks_t * callbacks;
  const char * implementation_identifier;
} CoreddsPublisherInfo;

typedef struct _CoreddsPublisherGID
{
  dds_InstanceHandle_t publication_handle;
} CoreddsPublisherGID;

typedef struct _CoreddsSubscriberInfo
{
  dds_Subscriber * subscriber;
  dds_DataReader * topic_reader;
  dds_ReadCondition * read_condition;
  bool ignore_local_publications;
  const message_type_support_callbacks_t * callbacks;
  const char * implementation_identifier;
} CoreddsSubscriberInfo;

typedef struct _CoreddsServiceInfo
{
  const message_type_support_callbacks_t * request_callbacks;
  const message_type_support_callbacks_t * response_callbacks;
  dds_Subscriber * dds_subscriber;
  dds_DataReader * request_reader;
  dds_Publisher * dds_publisher;
  dds_DataWriter * response_writer;
  dds_ReadCondition * read_condition;
  dds_DomainParticipant * participant;
  const char * implementation_identifier;
} CoreddsServiceInfo;

typedef struct _CoreddsClientInfo
{
  const message_type_support_callbacks_t * response_callbacks;
  const message_type_support_callbacks_t * request_callbacks;
  dds_Publisher * dds_publisher;
  dds_DataWriter * request_writer;
  dds_Subscriber * dds_subscriber;
  dds_DataReader * response_reader;
  dds_ReadCondition * read_condition;
  dds_DomainParticipant * participant;
  const char * implementation_identifier;
  int64_t sequence_number;
  int64_t writer_guid_0;
  int64_t writer_guid_1;
} CoreddsClientInfo;

#endif  // RMW_COREDDS_CPP__TYPES_HPP_

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

#ifndef RMW_GURUMDDS_CPP__TYPES_HPP_
#define RMW_GURUMDDS_CPP__TYPES_HPP_

#include <atomic>
#include <cassert>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "rmw/ret_types.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/visibility_control.h"

void on_participant_changed(
  const dds_DomainParticipant * a_participant,
  const dds_ParticipantBuiltinTopicData * data,
  dds_InstanceHandle_t handle);

void on_publication_changed(
  const dds_DomainParticipant * a_participant,
  const dds_PublicationBuiltinTopicData * data,
  dds_InstanceHandle_t handle);

void on_subscription_changed(
  const dds_DomainParticipant * a_participant,
  const dds_SubscriptionBuiltinTopicData * data,
  dds_InstanceHandle_t handle);

typedef struct _GurumddsWaitSetInfo
{
  dds_WaitSet * wait_set;
  dds_ConditionSeq * active_conditions;
  dds_ConditionSeq * attached_conditions;
} GurumddsWaitSetInfo;

typedef struct _GurumddsEventInfo
{
  virtual ~_GurumddsEventInfo() = default;
  virtual rmw_ret_t get_status(const dds_StatusMask mask, void * event) = 0;
  virtual dds_StatusCondition * get_statuscondition() = 0;
  virtual dds_StatusMask get_status_changes() = 0;
} GurumddsEventInfo;

typedef struct _GurumddsPublisherInfo : GurumddsEventInfo
{
  rmw_gid_t publisher_gid;
  dds_DataWriter * topic_writer;
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const char * implementation_identifier;
  rmw_context_impl_t * ctx;

  rmw_ret_t get_status(dds_StatusMask mask, void * event) override;
  dds_StatusCondition * get_statuscondition() override;
  dds_StatusMask get_status_changes() override;
} GurumddsPublisherInfo;

typedef struct _GurumddsPublisherGID
{
  uint8_t publication_handle[16];
} GurumddsPublisherGID;

typedef struct _GurumddsSubscriberInfo : GurumddsEventInfo
{
  rmw_gid_t subscriber_gid;
  dds_DataReader * topic_reader;
  dds_ReadCondition * read_condition;
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const char * implementation_identifier;
  rmw_context_impl_t * ctx;

  rmw_ret_t get_status(dds_StatusMask mask, void * event) override;
  dds_StatusCondition * get_statuscondition() override;
  dds_StatusMask get_status_changes() override;
} GurumddsSubscriberInfo;

typedef struct _GurumddsClientInfo
{
  const rosidl_service_type_support_t * service_typesupport;

  rmw_gid_t publisher_gid;
  rmw_gid_t subscriber_gid;
  dds_DataWriter * request_writer;
  dds_DataReader * response_reader;
  dds_ReadCondition * read_condition;

  const char * implementation_identifier;
  rmw_context_impl_t * ctx;

  int64_t sequence_number;
  int8_t writer_guid[16];
} GurumddsClientInfo;

typedef struct _GurumddsServiceInfo
{
  const rosidl_service_type_support_t * service_typesupport;

  rmw_gid_t publisher_gid;
  rmw_gid_t subscriber_gid;
  dds_DataWriter * response_writer;
  dds_DataReader * request_reader;
  dds_ReadCondition * read_condition;

  const char * implementation_identifier;
  rmw_context_impl_t * ctx;
} GurumddsServiceInfo;

#endif  // RMW_GURUMDDS_CPP__TYPES_HPP_

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
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "rmw/ret_types.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/guid.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/topic_cache.hpp"
#include "rmw_gurumdds_cpp/visibility_control.h"

enum EntityType {Publisher, Subscriber};

class ListenerContext
{
public:
  explicit ListenerContext(rmw_guard_condition_t * graph_guard_condition)
  : topic_cache(nullptr),
    graph_guard_condition(graph_guard_condition)
  {}

  virtual ~ListenerContext() = default;

  RMW_GURUMDDS_CPP_PUBLIC
  virtual void add_information(
    const GuidPrefix_t & participant_guid,
    const GuidPrefix_t & topic_guid,
    const std::string & topic_name,
    const std::string & type_name,
    rmw_qos_profile_t & qos,
    EntityType entity_type);

  RMW_GURUMDDS_CPP_PUBLIC
  virtual void remove_information(
    const GuidPrefix_t & topic_guid,
    const EntityType entity_type);

  RMW_GURUMDDS_CPP_PUBLIC
  virtual void trigger_graph_guard_condition(void);

  RMW_GURUMDDS_CPP_PUBLIC
  size_t count_topic(const char * topic_name);

  RMW_GURUMDDS_CPP_PUBLIC
  void fill_topic_names_and_types(
    bool no_demangle,
    std::map<std::string, std::set<std::string>> & topic_names_to_types);

  RMW_GURUMDDS_CPP_PUBLIC
  void fill_service_names_and_types(
    std::map<std::string, std::set<std::string>> & services);

  RMW_GURUMDDS_CPP_PUBLIC
  void fill_topic_names_and_types_by_guid(
    bool no_demangle,
    std::map<std::string, std::set<std::string>> & topic_names_to_types_by_guid,
    GuidPrefix_t & participant_guid);

  RMW_GURUMDDS_CPP_PUBLIC
  void fill_service_names_and_types_by_guid(
    std::map<std::string, std::set<std::string>> & services,
    GuidPrefix_t & participant_guid,
    const std::string suffix);

  std::mutex mutex_;
  TopicCache<GuidPrefix_t> * topic_cache;
  rmw_guard_condition_t * graph_guard_condition;
};

static inline void on_participant_changed(
  const dds_DomainParticipant * a_participant,
  const dds_ParticipantBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  dds_DomainParticipant * participant = const_cast<dds_DomainParticipant *>(a_participant);
  ListenerContext * pub_context =
    reinterpret_cast<ListenerContext *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0));
  ListenerContext * sub_context =
    reinterpret_cast<ListenerContext *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 1));

  GuidPrefix_t participant_guid;
  if (reinterpret_cast<void *>(handle) == NULL) {
    dds_BuiltinTopicKey_to_GUID(&participant_guid, data->key);
    {
      std::lock_guard<std::mutex> lg1(pub_context->mutex_);
      if (pub_context->topic_cache->remove_topic_by_puid(participant_guid) > 0) {
        pub_context->trigger_graph_guard_condition();
      }
    }
    {
      std::lock_guard<std::mutex> lg2(sub_context->mutex_);
      if (sub_context->topic_cache->remove_topic_by_puid(participant_guid) > 0) {
        sub_context->trigger_graph_guard_condition();
      }
    }
  }
}

static inline void on_publication_changed(
  const dds_DomainParticipant * a_participant,
  const dds_PublicationBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  dds_DomainParticipant * participant = const_cast<dds_DomainParticipant *>(a_participant);
  ListenerContext * context =
    reinterpret_cast<ListenerContext *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0));

  std::string topic_name, type_name;
  GuidPrefix_t guid, participant_guid;
  dds_BuiltinTopicKey_to_GUID(&participant_guid, data->participant_key);
  memcpy(guid.value, participant_guid.value, 12);
  memcpy(&guid.value[12], &data->key.value[0], 4);
  std::lock_guard<std::mutex> guard(context->mutex_);
  if (reinterpret_cast<void *>(handle) != NULL) {
    topic_name = std::string(data->topic_name);
    type_name = std::string(data->type_name);
    rmw_qos_profile_t qos = {
      RMW_QOS_POLICY_HISTORY_UNKNOWN,
      RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT,
      convert_reliability(data->reliability),
      convert_durability(data->durability),
      convert_deadline(data->deadline),
      convert_lifespan(data->lifespan),
      convert_liveliness(data->liveliness),
      convert_liveliness_lease_duration(data->liveliness),
      false,
    };
    context->topic_cache->add_topic(
      participant_guid, guid, std::move(topic_name),
      std::move(type_name), qos);
  } else {
    context->topic_cache->remove_topic(guid);
  }
  context->trigger_graph_guard_condition();
}

static inline void on_subscription_changed(
  const dds_DomainParticipant * a_participant,
  const dds_SubscriptionBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  dds_DomainParticipant * participant = const_cast<dds_DomainParticipant *>(a_participant);
  ListenerContext * context =
    reinterpret_cast<ListenerContext *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 1));

  std::string topic_name, type_name;
  GuidPrefix_t guid, participant_guid;
  dds_BuiltinTopicKey_to_GUID(&participant_guid, data->participant_key);
  memcpy(guid.value, participant_guid.value, 12);
  memcpy(&guid.value[12], &data->key.value[0], 4);
  std::lock_guard<std::mutex> guard(context->mutex_);
  if (reinterpret_cast<void *>(handle) != NULL) {
    topic_name = data->topic_name;
    type_name = data->type_name;
    rmw_qos_profile_t qos = {
      RMW_QOS_POLICY_HISTORY_UNKNOWN,  // TODO(clemjh): sbtd doesn't contain history qos policy
      RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT,
      convert_reliability(data->reliability),
      convert_durability(data->durability),
      convert_deadline(data->deadline),
      RMW_QOS_LIFESPAN_DEFAULT,
      convert_liveliness(data->liveliness),
      convert_liveliness_lease_duration(data->liveliness),
      false,
    };
    context->topic_cache->add_topic(
      participant_guid, guid, std::move(topic_name),
      std::move(type_name), qos);
  } else {
    context->topic_cache->remove_topic(guid);
  }
  context->trigger_graph_guard_condition();
}

typedef struct _GurumddsNodeInfo
{
  dds_DomainParticipant * participant;
  rmw_guard_condition_t * graph_guard_condition;
  ListenerContext * pub_context;
  ListenerContext * sub_context;
  std::list<dds_Publisher *> pub_list;
  std::list<dds_Subscriber *> sub_list;
} GurumddsNodeInfo;

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
  dds_Publisher * publisher;
  rmw_gid_t publisher_gid;
  dds_DataWriter * topic_writer;
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const char * implementation_identifier;

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
  dds_Subscriber * subscriber;
  dds_DataReader * topic_reader;
  dds_ReadCondition * read_condition;
  const rosidl_message_type_support_t * rosidl_message_typesupport;
  const char * implementation_identifier;

  rmw_ret_t get_status(dds_StatusMask mask, void * event) override;
  dds_StatusCondition * get_statuscondition() override;
  dds_StatusMask get_status_changes() override;
} GurumddsSubscriberInfo;

typedef struct _GurumddsServiceInfo
{
  const rosidl_service_type_support_t * service_typesupport;

  dds_Subscriber * dds_subscriber;
  dds_DataReader * request_reader;

  dds_Publisher * dds_publisher;
  dds_DataWriter * response_writer;

  dds_ReadCondition * read_condition;
  dds_DomainParticipant * participant;
  const char * implementation_identifier;
} GurumddsServiceInfo;

typedef struct _GurumddsClientInfo
{
  const rosidl_service_type_support_t * service_typesupport;

  dds_Publisher * dds_publisher;
  dds_DataWriter * request_writer;

  dds_Subscriber * dds_subscriber;
  dds_DataReader * response_reader;

  dds_ReadCondition * read_condition;
  dds_DomainParticipant * participant;
  const char * implementation_identifier;

  int64_t sequence_number;
  int8_t writer_guid[16];
} GurumddsClientInfo;

#endif  // RMW_GURUMDDS_CPP__TYPES_HPP_

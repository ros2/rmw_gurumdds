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

#ifndef RMW_GURUMDDS_SHARED_CPP__TYPES_HPP_
#define RMW_GURUMDDS_SHARED_CPP__TYPES_HPP_

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

#include "rmw/rmw.h"
#include "rmw/ret_types.h"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"
#include "rmw_gurumdds_shared_cpp/visibility_control.h"
#include "rmw_gurumdds_shared_cpp/guid.hpp"
#include "rmw_gurumdds_shared_cpp/topic_cache.hpp"
#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"

enum EntityType {Publisher, Subscriber};

typedef struct _ListenerContext
{
  std::mutex * mutex_;
  TopicCache<GuidPrefix_t> * topic_cache;
  rmw_guard_condition_t * graph_guard_condition;
  const char * implementation_identifier;
} ListenerContext;


typedef struct _GurumddsMessage
{
  void * sample;
  dds_SampleInfo * info;
  dds_UnsignedLong size;
} GurumddsMessage;

static void pub_on_data_available(const dds_DataReader * a_reader)
{
  dds_DataReader * reader = const_cast<dds_DataReader *>(a_reader);
  ListenerContext * context =
    reinterpret_cast<ListenerContext *>(dds_DataReader_get_listener_context(reader));
  if (context == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lock(*context->mutex_);
  dds_DataSeq * samples = dds_DataSeq_create(8);
  if (samples == nullptr) {
    fprintf(stderr, "failed to create data sample sequence\n");
    return;
  }
  dds_SampleInfoSeq * infos = dds_SampleInfoSeq_create(8);
  if (infos == nullptr) {
    dds_DataSeq_delete(samples);
    fprintf(stderr, "failed to create sample info sequence\n");
    return;
  }

  dds_ReturnCode_t ret = dds_DataReader_take(
    reader, samples, infos, dds_LENGTH_UNLIMITED,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);
  if (ret == dds_RETCODE_NO_DATA) {
    dds_DataReader_return_loan(reader, samples, infos);
    dds_DataSeq_delete(samples);
    dds_SampleInfoSeq_delete(infos);
    return;
  }
  if (ret != dds_RETCODE_OK) {
    fprintf(stderr, "failed to access data from the built-in reader\n");
    dds_DataReader_return_loan(reader, samples, infos);
    dds_DataSeq_delete(samples);
    dds_SampleInfoSeq_delete(infos);
    return;
  }

  for (dds_UnsignedLong i = 0; i < dds_DataSeq_length(samples); ++i) {
    std::string topic_name, type_name;
    GuidPrefix_t guid, participant_guid;
    dds_PublicationBuiltinTopicData * pbtd =
      reinterpret_cast<dds_PublicationBuiltinTopicData *>(dds_DataSeq_get(samples, i));
    dds_SampleInfo * info = dds_SampleInfoSeq_get(infos, i);
    if (reinterpret_cast<void *>(info->instance_handle) == NULL) {
      continue;
    }
    memcpy(guid.value, reinterpret_cast<void *>(info->instance_handle), 16);
    if (info->valid_data && info->instance_state == dds_ALIVE_INSTANCE_STATE) {
      dds_BuiltinTopicKey_to_GUID(&participant_guid, pbtd->participant_key);
      topic_name = std::string(pbtd->topic_name);
      type_name = std::string(pbtd->type_name);
      context->topic_cache->add_topic(participant_guid, guid, topic_name, type_name);
    } else {
      context->topic_cache->remove_topic(guid);
    }
  }

  if (dds_DataSeq_length(samples) > 0) {
    rmw_ret_t rmw_ret = shared__rmw_trigger_guard_condition(
      context->implementation_identifier, context->graph_guard_condition);
    if (rmw_ret != RMW_RET_OK) {
      fprintf(stderr, "failed to trigger graph guard condition: %s\n", rmw_get_error_string().str);
    }
  }

  dds_DataReader_return_loan(reader, samples, infos);

  dds_DataSeq_delete(samples);
  dds_SampleInfoSeq_delete(infos);

  dds_DataReader_set_listener_context(reader, context);
}

static void sub_on_data_available(const dds_DataReader * a_reader)
{
  dds_DataReader * reader = const_cast<dds_DataReader *>(a_reader);
  ListenerContext * context =
    reinterpret_cast<ListenerContext *>(dds_DataReader_get_listener_context(reader));
  if (context == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lock(*context->mutex_);
  dds_DataSeq * samples = dds_DataSeq_create(8);
  if (samples == nullptr) {
    fprintf(stderr, "failed to create data sample sequence\n");
    return;
  }
  dds_SampleInfoSeq * infos = dds_SampleInfoSeq_create(8);
  if (infos == nullptr) {
    dds_DataSeq_delete(samples);
    fprintf(stderr, "failed to create sample info sequence\n");
    return;
  }

  dds_ReturnCode_t ret = dds_DataReader_take(
    reader, samples, infos, dds_LENGTH_UNLIMITED,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);
  if (ret == dds_RETCODE_NO_DATA) {
    dds_DataReader_return_loan(reader, samples, infos);
    dds_DataSeq_delete(samples);
    dds_SampleInfoSeq_delete(infos);
    return;
  }
  if (ret != dds_RETCODE_OK) {
    fprintf(stderr, "failed to access data from the built-in reader\n");
    dds_DataReader_return_loan(reader, samples, infos);
    dds_DataSeq_delete(samples);
    dds_SampleInfoSeq_delete(infos);
    return;
  }

  for (dds_UnsignedLong i = 0; i < dds_DataSeq_length(samples); ++i) {
    std::string topic_name, type_name;
    GuidPrefix_t guid, participant_guid;
    dds_SubscriptionBuiltinTopicData * sbtd =
      reinterpret_cast<dds_SubscriptionBuiltinTopicData *>(dds_DataSeq_get(samples, i));
    dds_SampleInfo * info = dds_SampleInfoSeq_get(infos, i);
    if (reinterpret_cast<void *>(info->instance_handle) == NULL) {
      continue;
    }
    memcpy(guid.value, reinterpret_cast<void *>(info->instance_handle), 16);
    if (info->valid_data && info->instance_state == dds_ALIVE_INSTANCE_STATE) {
      dds_BuiltinTopicKey_to_GUID(&participant_guid, sbtd->participant_key);
      topic_name = sbtd->topic_name;
      type_name = sbtd->type_name;
      context->topic_cache->add_topic(participant_guid, guid, topic_name, type_name);
    } else {
      context->topic_cache->remove_topic(guid);
    }
  }

  if (dds_DataSeq_length(samples) > 0) {
    rmw_ret_t rmw_ret = shared__rmw_trigger_guard_condition(
      context->implementation_identifier, context->graph_guard_condition);
    if (rmw_ret != RMW_RET_OK) {
      fprintf(stderr, "failed to trigger graph guard condition: %s\n", rmw_get_error_string().str);
    }
  }

  dds_DataReader_return_loan(reader, samples, infos);

  dds_DataSeq_delete(samples);
  dds_SampleInfoSeq_delete(infos);

  dds_DataReader_set_listener_context(reader, context);
}

template<typename SubscriberInfo>
static void reader_on_data_available(const dds_DataReader * a_reader)
{
  const uint32_t MAX_SAMPLES = 64;
  dds_DataReader * reader = const_cast<dds_DataReader *>(a_reader);
  SubscriberInfo * subscriber_info =
    reinterpret_cast<SubscriberInfo *>(dds_DataReader_get_listener_context(reader));
  if (subscriber_info == nullptr) {
    RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_cpp", "Failed to take data: listener context is not set");
    return;
  }

  dds_DataSeq * sample_seq = dds_DataSeq_create(MAX_SAMPLES);
  if (sample_seq == nullptr) {
    return;
  }

  dds_SampleInfoSeq * info_seq = dds_SampleInfoSeq_create(MAX_SAMPLES);
  if (info_seq == nullptr) {
    dds_DataSeq_delete(sample_seq);
    return;
  }

  dds_UnsignedLongSeq * size_seq = dds_UnsignedLongSeq_create(MAX_SAMPLES);
  if (size_seq == nullptr) {
    dds_DataSeq_delete(sample_seq);
    dds_SampleInfoSeq_delete(info_seq);
    return;
  }

  dds_ReturnCode_t ret = dds_DataReader_raw_take(
    reader, dds_HANDLE_NIL, sample_seq, info_seq, size_seq, MAX_SAMPLES,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);
  if (ret != dds_RETCODE_OK) {
    if (ret != dds_RETCODE_NO_DATA) {
      RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_cpp", "Failed to take data");
    }
    dds_DataSeq_delete(sample_seq);
    dds_SampleInfoSeq_delete(info_seq);
    dds_UnsignedLongSeq_delete(size_seq);
    return;
  }

  subscriber_info->queue_mutex.lock();
  dds_GuardCondition_set_trigger_value(subscriber_info->queue_guard_condition, true);
  for (uint32_t i = 0; i < dds_DataSeq_length(sample_seq); i++) {
    GurumddsMessage msg;
    msg.sample = dds_DataSeq_get(sample_seq, i);
    msg.info = dds_SampleInfoSeq_get(info_seq, i);
    msg.size = dds_UnsignedLongSeq_get(size_seq, i);
    subscriber_info->message_queue.push(std::move(msg));
  }
  subscriber_info->queue_mutex.unlock();

  // return loan manually after deserialization
  // or before destruction of the queue using free()
  dds_DataSeq_delete(sample_seq);
  dds_SampleInfoSeq_delete(info_seq);
  dds_UnsignedLongSeq_delete(size_seq);
}

class GurumddsDataReaderListener
{
public:
  explicit GurumddsDataReaderListener(
    const char * implementation_identifier, rmw_guard_condition_t * graph_guard_condition)
  : graph_guard_condition(graph_guard_condition),
    implementation_identifier(implementation_identifier)
  {}

  virtual ~GurumddsDataReaderListener() = default;

  RMW_GURUMDDS_SHARED_CPP_PUBLIC
  virtual void add_information(
    const GuidPrefix_t & participant_guid,
    const GuidPrefix_t & topic_guid,
    const std::string & topic_name,
    const std::string & type_name,
    EntityType entity_type);

  RMW_GURUMDDS_SHARED_CPP_PUBLIC
  virtual void remove_information(
    const GuidPrefix_t & topic_guid,
    const EntityType entity_type);

  RMW_GURUMDDS_SHARED_CPP_PUBLIC
  virtual void trigger_graph_guard_condition(void);

  size_t count_topic(const char * topic_name);

  void fill_topic_names_and_types(
    bool no_demangle,
    std::map<std::string, std::set<std::string>> & topic_names_to_types);

  void fill_service_names_and_types(
    std::map<std::string, std::set<std::string>> & services);

  void fill_topic_names_and_types_by_guid(
    bool no_demangle,
    std::map<std::string, std::set<std::string>> & topic_names_to_types_by_guid,
    GuidPrefix_t & participant_guid);

  void fill_service_names_and_types_by_guid(
    std::map<std::string, std::set<std::string>> & services,
    GuidPrefix_t & participant_guid,
    const std::string suffix);

  dds_DataReaderListener dds_listener;
  ListenerContext context;
  dds_DataReader * dds_reader;

  std::mutex mutex_;
  TopicCache<GuidPrefix_t> topic_cache;
  rmw_guard_condition_t * graph_guard_condition;

  const char * implementation_identifier;

protected:
private:
};

class GurumddsPublisherListener : public GurumddsDataReaderListener
{
public:
  GurumddsPublisherListener(
    const char * implementation_identifier, rmw_guard_condition_t * graph_guard_condition)
  : GurumddsDataReaderListener(implementation_identifier, graph_guard_condition)
  {
    context.mutex_ = &(this->mutex_);
    context.topic_cache = &(this->topic_cache);
    context.graph_guard_condition = this->graph_guard_condition;
    context.implementation_identifier = this->implementation_identifier;
    dds_listener.on_data_available = pub_on_data_available;
  }

  ~GurumddsPublisherListener() {}
};

class GurumddsSubscriberListener : public GurumddsDataReaderListener
{
public:
  GurumddsSubscriberListener(
    const char * implementation_identifier, rmw_guard_condition_t * graph_guard_condition)
  : GurumddsDataReaderListener(implementation_identifier, graph_guard_condition)
  {
    context.mutex_ = &(this->mutex_);
    context.topic_cache = &(this->topic_cache);
    context.graph_guard_condition = this->graph_guard_condition;
    context.implementation_identifier = this->implementation_identifier;
    dds_listener.on_data_available = sub_on_data_available;
  }

  ~GurumddsSubscriberListener() {}
};

typedef struct _GurumddsNodeInfo
{
  dds_DomainParticipant * participant;
  rmw_guard_condition_t * graph_guard_condition;
  GurumddsPublisherListener * pub_listener;
  GurumddsSubscriberListener * sub_listener;
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

#endif  // RMW_GURUMDDS_SHARED_CPP__TYPES_HPP_

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

#include <chrono>
#include <string>
#include <utility>

#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/qos.hpp"

#include "tracetools/tracetools.h"

#include "rmw_gurumdds_cpp/event_converter.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/event_info_service.hpp"

#include "rmw_gurumdds_cpp/type_support.hpp"
#include "rmw_gurumdds_cpp/type_support_service.hpp"

extern "C"
{
rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(service_name, nullptr);
  if (strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("client topic is empty");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);

  // Adapt any 'best available' QoS options
  rmw_qos_profile_t adapted_qos_policies =
    rmw_dds_common::qos_profile_update_best_available_for_services(*qos_policies);

  if (!adapted_qos_policies.avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(service_name, &validation_result, nullptr);
    if (ret != RMW_RET_OK) {
      return nullptr;
    }
    if (validation_result != RMW_TOPIC_VALID) {
      const char * reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service name is invalid: %s", reason);
      return nullptr;
    }
  }

  const rosidl_service_type_support_t * type_support =
    get_service_typesupport_handle(type_supports, rosidl_typesupport_introspection_c__identifier);
  if (type_support == nullptr) {
    rcutils_reset_error();
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (type_support == nullptr) {
      rcutils_reset_error();
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  rmw_context_impl_t * ctx = node->context->impl;
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  rmw_gurumdds_cpp::ServiceInfo * service_info = nullptr;
  rmw_service_t * rmw_service = nullptr;

  dds_DomainParticipant * participant = ctx->participant;
  dds_Publisher * publisher = ctx->publisher;
  dds_Subscriber * subscriber = ctx->subscriber;

  dds_DataReaderQos datareader_qos{};
  dds_DataWriterQos datawriter_qos{};

  dds_DataReader * request_reader = nullptr;
  dds_DataReaderListener request_listener;
  dds_DataSeq* data_seq = nullptr;
  dds_SampleInfoSeq* info_seq = nullptr;
  dds_UnsignedLongSeq* raw_data_sizes = nullptr;
  dds_DataWriter * response_writer = nullptr;
  dds_ReadCondition * read_condition = nullptr;
  dds_TypeSupport * request_typesupport = nullptr;
  dds_TypeSupport * response_typesupport = nullptr;

  dds_TopicDescription * topic_desc = nullptr;
  dds_Topic * request_topic = nullptr;
  dds_Topic * response_topic = nullptr;

  dds_ReturnCode_t ret;

  std::pair<std::string, std::string> service_type_name;
  std::pair<std::string, std::string> service_metastring;
  std::string request_topic_name;
  std::string response_topic_name;
  std::string request_type_name;
  std::string response_type_name;
  std::string request_metastring;
  std::string response_metastring;
  std::string writer_profile_name;
  std::string reader_profile_name;

  const rosidl_type_hash_t* type_hash;

  // Create topic and type name strings
  service_type_name =
    rmw_gurumdds_cpp::create_service_type_name(type_support->data, type_support->typesupport_identifier);
  request_type_name = service_type_name.first;
  response_type_name = service_type_name.second;
  if (request_type_name.empty() || response_type_name.empty()) {
    RMW_SET_ERROR_MSG("failed to create type name");
    return nullptr;
  }

  writer_profile_name = service_name;
  writer_profile_name += "Reply";

  reader_profile_name = service_name;
  reader_profile_name += "Request";

  request_topic_name.reserve(256);
  response_topic_name.reserve(256);
  request_topic_name = rmw_gurumdds_cpp::create_topic_name(
    rmw_gurumdds_cpp::ros_service_requester_prefix, service_name, "Request", &adapted_qos_policies);
  response_topic_name = rmw_gurumdds_cpp::create_topic_name(
    rmw_gurumdds_cpp::ros_service_response_prefix, service_name, "Reply", &adapted_qos_policies);

  service_metastring =
    rmw_gurumdds_cpp::create_service_metastring(type_support->data, type_support->typesupport_identifier);
  request_metastring = service_metastring.first;
  response_metastring = service_metastring.second;
  if (request_metastring.empty() || response_metastring.empty()) {
    RMW_SET_ERROR_MSG("failed to create metastring");
    return nullptr;
  }

  request_typesupport = dds_TypeSupport_create(request_metastring.c_str());
  if (request_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("failed to create typesupport");
    goto fail;
  }

  ret =
    dds_TypeSupport_register_type(request_typesupport, participant, request_type_name.c_str());
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register type");
    goto fail;
  }

  response_typesupport = dds_TypeSupport_create(response_metastring.c_str());
  if (response_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("failed to create typesupport");
    goto fail;
  }

  ret =
    dds_TypeSupport_register_type(response_typesupport, participant, response_type_name.c_str());
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register type");
    goto fail;
  }

  // Create topics

  // Look for request topic
  topic_desc =
    dds_DomainParticipant_lookup_topicdescription(participant, request_topic_name.c_str());
  if (topic_desc == nullptr) {
    dds_TopicQos topic_qos;
    ret = dds_DomainParticipant_get_default_topic_qos(participant, &topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      goto fail;
    }

    request_topic = dds_DomainParticipant_create_topic(
      participant, request_topic_name.c_str(), request_type_name.c_str(), &topic_qos, nullptr, 0);
    if (request_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      dds_TopicQos_finalize(&topic_qos);
      goto fail;
    }

    ret = dds_TopicQos_finalize(&topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to finalize topic qos");
      goto fail;
    }
  } else {
    dds_Duration_t timeout;
    timeout.sec = 0;
    timeout.nanosec = 1;
    request_topic = dds_DomainParticipant_find_topic(
      participant, request_topic_name.c_str(), &timeout);
    if (request_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      goto fail;
    }
  }

  // Look for response topic
  topic_desc =
    dds_DomainParticipant_lookup_topicdescription(participant, response_topic_name.c_str());
  if (topic_desc == nullptr) {
    dds_TopicQos topic_qos;
    ret = dds_DomainParticipant_get_default_topic_qos(participant, &topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      goto fail;
    }

    response_topic = dds_DomainParticipant_create_topic(
      participant, response_topic_name.c_str(), response_type_name.c_str(), &topic_qos, nullptr, 0);
    if (response_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      dds_TopicQos_finalize(&topic_qos);
      goto fail;
    }

    ret = dds_TopicQos_finalize(&topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to finalize topic qos");
      goto fail;
    }
  } else {
    dds_Duration_t timeout;
    timeout.sec = 0;
    timeout.nanosec = 1;
    response_topic =
      dds_DomainParticipant_find_topic(participant, response_topic_name.c_str(), &timeout);
    if (response_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      goto fail;
    }
  }

  ret = dds_DomainParticipantFactory_get_datareader_qos_from_profile(reader_profile_name.c_str(), &datareader_qos);
  if(ret != dds_RETCODE_OK) {
    ret = dds_Subscriber_get_default_datareader_qos(subscriber, &datareader_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default datareader qos");
      return nullptr;
    }
  }

  type_hash = type_support->request_typesupport->get_type_hash_func(type_support->request_typesupport);
  if (!rmw_gurumdds_cpp::get_datareader_qos(&adapted_qos_policies, *type_hash, &datareader_qos)) {
    // Error message already set
    goto fail;
  }

  request_reader = dds_Subscriber_create_datareader(
    subscriber, request_topic, &datareader_qos, nullptr, 0);
  if (request_reader == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datareader");
    dds_DataReaderQos_finalize(&datareader_qos);
    goto fail;
  }

  ret = dds_DataReaderQos_finalize(&datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datareader qos");
    goto fail;
  }

  read_condition = dds_DataReader_create_readcondition(
    request_reader, dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);
  if (read_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create read condition");
    goto fail;
  }

  ret = dds_Publisher_get_default_datawriter_qos(publisher, &datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datawriter qos");
    return nullptr;
  }

  ret = dds_DomainParticipantFactory_get_datawriter_qos_from_profile(writer_profile_name.c_str(), &datawriter_qos);
  if(ret != dds_RETCODE_OK) {
    ret = dds_Publisher_get_default_datawriter_qos(publisher, &datawriter_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default datawriter qos");
      return nullptr;
    }
  }

  type_hash = type_support->response_typesupport->get_type_hash_func(type_support->response_typesupport);
  if (!rmw_gurumdds_cpp::get_datawriter_qos(&adapted_qos_policies, *type_hash, &datawriter_qos)) {
    // Error message already set
    goto fail;
  }

  response_writer = dds_Publisher_create_datawriter(
    publisher, response_topic, &datawriter_qos, nullptr, 0);
  if (response_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    dds_DataWriterQos_finalize(&datawriter_qos);
    goto fail;
  }

  ret = dds_DataWriterQos_finalize(&datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datawriter qos");
    goto fail;
  }

  service_info = new(std::nothrow) rmw_gurumdds_cpp::ServiceInfo();
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate ServiceInfo");
    goto fail;
  }

  data_seq = dds_DataSeq_create(1);
  if (nullptr == data_seq) {
    RMW_SET_ERROR_MSG("failed to allocate data_seq");
    return nullptr;
  }
  info_seq = dds_SampleInfoSeq_create(1);
  if (nullptr == info_seq) {
    RMW_SET_ERROR_MSG("failed to allocate info_seq");
    return nullptr;
  }
  raw_data_sizes = dds_UnsignedLongSeq_create(1);
  if (nullptr == raw_data_sizes) {
    RMW_SET_ERROR_MSG("failed to allocate raw_data_sizes");
    return nullptr;
  }

  dds_DataReader_set_listener_context(request_reader, service_info);
  request_listener.on_data_available = [](const dds_DataReader * request_reader){
    dds_DataReader* reader = const_cast<dds_DataReader*>(request_reader);
    rmw_gurumdds_cpp::ServiceInfo* info = static_cast<rmw_gurumdds_cpp::ServiceInfo*>(dds_DataReader_get_listener_context(reader));
    std::lock_guard<std::mutex> guard(info->event_callback_data.mutex);
    if(info->event_callback_data.callback) {
      info->event_callback_data.callback(info->event_callback_data.user_data, info->count_unread());
    }
  };

  service_info->response_writer = response_writer;
  service_info->request_reader = request_reader;
  service_info->read_condition = read_condition;
  service_info->request_listener = request_listener;
  service_info->data_seq = data_seq;
  service_info->info_seq = info_seq;
  service_info->raw_data_sizes = raw_data_sizes;
  service_info->implementation_identifier = RMW_GURUMDDS_ID;
  service_info->service_typesupport = type_support;
  service_info->ctx = ctx;

  rmw_gurumdds_cpp::entity_get_gid(
    reinterpret_cast<dds_Entity *>(service_info->response_writer),
    service_info->publisher_gid);
  rmw_gurumdds_cpp::entity_get_gid(
    reinterpret_cast<dds_Entity *>(service_info->request_reader),
    service_info->subscriber_gid);

  rmw_service = rmw_service_allocate();
  if (rmw_service == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service");
    goto fail;
  }
  std::memset(rmw_service, 0, sizeof(rmw_service_t));
  rmw_service->implementation_identifier = RMW_GURUMDDS_ID;
  rmw_service->data = service_info;
  rmw_service->service_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(service_name) + 1));
  if (rmw_service->service_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service name");
    goto fail;
  }
  std::memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  rmw_gurumdds_cpp::set_service_typesupport(response_writer, request_reader, type_support);
  if (rmw_gurumdds_cpp::graph_cache::on_service_created(ctx, node, service_info) != RMW_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to update graph for service creation");
    goto fail;
  }

  dds_TypeSupport_delete(request_typesupport);
  request_typesupport = nullptr;
  dds_TypeSupport_delete(response_typesupport);
  response_typesupport = nullptr;

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "Created server with service '%s' on node '%s%s%s'",
    service_name, node->namespace_,
    node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

  return rmw_service;

fail:
  if (rmw_service != nullptr) {
    if (rmw_service->service_name != nullptr) {
      rmw_free(const_cast<char *>(rmw_service->service_name));
    }
    rmw_service_free(rmw_service);
  }

  if (request_reader != nullptr) {
    if (read_condition != nullptr) {
      dds_DataReader_delete_readcondition(request_reader, read_condition);
    }
    dds_Subscriber_delete_datareader(subscriber, request_reader);
  }

  if (response_writer != nullptr) {
    dds_Publisher_delete_datawriter(publisher, response_writer);
  }

  if (request_topic != nullptr) {
    dds_DomainParticipant_delete_topic(participant, request_topic);
  }

  if (response_topic != nullptr) {
    dds_DomainParticipant_delete_topic(participant, response_topic);
  }

  if (request_typesupport != nullptr) {
    dds_TypeSupport_delete(request_typesupport);
  }

  if (response_typesupport != nullptr) {
    dds_TypeSupport_delete(response_typesupport);
  }

  delete service_info;

  return nullptr;
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  dds_ReturnCode_t ret;
  rmw_context_impl_t * ctx = node->context->impl;
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  rmw_gurumdds_cpp::ServiceInfo * service_info = static_cast<rmw_gurumdds_cpp::ServiceInfo *>(service->data);
  if (service_info != nullptr) {
    if (service_info->response_writer != nullptr) {
      ret = dds_Publisher_delete_datawriter(
        ctx->publisher, service_info->response_writer);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete datawriter");
        return RMW_RET_ERROR;
      }
    }

    dds_DataSeq_delete(service_info->data_seq);
    dds_SampleInfoSeq_delete(service_info->info_seq);
    dds_UnsignedLongSeq_delete(service_info->raw_data_sizes);

    if (service_info->request_reader != nullptr) {
      if (service_info->read_condition != nullptr) {
        ret = dds_DataReader_delete_readcondition(
          service_info->request_reader, service_info->read_condition);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete readcondition");
          return RMW_RET_ERROR;
        }
      }
      ret = dds_Subscriber_delete_datareader(
        ctx->subscriber, service_info->request_reader);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete datareader");
        return RMW_RET_ERROR;
      }
    }

    if (rmw_gurumdds_cpp::graph_cache::on_service_deleted(ctx, node, service_info) != RMW_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to update graph for service deletion");
      return RMW_RET_ERROR;
    }

    delete service_info;
    service->data = nullptr;
  }

  if (service->service_name != nullptr) {
    RCUTILS_LOG_DEBUG_NAMED(
      RMW_GURUMDDS_ID,
      "Deleted server with service '%s' on node '%s%s%s'",
      service->service_name, node->namespace_,
      node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);
    rmw_free(const_cast<char *>(service->service_name));
  }
  rmw_service_free(service);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_service_response_publisher_get_actual_qos(
  const rmw_service_t * service,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  rmw_gurumdds_cpp::ServiceInfo * service_info = static_cast<rmw_gurumdds_cpp::ServiceInfo *>(service->data);
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("service info is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * response_writer = service_info->response_writer;
  if (response_writer == nullptr) {
    RMW_SET_ERROR_MSG("response writer is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriterQos dds_qos;
  dds_ReturnCode_t ret = dds_DataWriter_get_qos(response_writer, &dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("publisher can't get data writer qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability = rmw_gurumdds_cpp::convert_reliability(&dds_qos.reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos.durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos.deadline);
  qos->lifespan = rmw_gurumdds_cpp::convert_lifespan(&dds_qos.lifespan);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos.liveliness);
  qos->liveliness_lease_duration = rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos.liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos.history);
  qos->depth = static_cast<size_t>(dds_qos.history.depth);

  ret = dds_DataWriterQos_finalize(&dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datawriter qos");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_service_request_subscription_get_actual_qos(
  const rmw_service_t * service,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  rmw_gurumdds_cpp::ServiceInfo * service_info = static_cast<rmw_gurumdds_cpp::ServiceInfo *>(service->data);
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("service info is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader * request_reader = service_info->request_reader;
  if (request_reader == nullptr) {
    RMW_SET_ERROR_MSG("request reader is null");
    return RMW_RET_ERROR;
  }

  dds_DataReaderQos dds_qos;
  dds_ReturnCode_t ret = dds_DataReader_get_qos(request_reader, &dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("subscription can't get data reader qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability = rmw_gurumdds_cpp::convert_reliability(&dds_qos.reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos.durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos.deadline);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos.liveliness);
  qos->liveliness_lease_duration = rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos.liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos.history);
  qos->depth = static_cast<size_t>(dds_qos.history.depth);

  ret = dds_DataReaderQos_finalize(&dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datareader qos");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_request(
  const rmw_service_t * service,
  rmw_service_info_t * request_header,
  void * ros_request,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier, RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  rmw_gurumdds_cpp::ServiceInfo * service_info = static_cast<rmw_gurumdds_cpp::ServiceInfo *>(service->data);
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("service info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader * request_reader = service_info->request_reader;
  if (request_reader == nullptr) {
    RMW_SET_ERROR_MSG("request reader is null");
    return RMW_RET_ERROR;
  }

  auto type_support = service_info->service_typesupport;
  if (type_support == nullptr) {
    RMW_SET_ERROR_MSG("typesupport handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataSeq * data_values = dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoSeq * sample_infos = dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    dds_DataSeq_delete(data_values);
    return RMW_RET_ERROR;
  }

  dds_UnsignedLongSeq * sample_sizes = dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  if (service_info->ctx->service_mapping_basic) {
    dds_ReturnCode_t ret = dds_DataReader_raw_take(
      request_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, 1,
      dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

    if (ret == dds_RETCODE_NO_DATA) {
      dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_OK;
    }

    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to take data");
      dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }

    dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);
    if (sample_info->valid_data) {
      void * sample = dds_DataSeq_get(data_values, 0);
      if (sample == nullptr) {
        dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return RMW_RET_ERROR;
      }
      uint32_t size = dds_UnsignedLongSeq_get(sample_sizes, 0);
      int32_t sn_high = 0;
      uint32_t sn_low = 0;
      int8_t client_guid[16] = {0};
      dds_SampleInfoEx * sampleinfo_ex = reinterpret_cast<dds_SampleInfoEx *>(sample_info);

      bool res = rmw_gurumdds_cpp::deserialize_request_basic(
        type_support->data,
        type_support->typesupport_identifier,
        ros_request,
        sample,
        static_cast<size_t>(size),
        &sn_high,
        &sn_low,
        client_guid
      );

      if (!res) {
        // Error message already set
        dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return RMW_RET_ERROR;
      }

      request_header->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;
      request_header->received_timestamp =
        sampleinfo_ex->reception_timestamp.sec * static_cast<int64_t>(1000000000) +
        sampleinfo_ex->reception_timestamp.nanosec;
      request_header->request_id.sequence_number = ((int64_t)sn_high) << 32 | sn_low;
      std::memcpy(request_header->request_id.writer_guid, client_guid, RMW_GID_STORAGE_SIZE);
    }

    dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
  } else {
    dds_ReturnCode_t ret = dds_DataReader_raw_take_w_sampleinfoex(
      request_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, 1,
      dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

    if (ret == dds_RETCODE_NO_DATA) {
      dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_OK;
    }

    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to take data");
      dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }

    dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);
    if (sample_info->valid_data) {
      void * sample = dds_DataSeq_get(data_values, 0);
      if (sample == nullptr) {
        dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return RMW_RET_ERROR;
      }
      uint32_t size = dds_UnsignedLongSeq_get(sample_sizes, 0);
      int64_t sequence_number = 0;
      int8_t client_guid[16] = {0};
      dds_SampleInfoEx * sampleinfo_ex = reinterpret_cast<dds_SampleInfoEx *>(sample_info);
      rmw_gurumdds_cpp::dds_guid_to_ros_guid(reinterpret_cast<int8_t *>(&sampleinfo_ex->src_guid), client_guid);
      rmw_gurumdds_cpp::dds_sn_to_ros_sn(sampleinfo_ex->seq, &sequence_number);

      bool res = rmw_gurumdds_cpp::deserialize_request_enhanced(
        type_support->data,
        type_support->typesupport_identifier,
        ros_request,
        sample,
        static_cast<size_t>(size)
      );

      if (!res) {
        // Error message already set
        dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
        dds_DataSeq_delete(data_values);
        dds_SampleInfoSeq_delete(sample_infos);
        dds_UnsignedLongSeq_delete(sample_sizes);
        return RMW_RET_ERROR;
      }

      request_header->source_timestamp =
        sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
        sample_info->source_timestamp.nanosec;
      request_header->received_timestamp =
        sampleinfo_ex->reception_timestamp.sec * static_cast<int64_t>(1000000000) +
        sampleinfo_ex->reception_timestamp.nanosec;
      request_header->request_id.sequence_number = sequence_number;
      std::memcpy(request_header->request_id.writer_guid, client_guid, RMW_GID_STORAGE_SIZE);
    }

    dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
  }

  *taken = true;

  TRACETOOLS_TRACEPOINT(
    rmw_take_request,
    static_cast<const void *>(service),
    static_cast<const void *>(ros_request),
    request_header->request_id.writer_guid,
    request_header->request_id.sequence_number,
    *taken);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier, RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);

  rmw_gurumdds_cpp::ServiceInfo * service_info = static_cast<rmw_gurumdds_cpp::ServiceInfo *>(service->data);
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("service info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * response_writer = service_info->response_writer;
  if (response_writer == nullptr) {
    RMW_SET_ERROR_MSG("response writer is null");
    return RMW_RET_ERROR;
  }

  auto type_support = service_info->service_typesupport;
  if (type_support == nullptr) {
    RMW_SET_ERROR_MSG("typesupport handle is null");
    return RMW_RET_ERROR;
  }

  size_t size = 0;

  if (service_info->ctx->service_mapping_basic) {
    void * dds_response = rmw_gurumdds_cpp::allocate_response_basic(
      type_support->data,
      type_support->typesupport_identifier,
      ros_response,
      &size
    );

    if (dds_response == nullptr) {
      return RMW_RET_ERROR;
    }

    bool res = rmw_gurumdds_cpp::serialize_response_basic(
      type_support->data,
      type_support->typesupport_identifier,
      ros_response,
      dds_response,
      size,
      request_header->sequence_number,
      request_header->writer_guid
    );

    if (!res) {
      RMW_SET_ERROR_MSG("failed to serialize message");
      free(dds_response);
      return RMW_RET_ERROR;
    }

    if (dds_DataWriter_raw_write(response_writer, dds_response, size) != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to publish data");
      free(dds_response);
      return RMW_RET_ERROR;
    }
    free(dds_response);
  } else {
    dds_SampleInfoEx sampleinfo_ex{};
    rmw_gurumdds_cpp::ros_sn_to_dds_sn(request_header->sequence_number, &sampleinfo_ex.seq);
    rmw_gurumdds_cpp::ros_guid_to_dds_guid(
      request_header->writer_guid,
      reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

    dds_Time_get_current_time(&sampleinfo_ex.info.source_timestamp);
    TRACETOOLS_TRACEPOINT(
      rmw_send_response,
      static_cast<const void *>(service),
      static_cast<const void *>(ros_response),
      request_header->writer_guid,
      request_header->sequence_number,
      rmw_gurumdds_cpp::dds_time_to_i64(sampleinfo_ex.info.source_timestamp));

    if (dds_DataWriter_write_w_sampleinfoex(
        response_writer, ros_response, &sampleinfo_ex) != dds_RETCODE_OK)
    {
      RMW_SET_ERROR_MSG("failed to send response");
      return RMW_RET_ERROR;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_service_set_on_new_request_callback(
  rmw_service_t * rmw_service,
  rmw_event_callback_t callback,
  const void * user_data)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    rmw_service,
    rmw_service->implementation_identifier,
    RMW_GURUMDDS_ID,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  auto service_info = static_cast<rmw_gurumdds_cpp::ServiceInfo *>(rmw_service->data);
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("invalid service data");
    return RMW_RET_ERROR;
  }

  std::lock_guard<std::mutex> guard(service_info->event_callback_data.mutex);
  dds_StatusMask mask = dds_DataReader_get_status_changes(service_info->request_reader);
  dds_ReturnCode_t dds_rc = dds_RETCODE_ERROR;

  if (callback) {
    size_t unread_count = service_info->count_unread();
    if (0 < unread_count) {
      callback(user_data, unread_count);
    }

    service_info->event_callback_data.callback = callback;
    service_info->event_callback_data.user_data = user_data;
    mask |= dds_DATA_AVAILABLE_STATUS;
    dds_rc = dds_DataReader_set_listener(service_info->request_reader, &service_info->request_listener, mask);
  } else {
    service_info->event_callback_data.callback = nullptr;
    service_info->event_callback_data.user_data = nullptr;
    mask &= ~dds_DATA_AVAILABLE_STATUS;
    dds_rc = dds_DataReader_set_listener(service_info->request_reader, &service_info->request_listener, mask);
  }

  return rmw_gurumdds_cpp::check_dds_ret_code(dds_rc);
}
}  // extern "C"

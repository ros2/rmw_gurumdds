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
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/qos.hpp"

#include "tracetools/tracetools.h"

#include "rmw_gurumdds_cpp/event_converter.hpp"
#include "rmw_gurumdds_cpp/event_info_common.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/raii.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/type_support.hpp"
#include "rmw_gurumdds_cpp/type_support_service.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"

extern "C" {
rmw_client_t * rmw_create_client(
  const rmw_node_t *node, const rosidl_service_type_support_t *type_supports,
  const char *service_name, const rmw_qos_profile_t *qos_policies)
{
  CHECK_ALL_PTRS_NULL(node, type_supports, service_name, qos_policies);
  CHECK_ID_NULL(node);

  if (strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("client topic is empty");
    return nullptr;
  }

  if (!rmw_gurumdds_cpp::is_valid_qos(qos_policies)) {
    return nullptr;
  }

  const rosidl_service_type_support_t *type_support =
    get_service_typesupport_handle(
          type_supports, rosidl_typesupport_introspection_c__identifier);
  if (type_support == nullptr) {
    rcutils_reset_error();
    type_support = get_service_typesupport_handle(
        type_supports,
        rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (type_support == nullptr) {
      rcutils_reset_error();
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  // Adapt any 'best available' QoS options
  rmw_qos_profile_t adapted_qos_policies =
    rmw_dds_common::qos_profile_update_best_available_for_services(
          *qos_policies);

  if (!adapted_qos_policies.avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret =
      rmw_validate_full_topic_name(service_name, &validation_result, nullptr);
    if (ret != RMW_RET_OK) {
      return nullptr;
    }
    if (validation_result != RMW_TOPIC_VALID) {
      const char *reason =
        rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service name is invalid: %s",
                                           reason);
      return nullptr;
    }
  }

  rmw_context_impl_t *ctx = node->context->impl;
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  rmw_gurumdds_cpp::ClientInfo *client_info = nullptr;
  rmw_client_t *rmw_client = nullptr;

  dds_DomainParticipant *participant = ctx->participant;
  dds_Publisher *publisher = ctx->publisher;
  dds_Subscriber *subscriber = ctx->subscriber;

  raii::dds_DataReaderQos datareader_qos;
  raii::dds_DataWriterQos datawriter_qos;

  dds_DataWriter *request_writer = nullptr;
  dds_DataReader *response_reader = nullptr;
  dds_DataReaderListener response_listener;

  dds_ReadCondition *read_condition = nullptr;
  raii::dds_TypeSupport request_typesupport;
  raii::dds_TypeSupport response_typesupport;

  dds_TopicDescription *topic_desc = nullptr;
  dds_Topic *request_topic = nullptr;
  dds_Topic *response_topic = nullptr;

  raii::dds_DataSeq data_seq;
  raii::dds_SampleInfoSeq info_seq;
  raii::dds_UnsignedLongSeq raw_data_sizes;

  uint8_t client_guid[16] = {0};
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
  const rosidl_type_hash_t *type_hash;
  const rosidl_type_hash_t *client_type_hash;
  const rosidl_message_type_support_t *req_typesupport;
  const rosidl_message_type_support_t *res_typesupport;

  client_type_hash = type_supports->get_type_hash_func(type_supports);

  // Create topic and type name strings
  service_type_name = rmw_gurumdds_cpp::create_service_type_name(
      type_support->data, type_support->typesupport_identifier);
  request_type_name = service_type_name.first;
  response_type_name = service_type_name.second;
  if (request_type_name.empty() || response_type_name.empty()) {
    RMW_SET_ERROR_MSG("failed to create type name");
    goto fail;
  }

  writer_profile_name = service_name;
  writer_profile_name += "Request";

  reader_profile_name = service_name;
  reader_profile_name += "Reply";

  request_topic_name.reserve(256);
  response_topic_name.reserve(256);
  request_topic_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_service_requester_prefix, service_name, "Request",
      &adapted_qos_policies);
  response_topic_name = rmw_gurumdds_cpp::create_topic_name(
      rmw_gurumdds_cpp::ros_service_response_prefix, service_name, "Reply",
      &adapted_qos_policies);

  service_metastring = rmw_gurumdds_cpp::create_service_metastring(
      type_support->data, type_support->typesupport_identifier);
  request_metastring = service_metastring.first;
  response_metastring = service_metastring.second;
  if (request_metastring.empty() || response_metastring.empty()) {
    RMW_SET_ERROR_MSG("failed to create metastring");
    goto fail;
  }

  request_typesupport = dds_TypeSupport_create(request_metastring.c_str());
  if (request_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("failed to create typesupport");
    goto fail;
  }

  ret = dds_TypeSupport_register_type(request_typesupport, participant,
                                      request_type_name.c_str());
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register type");
    goto fail;
  }

  response_typesupport = dds_TypeSupport_create(response_metastring.c_str());
  if (response_typesupport == nullptr) {
    RMW_SET_ERROR_MSG("failed to create typesupport");
    goto fail;
  }

  ret = dds_TypeSupport_register_type(response_typesupport, participant,
                                      response_type_name.c_str());
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register type");
    goto fail;
  }

  // Create topics

  // Look for request topic
  topic_desc = dds_DomainParticipant_lookup_topicdescription(
      participant, request_topic_name.c_str());
  if (topic_desc == nullptr) {
    raii::dds_TopicQos topic_qos;
    ret = raii::dds_DomainParticipant_get_default_topic_qos(participant,
                                                            topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      goto fail;
    }

    request_topic = dds_DomainParticipant_create_topic(
        participant, request_topic_name.c_str(), request_type_name.c_str(),
        topic_qos, nullptr, 0);
    if (request_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      // dds_TopicQos_finalize(&topic_qos);
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
  topic_desc = dds_DomainParticipant_lookup_topicdescription(
      participant, response_topic_name.c_str());
  if (topic_desc == nullptr) {
    raii::dds_TopicQos topic_qos;
    ret = raii::dds_DomainParticipant_get_default_topic_qos(participant,
                                                            topic_qos);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      goto fail;
    }

    response_topic = dds_DomainParticipant_create_topic(
        participant, response_topic_name.c_str(), response_type_name.c_str(),
        topic_qos, nullptr, 0);
    if (response_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to create topic");
      // dds_TopicQos_finalize(&topic_qos);
      goto fail;
    }

  } else {
    dds_Duration_t timeout;
    timeout.sec = 0;
    timeout.nanosec = 1;
    response_topic = dds_DomainParticipant_find_topic(
        participant, response_topic_name.c_str(), &timeout);
    if (response_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      goto fail;
    }
  }

  ret =
    raii::dds_Publisher_get_default_datawriter_qos(publisher, datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datawriter qos");
    goto fail;
  }

  // Create datawriter for request
  req_typesupport = type_support->request_typesupport;
  type_hash = req_typesupport->get_type_hash_func(req_typesupport);
  if (!rmw_gurumdds_cpp::get_datawriter_qos(&adapted_qos_policies, *type_hash,
                                            datawriter_qos,
                                            *client_type_hash))
  {
    goto fail;
  }

  request_writer = dds_Publisher_create_datawriter(publisher, request_topic,
                                                   datawriter_qos, nullptr, 0);
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    goto fail;
  }

  ret = raii::dds_Subscriber_get_default_datareader_qos(subscriber,
                                                        datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datareader qos");
    goto fail;
  }

  res_typesupport = type_support->response_typesupport;
  type_hash = res_typesupport->get_type_hash_func(res_typesupport);
  if (!rmw_gurumdds_cpp::get_datareader_qos(&adapted_qos_policies, *type_hash,
                                            datareader_qos,
                                            *client_type_hash))
  {
    // error message already set
    goto fail;
  }

  response_reader = dds_Subscriber_create_datareader(
      subscriber, response_topic, datareader_qos, nullptr, 0);
  if (response_reader == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datareader");
    goto fail;
  }

  read_condition = dds_DataReader_create_readcondition(
      response_reader, dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE,
      dds_ANY_INSTANCE_STATE);
  if (read_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create read condition");
    goto fail;
  }

  client_info = new (std::nothrow) rmw_gurumdds_cpp::ClientInfo();
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate ClientInfo");
    goto fail;
  }

  data_seq = raii::dds_DataSeq_create(1);
  if (nullptr == data_seq) {
    RMW_SET_ERROR_MSG("failed to allocate data_seq");
    goto fail;
  }
  info_seq = raii::dds_SampleInfoSeq_create(1);
  if (nullptr == info_seq) {
    RMW_SET_ERROR_MSG("failed to allocate info_seq");
    goto fail;
  }
  raw_data_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (nullptr == raw_data_sizes) {
    RMW_SET_ERROR_MSG("failed to allocate raw_data_sizes");
    goto fail;
  }

  dds_DataReader_set_listener_context(response_reader, client_info);
  response_listener.on_data_available =
    [](const dds_DataReader *response_reader) {
      auto *reader = const_cast<dds_DataReader *>(response_reader);
      auto *info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(
        dds_DataReader_get_listener_context(reader));
      std::lock_guard<std::mutex> guard(info->event_callback_data.mutex);
      if (info->event_callback_data.callback) {
        info->event_callback_data.callback(
              info->event_callback_data.user_data, info->count_unread());
      }
    };

  client_info->request_writer = request_writer;
  client_info->response_reader = response_reader;
  client_info->read_condition = read_condition;
  client_info->response_listener = response_listener;
  client_info->data_seq = data_seq;
  client_info->info_seq = info_seq;
  client_info->raw_data_sizes = raw_data_sizes;
  client_info->implementation_identifier = RMW_GURUMDDS_ID;
  client_info->service_typesupport = type_support;
  client_info->sequence_number = 0;
  client_info->ctx = ctx;

  // Set GUID
  dds_DataWriter_get_guid(request_writer, client_guid);
  std::memcpy(client_info->writer_guid, client_guid, sizeof(client_guid));

  rmw_gurumdds_cpp::entity_get_gid(
      reinterpret_cast<dds_Entity *>(client_info->request_writer),
      client_info->publisher_gid);
  rmw_gurumdds_cpp::entity_get_gid(
      reinterpret_cast<dds_Entity *>(client_info->response_reader),
      client_info->subscriber_gid);

  rmw_client = rmw_client_allocate();
  if (rmw_client == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client");
    goto fail;
  }
  std::memset(rmw_client, 0, sizeof(rmw_client_t));

  rmw_client->implementation_identifier = RMW_GURUMDDS_ID;
  rmw_client->data = client_info;
  rmw_client->service_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(service_name) + 1));
  if (rmw_client->service_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client name");
    goto fail;
  }
  std::memcpy(const_cast<char *>(rmw_client->service_name), service_name,
              strlen(service_name) + 1);

  rmw_gurumdds_cpp::set_client_typesupport(request_writer, response_reader,
                                           type_support);
  if (rmw_gurumdds_cpp::graph_cache::on_client_created(
          ctx, node, client_info) != RMW_RET_OK)
  {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID,
                            "failed to update graph for client creation");
    goto fail;
  }

  RCUTILS_LOG_DEBUG_NAMED(
      RMW_GURUMDDS_ID, "Created client with service '%s' on node '%s%s%s'",
      service_name, node->namespace_,
      node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/",
      node->name);

  if (TRACETOOLS_TRACEPOINT_ENABLED(rmw_client_init)) {
    TRACETOOLS_DO_TRACEPOINT(rmw_client_init,
                             static_cast<const void *>(rmw_client),
                             client_info->subscriber_gid.data);
  }

  return rmw_client;

fail:
  if (request_writer != nullptr) {
    dds_Publisher_delete_datawriter(publisher, request_writer);
  }

  if (response_reader != nullptr) {
    if (read_condition != nullptr) {
      dds_DataReader_delete_readcondition(response_reader, read_condition);
    }
    dds_Subscriber_delete_datareader(subscriber, response_reader);
  }

  if (request_topic != nullptr) {
    dds_DomainParticipant_delete_topic(participant, request_topic);
  }

  if (response_topic != nullptr) {
    dds_DomainParticipant_delete_topic(participant, response_topic);
  }

  if (client_info != nullptr) {
    delete client_info;
  }

  if (rmw_client != nullptr) {
    if (rmw_client->service_name != nullptr) {
      rmw_free(const_cast<char *>(rmw_client->service_name));
    }
    rmw_client_free(rmw_client);
  }

  return nullptr;
}

rmw_ret_t rmw_destroy_client(rmw_node_t *node, rmw_client_t *client)
{
  CHECK_ALL_PTRS_CODE(node, client);
  CHECK_ID_CODE(node);
  CHECK_ID_CODE(client);

  dds_ReturnCode_t ret;
  rmw_context_impl_t *ctx = node->context->impl;
  std::lock_guard<std::mutex> guard(ctx->endpoint_mutex);

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);

  if (client_info != nullptr) {
    if (client_info->request_writer != nullptr) {
      ret = dds_Publisher_delete_datawriter(ctx->publisher,
                                            client_info->request_writer);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete datawriter");
        return RMW_RET_ERROR;
      }
    }

    if (client_info->response_reader != nullptr) {
      if (client_info->read_condition != nullptr) {
        ret = dds_DataReader_delete_readcondition(client_info->response_reader,
                                                  client_info->read_condition);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete readcondition");
          return RMW_RET_ERROR;
        }
      }
      ret = dds_Subscriber_delete_datareader(ctx->subscriber,
                                             client_info->response_reader);
      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete datareader");
        return RMW_RET_ERROR;
      }
    }

    if (rmw_gurumdds_cpp::graph_cache::on_client_deleted(
            ctx, node, client_info) != RMW_RET_OK)
    {
      RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID,
                              "failed to update graph for client deletion");
      return RMW_RET_ERROR;
    }

    delete client_info;
    client->data = nullptr;
  }

  if (client->service_name != nullptr) {
    RCUTILS_LOG_DEBUG_NAMED(
        RMW_GURUMDDS_ID, "Deleted client with service '%s' on node '%s%s%s'",
        client->service_name, node->namespace_,
        node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/",
        node->name);
    rmw_free(const_cast<char *>(client->service_name));
  }

  rmw_client_free(client);

  return RMW_RET_OK;
}

rmw_ret_t rmw_service_server_is_available(
  const rmw_node_t *node,
  const rmw_client_t *client,
  bool *is_available)
{
  CHECK_ALL_PTRS_CODE(node, client, is_available);
  CHECK_ID_CODE(node);
  CHECK_ID_CODE(client);

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter *request_writer = client_info->request_writer;
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("request writer is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader *response_reader = client_info->response_reader;
  if (response_reader == nullptr) {
    RMW_SET_ERROR_MSG("response reader is null");
    return RMW_RET_ERROR;
  }

  *is_available = false;

  raii::dds_InstanceHandleSeq seq = raii::dds_InstanceHandleSeq_create(4);
  if (seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_ERROR;
  }

  // Look for matching request reader
  if (dds_DataWriter_get_matched_subscriptions(request_writer, seq) !=
    dds_RETCODE_OK)
  {
    RMW_SET_ERROR_MSG("failed to get matched subscriptions");
    dds_InstanceHandleSeq_delete(seq);
    return RMW_RET_ERROR;
  }

  auto sub_cnt = dds_InstanceHandleSeq_length(seq);
  dds_InstanceHandleSeq_delete(seq);

  // Matching request reader not found
  if (sub_cnt == 0) {
    return RMW_RET_OK;
  }

  seq = raii::dds_InstanceHandleSeq_create(4);
  if (seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_ERROR;
  }

  // Look for matching response writer
  if (dds_DataReader_get_matched_publications(response_reader, seq) !=
    dds_RETCODE_OK)
  {
    RMW_SET_ERROR_MSG("failed to get matched publications");
    dds_InstanceHandleSeq_delete(seq);
    return RMW_RET_ERROR;
  }

  auto pub_cnt = dds_InstanceHandleSeq_length(seq);
  dds_InstanceHandleSeq_delete(seq);

  // Matching response writer not found
  if (pub_cnt == 0) {
    return RMW_RET_OK;
  }

  *is_available = true;

  return RMW_RET_OK;
}

rmw_ret_t rmw_get_gid_for_client(const rmw_client_t *client, rmw_gid_t *gid)
{
  CHECK_ALL_PTRS_CODE(client, gid);
  CHECK_ID_CODE(client);

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info is null");
    return RMW_RET_ERROR;
  }

  *gid = client_info->publisher_gid;
  gid->implementation_identifier = RMW_GURUMDDS_ID;

  return RMW_RET_OK;
}

rmw_ret_t
rmw_client_request_publisher_get_actual_qos(
  const rmw_client_t *client,
  rmw_qos_profile_t *qos)
{
  CHECK_ALL_PTRS_CODE(client, qos);
  CHECK_ID_CODE(client);

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter *request_writer = client_info->request_writer;
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("request writer is null");
    return RMW_RET_ERROR;
  }

  raii::dds_DataWriterQos dds_qos;
  dds_ReturnCode_t ret = raii::dds_DataWriter_get_qos(request_writer, dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("publisher can't get data writer qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability =
    rmw_gurumdds_cpp::convert_reliability(&dds_qos->reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos->durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos->deadline);
  qos->lifespan = rmw_gurumdds_cpp::convert_lifespan(&dds_qos->lifespan);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos->liveliness);
  qos->liveliness_lease_duration =
    rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos->liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos->history);
  qos->depth = static_cast<size_t>(dds_qos->history.depth);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_client_response_subscription_get_actual_qos(
  const rmw_client_t *client,
  rmw_qos_profile_t *qos)
{
  CHECK_ALL_PTRS_CODE(client, qos);
  CHECK_ID_CODE(client);

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader *response_reader = client_info->response_reader;
  if (response_reader == nullptr) {
    RMW_SET_ERROR_MSG("response reader is null");
    return RMW_RET_ERROR;
  }

  raii::dds_DataReaderQos dds_qos;
  dds_ReturnCode_t ret = raii::dds_DataReader_get_qos(response_reader, dds_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("subscription can't get data reader qos policies");
    return RMW_RET_ERROR;
  }

  qos->reliability =
    rmw_gurumdds_cpp::convert_reliability(&dds_qos->reliability);
  qos->durability = rmw_gurumdds_cpp::convert_durability(&dds_qos->durability);
  qos->deadline = rmw_gurumdds_cpp::convert_deadline(&dds_qos->deadline);
  qos->liveliness = rmw_gurumdds_cpp::convert_liveliness(&dds_qos->liveliness);
  qos->liveliness_lease_duration =
    rmw_gurumdds_cpp::convert_liveliness_lease_duration(&dds_qos->liveliness);
  qos->history = rmw_gurumdds_cpp::convert_history(&dds_qos->history);
  qos->depth = static_cast<size_t>(dds_qos->history.depth);

  return RMW_RET_OK;
}

rmw_ret_t rmw_send_request(
  const rmw_client_t *client, const void *ros_request,
  int64_t *sequence_id)
{
  CHECK_ALL_PTRS_CODE(client, ros_request, sequence_id);
  CHECK_ID_CODE(client);

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter *request_writer = client_info->request_writer;
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("request writer is null");
    return RMW_RET_ERROR;
  }

  auto type_support = client_info->service_typesupport;
  if (type_support == nullptr) {
    RMW_SET_ERROR_MSG("typesupport handle is null");
    return RMW_RET_ERROR;
  }

  size_t size = 0;

  if (client_info->ctx->service_mapping_basic) {
    void *dds_request = rmw_gurumdds_cpp::allocate_request_basic(
        type_support->data, type_support->typesupport_identifier, ros_request,
        &size);

    if (dds_request == nullptr) {
      return RMW_RET_ERROR;
    }

    bool res = rmw_gurumdds_cpp::serialize_request_basic(
        type_support->data, type_support->typesupport_identifier, ros_request,
        dds_request, size, ++client_info->sequence_number,
        client_info->writer_guid);

    if (!res) {
      RMW_SET_ERROR_MSG("failed to serialize message");
      free(dds_request);
      return RMW_RET_ERROR;
    }

    if (dds_DataWriter_raw_write(request_writer, dds_request, size) !=
      dds_RETCODE_OK)
    {
      RMW_SET_ERROR_MSG("failed to send request");
      free(dds_request);
      return RMW_RET_ERROR;
    }
    free(dds_request);
  } else {
    dds_SampleInfoEx sampleinfo_ex{};
    rmw_gurumdds_cpp::ros_sn_to_dds_sn(++client_info->sequence_number,
                                       &sampleinfo_ex.seq);
    rmw_gurumdds_cpp::ros_guid_to_dds_guid(
        reinterpret_cast<const uint8_t *>(client_info->writer_guid),
        reinterpret_cast<uint8_t *>(&sampleinfo_ex.src_guid));

    TRACETOOLS_TRACEPOINT(rmw_send_request, static_cast<const void *>(client),
                          static_cast<const void *>(ros_request), *sequence_id);

    if (dds_DataWriter_write_w_sampleinfoex(request_writer, ros_request,
                                            &sampleinfo_ex) != dds_RETCODE_OK)
    {
      RMW_SET_ERROR_MSG("failed to send request");
      return RMW_RET_ERROR;
    }
  }

  *sequence_id = client_info->sequence_number;

  return RMW_RET_OK;
}

rmw_ret_t rmw_take_response(
  const rmw_client_t *client,
  rmw_service_info_t *request_header,
  void *ros_response, bool *taken)
{
  CHECK_ALL_PTRS_CODE(client, request_header, ros_response, taken);
  CHECK_ID_CODE(client);

  *taken = false;

  auto client_info = static_cast<rmw_gurumdds_cpp::ClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader *response_reader = client_info->response_reader;
  if (response_reader == nullptr) {
    RMW_SET_ERROR_MSG("response reader is null");
    return RMW_RET_ERROR;
  }

  auto type_support = client_info->service_typesupport;
  if (type_support == nullptr) {
    RMW_SET_ERROR_MSG("typesupport handle is null");
    return RMW_RET_ERROR;
  }

  raii::dds_DataSeq data_values = raii::dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_SampleInfoSeq sample_infos = raii::dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    return RMW_RET_ERROR;
  }

  raii::dds_UnsignedLongSeq sample_sizes = raii::dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_RETCODE_OK;

  auto ret_loan = rcpputils::make_scope_exit([&]() {
        dds_DataReader_raw_return_loan(response_reader, data_values, sample_infos,
                                   sample_sizes);
  });

  if (client_info->ctx->service_mapping_basic) {
    while (ret == dds_RETCODE_OK) {
      ret = dds_DataReader_raw_take(response_reader, dds_HANDLE_NIL,
                                    data_values, sample_infos, sample_sizes, 1,
                                    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE,
                                    dds_ANY_INSTANCE_STATE);

      if (ret == dds_RETCODE_NO_DATA) {
        return RMW_RET_OK;
      }

      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to take data");
        return RMW_RET_ERROR;
      }

      dds_SampleInfo *sample_info = dds_SampleInfoSeq_get(sample_infos, 0);
      if (sample_info->valid_data) {
        void *sample = dds_DataSeq_get(data_values, 0);
        if (sample == nullptr) {
          return RMW_RET_ERROR;
        }
        uint32_t size = dds_UnsignedLongSeq_get(sample_sizes, 0);
        int32_t sn_high = 0;
        uint32_t sn_low = 0;
        int8_t client_guid[16] = {0};
        dds_SampleInfoEx *sampleinfo_ex =
          reinterpret_cast<dds_SampleInfoEx *>(sample_info);
        bool res = rmw_gurumdds_cpp::deserialize_response_basic(
            type_support->data, type_support->typesupport_identifier,
            ros_response, sample, static_cast<size_t>(size), &sn_high, &sn_low,
            client_guid);

        if (!res) {
          // Error message already set
          return RMW_RET_ERROR;
        }

        if (std::memcmp(client_info->writer_guid, client_guid,
                        RMW_GID_STORAGE_SIZE) == 0)
        {
          request_header->source_timestamp =
            sample_info->source_timestamp.sec *
            static_cast<int64_t>(1000000000) +
            sample_info->source_timestamp.nanosec;
          request_header->received_timestamp =
            sampleinfo_ex->reception_timestamp.sec *
            static_cast<int64_t>(1000000000) +
            sampleinfo_ex->reception_timestamp.nanosec;
          request_header->request_id.sequence_number =
            ((int64_t)sn_high) << 32 | sn_low;
          std::memcpy(request_header->request_id.writer_guid, client_guid,
                      RMW_GID_STORAGE_SIZE);

          *taken = true;
        }
      }

      if (*taken) {
        break;
      }
    }
  } else {
    while (ret == dds_RETCODE_OK) {
      ret = dds_DataReader_raw_take_w_sampleinfoex(
          response_reader, dds_HANDLE_NIL, data_values, sample_infos,
          sample_sizes, 1, dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE,
          dds_ANY_INSTANCE_STATE);

      if (ret == dds_RETCODE_NO_DATA) {
        return RMW_RET_OK;
      }

      if (ret != dds_RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to take data");
        return RMW_RET_ERROR;
      }

      dds_SampleInfo *sample_info = dds_SampleInfoSeq_get(sample_infos, 0);
      if (sample_info->valid_data) {
        void *sample = dds_DataSeq_get(data_values, 0);
        if (sample == nullptr) {
          return RMW_RET_ERROR;
        }
        uint32_t size = dds_UnsignedLongSeq_get(sample_sizes, 0);
        int64_t sequence_number = 0;
        int8_t client_guid[16] = {0};
        dds_SampleInfoEx *sampleinfo_ex =
          reinterpret_cast<dds_SampleInfoEx *>(sample_info);
        rmw_gurumdds_cpp::dds_guid_to_ros_guid(
            reinterpret_cast<int8_t *>(&sampleinfo_ex->src_guid), client_guid);
        rmw_gurumdds_cpp::dds_sn_to_ros_sn(sampleinfo_ex->seq,
                                           &sequence_number);

        bool res = rmw_gurumdds_cpp::deserialize_response_enhanced(
            type_support->data, type_support->typesupport_identifier,
            ros_response, sample, static_cast<size_t>(size));

        if (!res) {
          // Error message already set
          return RMW_RET_ERROR;
        }

        if (std::memcmp(client_info->writer_guid, client_guid,
                        RMW_GID_STORAGE_SIZE) == 0)
        {
          request_header->source_timestamp =
            sample_info->source_timestamp.sec *
            static_cast<int64_t>(1000000000) +
            sample_info->source_timestamp.nanosec;
          request_header->received_timestamp =
            sampleinfo_ex->reception_timestamp.sec *
            static_cast<int64_t>(1000000000) +
            sampleinfo_ex->reception_timestamp.nanosec;
          request_header->request_id.sequence_number = sequence_number;
          std::memcpy(request_header->request_id.writer_guid, client_guid,
                      RMW_GID_STORAGE_SIZE);

          *taken = true;
        }
      }
      if (*taken) {
        break;
      }
    }
  }

  TRACETOOLS_TRACEPOINT(rmw_take_response, static_cast<const void *>(client),
                        static_cast<const void *>(ros_response),
                        request_header->request_id.sequence_number,
                        request_header->source_timestamp, *taken);

  return RMW_RET_OK;
}

rmw_ret_t rmw_client_set_on_new_response_callback(
  rmw_client_t *rmw_client,
  rmw_event_callback_t callback,
  const void *user_data)
{
  CHECK_ALL_PTRS_CODE(rmw_client);
  CHECK_ID_CODE(rmw_client);

  auto client_info =
    static_cast<rmw_gurumdds_cpp::ClientInfo *>(rmw_client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("invalid client data");
    return RMW_RET_ERROR;
  }

  std::lock_guard<std::mutex> guard(client_info->event_callback_data.mutex);
  dds_StatusMask mask =
    dds_DataReader_get_status_changes(client_info->response_reader);
  dds_ReturnCode_t dds_rc = dds_RETCODE_ERROR;

  if (callback) {
    size_t unread_count = client_info->count_unread();
    if (0 < unread_count) {
      callback(user_data, unread_count);
    }

    client_info->event_callback_data.callback = callback;
    client_info->event_callback_data.user_data = user_data;
    mask |= dds_DATA_AVAILABLE_STATUS;
    dds_rc = dds_DataReader_set_listener(client_info->response_reader,
                                         &client_info->response_listener, mask);
  } else {
    client_info->event_callback_data.callback = nullptr;
    client_info->event_callback_data.user_data = nullptr;
    mask &= ~dds_DATA_AVAILABLE_STATUS;
    dds_rc = dds_DataReader_set_listener(client_info->response_reader,
                                         &client_info->response_listener, mask);
  }

  return rmw_gurumdds_cpp::check_dds_ret_code(dds_rc);
}
} // extern "C"

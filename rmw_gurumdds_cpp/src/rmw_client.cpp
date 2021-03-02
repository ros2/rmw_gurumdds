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
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/qos.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"

#include "rmw_gurumdds_cpp/types.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"

#include "./type_support_service.hpp"

extern "C"
{
rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle, node->implementation_identifier,
    gurum_gurumdds_identifier, return nullptr)

  if (service_name == nullptr || strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("client topic is null or empty string");
    return nullptr;
  }

  if (qos_policies == nullptr) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return nullptr;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return nullptr;
  }

  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return nullptr;
  }

  const rosidl_service_type_support_t * type_support =
    get_service_typesupport_handle(type_supports, rosidl_typesupport_introspection_c__identifier);
  if (type_support == nullptr) {
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (type_support == nullptr) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  GurumddsClientInfo * client_info = nullptr;
  rmw_client_t * rmw_client = nullptr;

  dds_SubscriberQos subscriber_qos;
  dds_PublisherQos publisher_qos;
  dds_DataReaderQos datareader_qos;
  dds_DataWriterQos datawriter_qos;
  dds_DataReaderListener datareader_listener = {};

  dds_Publisher * dds_publisher = nullptr;
  dds_Subscriber * dds_subscriber = nullptr;
  dds_DataWriter * request_writer = nullptr;
  dds_DataReader * response_reader = nullptr;
  dds_GuardCondition * queue_guard_condition = nullptr;
  dds_TypeSupport * request_typesupport = nullptr;
  dds_TypeSupport * response_typesupport = nullptr;

  dds_TopicDescription * topic_desc = nullptr;
  dds_Topic * request_topic = nullptr;
  dds_Topic * response_topic = nullptr;

  uint64_t guid_temp = 0;
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  rmw_ret_t rmw_ret = RMW_RET_OK;

  std::pair<std::string, std::string> service_type_name;
  std::pair<std::string, std::string> service_metastring;
  std::string request_topic_name;
  std::string response_topic_name;
  std::string request_type_name;
  std::string response_type_name;
  std::string request_metastring;
  std::string response_metastring;

  // Random values are required for GUID
  std::random_device rd;
  std::default_random_engine dre(rd());
  std::uniform_int_distribution<uint64_t> uniform_dist(
    (std::numeric_limits<uint64_t>::min)(),
    (std::numeric_limits<uint64_t>::max)());

  // Create topic and type name strings
  service_type_name =
    create_service_type_name(type_support->data, type_support->typesupport_identifier);
  request_type_name = service_type_name.first;
  response_type_name = service_type_name.second;
  if (request_type_name.empty() || response_type_name.empty()) {
    RMW_SET_ERROR_MSG("failed to create type name");
    return nullptr;
  }

  if (!qos_policies->avoid_ros_namespace_conventions) {
    request_topic_name = std::string(ros_service_requester_prefix) + service_name;
    response_topic_name = std::string(ros_service_response_prefix) + service_name;
  } else {
    request_topic_name = service_name;
    response_topic_name = service_name;
  }
  request_topic_name += "Request";
  response_topic_name += "Reply";

  service_metastring =
    create_service_metastring(type_support->data, type_support->typesupport_identifier);
  request_metastring = service_metastring.first;
  response_metastring = service_metastring.second;
  if (request_metastring.empty() || response_metastring.empty()) {
    RMW_SET_ERROR_MSG("failed to create metastring");
    return nullptr;
  }

  client_info = new(std::nothrow) GurumddsClientInfo();
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate GurumddsClientInfo");
    goto fail;
  }

  client_info->participant = participant;
  client_info->implementation_identifier = gurum_gurumdds_identifier;
  client_info->service_typesupport = type_support;
  client_info->sequence_number = 0;

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
  topic_desc = dds_DomainParticipant_lookup_topicdescription(
    participant, response_topic_name.c_str());
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
    response_topic = dds_DomainParticipant_find_topic(
      participant, response_topic_name.c_str(), &timeout);
    if (response_topic == nullptr) {
      RMW_SET_ERROR_MSG("failed to find topic");
      goto fail;
    }
  }

  // Create datawriter for request
  ret = dds_DomainParticipant_get_default_publisher_qos(participant, &publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default publisher qos");
    goto fail;
  }

  dds_publisher = dds_DomainParticipant_create_publisher(participant, &publisher_qos, nullptr, 0);
  if (dds_publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to create publisher");
    dds_PublisherQos_finalize(&publisher_qos);
    goto fail;
  }
  client_info->dds_publisher = dds_publisher;

  ret = dds_PublisherQos_finalize(&publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize publisher qos");
    goto fail;
  }

  if (!get_datawriter_qos(dds_publisher, qos_policies, &datawriter_qos)) {
    // Error message already set
    goto fail;
  }

  request_writer = dds_Publisher_create_datawriter(
    dds_publisher, request_topic, &datawriter_qos, nullptr, 0);
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    goto fail;
  }
  client_info->request_writer = request_writer;

  ret = dds_DataWriterQos_finalize(&datawriter_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datawriter qos");
    goto fail;
  }

  // Create datareader for response
  ret = dds_DomainParticipant_get_default_subscriber_qos(participant, &subscriber_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default subscriber qos");
    goto fail;
  }

  dds_subscriber = dds_DomainParticipant_create_subscriber(
    participant, &subscriber_qos, nullptr, 0);
  if (dds_subscriber == nullptr) {
    RMW_SET_ERROR_MSG("failed to create subscriber");
    dds_SubscriberQos_finalize(&subscriber_qos);
    goto fail;
  }
  client_info->dds_subscriber = dds_subscriber;

  ret = dds_SubscriberQos_finalize(&subscriber_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize subscriber qos");
    goto fail;
  }

  if (!get_datareader_qos(dds_subscriber, qos_policies, &datareader_qos)) {
    // error message already set
    goto fail;
  }

  datareader_listener.on_data_available = reader_on_data_available<GurumddsClientInfo>;
  response_reader = dds_Subscriber_create_datareader(
    dds_subscriber, response_topic, &datareader_qos, &datareader_listener,
    dds_DATA_AVAILABLE_STATUS);
  if (response_reader == nullptr) {
    RMW_SET_ERROR_MSG("failed to create datareader");
    goto fail;
  }
  client_info->response_reader = response_reader;

  ret = dds_DataReaderQos_finalize(&datareader_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize datareader qos");
    goto fail;
  }

  dds_DataReader_set_listener_context(client_info->response_reader, client_info);

  queue_guard_condition = dds_GuardCondition_create();
  if (queue_guard_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create guard condition");
    goto fail;
  }
  client_info->queue_guard_condition = queue_guard_condition;

  // Set GUID
  guid_temp = uniform_dist(dre);
  memcpy(client_info->writer_guid, &guid_temp, sizeof(guid_temp));
  guid_temp = uniform_dist(dre);
  memcpy(client_info->writer_guid + sizeof(guid_temp), &guid_temp, sizeof(guid_temp));

  rmw_client = rmw_client_allocate();
  if (rmw_client == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client");
    goto fail;
  }
  memset(rmw_client, 0, sizeof(rmw_client_t));

  rmw_client->implementation_identifier = gurum_gurumdds_identifier;
  rmw_client->data = client_info;
  rmw_client->service_name = reinterpret_cast<const char *>(rmw_allocate(strlen(service_name) + 1));
  if (rmw_client->service_name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_client->service_name), service_name, strlen(service_name) + 1);

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);
  if (rmw_ret != RMW_RET_OK) {
    // Error message already set
    goto fail;
  }

  dds_TypeSupport_delete(request_typesupport);
  request_typesupport = nullptr;
  dds_TypeSupport_delete(response_typesupport);
  response_typesupport = nullptr;

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_cpp",
    "Created client with service '%s' on node '%s%s%s'",
    service_name, node->namespace_,
    node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

  return rmw_client;

fail:
  if (rmw_client != nullptr) {
    rmw_client_free(rmw_client);
  }

  if (dds_publisher != nullptr) {
    if (request_writer != nullptr) {
      dds_Publisher_delete_datawriter(dds_publisher, request_writer);
    }
    dds_DomainParticipant_delete_publisher(participant, dds_publisher);
  }

  if (dds_subscriber != nullptr) {
    if (response_reader != nullptr) {
      if (queue_guard_condition != nullptr) {
        dds_GuardCondition_delete(queue_guard_condition);
      }
      dds_Subscriber_delete_datareader(dds_subscriber, response_reader);
    }
    dds_DomainParticipant_delete_subscriber(participant, dds_subscriber);
  }

  if (client_info != nullptr) {
    delete client_info;
  }
  return nullptr;
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (client == nullptr) {
    RMW_SET_ERROR_MSG("client handle is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client handle,
    client->implementation_identifier, gurum_gurumdds_identifier,
    return RMW_RET_ERROR)

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);

  rmw_ret_t rmw_ret = RMW_RET_OK;
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  GurumddsClientInfo * client_info = static_cast<GurumddsClientInfo *>(client->data);

  if (client_info != nullptr) {
    if (client_info->participant != nullptr) {
      if (client_info->dds_publisher != nullptr) {
        if (client_info->request_writer != nullptr) {
          ret = dds_Publisher_delete_datawriter(
            client_info->dds_publisher, client_info->request_writer);
          if (ret != dds_RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to delete datawriter");
            rmw_ret = RMW_RET_ERROR;
          }
        }
        ret = dds_DomainParticipant_delete_publisher(
          client_info->participant, client_info->dds_publisher);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete publisher");
          rmw_ret = RMW_RET_ERROR;
        }
      } else if (client_info->request_writer != nullptr) {
        RMW_SET_ERROR_MSG("cannot delete datawriter because the publisher is null");
        rmw_ret = RMW_RET_ERROR;
      }

      if (client_info->dds_subscriber != nullptr) {
        if (client_info->response_reader != nullptr) {
          ret = dds_Subscriber_delete_datareader(
            client_info->dds_subscriber, client_info->response_reader);
          if (ret != dds_RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to delete datareader");
            rmw_ret = RMW_RET_ERROR;
          }
        }
        ret = dds_DomainParticipant_delete_subscriber(
          client_info->participant, client_info->dds_subscriber);
        if (ret != dds_RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete subscriber");
          rmw_ret = RMW_RET_ERROR;
        }
      } else if (client_info->response_reader != nullptr) {
        RMW_SET_ERROR_MSG("cannot delete datareader because the subscriber is null");
        rmw_ret = RMW_RET_ERROR;
      }

    } else if (client_info->dds_publisher != nullptr || client_info->dds_subscriber != nullptr) {
      RMW_SET_ERROR_MSG(
        "cannot delete publisher and subscriber because the domain participant is null");
      rmw_ret = RMW_RET_ERROR;
    }

    if (client_info->queue_guard_condition != nullptr) {
      dds_GuardCondition_delete(client_info->queue_guard_condition);
      client_info->queue_guard_condition = nullptr;
    }

    while (!client_info->message_queue.empty()) {
      auto msg = client_info->message_queue.front();
      if (msg.sample != nullptr) {
        free(msg.sample);
      }
      if (msg.info !- nullptr) {
        free(msg.info);
      }
      client_info->message_queue.pop();
    }

    delete client_info;
    client->data = nullptr;
    if (client->service_name != nullptr) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_cpp",
        "Deleted client with service '%s' on node '%s%s%s'",
        client->service_name, node->namespace_,
        node->namespace_[strlen(node->namespace_) - 1] == '/' ? "" : "/", node->name);

      rmw_free(const_cast<char *>(client->service_name));
    }
  }

  rmw_client_free(client);

  rmw_ret = rmw_trigger_guard_condition(node_info->graph_guard_condition);

  return rmw_ret;
}

rmw_ret_t
rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, gurum_gurumdds_identifier,
    return RMW_RET_ERROR)

  if (client == nullptr) {
    RMW_SET_ERROR_MSG("client handle is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client handle,
    client->implementation_identifier, gurum_gurumdds_identifier,
    return RMW_RET_ERROR)

  if (is_available == nullptr) {
    RMW_SET_ERROR_MSG("is_available is null");
    return RMW_RET_ERROR;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  GurumddsClientInfo * client_info = static_cast<GurumddsClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * request_writer = client_info->request_writer;
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("request writer is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader * response_reader = client_info->response_reader;
  if (response_reader == nullptr) {
    RMW_SET_ERROR_MSG("response reader is null");
    return RMW_RET_ERROR;
  }

  *is_available = false;

  dds_InstanceHandleSeq * seq = dds_InstanceHandleSeq_create(4);
  if (seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_ERROR;
  }

  // Look for matching request reader
  if (dds_DataWriter_get_matched_subscriptions(request_writer, seq) != dds_RETCODE_OK) {
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

  seq = dds_InstanceHandleSeq_create(4);
  if (seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_ERROR;
  }

  // Look for matching response writer
  if (dds_DataReader_get_matched_publications(response_reader, seq) != dds_RETCODE_OK) {
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
}  // extern "C"

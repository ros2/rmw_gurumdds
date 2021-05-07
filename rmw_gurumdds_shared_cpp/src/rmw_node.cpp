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

#include <array>
#include <utility>
#include <set>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/sanity_checks.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"

#include "rmw_gurumdds_shared_cpp/dds_include.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"

rmw_node_t *
shared__rmw_create_node(
  const char * implementation_identifier,
  rmw_context_t * context,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  bool localhost_only)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    implementation_identifier,
    return NULL
  );

  dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();
  if (factory == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    return nullptr;
  }

  dds_DomainParticipantQos participant_qos;
  dds_ReturnCode_t ret =
    dds_DomainParticipantFactory_get_default_participant_qos(factory, &participant_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default domain participant qos");
    return nullptr;
  }

  // This is used to get node name from discovered participants
  std::string node_user_data =
    std::string("name=") + std::string(name) + std::string(";namespace=") +
    std::string(namespace_) + std::string(";securitycontext=") +
    std::string(context->options.enclave) + std::string(";");
  if (node_user_data.size() > sizeof(participant_qos.user_data.value)) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_shared_cpp",
      "node name, namespace and security context are too long - "
      "the sum of their lengths must be less than %zu",
      sizeof(participant_qos.user_data.value) - strlen("name=;namespace=;enclave=;"));
    return nullptr;
  }

  participant_qos.user_data.size = node_user_data.size();
  memset(participant_qos.user_data.value, 0, sizeof(participant_qos.user_data.value));
  memcpy(participant_qos.user_data.value, node_user_data.c_str(), node_user_data.size());

  rmw_node_t * node_handle = nullptr;
  GurumddsNodeInfo * node_info = nullptr;
  rmw_guard_condition_t * graph_guard_condition = nullptr;
  GurumddsPublisherListener * publisher_listener = nullptr;
  GurumddsSubscriberListener * subscriber_listener = nullptr;
  dds_Subscriber * builtin_subscriber = nullptr;
  dds_DataReader * builtin_publication_datareader = nullptr;
  dds_DataReader * builtin_subscription_datareader = nullptr;

  dds_DomainParticipant * participant = nullptr;

  // TODO(clemjh): Implement security features

  if (localhost_only) {
    dds_StringProperty props[] = {
      {const_cast<char *>("rtps.interface.ip"),
        const_cast<void *>(static_cast<const void *>("127.0.0.1"))},
      {nullptr, nullptr},
    };
    participant = dds_DomainParticipantFactory_create_participant_w_props(
      factory, domain_id, &participant_qos, nullptr, 0, props);
  } else {
    participant = dds_DomainParticipantFactory_create_participant(
      factory, domain_id, &participant_qos, nullptr, 0);
  }
  graph_guard_condition = shared__rmw_create_guard_condition(implementation_identifier);
  if (graph_guard_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create graph guard condition");
    goto fail;
  }

  publisher_listener =
    new(std::nothrow) GurumddsPublisherListener(implementation_identifier, graph_guard_condition);
  if (publisher_listener == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate GurumddsPublisherListener");
    return nullptr;
  }

  subscriber_listener =
    new(std::nothrow) GurumddsSubscriberListener(implementation_identifier, graph_guard_condition);
  if (subscriber_listener == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate GurumddsSubscriberListener");
    return nullptr;
  }

  node_handle = rmw_node_allocate();
  if (node_handle == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node handle");
    goto fail;
  }

  node_handle->implementation_identifier = implementation_identifier;
  node_handle->data = participant;
  node_handle->name = reinterpret_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
  if (node_handle->name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node name");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    reinterpret_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
  if (node_handle->namespace_ == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node namespace");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);

  node_info = new(std::nothrow) GurumddsNodeInfo();
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate GurumddsNodeInfo");
    goto fail;
  }

  node_info->participant = participant;
  node_info->graph_guard_condition = graph_guard_condition;
  node_info->pub_listener = publisher_listener;
  node_info->sub_listener = subscriber_listener;

  node_handle->implementation_identifier = implementation_identifier;
  node_handle->data = node_info;
  node_handle->context = context;

  // set listeners
  builtin_subscriber = dds_DomainParticipant_get_builtin_subscriber(participant);
  builtin_publication_datareader =
    dds_Subscriber_lookup_datareader(builtin_subscriber, "BuiltinPublications");
  if (builtin_publication_datareader == nullptr) {
    RMW_SET_ERROR_MSG("builtin publication datareader handle is null");
    goto fail;
  }
  builtin_subscription_datareader =
    dds_Subscriber_lookup_datareader(builtin_subscriber, "BuiltinSubscriptions");
  if (builtin_subscription_datareader == nullptr) {
    RMW_SET_ERROR_MSG("builtin subscription datareader handle is null");
    goto fail;
  }

  node_info->pub_listener->dds_reader = builtin_publication_datareader;
  dds_DataReader_set_listener(
    builtin_publication_datareader,
    &node_info->pub_listener->dds_listener, dds_DATA_AVAILABLE_STATUS);
  dds_DataReader_set_listener_context(
    builtin_publication_datareader, &node_info->pub_listener->context);
  node_info->sub_listener->dds_reader = builtin_subscription_datareader;
  dds_DataReader_set_listener(
    builtin_subscription_datareader,
    &node_info->sub_listener->dds_listener, dds_DATA_AVAILABLE_STATUS);
  dds_DataReader_set_listener_context(
    builtin_subscription_datareader, &node_info->sub_listener->context);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_shared_cpp",
    "Created node '%s' in namespace '%s'", name, namespace_);

  return node_handle;

fail:
  if (participant != nullptr) {
    dds_DomainParticipantFactory_delete_participant(factory, participant);
  }

  if (graph_guard_condition != nullptr) {
    rmw_ret_t rmw_ret =
      shared__rmw_destroy_guard_condition(implementation_identifier, graph_guard_condition);
    if (rmw_ret != RMW_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED("rmw_gurumdds_shared_cpp", "Failed to delete guard condition");
    }
  }

  if (node_handle != nullptr) {
    if (node_handle->name != nullptr) {
      rmw_free(const_cast<char *>(node_handle->name));
    }

    if (node_handle->namespace_ != nullptr) {
      rmw_free(const_cast<char *>(node_handle->namespace_));
    }

    rmw_free(node_handle);
  }

  if (publisher_listener != nullptr) {
    delete publisher_listener;
  }

  if (subscriber_listener != nullptr) {
    delete subscriber_listener;
  }

  if (node_info != nullptr) {
    delete node_info;
  }

  return nullptr;
}

rmw_ret_t
shared__rmw_destroy_node(const char * implementation_identifier, rmw_node_t * node)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node_handle,
    node->implementation_identifier, implementation_identifier,
    return RMW_RET_ERROR);

  dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();
  if (factory == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    return RMW_RET_ERROR;
  }

  auto node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return RMW_RET_ERROR;
  }

  dds_InstanceHandleSeq * pub_seq = dds_InstanceHandleSeq_create(4);
  if (pub_seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_ERROR;
  }

  dds_InstanceHandleSeq * sub_seq = dds_InstanceHandleSeq_create(4);
  if (sub_seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    dds_InstanceHandleSeq_delete(pub_seq);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret =
    dds_DomainParticipant_get_contained_entities(participant, pub_seq, sub_seq, NULL, NULL);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get contained entities of the domain participant");
    dds_InstanceHandleSeq_delete(pub_seq);
    dds_InstanceHandleSeq_delete(sub_seq);
    return RMW_RET_ERROR;
  }

  int32_t cnt = static_cast<int32_t>(dds_InstanceHandleSeq_length(pub_seq));
  for (int32_t i = cnt - 1; i >= 0; i--) {
    dds_Publisher * pub =
      reinterpret_cast<dds_Publisher *>(dds_InstanceHandleSeq_remove(pub_seq, i));
    dds_InstanceHandleSeq * dw_seq = dds_InstanceHandleSeq_create(1);
    if (dw_seq == nullptr) {
      RMW_SET_ERROR_MSG("failed to create instance handle sequence");
      dds_InstanceHandleSeq_delete(pub_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    ret = dds_Publisher_get_contained_entities(pub, dw_seq);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get contained entities of the publisher");
      dds_InstanceHandleSeq_delete(dw_seq);
      dds_InstanceHandleSeq_delete(pub_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    if (dds_InstanceHandleSeq_length(dw_seq) > 1) {
      dds_InstanceHandleSeq_delete(dw_seq);
      continue;
    }

    dds_DataWriter * dw = reinterpret_cast<dds_DataWriter *>(dds_InstanceHandleSeq_get(dw_seq, 0));
    ret = dds_Publisher_delete_datawriter(pub, dw);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete datawriter");
      dds_InstanceHandleSeq_delete(dw_seq);
      dds_InstanceHandleSeq_delete(pub_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    ret = dds_DomainParticipant_delete_publisher(participant, pub);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete Publisher");
      dds_InstanceHandleSeq_delete(dw_seq);
      dds_InstanceHandleSeq_delete(pub_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    dds_InstanceHandleSeq_delete(dw_seq);
  }
  dds_InstanceHandleSeq_delete(pub_seq);

  cnt = static_cast<int32_t>(dds_InstanceHandleSeq_length(sub_seq));
  for (int32_t i = cnt - 1; i >= 0; i--) {
    dds_Subscriber * sub =
      reinterpret_cast<dds_Subscriber *>(dds_InstanceHandleSeq_remove(sub_seq, i));
    dds_InstanceHandleSeq * dr_seq = dds_InstanceHandleSeq_create(1);
    if (dr_seq == nullptr) {
      RMW_SET_ERROR_MSG("failed to create instance handle sequence");
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    ret = dds_Subscriber_get_contained_entities(sub, dr_seq);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get contained entities of the subscriber");
      dds_InstanceHandleSeq_delete(dr_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    if (dds_InstanceHandleSeq_length(dr_seq) > 1) {
      dds_InstanceHandleSeq_delete(dr_seq);
      continue;
    }

    dds_DataReader * dr = reinterpret_cast<dds_DataReader *>(dds_InstanceHandleSeq_get(dr_seq, 0));
    ret = dds_Subscriber_delete_datareader(sub, dr);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete datareader");
      dds_InstanceHandleSeq_delete(dr_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    ret = dds_DomainParticipant_delete_subscriber(participant, sub);
    if (ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to delete Subscriber");
      dds_InstanceHandleSeq_delete(dr_seq);
      dds_InstanceHandleSeq_delete(sub_seq);
      return RMW_RET_ERROR;
    }

    dds_InstanceHandleSeq_delete(dr_seq);
  }
  dds_InstanceHandleSeq_delete(sub_seq);

  ret = dds_DomainParticipantFactory_delete_participant(factory, participant);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to delete participant");
    return RMW_RET_ERROR;
  }

  if (node_info->pub_listener != nullptr) {
    delete node_info->pub_listener;
    node_info->pub_listener = nullptr;
  }

  if (node_info->sub_listener != nullptr) {
    delete node_info->sub_listener;
    node_info->sub_listener = nullptr;
  }

  if (node_info->graph_guard_condition != nullptr) {
    rmw_ret_t rmw_ret = shared__rmw_destroy_guard_condition(
      implementation_identifier, node_info->graph_guard_condition);
    if (rmw_ret != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("failed to delete graph guard condition");
      return RMW_RET_ERROR;
    }
    node_info->graph_guard_condition = nullptr;
  }

  delete node_info;
  node->data = nullptr;

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_gurumdds_shared_cpp",
    "Deleted node '%s' in namespace '%s'", node->name, node->namespace_);

  rmw_free(const_cast<char *>(node->name));
  node->name = nullptr;
  rmw_free(const_cast<char *>(node->namespace_));
  node->namespace_ = nullptr;
  rmw_node_free(node);

  return RMW_RET_OK;
}

const rmw_guard_condition_t *
shared__rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  auto node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return nullptr;
  }

  return node_info->graph_guard_condition;
}

rmw_ret_t
_get_node_names(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces,
  rcutils_string_array_t * enclaves)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (rmw_check_zero_rmw_string_array(node_names) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (rmw_check_zero_rmw_string_array(node_namespaces) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (enclaves != nullptr &&
    rmw_check_zero_rmw_string_array(enclaves) != RMW_RET_OK)
  {
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != implementation_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("domain participant handle is null");
    return RMW_RET_ERROR;
  }

  dds_InstanceHandleSeq * handle_seq = dds_InstanceHandleSeq_create(4);
  if (handle_seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_BAD_ALLOC;
  }

  // Get discovered participants
  dds_ReturnCode_t ret = dds_DomainParticipant_get_discovered_participants(participant, handle_seq);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("unable to fetch discovered participants.");
    dds_InstanceHandleSeq_delete(handle_seq);
    return RMW_RET_ERROR;
  }

  uint32_t length = dds_InstanceHandleSeq_length(handle_seq);
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  rmw_ret_t fail_ret = RMW_RET_ERROR;

  rcutils_string_array_t node_list = rcutils_get_zero_initialized_string_array();
  rcutils_string_array_t ns_list = rcutils_get_zero_initialized_string_array();
  rcutils_string_array_t enclave_list = rcutils_get_zero_initialized_string_array();
  int n = 0;

  rcutils_ret_t rcutils_ret = rcutils_string_array_init(&node_list, length, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    rcutils_reset_error();
    fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    goto fail;
  }

  rcutils_ret = rcutils_string_array_init(&ns_list, length, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    rcutils_reset_error();
    fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    goto fail;
  }

  if (enclaves != nullptr) {
    rcutils_ret = rcutils_string_array_init(&enclave_list, length, &allocator);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
      rcutils_reset_error();
      fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
      goto fail;
    }
  }

  for (uint32_t i = 0; i < length; ++i) {
    dds_ParticipantBuiltinTopicData pbtd;
    dds_InstanceHandle_t handle = dds_InstanceHandleSeq_get(handle_seq, i);
    ret = dds_DomainParticipant_get_discovered_participant_data(participant, &pbtd, handle);
    std::string name;
    std::string namespace_;
    std::string enclave;
    if (ret == dds_RETCODE_OK) {
      // Get node name and namespace from user_data
      uint8_t * data = pbtd.user_data.value;
      std::vector<uint8_t> kv(data, data + pbtd.user_data.size);
      auto map = rmw::impl::cpp::parse_key_value(kv);
      auto name_found = map.find("name");
      auto ns_found = map.find("namespace");
      auto enclave_found = map.find("securitycontext");

      if (name_found != map.end()) {
        name = std::string(name_found->second.begin(), name_found->second.end());
      }

      if (ns_found != map.end()) {
        namespace_ = std::string(ns_found->second.begin(), ns_found->second.end());
      }

      if (enclave_found != map.end()) {
        enclave = std::string(enclave_found->second.begin(), enclave_found->second.end());
      }
    }

    if (name.empty()) {  // Ignore discovered participants without a name
      continue;
    }

    node_list.data[n] = rcutils_strdup(name.c_str(), allocator);
    if (node_list.data[n] == nullptr) {
      RMW_SET_ERROR_MSG("could not allocate memory for node name");
      fail_ret = RMW_RET_BAD_ALLOC;
      goto fail;
    }

    ns_list.data[n] = rcutils_strdup(namespace_.c_str(), allocator);
    if (ns_list.data[n] == nullptr) {
      RMW_SET_ERROR_MSG("could not allocate memory for node namspace");
      fail_ret = RMW_RET_BAD_ALLOC;
      goto fail;
    }

    if (enclaves != nullptr) {
      enclave_list.data[n] = rcutils_strdup(enclave.c_str(), allocator);
      if (enclave_list.data[n] == nullptr) {
        RMW_SET_ERROR_MSG("could not allocate memory for security context");
        fail_ret = RMW_RET_BAD_ALLOC;
        goto fail;
      }
    }

    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_gurumdds_cpp", "node found: %s %s", namespace_.c_str(), name.c_str());

    n++;
  }
  dds_InstanceHandleSeq_delete(handle_seq);
  handle_seq = nullptr;

  rcutils_ret = rcutils_string_array_init(node_names, n, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    rcutils_reset_error();
    fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    goto fail;
  }

  rcutils_ret = rcutils_string_array_init(node_namespaces, n, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    rcutils_reset_error();
    fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    goto fail;
  }

  if (enclaves != nullptr) {
    rcutils_ret = rcutils_string_array_init(enclaves, n, &allocator);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
      rcutils_reset_error();
      fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
      goto fail;
    }
  }

  for (int i = 0; i < n; ++i) {
    node_names->data[i] = node_list.data[i];
    node_list.data[i] = nullptr;
    node_namespaces->data[i] = ns_list.data[i];
    ns_list.data[i] = nullptr;
    if (enclaves != nullptr) {
      enclaves->data[i] = enclave_list.data[i];
      enclave_list.data[i] = nullptr;
    }
  }

  rcutils_ret = rcutils_string_array_fini(&node_list);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_shared_cpp",
      "failed to delete string array: %s", rcutils_get_error_string().str);
    rcutils_reset_error();
    fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    goto fail;
  }

  rcutils_ret = rcutils_string_array_fini(&ns_list);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_shared_cpp",
      "failed to delete string array: %s", rcutils_get_error_string().str);
    rcutils_reset_error();
    fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    goto fail;
  }

  if (enclaves != nullptr) {
    rcutils_ret = rcutils_string_array_fini(&enclave_list);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_gurumdds_shared_cpp",
        "failed to delete string array: %s", rcutils_get_error_string().str);
      rcutils_reset_error();
      fail_ret = rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
      goto fail;
    }
  }

  return RMW_RET_OK;

fail:
  if (handle_seq != nullptr) {
    dds_InstanceHandleSeq_delete(handle_seq);
  }

  rcutils_ret = rcutils_string_array_fini(&node_list);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_cpp",
      "failed to cleanup during error handling: %s", rcutils_get_error_string().str);
    rcutils_reset_error();
  }

  rcutils_ret = rcutils_string_array_fini(&ns_list);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_cpp",
      "failed to cleanup during error handling: %s", rcutils_get_error_string().str);
    rcutils_reset_error();
  }

  rcutils_ret = rcutils_string_array_fini(&enclave_list);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_gurumdds_cpp",
      "failed to cleanup during error handling: %s", rcutils_get_error_string().str);
    rcutils_reset_error();
  }

  if (node_names != nullptr) {
    rcutils_ret = rcutils_string_array_fini(node_names);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_gurumdds_cpp",
        "failed to cleanup during error handling; %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }

  if (node_namespaces != nullptr) {
    rcutils_ret = rcutils_string_array_fini(node_namespaces);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_gurumdds_cpp",
        "failed to cleanup during error handling; %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }

  if (enclaves != nullptr) {
    rcutils_ret = rcutils_string_array_fini(enclaves);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_gurumdds_cpp",
        "failed to cleanup during error handling; %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }

  return fail_ret;
}

rmw_ret_t
shared__rmw_get_node_names(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  return _get_node_names(implementation_identifier, node, node_names, node_namespaces, nullptr);
}

rmw_ret_t
shared__rmw_get_node_names_with_enclaves(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces,
  rcutils_string_array_t * enclaves)
{
  return _get_node_names(
    implementation_identifier, node, node_names, node_namespaces, enclaves);
}

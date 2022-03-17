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

#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"
#include "rmw_gurumdds_shared_cpp/guid.hpp"
#include "rmw_gurumdds_shared_cpp/names_and_types_helpers.hpp"
#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"

typedef std::map<std::string, std::set<std::string>> TopicMap;

bool
__is_node_match(
  dds_UserDataQosPolicy user_data,
  const char * node_name,
  const char * node_namespace)
{
  uint8_t * buf = user_data.value;
  if (buf != nullptr) {
    std::vector<uint8_t> kv(buf, buf + user_data.size);
    auto map = rmw::impl::cpp::parse_key_value(kv);
    auto name_found = map.find("name");
    auto ns_found = map.find("namespace");

    if (name_found != map.end() && ns_found != map.end()) {
      std::string name(name_found->second.begin(), name_found->second.end());
      std::string ns(ns_found->second.begin(), ns_found->second.end());
      return strcmp(node_name, name.c_str()) == 0 && strcmp(node_namespace, ns.c_str()) == 0;
    }
  }
  return false;
}

rmw_ret_t
__get_key(
  GurumddsNodeInfo * node_info,
  const char * node_name,
  const char * node_namespace,
  GuidPrefix_t & key)
{
  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return RMW_RET_ERROR;
  }

  dds_DomainParticipantQos qos;
  dds_ReturnCode_t ret = dds_DomainParticipant_get_qos(participant, &qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("Can't get domainparticipant qos policies");
    return RMW_RET_ERROR;
  }
  dds_InstanceHandleSeq * handle_seq = dds_InstanceHandleSeq_create(4);
  if (handle_seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_ERROR;
  }

  ret = dds_DomainParticipant_get_discovered_participants(participant, handle_seq);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to fetch discovered participants.");
    dds_InstanceHandleSeq_delete(handle_seq);
    return RMW_RET_ERROR;
  }

  dds_UnsignedLong len = dds_InstanceHandleSeq_length(handle_seq);
  for (dds_UnsignedLong i = 0; i < len; ++i) {
    dds_ParticipantBuiltinTopicData pbtd;
    ret = dds_DomainParticipant_get_discovered_participant_data(
      participant, &pbtd, dds_InstanceHandleSeq_get(handle_seq, i));
    if (ret == dds_RETCODE_OK) {
      uint8_t * buf = pbtd.user_data.value;
      if (buf != nullptr) {
        std::vector<uint8_t> kv(buf, buf + pbtd.user_data.size);
        auto map = rmw::impl::cpp::parse_key_value(kv);
        auto name_found = map.find("name");
        auto ns_found = map.find("namespace");

        if (name_found != map.end() && ns_found != map.end()) {
          std::string name(name_found->second.begin(), name_found->second.end());
          std::string ns(ns_found->second.begin(), ns_found->second.end());
          if (strcmp(node_name, name.c_str()) == 0 && strcmp(node_namespace, ns.c_str()) == 0) {
            dds_BuiltinTopicKey_to_GUID(&key, pbtd.key);
            dds_InstanceHandleSeq_delete(handle_seq);
            return RMW_RET_OK;
          }
        }
      }
    } else {
      RMW_SET_ERROR_MSG("failed to fetch discovered participants data");
      dds_InstanceHandleSeq_delete(handle_seq);
      return RMW_RET_ERROR;
    }
  }
  RMW_SET_ERROR_MSG("failed to match node name/namespace with discovered nodes");
  dds_InstanceHandleSeq_delete(handle_seq);
  return RMW_RET_NODE_NAME_NON_EXISTENT;
}

rmw_ret_t
validate_names_and_namespace(
  const char * node_name,
  const char * node_namespace)
{
  if (node_name == nullptr) {
    RMW_SET_ERROR_MSG("node name is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (node_namespace == nullptr) {
    RMW_SET_ERROR_MSG("node namespace is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  return RMW_RET_OK;
}

rmw_ret_t
shared__rmw_get_subscriber_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  ret = validate_names_and_namespace(node_name, node_namespace);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  GuidPrefix_t key;
  ret = __get_key(node_info, node_name, node_namespace, key);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  TopicMap topics;
  node_info->sub_listener->fill_topic_names_and_types_by_guid(no_demangle, topics, key);

  ret = copy_topics_names_and_types(topics, allocator, no_demangle, topic_names_and_types);

  return ret;
}

rmw_ret_t
shared__rmw_get_publisher_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  ret = validate_names_and_namespace(node_name, node_namespace);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  GuidPrefix_t key;
  ret = __get_key(node_info, node_name, node_namespace, key);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  TopicMap topics;
  node_info->pub_listener->fill_topic_names_and_types_by_guid(no_demangle, topics, key);

  ret = copy_topics_names_and_types(topics, allocator, no_demangle, topic_names_and_types);

  return ret;
}

rmw_ret_t
__get_service_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types,
  bool is_service)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t ret = rmw_names_and_types_check_zero(service_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  ret = validate_names_and_namespace(node_name, node_namespace);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  GurumddsNodeInfo * node_info = static_cast<GurumddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  GuidPrefix_t key;
  ret = __get_key(node_info, node_name, node_namespace, key);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  TopicMap services;
  node_info->sub_listener->fill_service_names_and_types_by_guid(
    services, key, is_service ? "Request" : "Reply");

  ret = copy_services_to_names_and_types(
    services, allocator, service_names_and_types);

  return ret;
}

rmw_ret_t
shared__rmw_get_service_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  return __get_service_names_and_types_by_node(
    implementation_identifier, node, allocator, node_name,
    node_namespace, service_names_and_types, true);
}

rmw_ret_t
shared__rmw_get_client_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * client_names_and_types)
{
  return __get_service_names_and_types_by_node(
    implementation_identifier, node, allocator, node_name,
    node_namespace, client_names_and_types, false);
}

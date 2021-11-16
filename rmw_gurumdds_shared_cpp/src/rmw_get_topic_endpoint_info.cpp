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

#include <mutex>
#include <string>
#include <vector>

#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/topic_endpoint_info.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/impl/cpp/key_value.hpp"

#include "rmw_gurumdds_shared_cpp/demangle.hpp"
#include "rmw_gurumdds_shared_cpp/guid.hpp"
#include "rmw_gurumdds_shared_cpp/namespace_prefix.hpp"
#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"

static rmw_ret_t
_get_endpoint_info_by_topic(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * endpoints_info,
  rmw_endpoint_type_t endpoint_type)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node_handle,
    node->implementation_identifier, implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator is null", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, RMW_RET_INVALID_ARGUMENT);

  if (endpoints_info == nullptr) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s info is null",
      endpoint_type == RMW_ENDPOINT_PUBLISHER ? "publishers" : "subscrptions");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto node_info = static_cast<GurumddsNodeInfo *>(node->data);
  auto & topic_cache =
    endpoint_type == RMW_ENDPOINT_PUBLISHER ?
    node_info->pub_listener->topic_cache : node_info->sub_listener->topic_cache;
  auto & tc_mutex =
    endpoint_type == RMW_ENDPOINT_PUBLISHER ?
    node_info->pub_listener->mutex_ : node_info->pub_listener->mutex_;

  rmw_ret_t ret = RMW_RET_OK;

  {
    std::lock_guard<std::mutex> lock(tc_mutex);

    auto tnti = topic_cache.get_topic_name_to_info();
    std::vector<rmw_topic_endpoint_info_t> info_vec;

    std::vector<std::string> tn_vec;
    tn_vec.push_back(topic_name);
    if (!no_mangle) {
      auto & prefixes = _get_all_ros_prefixes();
      for (auto & prefix : prefixes) {
        tn_vec.push_back(prefix + topic_name);
      }
    }

    dds_DomainParticipant * participant = node_info->participant;

    dds_InstanceHandleSeq * handle_seq = dds_InstanceHandleSeq_create(4);
    if (handle_seq == nullptr) {
      RMW_SET_ERROR_MSG("failed to create instance handle sequence");
      return RMW_RET_BAD_ALLOC;
    }

    // Get discovered participants
    dds_ReturnCode_t dds_ret =
      dds_DomainParticipant_get_discovered_participants(participant, handle_seq);
    if (dds_ret != dds_RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to fetch discovered participants");
      dds_InstanceHandleSeq_delete(handle_seq);
      return RMW_RET_ERROR;
    }

    for (auto & tn : tn_vec) {
      auto it = tnti.find(tn);
      if (it == tnti.end()) {
        continue;
      }

      for (auto & topic_info : it->second) {
        rmw_topic_endpoint_info_t ep_info = rmw_get_zero_initialized_topic_endpoint_info();

#define check_ok() \
  do { \
    if (ret != RMW_RET_OK) { \
      rmw_ret_t rret = RMW_RET_OK; \
      rret = rmw_topic_endpoint_info_fini(&ep_info, allocator); \
      if (rret != RMW_RET_OK) { \
        RCUTILS_LOG_ERROR_NAMED( \
          "rmw_gurumdds_cpp", \
          "rmw_topic_endpoint_info_fini failed: %s", \
          rmw_get_error_string().str); \
        rmw_reset_error(); \
      } \
      for (auto & info : info_vec) { \
        rret = rmw_topic_endpoint_info_fini(&info, allocator); \
        if (rret != RMW_RET_OK) { \
          RCUTILS_LOG_ERROR_NAMED( \
            "rmw_gurumdds_cpp", \
            "rmw_topic_endpoint_info_fini failed: %s", \
            rmw_get_error_string().str); \
          rmw_reset_error(); \
        } \
      } \
      return ret; \
    } \
  } while (0);

        std::string node_name = "_NODE_NAME_UNKNOWN_";
        std::string node_namespace = "_NODE_NAMESPACE_UNKNOWN_";
        for (uint32_t i = 0; i < dds_InstanceHandleSeq_length(handle_seq); i++) {
          dds_ParticipantBuiltinTopicData pbtd;
          dds_InstanceHandle_t handle = dds_InstanceHandleSeq_get(handle_seq, i);
          dds_ret =
            dds_DomainParticipant_get_discovered_participant_data(participant, &pbtd, handle);
          if (dds_ret != dds_RETCODE_OK) {
            continue;
          }

          GuidPrefix_t temp_guid;
          dds_BuiltinTopicKey_to_GUID(&temp_guid, pbtd.key);

          if (temp_guid == topic_info.participant_guid) {
            uint8_t * data = pbtd.user_data.value;
            std::vector<uint8_t> kv(data, data + pbtd.user_data.size);
            auto map = rmw::impl::cpp::parse_key_value(kv);
            auto name_found = map.find("name");
            auto ns_found = map.find("namespace");

            if (name_found != map.end()) {
              node_name = std::string(name_found->second.begin(), name_found->second.end());
            }

            if (ns_found != map.end()) {
              node_namespace = std::string(ns_found->second.begin(), ns_found->second.end());
            }

            break;
          }
        }

        ret = rmw_topic_endpoint_info_set_node_name(&ep_info, node_name.c_str(), allocator);
        check_ok();

        ret =
          rmw_topic_endpoint_info_set_node_namespace(&ep_info, node_namespace.c_str(), allocator);
        check_ok();

        std::string type_name =
          no_mangle ? topic_info.type : _demangle_if_ros_type(topic_info.type);
        ret = rmw_topic_endpoint_info_set_topic_type(&ep_info, type_name.c_str(), allocator);
        check_ok();

        ret = rmw_topic_endpoint_info_set_endpoint_type(&ep_info, endpoint_type);
        check_ok();

        uint8_t gid[RMW_GID_STORAGE_SIZE];
        memset(gid, 0, RMW_GID_STORAGE_SIZE);
        memcpy(gid, topic_info.entity_guid.value, GuidPrefix_t::kSize);
        ret = rmw_topic_endpoint_info_set_gid(&ep_info, gid, GuidPrefix_t::kSize);
        check_ok();

        ret = rmw_topic_endpoint_info_set_qos_profile(&ep_info, &topic_info.qos);
        check_ok();

        info_vec.push_back(ep_info);
#undef check_ok
      }
    }
    dds_InstanceHandleSeq_delete(handle_seq);

    ret = rmw_topic_endpoint_info_array_init_with_size(endpoints_info, info_vec.size(), allocator);
    if (ret != RMW_RET_OK) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "failed to initialize topic_endpoint_info_array: %s", rmw_get_error_string().str);

      for (auto & info : info_vec) {
        rmw_ret_t rret = rmw_topic_endpoint_info_fini(&info, allocator);
        if (rret != RMW_RET_OK) {
          RCUTILS_LOG_ERROR_NAMED(
            "rmw_gurumdds_cpp",
            "rmw_topic_endpoint_info_fini failed: %s",
            rmw_get_error_string().str);
          rmw_reset_error();
        }
      }

      return ret;
    }

    for (size_t i = 0; i < info_vec.size(); i++) {
      endpoints_info->info_array[i] = info_vec.at(i);
    }
  }

  return ret;
}

rmw_ret_t
shared__rmw_get_publishers_info_by_topic(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * publishers_info)
{
  return _get_endpoint_info_by_topic(
    implementation_identifier,
    node,
    allocator,
    topic_name,
    no_mangle,
    publishers_info,
    RMW_ENDPOINT_PUBLISHER);
}

rmw_ret_t
shared__rmw_get_subscriptions_info_by_topic(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * subscriptions_info)
{
  return _get_endpoint_info_by_topic(
    implementation_identifier,
    node,
    allocator,
    topic_name,
    no_mangle,
    subscriptions_info,
    RMW_ENDPOINT_SUBSCRIPTION);
}

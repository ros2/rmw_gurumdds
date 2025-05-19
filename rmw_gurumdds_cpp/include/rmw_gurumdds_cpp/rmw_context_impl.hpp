// Copyright 2022 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__RMW_CONTEXT_IMPL_HPP_
#define RMW_GURUMDDS_CPP__RMW_CONTEXT_IMPL_HPP_

#include <memory>
#include <mutex>
#include <regex>
#include <string>

#include "rcpputils/scope_exit.hpp"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/types.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"

namespace rmw_gurumdds_cpp
{
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
}  // namespace rmw_gurumdds_cpp

struct rmw_context_impl_s
{
  rmw_dds_common::Context common_ctx;
  rmw_context_t * base;

  dds_DomainId_t domain_id;
  dds_DomainParticipant * participant;

  /* used for all DDS writers/readers created to support
   * rmw_gurumdds_cpp::(Publisher/Subscriber)Info.
   * */
  dds_Publisher * publisher;
  dds_Subscriber * subscriber;

  bool service_mapping_basic;

  /* Participant reference count */
  size_t node_count{0};

  /* Mutex used to protect initialization/destruction. */
  std::mutex initialization_mutex;

  /* Shutdown flag. */
  bool is_shutdown;

  std::mutex endpoint_mutex;

  explicit rmw_context_impl_s(rmw_context_t * const base);
  ~rmw_context_impl_s();

  // Initializes the participant, if it wasn't done already.
  // node_count is increased
  rmw_ret_t
  initialize_node(const char * node_name, const char * node_namespace);

  // Destroys the participant, when node_count reaches 0.
  rmw_ret_t
  finalize_node();

  // Initialize the DomainParticipant associated with the context.
  rmw_ret_t
  initialize_participant(const char * node_name, const char * node_namespace);

  // Finalize the DomainParticipant associated with the context.
  rmw_ret_t
  finalize_participant();

  rmw_ret_t
  finalize();
};

#endif  // RMW_GURUMDDS_CPP__RMW_CONTEXT_IMPL_HPP_

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

#include <stdio.h>

#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <string>

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/topic_endpoint_info_array.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "rmw_gurumdds_cpp/dds_include.hpp"
#include "rmw_gurumdds_cpp/identifier.hpp"

#include "rcutils/strdup.h"

struct rmw_context_impl_t
{
  rmw_dds_common::Context common_ctx;
  rmw_context_t * base;

  dds_DomainId_t domain_id;
  dds_DomainParticipant * participant;

  /* used for all DDS writers/readers created to support RMW Gurumdds(Publisher/Subscriber)Info. */
  dds_Publisher * publisher;
  dds_Subscriber * subscriber;

  bool localhost_only;
  bool service_mapping_basic;

  /* Participant reference count */
  size_t node_count{0};

  /* Mutex used to protect initialization/destruction. */
  std::mutex initialization_mutex;

  /* Shutdown flag. */
  bool is_shutdown;

  std::mutex endpoint_mutex;

  explicit rmw_context_impl_t(rmw_context_t * const base)
  : common_ctx(),
    base(base),
    domain_id(0u),
    participant(nullptr),
    publisher(nullptr),
    subscriber(nullptr),
    localhost_only(base->options.localhost_only == RMW_LOCALHOST_ONLY_ENABLED)
  {
    /* destructor relies on these being initialized properly */
    common_ctx.thread_is_running.store(false);
    common_ctx.graph_guard_condition = nullptr;
    common_ctx.pub = nullptr;
    common_ctx.sub = nullptr;
  }

  ~rmw_context_impl_t()
  {
    if (0u != this->node_count) {
      RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "not all nodes finalized: %lu", this->node_count);
    }
  }

  // Initializes the participant, if it wasn't done already.
  // node_count is increased
  rmw_ret_t
  initialize_node(const char * node_name, const char * node_namespace, const bool localhost_only);

  // Destroys the participant, when node_count reaches 0.
  rmw_ret_t
  finalize_node();

  // Initialize the DomainParticipant associated with the context.
  rmw_ret_t
  initialize_participant(
    const char * node_name,
    const char * node_namespace,
    const bool localhost_only);

  // Finalize the DomainParticipant associated with the context.
  rmw_ret_t
  finalize_participant();

  rmw_ret_t
  finalize();
};

#endif  // RMW_GURUMDDS_CPP__RMW_CONTEXT_IMPL_HPP_

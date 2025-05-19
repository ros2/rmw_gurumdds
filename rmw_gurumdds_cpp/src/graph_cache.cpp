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

#include "rcpputils/scope_exit.hpp"

#include "rmw/publisher_options.h"
#include "rmw/subscription_options.h"
#include "rmw/qos_profiles.h"
#include "rmw_dds_common/qos.hpp"

#include "rmw_gurumdds_cpp/context_listener_thread.hpp"
#include "rmw_gurumdds_cpp/graph_cache.hpp"
#include "rmw_gurumdds_cpp/qos.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"
#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/rmw_publisher.hpp"
#include "rmw_gurumdds_cpp/rmw_subscription.hpp"

#include "rosidl_typesupport_cpp/message_type_support.hpp"

static rmw_ret_t add_entity(
  rmw_context_impl_t * ctx,
  const rmw_gid_t * const endp_gid,
  const rmw_gid_t * const dp_gid,
  const char * const topic_name,
  const char * const type_name,
  const rosidl_type_hash_s & type_hash,
  const dds_HistoryQosPolicy * const history,
  const dds_ReliabilityQosPolicy * const reliability,
  const dds_DurabilityQosPolicy * const durability,
  const dds_DeadlineQosPolicy * const deadline,
  const dds_LivelinessQosPolicy * const liveliness,
  const dds_LifespanQosPolicy * const lifespan,
  const bool is_reader,
  const bool local) {
  RCUTILS_UNUSED(local);
  size_t history_depth = RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT;
  rmw_qos_history_policy_e history_kind = RMW_QOS_POLICY_HISTORY_UNKNOWN;
  if (history != nullptr) {
    history_depth = static_cast<size_t>(history->depth);
    history_kind = rmw_gurumdds_cpp::convert_history(history);
  }

  rmw_qos_profile_t qos_profile{
    history_kind,
    history_depth,
    rmw_gurumdds_cpp::convert_reliability(reliability),
    rmw_gurumdds_cpp::convert_durability(durability),
    rmw_gurumdds_cpp::convert_deadline(deadline),
    rmw_gurumdds_cpp::convert_lifespan(lifespan),
    rmw_gurumdds_cpp::convert_liveliness(liveliness),
    rmw_gurumdds_cpp::convert_liveliness_lease_duration(liveliness),
    false,
  };

  const uint32_t * const dp_gid_32_arr = reinterpret_cast<const uint32_t *>(dp_gid->data);
  const uint32_t * const endp_gid_32_arr = reinterpret_cast<const uint32_t *>(endp_gid->data);
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "[context_listener thread] assert endpoint: "
    "ctx=%p, "
    "cache=%p, "
    "dp_gid=0x%08X.0x%08X.0x%08X.0x%08X, "
    "gid=0x%08X.0x%08X.0x%08X.0x%08X, "
    "topic=%s, "
    "type=%s, "
    "reader=%d, "
    "local=%d",
    reinterpret_cast<void *>(ctx),
    reinterpret_cast<void *>(&ctx->common_ctx.graph_cache),
    dp_gid_32_arr[0],
    dp_gid_32_arr[1],
    dp_gid_32_arr[2],
    dp_gid_32_arr[3],
    endp_gid_32_arr[0],
    endp_gid_32_arr[1],
    endp_gid_32_arr[2],
    endp_gid_32_arr[3],
    topic_name,
    type_name,
    is_reader,
    local);

  if (!ctx->common_ctx.graph_cache.add_entity(
    *endp_gid,
    std::string(topic_name),
    std::string(type_name),
    type_hash,
    *dp_gid,
    qos_profile,
    is_reader)) {
    RCUTILS_LOG_DEBUG_NAMED(
      RMW_GURUMDDS_ID,
      "failed to add entity to cache: "
      "gid=0x%08X.0x%08X.0x%08X.0x%08X, "
      "topic=%s, "
      "type=%s",
      endp_gid_32_arr[0],
      endp_gid_32_arr[1],
      endp_gid_32_arr[2],
      endp_gid_32_arr[3],
      topic_name,
      type_name);
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

static rmw_ret_t remove_entity(
  rmw_context_impl_t * const ctx,
  const rmw_gid_t gid,
  const bool is_reader) {
  if (!ctx->common_ctx.graph_cache.remove_entity(gid, is_reader)) {
    RMW_SET_ERROR_MSG("failed to remove entity from graph_cache");
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "[context_listener thread] remove endpoint: "
    "ctx=%p, "
    "cache=%p, "
    "gid=0x%08X.0x%08X.0x%08X.0x%08X, ",
    reinterpret_cast<void *>(ctx),
    reinterpret_cast<void *>(&ctx->common_ctx.graph_cache),
    reinterpret_cast<const uint32_t *>(gid.data)[0],
    reinterpret_cast<const uint32_t *>(gid.data)[1],
    reinterpret_cast<const uint32_t *>(gid.data)[2],
    reinterpret_cast<const uint32_t *>(gid.data)[3]);

  return RMW_RET_OK;
}

static rmw_ret_t add_local_publisher(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  dds_DataWriter * const datawriter,
  const rosidl_type_hash_s & type_hash,
  const rmw_gid_t gid) {
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "[graph] local publisher created: "
    "node=%s::%s, "
    "dp_gid=%08X.%08X.%08X.%08X, "
    "gid=%08X.%08X.%08X.%08X",
    node->namespace_,
    node->name,
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[0],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[1],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[2],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[3],
    reinterpret_cast<const uint32_t *>(gid.data)[0],
    reinterpret_cast<const uint32_t *>(gid.data)[1],
    reinterpret_cast<const uint32_t *>(gid.data)[2],
    reinterpret_cast<const uint32_t *>(gid.data)[3]);

  dds_DataWriterQos dw_qos;
  dds_DataWriterQos * dw_qos_ptr = &dw_qos;

  dds_Topic * topic = dds_DataWriter_get_topic(datawriter);
  const char * topic_name = dds_Topic_get_name(topic);
  const char * type_name = dds_Topic_get_type_name(topic);

  dds_ReturnCode_t ret = dds_DataWriterQos_copy(&dw_qos, &dds_DATAWRITER_QOS_DEFAULT);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to initialize DataWriterQos");
    return RMW_RET_ERROR;
  }

  auto scope_exit_qos_reset = rcpputils::make_scope_exit([dw_qos_ptr]() {
    if (dds_RETCODE_OK != dds_DataWriterQos_finalize(dw_qos_ptr)) {
      RMW_SET_ERROR_MSG("failed to finalize DataWriterQos");
    }
  });

  if (dds_RETCODE_OK != dds_DataWriter_get_qos(datawriter, &dw_qos)) {
    RMW_SET_ERROR_MSG("failed to get DataWriterQos");
    return RMW_RET_ERROR;
  }

  return add_entity(
    ctx,
    &gid,
    &ctx->common_ctx.gid,
    topic_name,
    type_name,
    type_hash,
    &dw_qos.history,
    &dw_qos.reliability,
    &dw_qos.durability,
    &dw_qos.deadline,
    &dw_qos.liveliness,
    &dw_qos.lifespan,
    false,
    true);
}

static rmw_ret_t add_local_subscriber(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  dds_DataReader * const datareader,
  const rosidl_type_hash_s & type_hash,
  const rmw_gid_t gid) {
  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "[graph] local subscriber created: "
    "node=%s::%s, "
    "dp_gid=%08X.%08X.%08X.%08X, "
    "gid=%08X.%08X.%08X.%08X",
    node->namespace_, node->name,
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[0],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[1],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[2],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[3],
    reinterpret_cast<const uint32_t *>(gid.data)[0],
    reinterpret_cast<const uint32_t *>(gid.data)[1],
    reinterpret_cast<const uint32_t *>(gid.data)[2],
    reinterpret_cast<const uint32_t *>(gid.data)[3]);

  dds_DataReaderQos dr_qos;
  dds_DataReaderQos * dr_qos_ptr = &dr_qos;

  auto topic = reinterpret_cast<dds_Topic *>(dds_DataReader_get_topicdescription(datareader));
  const char * topic_name = dds_Topic_get_name(topic);
  const char * type_name = dds_Topic_get_type_name(topic);

  dds_ReturnCode_t ret = dds_DataReaderQos_copy(&dr_qos, &dds_DATAREADER_QOS_DEFAULT);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to initialize DataReaderQos");
    return RMW_RET_ERROR;
  }

  auto scope_exit_qos_reset = rcpputils::make_scope_exit([dr_qos_ptr]() {
    if (dds_RETCODE_OK != dds_DataReaderQos_finalize(dr_qos_ptr)) {
      RMW_SET_ERROR_MSG("failed to finalize DataReaderQos");
    }
  });

  if (dds_RETCODE_OK != dds_DataReader_get_qos(datareader, &dr_qos)) {
    RMW_SET_ERROR_MSG("failed to get DataReaderQos");
    return RMW_RET_ERROR;
  }

  return add_entity(
    ctx,
    &gid,
    &ctx->common_ctx.gid,
    topic_name,
    type_name,
    type_hash,
    &dr_qos.history,
    &dr_qos.reliability,
    &dr_qos.durability,
    &dr_qos.deadline,
    &dr_qos.liveliness,
    nullptr,
    true,
    true);
}

namespace rmw_gurumdds_cpp::graph_cache {
rmw_ret_t
initialize(rmw_context_impl_t * const ctx)
{
  rmw_qos_profile_t qos = rmw_qos_profile_default;
  qos.avoid_ros_namespace_conventions = true;
  qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
  qos.depth = 100;
  qos.durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
  qos.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;

  rmw_publisher_options_t publisher_options = rmw_get_default_publisher_options();
  rmw_subscription_options_t subscription_options = rmw_get_default_subscription_options();

  // This is currently not implemented in gurumdds
  subscription_options.ignore_local_publications = true;

  const rosidl_message_type_support_t * const type_supports_partinfo =
    rosidl_typesupport_cpp::get_message_type_support_handle<
    rmw_dds_common::msg::ParticipantEntitiesInfo>();

  const char * const topic_name_partinfo = "ros_discovery_info";

  ctx->common_ctx.pub =
    rmw_gurumdds_cpp::create_publisher(
    ctx,
    nullptr,
    ctx->participant,
    ctx->publisher,
    type_supports_partinfo,
    topic_name_partinfo,
    &qos,
    &publisher_options,
    true);

  if (ctx->common_ctx.pub == nullptr) {
    RCUTILS_LOG_ERROR_NAMED(
      RMW_GURUMDDS_ID, "failed to create publisher for ParticipantEntityInfo");
    return RMW_RET_ERROR;
  }

  qos.history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;

  ctx->common_ctx.sub =
    create_subscription(
    ctx,
    nullptr,
    ctx->participant,
    ctx->subscriber,
    type_supports_partinfo,
    topic_name_partinfo,
    &qos,
    &subscription_options,
    true);

  if (ctx->common_ctx.sub == nullptr) {
    RCUTILS_LOG_ERROR_NAMED(
      RMW_GURUMDDS_ID, "failed to create subscription for ParticipantEntityInfo");
    return RMW_RET_ERROR;
  }

  ctx->common_ctx.graph_guard_condition =
    rmw_create_guard_condition(ctx->base);
  if (ctx->common_ctx.graph_guard_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create graph guard condition");
    return RMW_RET_BAD_ALLOC;
  }

  ctx->common_ctx.graph_cache.set_on_change_callback(
    [gcond = ctx->common_ctx.graph_guard_condition]()
    {
      rmw_ret_t ret = rmw_trigger_guard_condition(gcond);
      if (ret != RMW_RET_OK) {
        RMW_SET_ERROR_MSG("failed to trigger graph cache on_change_callback");
      }
    });

  ctx->common_ctx.publish_callback = [](const rmw_publisher_t * pub, const void * msg) {
    return rmw_gurumdds_cpp::publish(
      RMW_GURUMDDS_ID,
      pub,
      msg,
      nullptr);
  };

  entity_get_gid(reinterpret_cast<dds_Entity *>(ctx->participant), ctx->common_ctx.gid);
  std::string dp_enclave = ctx->base->options.enclave;
  ctx->common_ctx.graph_cache.add_participant(ctx->common_ctx.gid, dp_enclave);

  return RMW_RET_OK;
}

rmw_ret_t
finalize(rmw_context_impl_t * const ctx)
{
  if (RMW_RET_OK != rmw_gurumdds_cpp::stop_listener_thread(ctx->base)) {
    RMW_SET_ERROR_MSG("failed to stop listener thread");
    return RMW_RET_ERROR;
  }

  ctx->common_ctx.graph_cache.clear_on_change_callback();

  if (ctx->common_ctx.graph_guard_condition) {
    if (RMW_RET_OK !=
      rmw_destroy_guard_condition(ctx->common_ctx.graph_guard_condition))
    {
      RMW_SET_ERROR_MSG("failed to destroy graph guard condition");
      return RMW_RET_ERROR;
    }
    ctx->common_ctx.graph_guard_condition = nullptr;
  }

  if (ctx->common_ctx.sub != nullptr) {
    if (RMW_RET_OK !=
      destroy_subscription(ctx, ctx->common_ctx.sub))
    {
      RMW_SET_ERROR_MSG("failed to destroy discovery subscriber");
      return RMW_RET_ERROR;
    }
    ctx->common_ctx.sub = nullptr;
  }

  if (ctx->common_ctx.pub != nullptr) {
    if (RMW_RET_OK !=
      rmw_gurumdds_cpp::destroy_publisher(ctx, ctx->common_ctx.pub))
    {
      RMW_SET_ERROR_MSG("failed to destroy discovery publisher");
      return RMW_RET_ERROR;
    }
    ctx->common_ctx.pub = nullptr;
  }

  return RMW_RET_OK;
}

rmw_ret_t
enable(rmw_context_t * const ctx)
{
  if (rmw_gurumdds_cpp::run_listener_thread(ctx) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to start discovery listener thread");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
publish_update(
  rmw_context_impl_t * const ctx,
  void * const msg)
{
  if (ctx->common_ctx.pub == nullptr) {
    RMW_SET_ERROR_MSG("context already finalized, message not published");
    return RMW_RET_OK;
  }

  if (rmw_publish(ctx->common_ctx.pub, msg, nullptr) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to publish discovery sample");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
on_node_created(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node)
{
  rmw_ret_t rc = ctx->common_ctx.add_node_graph(
    node->name, node->namespace_);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to add node graph");
    return RMW_RET_ERROR;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "[graph] local node created: "
    "node=%s::%s, "
    "dp_gid=%08X.%08X.%08X.%08X",
    node->namespace_,
    node->name,
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[0],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[1],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[2],
    reinterpret_cast<const uint32_t *>(ctx->common_ctx.gid.data)[3]);

  return RMW_RET_OK;
}

rmw_ret_t
on_node_deleted(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node)
{
  rmw_ret_t rc = ctx->common_ctx.remove_node_graph(
    node->name, node->namespace_);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to remove node graph");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
on_publisher_created(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  PublisherInfo * const pub)
{
  auto msg_typesupport = pub->rosidl_message_typesupport;
  auto& type_hash = *msg_typesupport->get_type_hash_func(msg_typesupport);
  rmw_ret_t rc = add_local_publisher(ctx, node, pub->topic_writer, type_hash, pub->publisher_gid);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to add local publisher");
    return RMW_RET_ERROR;
  }

  rc = ctx->common_ctx.add_publisher_graph(
    pub->publisher_gid, node->name, node->namespace_);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to add publisher graph");
    remove_entity(ctx, pub->publisher_gid, false);
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
on_publisher_deleted(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  PublisherInfo * const pub)
{
  rmw_ret_t rc = remove_entity(ctx, pub->publisher_gid, false);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to remove entity of publisher");
    return RMW_RET_ERROR;
  }

  rc = ctx->common_ctx.remove_publisher_graph(
    pub->publisher_gid, node->name, node->namespace_);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to remove remove publisher graph");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
on_subscriber_created(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  SubscriberInfo * const sub)
{
  auto msg_typesupport = sub->rosidl_message_typesupport;
  auto& type_hash = *msg_typesupport->get_type_hash_func(msg_typesupport);
  rmw_ret_t rc = add_local_subscriber(ctx, node, sub->topic_reader, type_hash, sub->subscriber_gid);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to add local subscriber");
    remove_entity(ctx, sub->subscriber_gid, true);
    return RMW_RET_ERROR;
  }

  rc = ctx->common_ctx.add_subscriber_graph(
    sub->subscriber_gid, node->name, node->namespace_);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to add subscriber graph");
    remove_entity(ctx, sub->subscriber_gid, false);
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
on_subscriber_deleted(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  SubscriberInfo * const sub)
{
  rmw_ret_t rc = remove_entity(ctx, sub->subscriber_gid, true);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to remove entity of subscriber");
    return RMW_RET_ERROR;
  }

  rc = ctx->common_ctx.remove_subscriber_graph(
    sub->subscriber_gid, node->name, node->namespace_);
  if (RMW_RET_OK != rc) {
    RMW_SET_ERROR_MSG("failed to remove remove subscriber graph");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
on_service_created(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  ServiceInfo * const svc)
{
  const rmw_gid_t pub_gid = svc->publisher_gid;
  const rmw_gid_t sub_gid = svc->subscriber_gid;
  bool add_pub = false;
  bool add_sub = false;

  auto scope_exit_entities_reset = rcpputils::make_scope_exit(
    [ctx, pub_gid, sub_gid, add_sub, add_pub]()
    {
      if (add_sub) {
        remove_entity(ctx, sub_gid, true);
      }
      if (add_pub) {
        remove_entity(ctx, pub_gid, false);
      }
    });

  const rosidl_message_type_support_t* type_support;
  const rosidl_type_hash_s* type_hash;
  type_support = svc->service_typesupport->request_typesupport;
  type_hash = type_support->get_type_hash_func(type_support);
  if (RMW_RET_OK != add_local_subscriber(ctx, node, svc->request_reader, *type_hash,  sub_gid)) {
    RMW_SET_ERROR_MSG("failed to add local subscriber");
    return RMW_RET_ERROR;
  }
  add_sub = true;

  type_support = svc->service_typesupport->response_typesupport;
  type_hash = type_support->get_type_hash_func(type_support);
  if (RMW_RET_OK != add_local_publisher(ctx, node, svc->response_writer, *type_hash, pub_gid)) {
    RMW_SET_ERROR_MSG("failed to add local publisher");
    return RMW_RET_ERROR;
  }
  add_pub = true;

  if(RMW_RET_OK != ctx->common_ctx.add_service_graph(
    sub_gid, pub_gid, node->name, node->namespace_)) {
    RMW_SET_ERROR_MSG("failed to add service graph");
    return RMW_RET_ERROR;
  }

  scope_exit_entities_reset.cancel();
  return RMW_RET_OK;
}

rmw_ret_t
on_service_deleted(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  ServiceInfo * const svc)
{
  bool failed = false;
  rmw_ret_t rc = remove_entity(ctx, svc->subscriber_gid, true);
  failed = failed && (RMW_RET_OK == rc);

  rc = remove_entity(ctx, svc->publisher_gid, false);
  failed = failed && (RMW_RET_OK == rc);

  rc = ctx->common_ctx.remove_service_graph(
    svc->subscriber_gid, svc->publisher_gid, node->name, node->namespace_
  );
  failed = failed && (RMW_RET_OK == rc);

  return failed ? RMW_RET_ERROR : RMW_RET_OK;
}

rmw_ret_t
on_client_created(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  ClientInfo * const client)
{
  const rmw_gid_t pub_gid = client->publisher_gid;
  const rmw_gid_t sub_gid = client->subscriber_gid;
  bool add_pub = false;
  bool add_sub = false;

  auto scope_exit_entities_reset = rcpputils::make_scope_exit(
    [ctx, pub_gid, sub_gid, add_sub, add_pub]()
    {
      if (add_sub) {
        remove_entity(ctx, sub_gid, true);
      }
      if (add_pub) {
        remove_entity(ctx, pub_gid, false);
      }
    });

  const rosidl_message_type_support_t* type_support;
  const rosidl_type_hash_s* type_hash;
  type_support = client->service_typesupport->response_typesupport;
  type_hash = type_support->get_type_hash_func(type_support);
  if (RMW_RET_OK != add_local_subscriber(ctx, node, client->response_reader, *type_hash,
                                         sub_gid)) {
    RMW_SET_ERROR_MSG("failed to add local subscriber");
    return RMW_RET_ERROR;
  }
  add_sub = true;

  type_support = client->service_typesupport->request_typesupport;
  type_hash = type_support->get_type_hash_func(type_support);
  if (RMW_RET_OK != add_local_publisher(ctx, node, client->request_writer, *type_hash, pub_gid)) {
    RMW_SET_ERROR_MSG("failed to add local publisher");
    return RMW_RET_ERROR;
  }
  add_pub = true;

  if(RMW_RET_OK != ctx->common_ctx.add_client_graph(pub_gid, sub_gid, node->name,
                                                    node->namespace_)) {
    RMW_SET_ERROR_MSG("failed to add client graph");
    return RMW_RET_ERROR;
  }

  scope_exit_entities_reset.cancel();
  return RMW_RET_OK;
}

rmw_ret_t
on_client_deleted(
  rmw_context_impl_t * const ctx,
  const rmw_node_t * const node,
  ClientInfo * const client)
{
  bool failed = false;
  rmw_ret_t rc = remove_entity(ctx, client->subscriber_gid, true);
  failed = failed && (RMW_RET_OK == rc);

  rc = remove_entity(ctx, client->publisher_gid, false);
  failed = failed && (RMW_RET_OK == rc);

  rc = ctx->common_ctx.remove_service_graph(
    client->subscriber_gid, client->publisher_gid, node->name, node->namespace_
  );
  failed = failed && (RMW_RET_OK == rc);

  return failed ? RMW_RET_ERROR : RMW_RET_OK;
}

rmw_ret_t
on_participant_info(rmw_context_impl_t * ctx)
{
  bool taken = false;
  rmw_dds_common::msg::ParticipantEntitiesInfo msg;

  do {
    if (RMW_RET_OK != rmw_take(ctx->common_ctx.sub, &msg, &taken,
                               nullptr)) {
      RMW_SET_ERROR_MSG("failed to take discovery sample");
      return RMW_RET_ERROR;
    }
    if (taken) {
      if (std::memcmp(&msg.gid.data, ctx->common_ctx.gid.data,
                      RMW_GID_STORAGE_SIZE) == 0) {
        continue;
      }

      RCUTILS_LOG_DEBUG_NAMED(
        RMW_GURUMDDS_ID,
        "---- updating participant entities: "
        "0x%08X.0x%08X.0x%08X.0x%08X",
        reinterpret_cast<const uint32_t *>(&msg.gid.data)[0],
        reinterpret_cast<const uint32_t *>(&msg.gid.data)[1],
        reinterpret_cast<const uint32_t *>(&msg.gid.data)[2],
        reinterpret_cast<const uint32_t *>(&msg.gid.data)[3]);

      ctx->common_ctx.graph_cache.update_participant_entities(msg);
    }
  } while (taken);

  return RMW_RET_OK;
}

rmw_ret_t
add_participant(
  rmw_context_impl_t * const ctx,
  const dds_GUID_t * const dp_guid,
  const char * const enclave)
{
  rmw_gid_t gid;
  rmw_gurumdds_cpp::guid_to_gid(*dp_guid, gid);

  if (0 == std::memcmp(gid.data, ctx->common_ctx.gid.data, RMW_GID_STORAGE_SIZE)) {
    // Ignore own announcements
    return RMW_RET_OK;
  }

  std::string enclave_str;
  if (nullptr != enclave) {
    enclave_str = enclave;
  }

  ctx->common_ctx.graph_cache.add_participant(gid, enclave_str);

  return RMW_RET_OK;
}

rmw_ret_t
remove_participant(
  rmw_context_impl_t * const ctx,
  const dds_GUID_t * const dp_guid)
{
  rmw_gid_t gid;
  rmw_gurumdds_cpp::guid_to_gid(*dp_guid, gid);

  if (0 == std::memcmp(gid.data, ctx->common_ctx.gid.data, RMW_GID_STORAGE_SIZE)) {
    // Ignore own announcements
    return RMW_RET_OK;
  }

  ctx->common_ctx.graph_cache.remove_participant(gid);

  return RMW_RET_OK;
}

rmw_ret_t
add_remote_entity(
  rmw_context_impl_t * ctx,
  const dds_GUID_t * const endp_guid,
  const dds_GUID_t * const dp_guid,
  const char * const topic_name,
  const char * const type_name,
  const dds_UserDataQosPolicy& user_data,
  const dds_ReliabilityQosPolicy * const reliability,
  const dds_DurabilityQosPolicy * const durability,
  const dds_DeadlineQosPolicy * const deadline,
  const dds_LivelinessQosPolicy * const liveliness,
  const dds_LifespanQosPolicy * const lifespan,
  const bool is_reader)
{
  rmw_gid_t endp_gid, dp_gid;
  rmw_gurumdds_cpp::guid_to_gid(*endp_guid, endp_gid);
  rmw_gurumdds_cpp::guid_to_gid(*dp_guid, dp_gid);

  if (0 == std::memcmp(dp_gid.data, ctx->common_ctx.gid.data, RMW_GID_STORAGE_SIZE)) {
    // Ignore own announcements
    return RMW_RET_OK;
  }

  rosidl_type_hash_s type_hash;
  if(RMW_RET_OK != rmw_dds_common::parse_type_hash_from_user_data(
    user_data.value, user_data.size, type_hash)) {
    type_hash = rosidl_get_zero_initialized_type_hash();
    rmw_reset_error();
  }

  if (RMW_RET_OK != add_entity(
      ctx,
      &endp_gid,
      &dp_gid,
      topic_name,
      type_name,
      type_hash,
      nullptr,
      reliability,
      durability,
      deadline,
      liveliness,
      lifespan,
      is_reader,
      false))
  {
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
remove_entity(
  rmw_context_impl_t * const ctx,
  const dds_GUID_t * const guid,
  const bool is_reader)
{
  rmw_gid_t gid;
  rmw_gurumdds_cpp::guid_to_gid(*guid, gid);

  if (0 == std::memcmp(gid.data, ctx->common_ctx.gid.data, 12)) {
    // compare entities' GUID prefixes to determine whether they belong to the same participat
    // (hence 12 instead of 16 bytes)
    // Ignore own announcements
    return RMW_RET_OK;
  }

  return remove_entity(ctx, gid, is_reader);
}
}  // namespace rmw_gurumdds_cpp::graph_cache

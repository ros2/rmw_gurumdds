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

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "rmw/impl/cpp/key_value.hpp"

#include "rmw_gurumdds_cpp/gid.hpp"
#include "rmw_gurumdds_cpp/rmw_context_impl.hpp"

using rmw_dds_common::msg::ParticipantEntitiesInfo;

namespace rmw_gurumdds_cpp
{
static inline std::map<std::string, std::vector<uint8_t>>
parse_map(uint8_t * const data, const uint32_t data_len)
{
  std::vector<uint8_t> data_vec(data, data + data_len);
  std::map<std::string, std::vector<uint8_t>> map
    = rmw::impl::cpp::parse_key_value(data_vec);
  return map;
}

static inline rmw_ret_t
get_user_data_key(
  dds_ParticipantBuiltinTopicData * data,
  const std::string& key,
  std::string & value,
  bool & found)
{
  found = false;
  uint8_t * user_data =
    static_cast<uint8_t *>(data->user_data.value);
  const uint32_t user_data_len = data->user_data.size;
  if (user_data_len == 0) {
    return RMW_RET_OK;
  }

  auto map = parse_map(user_data, user_data_len);
  auto name_found = map.find(key);
  if (name_found != map.end()) {
    value = std::string(name_found->second.begin(), name_found->second.end());
    found = true;
  }

  return RMW_RET_OK;
}

void on_participant_changed(
  const dds_DomainParticipant * a_participant,
  const dds_ParticipantBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  auto * participant = const_cast<dds_DomainParticipant *>(a_participant);
  auto * ctx = reinterpret_cast<rmw_context_impl_t *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0)
  );
  if (ctx == nullptr) {
    return;
  }

  rmw_gurumdds_cpp::Guid_t dp_guid{*data};
  if (handle == dds_HANDLE_NIL) {
    rmw_gurumdds_cpp::graph_cache::remove_participant(ctx, &dp_guid);
  } else {
    std::string enclave_str;
    bool enclave_found;
    dds_ReturnCode_t rc =
      get_user_data_key(
      const_cast<dds_ParticipantBuiltinTopicData *>(data),
      "securitycontext", enclave_str, enclave_found);
    if (RMW_RET_OK != rc) {
      RMW_SET_ERROR_MSG("failed to parse user data for enclave");
    }

    const char * enclave = nullptr;
    if (enclave_found) {
      enclave = enclave_str.c_str();
    }

    if (RMW_RET_OK != rmw_gurumdds_cpp::graph_cache::add_participant(ctx, &dp_guid, enclave)) {
      RMW_SET_ERROR_MSG("failed to assert remote participant in graph");
    }
  }
}

void on_publication_changed(
  const dds_DomainParticipant * a_participant,
  const dds_PublicationBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  auto * participant = const_cast<dds_DomainParticipant *>(a_participant);
  auto * ctx = reinterpret_cast<rmw_context_impl_t *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0)
  );
  if (ctx == nullptr) {
    return;
  }

  rmw_gurumdds_cpp::Guid_t dp_guid = rmw_gurumdds_cpp::Guid_t::for_participant(*data);
  rmw_gurumdds_cpp::Guid_t endp_guid{*data};
  const auto * dp_guid_prefix = reinterpret_cast<const uint32_t *>(dp_guid.prefix);
  const auto * endp_guid_prefix = reinterpret_cast<const uint32_t *>(endp_guid.prefix);
  if (handle == dds_HANDLE_NIL) {
    RCUTILS_LOG_DEBUG_NAMED(
      "pub on data available",
      "[ud] endp_gid=0x%08X.0x%08X.0x%08X.0x%08X ",
      endp_guid_prefix[0],
      endp_guid_prefix[1],
      endp_guid_prefix[2],
      endp_guid.entityId);
    rmw_gurumdds_cpp::graph_cache::remove_entity(ctx, &endp_guid, false);
  } else {
    rmw_gurumdds_cpp::graph_cache::add_remote_entity(
      ctx,
      &endp_guid,
      &dp_guid,
      data->topic_name,
      data->type_name,
      data->user_data,
      &data->reliability,
      &data->durability,
      &data->deadline,
      &data->liveliness,
      &data->lifespan,
      false);
    RCUTILS_LOG_DEBUG_NAMED(
      "pub on data available",
      "dp_gid=0x%08X.0x%08X.0x%08X.0x%08X, "
      "gid=0x%08X.0x%08X.0x%08X.0x%08X, ",
      dp_guid_prefix[0],
      dp_guid_prefix[1],
      dp_guid_prefix[2],
      dp_guid.entityId,
      endp_guid_prefix[0],
      endp_guid_prefix[1],
      endp_guid_prefix[2],
      endp_guid.entityId);
  }
}

void on_subscription_changed(
  const dds_DomainParticipant * a_participant,
  const dds_SubscriptionBuiltinTopicData * data,
  dds_InstanceHandle_t handle)
{
  auto * participant = const_cast<dds_DomainParticipant *>(a_participant);
  auto * ctx = reinterpret_cast<rmw_context_impl_t *>(
    dds_Entity_get_context(reinterpret_cast<dds_Entity *>(participant), 0)
  );
  if (ctx == nullptr) {
    return;
  }

  rmw_gurumdds_cpp::Guid_t dp_guid = rmw_gurumdds_cpp::Guid_t::for_participant(*data);
  rmw_gurumdds_cpp::Guid_t endp_guid{*data};
  const auto * dp_guid_prefix = reinterpret_cast<const uint32_t *>(dp_guid.prefix);
  const auto * endp_guid_prefix = reinterpret_cast<const uint32_t *>(endp_guid.prefix);
  if (handle == dds_HANDLE_NIL) {
    RCUTILS_LOG_DEBUG_NAMED(
      "sub on data available",
      "[ud] endp_gid=0x%08X.0x%08X.0x%08X.0x%08X ",
      endp_guid_prefix[0],
      endp_guid_prefix[1],
      endp_guid_prefix[2],
      endp_guid.entityId);
    rmw_gurumdds_cpp::graph_cache::remove_entity(ctx, &endp_guid, false);
  } else {
    rmw_gurumdds_cpp::graph_cache::add_remote_entity(
      ctx,
      &endp_guid,
      &dp_guid,
      data->topic_name,
      data->type_name,
      data->user_data,
      &data->reliability,
      &data->durability,
      &data->deadline,
      &data->liveliness,
      nullptr,
      true);
    RCUTILS_LOG_DEBUG_NAMED(
      "sub on data available",
      "dp_gid=0x%08X.0x%08X.0x%08X.0x%08X, "
      "gid=0x%08X.0x%08X.0x%08X.0x%08X, ",
      dp_guid_prefix[0],
      dp_guid_prefix[1],
      dp_guid_prefix[2],
      dp_guid.entityId,
      endp_guid_prefix[0],
      endp_guid_prefix[1],
      endp_guid_prefix[2],
      endp_guid.entityId);
  }
}
} // namespace rmw_gurumdds_cpp

rmw_context_impl_s::rmw_context_impl_s(rmw_context_t* const base)
  : common_ctx(),
  base(base),
  domain_id(base->actual_domain_id),
  participant(nullptr),
  publisher(nullptr),
  subscriber(nullptr)
{
  /* destructor relies on these being initialized properly */
  common_ctx.thread_is_running.store(false);
  common_ctx.graph_guard_condition = nullptr;
  common_ctx.pub = nullptr;
  common_ctx.sub = nullptr;
}

rmw_context_impl_s::~rmw_context_impl_s()
{
  if (0u != this->node_count) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "not all nodes finalized: %lu", this->node_count);
  }
}

rmw_ret_t
rmw_context_impl_t::initialize_node(const char * node_name, const char * node_namespace)
{
  if (this->node_count != 0u) {
    this->node_count += 1;

    RCUTILS_LOG_DEBUG_NAMED(
      RMW_GURUMDDS_ID, "initialized new node, total node=%lu", this->node_count);

    return RMW_RET_OK;
  }

  rmw_ret_t ret = this->initialize_participant(node_name, node_namespace);
  if (ret != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to initialize DomainParticipant");
    return ret;
  }

  if (rmw_gurumdds_cpp::graph_cache::enable(this->base) != RMW_RET_OK) {
    dds_Entity_set_context(
      reinterpret_cast<dds_Entity *>(this->participant), 0, nullptr);
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "failed to enable graph cache");
    return RMW_RET_ERROR;
  }

  this->node_count = 1;

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID, "initialized first node, total node=%lu", this->node_count);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_context_impl_s::finalize_node()
{
  this->node_count--;
  if (0u != this->node_count) {
    // destruction shouldn't happen yet
    return RMW_RET_OK;
  }

  return this->finalize_participant();
}

rmw_ret_t
rmw_context_impl_s::initialize_participant(
  const char * node_name,
  const char * node_namespace)
{
  dds_PublisherQos publisher_qos{};
  dds_SubscriberQos subscriber_qos{};
  rmw_context_impl_s * const ctx = this;

  auto scope_exit_dp_finalize = rcpputils::make_scope_exit(
    [ctx, &publisher_qos, &subscriber_qos]()
    {
      dds_PublisherQos_finalize(&publisher_qos);
      dds_SubscriberQos_finalize(&subscriber_qos);

      if (RMW_RET_OK != ctx->finalize_participant()) {
        RMW_SET_ERROR_MSG("failed to finalize participant on error");
      }
    });

  dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();
  if (factory == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    return RMW_RET_ERROR;
  }

  const char * const * check_props = nullptr;
  uint32_t props_count;
  bool remote_support = false;
  const char * props_ptr;

  dds_DomainParticipantFactory_get_supported_participant_props(factory, &check_props, &props_count);
  for (uint32_t i = 0; i < props_count; i++) {
    props_ptr = strstr(check_props[i], "on_remote");
    if (props_ptr != nullptr) {
      remote_support = true;
      break;
    }
  }
  if (!remote_support) {
    RCUTILS_LOG_ERROR_NAMED(RMW_GURUMDDS_ID, "on_remote_callback is not supported");
    return RMW_RET_ERROR;
  }

  dds_DomainParticipantQos participant_qos;
  dds_ReturnCode_t ret =
    dds_DomainParticipantFactory_get_default_participant_qos(factory, &participant_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default domain participant qos");
    return RMW_RET_ERROR;
  }

  std::string node_user_data;
  node_user_data.reserve(256);
  node_user_data += "name=";
  node_user_data += node_name;
  node_user_data += ";namespace=";
  node_user_data += node_namespace;
  node_user_data += ";securitycontext=";
  node_user_data += base->options.enclave;
  node_user_data += ";";

  if (node_user_data.size() > sizeof(participant_qos.user_data.value)) {
    RCUTILS_LOG_ERROR_NAMED(
      RMW_GURUMDDS_ID,
      "node name, namespace and security context are too long - "
      "the sum of their lengths must be less than %zu",
      sizeof(participant_qos.user_data.value) - strlen("name=;namespace=;enclave=;"));
    return RMW_RET_ERROR;
  }

  participant_qos.user_data.size = node_user_data.size();
  std::memset(participant_qos.user_data.value, 0, sizeof(participant_qos.user_data.value));
  std::memcpy(participant_qos.user_data.value, node_user_data.c_str(), node_user_data.size());

  std::string static_discovery_id;
  static_discovery_id += node_namespace;
  static_discovery_id += node_name;

  /* Create DomainParticipant */
  if (RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST == this->base->options.discovery_options.automatic_discovery_range) {
    // TODO: localhost only
    dds_StringProperty props[] = {
      {const_cast<char *>("rtps.interface.ip"),
        const_cast<void *>(static_cast<const void *>("127.0.0.1"))},
      {const_cast<char *>("gurumdds.static_discovery.id"),
        const_cast<void *>(static_cast<const void *>(static_discovery_id.c_str()))},
      {const_cast<char *>("dcps.participant.listener.on_remote_participant_changed"),
        reinterpret_cast<void *>(rmw_gurumdds_cpp::on_participant_changed)},
      {const_cast<char *>("dcps.participant.listener.on_remote_publication_changed"),
        reinterpret_cast<void *>(rmw_gurumdds_cpp::on_publication_changed)},
      {const_cast<char *>("dcps.participant.listener.on_remote_subscription_changed"),
        reinterpret_cast<void *>(rmw_gurumdds_cpp::on_subscription_changed)},
      {nullptr, nullptr},
    };
    this->participant = dds_DomainParticipantFactory_create_participant_w_props(
      factory, this->domain_id, &participant_qos, nullptr, 0, props);
  } else {
    dds_StringProperty props[] = {
      {const_cast<char *>("gurumdds.static_discovery.id"),
        const_cast<void *>(static_cast<const void *>(static_discovery_id.c_str()))},
      {const_cast<char *>("dcps.participant.listener.on_remote_participant_changed"),
        reinterpret_cast<void *>(rmw_gurumdds_cpp::on_participant_changed)},
      {const_cast<char *>("dcps.participant.listener.on_remote_publication_changed"),
        reinterpret_cast<void *>(rmw_gurumdds_cpp::on_publication_changed)},
      {const_cast<char *>("dcps.participant.listener.on_remote_subscription_changed"),
        reinterpret_cast<void *>(rmw_gurumdds_cpp::on_subscription_changed)},
      {nullptr, nullptr},
    };
    this->participant = dds_DomainParticipantFactory_create_participant_w_props(
      factory, this->domain_id, &participant_qos, nullptr, 0, props);
  }

  if (this->participant == nullptr) {
    RMW_SET_ERROR_MSG("failed to create DomainParticipant");
    return RMW_RET_ERROR;
  }

  /* Create Publisher */
  ret = dds_DomainParticipant_get_default_publisher_qos(this->participant, &publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default publisher qos");
    return RMW_RET_ERROR;
  }

  this->publisher =
    dds_DomainParticipant_create_publisher(this->participant, &publisher_qos, nullptr, 0);
  if (this->publisher == nullptr) {
    RMW_SET_ERROR_MSG("failed to create publisher");
    return RMW_RET_ERROR;
  }

  ret = dds_PublisherQos_finalize(&publisher_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to finalize publisher qos");
    return RMW_RET_ERROR;
  }

  /* Create Subscriber */
  ret = dds_DomainParticipant_get_default_subscriber_qos(this->participant, &subscriber_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default subscriber qos");
    return RMW_RET_ERROR;
  }

  this->subscriber =
    dds_DomainParticipant_create_subscriber(this->participant, &subscriber_qos, nullptr, 0);
  if (this->subscriber == nullptr) {
    RMW_SET_ERROR_MSG("failed to create subscriber");
    return RMW_RET_ERROR;
  }

  ret = dds_SubscriberQos_finalize(&subscriber_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failedto finalize subscriber qos");
    return RMW_RET_ERROR;
  }

  // Initialize graph_cache
  if (rmw_gurumdds_cpp::graph_cache::initialize(this) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("failed to initialize graph cache");
    return RMW_RET_ERROR;
  }

  dds_Entity_set_context(
    reinterpret_cast<dds_Entity *>(this->participant), 0, reinterpret_cast<void *>(this));
  scope_exit_dp_finalize.cancel();

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "DomainParticipant initialized");

  return RMW_RET_OK;
}

rmw_ret_t
rmw_context_impl_s::finalize_participant()
{
  // Finalize graph_cache
  if (RMW_RET_OK != rmw_gurumdds_cpp::graph_cache::finalize(this)) {
    RMW_SET_ERROR_MSG("failed to finalize graph cache");
    return RMW_RET_ERROR;
  }

  /* Delete publisher */
  if (this->publisher != nullptr) {
    if (dds_RETCODE_OK !=
      dds_Publisher_delete_contained_entities(this->publisher))
    {
      RMW_SET_ERROR_MSG("failed to delete publisher's entities");
      return RMW_RET_ERROR;
    }

    if (dds_RETCODE_OK !=
      dds_DomainParticipant_delete_publisher(this->participant, this->publisher))
    {
      RMW_SET_ERROR_MSG("failed to delete publisher");
      return RMW_RET_ERROR;
    }
    this->publisher = nullptr;
  }

  /* Delete subscriber */
  if (this->subscriber != nullptr) {
    if (dds_RETCODE_OK !=
      dds_Subscriber_delete_contained_entities(this->subscriber))
    {
      RMW_SET_ERROR_MSG("failed to delete subscriber's entities");
      return RMW_RET_ERROR;
    }

    if (dds_RETCODE_OK !=
      dds_DomainParticipant_delete_subscriber(this->participant, this->subscriber))
    {
      RMW_SET_ERROR_MSG("failed to delete subscriber");
      return RMW_RET_ERROR;
    }
    this->subscriber = nullptr;
  }

  /* Delete DomainParticipant */
  if (this->participant != nullptr) {
    dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();
    if (factory == nullptr) {
      RMW_SET_ERROR_MSG("failed to get domain participant factory");
      return RMW_RET_ERROR;
    }

    if (dds_RETCODE_OK !=
      dds_DomainParticipantFactory_delete_participant(
        factory, this->participant))
    {
      RMW_SET_ERROR_MSG("failed to delete DomainParticipant");
      return RMW_RET_ERROR;
    }
    this->participant = nullptr;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "DomainParticipant finalized");

  return RMW_RET_OK;
}

rmw_ret_t
rmw_context_impl_s::finalize()
{
  dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();

  if (this->participant != nullptr) {
    if (dds_RETCODE_OK !=
      dds_DomainParticipantFactory_delete_participant(factory, this->participant))
    {
      RMW_SET_ERROR_MSG("failed to delete DomainParticipant");
      return RMW_RET_ERROR;
    }
    this->participant = nullptr;
  }

  RCUTILS_LOG_DEBUG_NAMED(
    RMW_GURUMDDS_ID,
    "RMW context finalized: %p",
    reinterpret_cast<void *>(this));

  return RMW_RET_OK;
}

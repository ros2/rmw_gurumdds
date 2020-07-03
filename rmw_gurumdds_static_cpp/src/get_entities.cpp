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

#include "rmw_gurumdds_shared_cpp/types.hpp"
#include "rmw_gurumdds_static_cpp/get_entities.hpp"
#include "rmw_gurumdds_static_cpp/identifier.hpp"
#include "rmw_gurumdds_static_cpp/types.hpp"

namespace rmw_gurumdds_static_cpp
{
dds_DomainParticipant *
get_participant(rmw_node_t * node)
{
  if (node == nullptr) {
    return nullptr;
  }

  if (node->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsNodeInfo * impl = static_cast<GurumddsNodeInfo *>(node->data);
  return impl->participant;
}

dds_Publisher *
get_publisher(rmw_publisher_t * publisher)
{
  if (publisher == nullptr) {
    return nullptr;
  }

  if (publisher->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsPublisherInfo * impl = static_cast<GurumddsPublisherInfo *>(publisher->data);
  return impl->publisher;
}

dds_DataWriter *
get_data_writer(rmw_publisher_t * publisher)
{
  if (publisher == nullptr) {
    return nullptr;
  }

  if (publisher->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsPublisherInfo * impl = static_cast<GurumddsPublisherInfo *>(publisher->data);
  return impl->topic_writer;
}

dds_Subscriber *
get_subscriber(rmw_subscription_t * subscription)
{
  if (subscription == nullptr) {
    return nullptr;
  }

  if (subscription->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsSubscriberInfo * impl = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  return impl->subscriber;
}

dds_DataReader *
get_data_reader(rmw_subscription_t * subscription)
{
  if (subscription == nullptr) {
    return nullptr;
  }

  if (subscription->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsSubscriberInfo * impl = static_cast<GurumddsSubscriberInfo *>(subscription->data);
  return impl->topic_reader;
}

dds_DataWriter *
get_request_data_writer(rmw_client_t * client)
{
  if (client == nullptr) {
    return nullptr;
  }

  if (client->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsClientInfo * impl = static_cast<GurumddsClientInfo *>(client->data);
  return impl->request_writer;
}

dds_DataReader *
get_response_data_writer(rmw_client_t * client)
{
  if (client == nullptr) {
    return nullptr;
  }

  if (client->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsClientInfo * impl = static_cast<GurumddsClientInfo *>(client->data);
  return impl->response_reader;
}

dds_DataReader *
get_request_data_reader(rmw_service_t * service)
{
  if (service == nullptr) {
    return nullptr;
  }

  if (service->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsServiceInfo * impl = static_cast<GurumddsServiceInfo *>(service->data);
  return impl->request_reader;
}

dds_DataWriter *
get_response_data_writer(rmw_service_t * service)
{
  if (service == nullptr) {
    return nullptr;
  }

  if (service->implementation_identifier != gurum_gurumdds_static_identifier) {
    return nullptr;
  }

  GurumddsServiceInfo * impl = static_cast<GurumddsServiceInfo *>(service->data);
  return impl->response_writer;
}
}  // namespace rmw_gurumdds_static_cpp

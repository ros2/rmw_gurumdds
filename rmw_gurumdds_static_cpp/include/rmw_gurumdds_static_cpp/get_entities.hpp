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

#ifndef RMW_GURUMDDS_STATIC_CPP__GET_ENTITIES_HPP_
#define RMW_GURUMDDS_STATIC_CPP__GET_ENTITIES_HPP_

#include "rmw/rmw.h"
#include "rmw_gurumdds_static_cpp/visibility_control.h"
#include "rmw_gurumdds_shared_cpp/dds_include.hpp"

namespace rmw_gurumdds_static_cpp
{

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DomainParticipant *
get_participant(rmw_node_t * node);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_Publisher *
get_publisher(rmw_publisher_t * publisher);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DataWriter *
get_data_writer(rmw_publisher_t * publisher);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_Subscriber *
get_subscriber(rmw_subscription_t * subscription);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DataReader *
get_data_reader(rmw_subscription_t * subscription);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DataWriter *
get_request_data_writer(rmw_client_t * client);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DataReader *
get_response_data_reader(rmw_client_t * client);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DataReader *
get_request_data_reader(rmw_service_t * service);

RMW_GURUMDDS_STATIC_CPP_PUBLIC
dds_DataWriter *
get_response_data_writer(rmw_service_t * service);

}  // namespace rmw_gurumdds_static_cpp

#endif  // RMW_GURUMDDS_STATIC_CPP__GET_ENTITIES_HPP_

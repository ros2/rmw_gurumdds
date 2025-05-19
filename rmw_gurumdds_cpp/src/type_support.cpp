// Copyright 2024 GurumNetworks, Inc.
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

#include "rmw_gurumdds_cpp/type_support.hpp"
#include "rmw/error_handling.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/type_support_common.hpp"
#include "rmw_gurumdds_cpp/type_support_service.hpp"

namespace rmw_gurumdds_cpp
{

static size_t get_size(void* context) {
  auto rosidl_typesupport = reinterpret_cast<const rosidl_message_type_support_t *>(context);
  if (rosidl_typesupport->typesupport_identifier ==
  rosidl_typesupport_introspection_c__identifier) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
        rosidl_typesupport->data);
    return members->size_of_;
  } else if (rosidl_typesupport->typesupport_identifier ==
  rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
        rosidl_typesupport->data);
    return members->size_of_;
  } else {
    return 0;
  }
}

static size_t get_serialize_size(void* context, void* data) {
  auto rosidl_typesupport = reinterpret_cast<const rosidl_message_type_support_t *>(context);
  return get_serialized_size(rosidl_typesupport->data,
                             rosidl_typesupport->typesupport_identifier, data);
}

static size_t serialize_direct(void* context, void* data, void* buffer, size_t buffer_size) {
  auto rosidl_typesupport = reinterpret_cast<const rosidl_message_type_support_t *>(context);
  serialize_ros_to_cdr(
    rosidl_typesupport->data,
    rosidl_typesupport->typesupport_identifier,
    data,
    buffer,
    buffer_size);
  return buffer_size;
}

static bool deserialize_direct(void* context, void* buffer, size_t buffer_size, void* data) {
  auto rosidl_typesupport = reinterpret_cast<const rosidl_message_type_support_t *>(context);
  return deserialize_cdr_to_ros(
    rosidl_typesupport->data,
    rosidl_typesupport->typesupport_identifier,
    data,
    buffer,
    buffer_size);
}

dds_TypeSupport*
create_type_support_and_register(
  dds_DomainParticipant * participant,
  const rosidl_message_type_support_t * type_support,
  const std::string & type_name,
  const std::string & metastring) {
  dds_ReturnCode_t ret = dds_RETCODE_OK;
  dds_TypeSupport * dds_type_support{};
  dds_TypeSupport_ops dds_ops{};
  dds_type_support = dds_TypeSupport_create(metastring.c_str());
  if(nullptr == dds_type_support) {
    RMW_SET_ERROR_MSG("failed to create typesupport");
    return nullptr;
  }

  dds_ops.context = const_cast<rosidl_message_type_support_t *>(type_support);
  dds_ops.get_size = get_size;
  dds_ops.get_serialized_size = get_serialize_size;
  dds_ops.serialize_direct = serialize_direct;
  dds_ops.deserialize_direct = deserialize_direct;
  dds_TypeSupport_set_operations(dds_type_support, &dds_ops);
  ret = dds_TypeSupport_register_type(dds_type_support, participant, type_name.c_str());
  if(dds_RETCODE_OK != ret) {
    RMW_SET_ERROR_MSG("failed to register type to domain participant");
    dds_TypeSupport_delete(dds_type_support);
    return nullptr;
  }

  return dds_type_support;
}

void set_type_support_ops(
  dds_TypeSupport* dds_type_support,
  const rosidl_message_type_support_t* type_support) {
  dds_TypeSupport_ops dds_ops{};
  dds_ops.context = const_cast<rosidl_message_type_support_t *>(type_support);
  dds_ops.get_size = get_size;
  dds_ops.get_serialized_size = get_serialize_size;
  dds_ops.serialize_direct = serialize_direct;
  dds_ops.deserialize_direct = deserialize_direct;
  dds_TypeSupport_set_operations(dds_type_support, &dds_ops);
}
template<typename ServiceMembersT>
size_t get_request_enhanced_size_tmpl(const void * untyped_members)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return 0;
  }

  return members->request_members_->size_of_;
}

size_t get_request_enhanced_size(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return get_request_enhanced_size_tmpl<rosidl_typesupport_introspection_c__ServiceMembers>(
        untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return get_request_enhanced_size_tmpl<rosidl_typesupport_introspection_cpp::ServiceMembers>(
        untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return 0;
}

template<typename ServiceMembersT>
size_t get_response_enhanced_size_tmpl(const void * untyped_members)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return 0;
  }

  return members->response_members_->size_of_;
}

size_t get_response_enhanced_size(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return get_response_enhanced_size_tmpl<rosidl_typesupport_introspection_c__ServiceMembers>(
        untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return get_response_enhanced_size_tmpl<rosidl_typesupport_introspection_cpp::ServiceMembers>(
        untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return 0;
}

size_t get_service_request_size(void* context) {
  auto rosidl_typesupport = reinterpret_cast<rosidl_service_type_support_t *>(context);
  return get_request_enhanced_size(rosidl_typesupport->data,
                                   rosidl_typesupport->typesupport_identifier);
}

size_t get_service_request_serialized_size(void* context, void* data) {
  auto rosidl_typesupport =
      reinterpret_cast<rosidl_service_type_support_t *>(context);
  if (rosidl_typesupport->typesupport_identifier ==
  rosidl_typesupport_introspection_c__identifier) {
    auto untyped_member =
        static_cast<const rosidl_typesupport_introspection_c__ServiceMembers*>(
            rosidl_typesupport->data)->request_members_;
    return get_serialized_size(untyped_member,
                               rosidl_typesupport->typesupport_identifier, data);
  } else if (rosidl_typesupport->typesupport_identifier ==
  rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    auto untyped_member =
        static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers*>(
            rosidl_typesupport->data)->request_members_;
    return get_serialized_size(untyped_member, rosidl_typesupport->typesupport_identifier, data);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return 0;
}

size_t service_request_serialize_direct(void* context, void* data, void* buffer,
                                        size_t buffer_size) {
  bool ret = false;
  auto rosidl_typesupport =
      reinterpret_cast<rosidl_service_type_support_t *>(context);
  ret = serialize_request_enhanced(rosidl_typesupport->data,
                                   rosidl_typesupport->typesupport_identifier,
                                   static_cast<uint8_t*>(data),
                                   static_cast<uint8_t*>(buffer), buffer_size);
  return ret ? buffer_size : 0;
}

bool service_request_deserialize_direct(void* context, void* buffer, size_t buffer_size,
                                        void* data) {
  bool ret = false;
  auto rosidl_typesupport =
      reinterpret_cast<rosidl_service_type_support_t *>(context);
  ret = deserialize_request_enhanced(rosidl_typesupport->data,
                                     rosidl_typesupport->typesupport_identifier,
                                     static_cast<uint8_t*>(data),
                                     static_cast<uint8_t*>(buffer), buffer_size);
  return ret;
}

size_t get_service_response_size(void* context) {
  auto rosidl_typesupport = reinterpret_cast<rosidl_service_type_support_t *>(context);
  return get_response_enhanced_size(rosidl_typesupport->data,
                                    rosidl_typesupport->typesupport_identifier);
}

size_t get_service_response_serialized_size(void* context, void* data) {
  auto rosidl_typesupport =
      reinterpret_cast<rosidl_service_type_support_t *>(context);
  if (rosidl_typesupport->typesupport_identifier ==
  rosidl_typesupport_introspection_c__identifier) {
    auto untyped_member =
        static_cast<const rosidl_typesupport_introspection_c__ServiceMembers*>(
            rosidl_typesupport->data)->response_members_;
    return get_serialized_size(untyped_member, rosidl_typesupport->typesupport_identifier, data);
  } else if (rosidl_typesupport->typesupport_identifier ==
  rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    auto untyped_member =
        static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers*>(
            rosidl_typesupport->data)->response_members_;
    return get_serialized_size(untyped_member, rosidl_typesupport->typesupport_identifier,
                               data);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return 0;
}

size_t service_response_serialize_direct(void* context, void* data, void* buffer,
                                         size_t buffer_size) {
  bool ret = false;
  auto rosidl_typesupport =
      reinterpret_cast<rosidl_service_type_support_t *>(context);
  ret = serialize_response_enhanced(rosidl_typesupport->data,
                                    rosidl_typesupport->typesupport_identifier,
                                    static_cast<uint8_t*>(data),
                                    static_cast<uint8_t*>(buffer), buffer_size);
  return ret ? buffer_size : 0;
}

bool service_response_deserialize_direct(void* context, void* buffer, size_t buffer_size,
                                         void* data) {
  bool ret = false;
  auto rosidl_typesupport =
      reinterpret_cast<rosidl_service_type_support_t *>(context);
  ret = deserialize_response_enhanced(rosidl_typesupport->data,
                                      rosidl_typesupport->typesupport_identifier,
                                      static_cast<uint8_t*>(data),
                                      static_cast<uint8_t*>(buffer), buffer_size);
  return ret;
}

void init_service_request_typesupport_ops(dds_TypeSupport_ops& ops) {
  ops.get_size = get_service_request_size;
  ops.get_serialized_size = get_service_request_serialized_size;
  ops.serialize_direct = service_request_serialize_direct;
  ops.deserialize_direct = service_request_deserialize_direct;
}

void init_service_reply_typesupport_ops(dds_TypeSupport_ops& ops) {
  ops.get_size = get_service_response_size;
  ops.get_serialized_size = get_service_response_serialized_size;
  ops.serialize_direct = service_response_serialize_direct;
  ops.deserialize_direct = service_response_deserialize_direct;
}

void set_service_typesupport(dds_DataWriter* writer, dds_DataReader* reader,
                             const rosidl_service_type_support_t* rosidl_typesupport) {
  dds_TypeSupport_ops dds_request_ops{};
  dds_TypeSupport_ops dds_response_ops{};
  dds_request_ops.context = const_cast<rosidl_service_type_support_t *>(rosidl_typesupport);
  dds_response_ops.context = const_cast<rosidl_service_type_support_t *>(rosidl_typesupport);
  init_service_request_typesupport_ops(dds_request_ops);
  init_service_reply_typesupport_ops(dds_response_ops);
  auto request_typesupport = dds_DataReader_get_typesupport(reader);
  auto response_typesupport = dds_DataWriter_get_typesupport(writer);

  dds_TypeSupport_set_operations(request_typesupport, &dds_request_ops);
  dds_TypeSupport_set_operations(response_typesupport, &dds_response_ops);
}

void set_client_typesupport(dds_DataWriter* writer, dds_DataReader* reader,
                            const rosidl_service_type_support_t* rosidl_typesupport) {
  dds_TypeSupport_ops dds_request_ops{};
  dds_TypeSupport_ops dds_response_ops{};
  dds_request_ops.context = const_cast<rosidl_service_type_support_t *>(rosidl_typesupport);
  dds_response_ops.context = const_cast<rosidl_service_type_support_t *>(rosidl_typesupport);
  init_service_request_typesupport_ops(dds_request_ops);
  init_service_reply_typesupport_ops(dds_response_ops);
  auto request_typesupport = dds_DataWriter_get_typesupport(writer);
  auto response_typesupport = dds_DataReader_get_typesupport(reader);

  dds_TypeSupport_set_operations(request_typesupport, &dds_request_ops);
  dds_TypeSupport_set_operations(response_typesupport, &dds_response_ops);
}
}  // namespace rmw_gurumdds_cpp

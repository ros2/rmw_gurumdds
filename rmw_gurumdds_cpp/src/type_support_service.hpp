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

#ifndef TYPE_SUPPORT_SERVICE_HPP_
#define TYPE_SUPPORT_SERVICE_HPP_

#include <string>
#include <utility>

#include "./type_support_common.hpp"

#define GET_TYPENAME(T) \
  typename std::remove_pointer<typename std::remove_const<decltype(T)>::type>::type

template<typename ServiceMembersT>
std::pair<std::string, std::string>
_create_service_type_name(const void * untyped_members)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return {"", ""};
  }

  return {
    _create_type_name<GET_TYPENAME(members->request_members_)>(
      static_cast<const void *>(members->request_members_)
    ),
    _create_type_name<GET_TYPENAME(members->response_members_)>(
      static_cast<const void *>(members->response_members_)
    )
  };
}

inline std::pair<std::string, std::string>
create_service_type_name(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _create_service_type_name<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _create_service_type_name<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return {"", ""};
}

template<typename ServiceMembersT>
std::pair<std::string, std::string>
_create_service_metastring(const void * untyped_members)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return {"", ""};
  }

  return {
    _create_metastring<GET_TYPENAME(members->request_members_)>(
      static_cast<const void *>(members->request_members_),
      true
    ),
    _create_metastring<GET_TYPENAME(members->response_members_)>(
      static_cast<const void *>(members->response_members_),
      true
    )
  };
}

inline std::pair<std::string, std::string>
create_service_metastring(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _create_service_metastring<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _create_service_metastring<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return {"", ""};
}

template<typename ServiceMembersT>
void *
_allocate_request(
  const void * untyped_members,
  const void * ros_request,
  size_t * size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  return _allocate_message<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    static_cast<const uint8_t *>(ros_request),
    size,
    true
  );
}

inline void *
allocate_request(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_request<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_request<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

template<typename ServiceMembersT>
void *
_allocate_response(
  const void * untyped_members,
  const void * ros_response,
  size_t * size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  return _allocate_message<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    static_cast<const uint8_t *>(ros_response),
    size,
    true
  );
}

inline void *
allocate_response(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_response<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_response<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

template<typename MessageMembersT>
bool
_serialize_service(
  const void * untyped_members,
  const uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int64_t sequence_number,
  const int8_t * client_guid)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  try {
    auto buffer = CDRSerializationBuffer(dds_service, size);
    auto serializer = MessageSerializer(buffer);
    serializer.serialize(members, ros_service, true);
    buffer << *(reinterpret_cast<uint64_t *>(&sequence_number));
    buffer << *(reinterpret_cast<const uint64_t *>(client_guid));
    buffer << *(reinterpret_cast<const uint64_t *>(client_guid + 8));
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to serialize ros message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
serialize_service(
  const void * untyped_members,
  const char * identifier,
  const void * ros_service,
  void * dds_service,
  size_t size,
  int64_t sequence_number,
  const int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_service<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_service<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sequence_number,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_serialize_request(
  const void * untyped_members,
  const uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int64_t sequence_number,
  const int8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _serialize_service<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size,
    sequence_number,
    client_guid
  );
}

inline bool
serialize_request(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  void * dds_request,
  size_t size,
  int64_t sequence_number,
  const int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_request<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_request<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sequence_number,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_serialize_response(
  const void * untyped_members,
  const uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int64_t sequence_number,
  const int8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _serialize_service<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size,
    sequence_number,
    client_guid
  );
}

inline bool
serialize_response(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  void * dds_response,
  size_t size,
  int64_t sequence_number,
  const int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_response<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_response<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size,
      sequence_number,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename MessageMembersT>
bool
_deserialize_service(
  const void * untyped_members,
  uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int64_t * sequence_number,
  int8_t * client_guid)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  try {
    auto buffer = CDRDeserializationBuffer(dds_service, size);
    auto deserializer = MessageDeserializer(buffer);
    deserializer.deserialize(members, ros_service, true);
    buffer >> *(reinterpret_cast<uint64_t *>(sequence_number));
    buffer >> *(reinterpret_cast<uint64_t *>(client_guid));
    buffer >> *(reinterpret_cast<uint64_t *>(client_guid + 8));
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
deserialize_service(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size,
  int64_t * sequence_number,
  int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_service<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_service<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sequence_number,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_deserialize_request(
  const void * untyped_members,
  uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int64_t * sequence_number,
  int8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _deserialize_service<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size,
    sequence_number,
    client_guid
  );
}

inline bool
deserialize_request(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size,
  int64_t * sequence_number,
  int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_request<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_request<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sequence_number,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_deserialize_response(
  const void * untyped_members,
  uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int64_t * sequence_number,
  int8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _deserialize_service<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size,
    sequence_number,
    client_guid
  );
}

inline bool
deserialize_response(
  const void * untyped_members,
  const char * identifier,
  void * ros_reponse,
  void * dds_reponse,
  size_t size,
  int64_t * sequence_number,
  int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_response<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_reponse),
      reinterpret_cast<uint8_t *>(dds_reponse),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_response<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_reponse),
      reinterpret_cast<uint8_t *>(dds_reponse),
      size,
      sequence_number,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

#endif  // TYPE_SUPPORT_SERVICE_HPP_

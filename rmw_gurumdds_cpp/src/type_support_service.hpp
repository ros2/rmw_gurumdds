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

#include "type_support_common.hpp"

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
_allocate_request_basic(
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
allocate_request_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_request_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_request_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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
_allocate_response_basic(
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
allocate_response_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_response_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_response_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

template<typename ServiceMembersT>
void *
_allocate_request_enhanced(
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
    false
  );
}

inline void *
allocate_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_request_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_request_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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
_allocate_response_enhanced(
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
    false
  );
}

inline void *
allocate_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _allocate_response_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _allocate_response_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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
_serialize_service_basic(
  const void * untyped_members,
  const uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid,
  bool is_request)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  int32_t sn_high = static_cast<int32_t>((sequence_number & 0xFFFFFFFF00000000LL) >> 8);
  uint32_t sn_low = static_cast<uint32_t>(sequence_number & 0x00000000FFFFFFFFLL);

  try {
    auto buffer = CDRSerializationBuffer(dds_service, size);
    auto serializer = MessageSerializer(buffer);
    buffer << *(reinterpret_cast<const uint64_t *>(client_guid));
    buffer << *(reinterpret_cast<const uint64_t *>(client_guid + 8));
    buffer << *(reinterpret_cast<uint32_t *>(&sn_high));
    buffer << *(reinterpret_cast<uint32_t *>(&sn_low));
    if (is_request) {
      std::string instance_name = "";
      buffer << instance_name;
    } else {
      int32_t remoteEx = 0;
      buffer << *(reinterpret_cast<uint32_t *>(&remoteEx));
    }
    serializer.serialize(members, ros_service, true);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to serialize ros message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
serialize_service_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_service,
  void * dds_service,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid,
  bool is_request)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_service_basic<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sequence_number,
      client_guid,
      is_request
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_service_basic<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sequence_number,
      client_guid,
      is_request
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_serialize_request_basic(
  const void * untyped_members,
  const uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _serialize_service_basic<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size,
    sequence_number,
    client_guid,
    true
  );
}

inline bool
serialize_request_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  void * dds_request,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_request_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_request_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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
_serialize_response_basic(
  const void * untyped_members,
  const uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _serialize_service_basic<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size,
    sequence_number,
    client_guid,
    false
  );
}

inline bool
serialize_response_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  void * dds_response,
  size_t size,
  int64_t sequence_number,
  const uint8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_response_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_response_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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
_serialize_service_enhanced(
  const void * untyped_members,
  const uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size)
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
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to serialize ros message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
serialize_service_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_service,
  void * dds_service,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_service_enhanced<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_service_enhanced<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_serialize_request_enhanced(
  const void * untyped_members,
  const uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _serialize_service_enhanced<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size
  );
}

inline bool
serialize_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  void * dds_request,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_request_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_request_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_serialize_response_enhanced(
  const void * untyped_members,
  const uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _serialize_service_enhanced<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size
  );
}

inline bool
serialize_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  void * dds_response,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _serialize_response_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _serialize_response_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename MessageMembersT>
bool
_deserialize_service_basic(
  const void * untyped_members,
  uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  uint8_t * client_guid,
  bool is_request)
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
    buffer >> *(reinterpret_cast<uint64_t *>(client_guid));
    buffer >> *(reinterpret_cast<uint64_t *>(client_guid + 8));
    buffer >> *(reinterpret_cast<uint32_t *>(sn_high));
    buffer >> *(reinterpret_cast<uint32_t *>(sn_low));
    if (is_request) {
      std::string instance_name;
      buffer >> instance_name;
    } else {
      int32_t remoteEx;
      buffer >> *(reinterpret_cast<uint32_t *>(&remoteEx));
    }
    deserializer.deserialize(members, ros_service, true);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
deserialize_service_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  uint8_t * client_guid,
  bool is_request)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_service_basic<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sn_high,
      sn_low,
      client_guid,
      is_request
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_service_basic<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size,
      sn_high,
      sn_low,
      client_guid,
      is_request
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_deserialize_request_basic(
  const void * untyped_members,
  uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  uint8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _deserialize_service_basic<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size,
    sn_high,
    sn_low,
    client_guid,
    true
  );
}

inline bool
deserialize_request_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  uint8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_request_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_request_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_deserialize_response_basic(
  const void * untyped_members,
  uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  uint8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _deserialize_service_basic<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size,
    sn_high,
    sn_low,
    client_guid,
    false
  );
}

inline bool
deserialize_response_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_reponse,
  void * dds_reponse,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  uint8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_response_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_reponse),
      reinterpret_cast<uint8_t *>(dds_reponse),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_response_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_reponse),
      reinterpret_cast<uint8_t *>(dds_reponse),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename MessageMembersT>
bool
_deserialize_service_enhanced(
  const void * untyped_members,
  uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size)
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
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

inline bool
deserialize_service_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_service_enhanced<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_service_enhanced<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_deserialize_request_enhanced(
  const void * untyped_members,
  uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _deserialize_service_enhanced<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size
  );
}

inline bool
deserialize_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_request_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_request_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename ServiceMembersT>
bool
_deserialize_response_enhanced(
  const void * untyped_members,
  uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return _deserialize_service_enhanced<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size
  );
}

inline bool
deserialize_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_reponse,
  void * dds_reponse,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return _deserialize_response_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_reponse),
      reinterpret_cast<uint8_t *>(dds_reponse),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return _deserialize_response_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_reponse),
      reinterpret_cast<uint8_t *>(dds_reponse),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

inline void
ros_guid_to_dds_guid(uint8_t * guid_ros, uint8_t * guid_dds)
{
  memcpy(guid_dds, guid_ros, 12);
  memcpy(&guid_dds[12], &guid_ros[12], 4);
}

inline void
dds_guid_to_ros_guid(uint8_t * guid_dds, uint8_t * guid_ros)
{
  memcpy(guid_ros, guid_dds, 12);
  memcpy(&guid_ros[12], &guid_dds[12], 4);
}

inline void
ros_sn_to_dds_sn(int64_t sn_ros, uint64_t * sn_dds)
{
  *sn_dds = ((sn_ros) & 0xFFFFFFFF00000000LL) >> 32;
  *sn_dds = *sn_dds | ((sn_ros & 0x00000000FFFFFFFFLL) << 32);
}

inline void
dds_sn_to_ros_sn(uint64_t sn_dds, int64_t * sn_ros)
{
  *sn_ros = ((sn_dds & 0x00000000FFFFFFFF) << 32) | ((sn_dds & 0xFFFFFFFF00000000) >> 32);
}
#endif  // TYPE_SUPPORT_SERVICE_HPP_

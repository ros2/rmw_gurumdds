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

#include "rmw_gurumdds_cpp/type_support_service.hpp"

namespace rmw_gurumdds_cpp
{
std::pair<std::string, std::string>
create_service_type_name(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return create_service_type_name<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return create_service_type_name<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return {"", ""};
}

std::pair<std::string, std::string>
create_service_metastring(const void * untyped_members, const char * identifier)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return create_service_metastring<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return create_service_metastring<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return {"", ""};
}

void *
allocate_request_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return allocate_request_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return allocate_request_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
allocate_response_basic(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return allocate_response_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return allocate_response_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
allocate_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return allocate_request_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return allocate_request_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
allocate_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  size_t * size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return allocate_response_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return allocate_response_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

bool
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
    return serialize_service_basic<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members, reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service), size,
      sequence_number, client_guid, is_request);
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_service_basic<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members, reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service), size,
      sequence_number, client_guid, is_request);
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
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
    return serialize_request_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_request_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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

bool
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
    return serialize_response_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size,
      sequence_number,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_response_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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

bool
serialize_service_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_service,
  void * dds_service,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return serialize_service_enhanced<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_service_enhanced<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
serialize_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_request,
  void * dds_request,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return serialize_request_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_request_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
serialize_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  const void * ros_response,
  void * dds_response,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return serialize_response_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return serialize_response_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<const uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
deserialize_service_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid,
  bool is_request)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_service_basic<rosidl_typesupport_introspection_c__MessageMembers>(
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
    return deserialize_service_basic<rosidl_typesupport_introspection_cpp::MessageMembers>(
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

bool
deserialize_request_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_request_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return deserialize_request_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
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

bool
deserialize_response_basic(
  const void * untyped_members,
  const char * identifier,
  void * ros_response,
  void * dds_response,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_response_basic<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return deserialize_response_basic<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size,
      sn_high,
      sn_low,
      client_guid
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
deserialize_service_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_service,
  void * dds_service,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_service_enhanced<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return deserialize_service_enhanced<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_service),
      reinterpret_cast<uint8_t *>(dds_service),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
deserialize_request_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_request,
  void * dds_request,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_request_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return deserialize_request_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_request),
      reinterpret_cast<uint8_t *>(dds_request),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
deserialize_response_enhanced(
  const void * untyped_members,
  const char * identifier,
  void * ros_response,
  void * dds_response,
  size_t size)
{
  if (identifier == rosidl_typesupport_introspection_c__identifier) {
    return deserialize_response_enhanced<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size
    );
  } else if (identifier == rosidl_typesupport_introspection_cpp::typesupport_identifier) {
    return deserialize_response_enhanced<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_members,
      reinterpret_cast<uint8_t *>(ros_response),
      reinterpret_cast<uint8_t *>(dds_response),
      size
    );
  }

  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}
}  // namespace rmw_gurumdds_cpp

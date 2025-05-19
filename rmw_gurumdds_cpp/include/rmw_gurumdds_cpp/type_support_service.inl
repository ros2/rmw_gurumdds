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

#ifndef RMW_GURUMDDS_CPP__TYPE_SUPPORT_SERVICE_INL
#define RMW_GURUMDDS_CPP__TYPE_SUPPORT_SERVICE_INL

namespace rmw_gurumdds_cpp
{
template<typename ServiceMembersT>
std::pair<std::string, std::string>
create_service_type_name(const void * untyped_members)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return {"", ""};
  }

  return {
    create_type_name<GET_TYPENAME(members->request_members_)>(
      static_cast<const void *>(members->request_members_)
        ),
    create_type_name<GET_TYPENAME(members->response_members_)>(
      static_cast<const void *>(members->response_members_)
        )
  };
}

template<typename ServiceMembersT>
std::pair<std::string, std::string>
create_service_metastring(const void * untyped_members)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return {"", ""};
  }

  return {
    create_metastring<GET_TYPENAME(members->request_members_)>(static_cast<const void *>(members->request_members_)),
    create_metastring<GET_TYPENAME(members->response_members_)>(static_cast<const void *>(members->response_members_))
  };
}

template<typename ServiceMembersT>
void *
allocate_request_basic(
  const void * untyped_members,
  const void * ros_request,
  size_t * size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  return allocate_message<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    static_cast<const uint8_t *>(ros_request),
    size
  );
}

template<typename ServiceMembersT>
void *
allocate_response_basic(
  const void * untyped_members,
  const void * ros_response,
  size_t * size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  return allocate_message<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    static_cast<const uint8_t *>(ros_response),
    size
  );
}

template<typename ServiceMembersT>
void *
allocate_request_enhanced(
  const void * untyped_members,
  const void * ros_request,
  size_t * size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  return allocate_message<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    static_cast<const uint8_t *>(ros_request),
    size
  );
}

template<typename ServiceMembersT>
void *
allocate_response_enhanced(
  const void * untyped_members,
  const void * ros_response,
  size_t * size)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  return allocate_message<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    static_cast<const uint8_t *>(ros_response),
    size
  );
}

template<typename MessageMembersT>
bool
serialize_service_basic(
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
    rmw_gurumdds_cpp::CdrSerializationBuffer<true> buffer{dds_service, size};
    rmw_gurumdds_cpp::MessageSerializer<true, MessageMembersT> serializer{buffer};
    buffer << *(reinterpret_cast<const uint64_t *>(client_guid));
    buffer << *(reinterpret_cast<const uint64_t *>(client_guid + 8));
    buffer << *(reinterpret_cast<uint32_t *>(&sn_high));
    buffer << *(reinterpret_cast<uint32_t *>(&sn_low));
    if (is_request) {
      std::string instance_name;
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

template<typename ServiceMembersT>
bool
serialize_request_basic(
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

  return serialize_service_basic<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size,
    sequence_number,
    client_guid,
    true
  );
}

template<typename ServiceMembersT>
bool
serialize_response_basic(
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

  return serialize_service_basic<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size,
    sequence_number,
    client_guid,
    false
  );
}

template<typename MessageMembersT>
bool
serialize_service_enhanced(
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
    rmw_gurumdds_cpp::CdrSerializationBuffer<true> buffer{dds_service, size};
    rmw_gurumdds_cpp::MessageSerializer<true, MessageMembersT> serializer{buffer};
    serializer.serialize(members, ros_service, true);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to serialize ros message: %s", e.what());
    return false;
  }

  return true;
}

template<typename ServiceMembersT>
bool
serialize_request_enhanced(
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

  return serialize_service_enhanced<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size
  );
}

template<typename ServiceMembersT>
bool
serialize_response_enhanced(
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

  return serialize_service_enhanced<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size
  );
}

template<typename MessageMembersT>
bool
deserialize_service_basic(
  const void * untyped_members,
  uint8_t * ros_service,
  uint8_t * dds_service,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid,
  bool is_request)
{
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  try {
    auto buffer = rmw_gurumdds_cpp::CdrDeserializationBuffer(dds_service, size);
    auto deserializer = rmw_gurumdds_cpp::MessageDeserializer<MessageMembersT>(buffer);
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
    deserializer.deserialize(members, ros_service);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

template<typename ServiceMembersT>
bool
deserialize_request_basic(
  const void * untyped_members,
  uint8_t * ros_request,
  uint8_t * dds_request,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return deserialize_service_basic<GET_TYPENAME(members->request_members_)>(
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

template<typename ServiceMembersT>
bool
deserialize_response_basic(
  const void * untyped_members,
  uint8_t * ros_response,
  uint8_t * dds_response,
  size_t size,
  int32_t * sn_high,
  uint32_t * sn_low,
  int8_t * client_guid)
{
  auto members = static_cast<const ServiceMembersT *>(untyped_members);
  if (members == nullptr) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return false;
  }

  return deserialize_service_basic<GET_TYPENAME(members->response_members_)>(
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

template<typename MessageMembersT>
bool
deserialize_service_enhanced(
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
    auto buffer = rmw_gurumdds_cpp::CdrDeserializationBuffer(dds_service, size);
    auto deserializer = rmw_gurumdds_cpp::MessageDeserializer<MessageMembersT>(buffer);
    deserializer.deserialize(members, ros_service);
  } catch (std::runtime_error & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to deserialize dds message: %s", e.what());
    return false;
  }

  return true;
}

template<typename ServiceMembersT>
bool
deserialize_request_enhanced(
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

  return deserialize_service_enhanced<GET_TYPENAME(members->request_members_)>(
    static_cast<const void *>(members->request_members_),
    ros_request,
    dds_request,
    size
  );
}

template<typename ServiceMembersT>
bool
deserialize_response_enhanced(
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

  return deserialize_service_enhanced<GET_TYPENAME(members->response_members_)>(
    static_cast<const void *>(members->response_members_),
    ros_response,
    dds_response,
    size
  );
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
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__TYPE_SUPPORT_SERVICE_INL

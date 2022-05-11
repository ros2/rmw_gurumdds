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

#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "type_support_common.hpp"

extern "C"
{
rmw_ret_t
rmw_serialize(
  const void * ros_message,
  const rosidl_message_type_support_t * type_support,
  rmw_serialized_message_t * serialized_message)
{
  const rosidl_message_type_support_t * ts =
    get_message_typesupport_handle(type_support, rosidl_typesupport_introspection_c__identifier);
  if (ts == nullptr) {
    ts = get_message_typesupport_handle(
      type_support, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (ts == nullptr) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return RMW_RET_ERROR;
    }
  }

  ssize_t ssize = get_serialized_size(
    ts->data,
    ts->typesupport_identifier,
    ros_message
  );
  if (ssize < 0) {
    RMW_SET_ERROR_MSG("failed to get size of serialized message");
    return RMW_RET_ERROR;
  }

  size_t size = static_cast<size_t>(ssize);

  serialized_message->buffer_length = size;
  if (serialized_message->buffer_capacity < size) {
    serialized_message->allocator.deallocate(
      serialized_message->buffer, serialized_message->allocator.state);
    serialized_message->buffer = static_cast<uint8_t *>(
      serialized_message->allocator.allocate(
        serialized_message->buffer_length,
        serialized_message->allocator.state));
    serialized_message->buffer_capacity = size;
  }

  bool res = serialize_ros_to_cdr(
    ts->data,
    ts->typesupport_identifier,
    ros_message,
    serialized_message->buffer,
    size
  );
  if (!res) {
    // Error message already set
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_deserialize(
  const rmw_serialized_message_t * serialized_message,
  const rosidl_message_type_support_t * type_support,
  void * ros_message)
{
  const rosidl_message_type_support_t * ts =
    get_message_typesupport_handle(type_support, rosidl_typesupport_introspection_c__identifier);
  if (ts == nullptr) {
    ts = get_message_typesupport_handle(
      type_support, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (ts == nullptr) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return RMW_RET_ERROR;
    }
  }

  bool res = deserialize_cdr_to_ros(
    ts->data,
    ts->typesupport_identifier,
    ros_message,
    serialized_message->buffer,
    serialized_message->buffer_length
  );
  if (!res) {
    // Error message already set
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_get_serialized_message_size(
  const rosidl_message_type_support_t * /*type_support*/,
  const rosidl_runtime_c__Sequence__bound * /*message_bounds*/,
  size_t * /*size*/)
{
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"

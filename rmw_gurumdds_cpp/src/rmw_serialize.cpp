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
#include "rmw/rmw.h"
#include "rmw/serialized_message.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rmw_gurumdds_cpp/fastrtps.hpp"
#include "rmw_gurumdds_cpp/type_support_common.hpp"
#include "rmw_gurumdds_cpp/utils.hpp"

extern "C" {
rmw_ret_t rmw_serialize(
  const void *ros_message,
  const rosidl_message_type_support_t *type_support,
  rmw_serialized_message_t *serialized_message)
{
  CHECK_ALL_PTRS_CODE(ros_message, type_support, serialized_message);

  const rosidl_message_type_support_t *ts = get_message_typesupport_handle(
    type_support, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!ts) {
    ts = get_message_typesupport_handle(
      type_support, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!ts) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return RMW_RET_ERROR;
    }
  }

  auto *callbacks =
    static_cast<const message_type_support_callbacks_t *>(ts->data);

  const size_t ser_size = callbacks->get_serialized_size(ros_message);

  size_t buffer_size = ser_size + 4;

  std::vector<char> buffer(buffer_size);

  eprosima::fastcdr::FastBuffer fastbuffer(buffer.data(), buffer_size);

  eprosima::fastcdr::Cdr ser(
    fastbuffer,
    eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::CdrVersion::XCDRv1);

  ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

  try {
    ser.serialize_encapsulation();

    if (!callbacks->cdr_serialize(ros_message, ser)) {
      RMW_SET_ERROR_MSG(
        "failed to serialize ROS message with FastRTPS typesupport");
      return RMW_RET_ERROR;
    }
  } catch (const std::exception & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "failed to serialize ROS message: %s", e.what());
    return RMW_RET_ERROR;
  } catch (...) {
    RMW_SET_ERROR_MSG("failed to serialize ROS message: unknown exception");
    return RMW_RET_ERROR;
  }

  const size_t serialized_size = ser.get_serialized_data_length();

  if (serialized_message->buffer_capacity < serialized_size) {
    rcutils_ret_t resize_ret =
      rcutils_uint8_array_resize(serialized_message, serialized_size);

    if (resize_ret != RCUTILS_RET_OK) {
      RMW_SET_ERROR_MSG("failed to resize serialized message");
      return RMW_RET_ERROR;
    }
  }

  std::memcpy(serialized_message->buffer, buffer.data(), serialized_size);

  serialized_message->buffer_length = serialized_size;

  return RMW_RET_OK;
}

rmw_ret_t rmw_deserialize(
  const rmw_serialized_message_t *serialized_message,
  const rosidl_message_type_support_t *type_support,
  void *ros_message)
{
  CHECK_ALL_PTRS_CODE(ros_message, type_support, serialized_message);
  CHECK_ALL_PTRS_CODE(serialized_message->buffer);

  if (serialized_message->buffer_length == 0) {
    RMW_SET_ERROR_MSG("serialized message buffer is empty");
    return RMW_RET_INVALID_ARGUMENT;
  }

  const rosidl_message_type_support_t *ts = get_message_typesupport_handle(
    type_support, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!ts) {
    ts = get_message_typesupport_handle(
      type_support, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!ts) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return RMW_RET_ERROR;
    }
  }

  auto *callbacks =
    static_cast<const message_type_support_callbacks_t *>(ts->data);

  eprosima::fastcdr::FastBuffer fastbuffer(
    reinterpret_cast<char *>(serialized_message->buffer),
    serialized_message->buffer_length);

  eprosima::fastcdr::Cdr deser(
    fastbuffer,
    eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::CdrVersion::XCDRv1);

  deser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

  try {
    deser.read_encapsulation();

    if (!callbacks->cdr_deserialize(deser, ros_message)) {
      RMW_SET_ERROR_MSG(
        "failed to deserialize ROS message with FastRTPS typesupport");
      return RMW_RET_ERROR;
    }
  } catch (const std::exception & e) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "failed to deserialize ROS message: %s", e.what());
    return RMW_RET_ERROR;
  } catch (...) {
    RMW_SET_ERROR_MSG("failed to deserialize ROS message: unknown exception");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_get_serialized_message_size(
  const rosidl_message_type_support_t * /*type_support*/,
  const rosidl_runtime_c__Sequence__bound * /*message_bounds*/,
  size_t * /*size*/)
{
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}
} // extern "C"

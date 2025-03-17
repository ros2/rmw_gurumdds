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

#ifndef RMW_GURUMDDS__TYPE_SUPPORT_COMMON_INL
#define RMW_GURUMDDS__TYPE_SUPPORT_COMMON_INL

#include <sstream>

namespace rmw_gurumdds_cpp
{
template<typename MessageMembersT>
std::string
create_type_name(const void * untyped_members) {
  auto members = static_cast<const MessageMembersT *>(untyped_members);
  if (nullptr == members) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return "";
  }

  std::ostringstream type_name;
  std::string message_namespace = members->message_namespace_;
  if (!message_namespace.empty()) {
    size_t pos = 0;
    while ((pos = message_namespace.find("__", pos)) != std::string::npos) {
      message_namespace.replace(pos, 2, "::");
    }
    type_name << message_namespace << "::";
  }
  type_name << "dds_::" << members->message_name_ << "_";
  return type_name.str();
}

template<typename MessageMembersT>
void *
allocate_message(
  const void * untyped_members,
  const uint8_t * ros_message,
  size_t * size) {
  auto members =
    static_cast<const MessageMembersT *>(untyped_members);
  if (nullptr == members) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return nullptr;
  }

  if (nullptr == ros_message) {
    RMW_SET_ERROR_MSG("ros message is null");
    return nullptr;
  }

  if (nullptr == size) {
    RMW_SET_ERROR_MSG("size pointer is null");
    return nullptr;
  }

  rmw_gurumdds_cpp::CdrSerializationBuffer<false> buffer{nullptr, 0};
  rmw_gurumdds_cpp::MessageSerializer<false, MessageMembersT> serializer{buffer};
  serializer.serialize(members, ros_message, true);

  *size = buffer.get_offset() + 4;
  void * message = malloc(*size);
  if (nullptr == message) {
    RMW_SET_ERROR_MSG("Failed to allocate memory for dds message");
    return nullptr;
  }

  return message;
}

template<typename MessageMembersT>
std::string
parse_struct(const MessageMembersT * members, const char * field_name)
{
  if (nullptr == members) {
    RMW_SET_ERROR_MSG("Members handle is null");
    return "";
  }

  std::ostringstream metastring;
  metastring <<
    "{(" <<
    (field_name ? std::string("name=") + field_name + "_," : "") <<
    "type=" <<
    create_type_name<MessageMembersT>(members) <<
    ",member=" <<
    members->member_count_ <<
    ")";

  for (size_t i = 0; i < members->member_count_; i++) {
    auto member = members->members_ + i;
    if (member->is_array_) {
      if (member->array_size_ > 0) {
        if (!member->is_upper_bound_) {
          // Array
          metastring << "[(name=" << member->name_ << "_,dimension=" << member->array_size_ << ")";
        } else {
          // BoundedSequence
          metastring << "<(name=" << member->name_ << "_,maximum=" << member->array_size_ << ")";
        }
      } else {
        // UnboundedSequence
        metastring << "<(name=" << member->name_ << "_)";
      }
    }

    if (member->type_id_ == rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE) {
      if (member->members_ == nullptr || member->members_->data == nullptr) {
        RMW_SET_ERROR_MSG("Members handle is null");
        return "";
      }

      auto inner_struct = static_cast<const MessageMembersT *>(member->members_->data);
      std::string inner_metastring = parse_struct<MessageMembersT>(inner_struct, member->is_array_ ? nullptr : member->name_);
      if (inner_metastring.empty()) {
        return "";
      }
      metastring << inner_metastring;
    } else {
      switch (member->type_id_) {
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT:
          metastring << "f";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_DOUBLE:
          metastring << "d";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_OCTET:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          metastring << "B";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_WCHAR:
          metastring << "w";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOLEAN:
          metastring << "z";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          metastring << "S";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          metastring << "s";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          metastring << "I";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          metastring << "i";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          metastring << "L";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          metastring << "l";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          metastring << "'";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_WSTRING:
          metastring << "W";
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
          break;
        default:
          RMW_SET_ERROR_MSG("Unknown type");
          return "";
      }

      metastring <<
        "(" <<
        (member->is_array_ ? "" : std::string("name=") + member->name_ + "_") <<
        ")";
    }
  }

  return metastring.str();
}

template<typename MessageMembersT>
std::string
create_metastring(const void * untyped_members)
{
  auto members = static_cast<const MessageMembersT *>(untyped_members);
  if (nullptr == members) {
    RMW_SET_ERROR_MSG("Null members handle is given");
    return "";
  }

  std::ostringstream metastring;
  metastring <<
    "!1" <<
    parse_struct<MessageMembersT>(
      static_cast<const MessageMembersT *>(members), nullptr);

  return metastring.str();
}
} // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS__TYPE_SUPPORT_COMMON_INL

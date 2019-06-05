#include <map>

#include "rmw/error_handling.h"

#include "rmw_coredds_shared_cpp/rmw_common.hpp"
#include "rmw_coredds_shared_cpp/demangle.hpp"
#include "rmw_coredds_shared_cpp/types.hpp"

rmw_ret_t
shared__rmw_count_publishers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  if (node->implementation_identifier != implementation_identifier) {
    RMW_SET_ERROR_MSG("node handle is not from this rmw implementation");
    return RMW_RET_ERROR;
  }
  if (topic_name == nullptr) {
    RMW_SET_ERROR_MSG("topic name is null");
    return RMW_RET_ERROR;
  }
  if (count == nullptr) {
    RMW_SET_ERROR_MSG("count handle is null");
    return RMW_RET_ERROR;
  }

  CoreddsNodeInfo * node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  if (node_info->pub_listener == nullptr) {
    RMW_SET_ERROR_MSG("publisher listener handle is null");
    return RMW_RET_ERROR;
  }

  *count = node_info->pub_listener->count_topic(topic_name);

  return RMW_RET_OK;
}

rmw_ret_t
shared__rmw_count_subscribers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  if (node->implementation_identifier != implementation_identifier) {
    RMW_SET_ERROR_MSG("node handle is not from this rmw implementation");
    return RMW_RET_ERROR;
  }
  if (topic_name == nullptr) {
    RMW_SET_ERROR_MSG("topic name is null");
    return RMW_RET_ERROR;
  }
  if (count == nullptr) {
    RMW_SET_ERROR_MSG("count handle is null");
    return RMW_RET_ERROR;
  }

  CoreddsNodeInfo * node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  if (node_info->sub_listener == nullptr) {
    RMW_SET_ERROR_MSG("sublisher listener handle is null");
    return RMW_RET_ERROR;
  }

  *count = node_info->sub_listener->count_topic(topic_name);

  return RMW_RET_OK;
}

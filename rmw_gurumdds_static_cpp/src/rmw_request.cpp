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
#include "rmw/types.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_gurumdds_shared_cpp/rmw_common.hpp"
#include "rmw_gurumdds_shared_cpp/types.hpp"

#include "rmw_gurumdds_static_cpp/identifier.hpp"
#include "rmw_gurumdds_static_cpp/types.hpp"

extern "C"
{
rmw_ret_t
rmw_send_request(
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  if (client == nullptr) {
    RMW_SET_ERROR_MSG("client handle is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client handle,
    client->implementation_identifier, gurum_gurumdds_static_identifier,
    return RMW_RET_ERROR)

  if (ros_request == nullptr) {
    RMW_SET_ERROR_MSG("ros request handle is null");
    return RMW_RET_ERROR;
  }

  GurumddsClientInfo * client_info = static_cast<GurumddsClientInfo *>(client->data);
  if (client_info == nullptr) {
    RMW_SET_ERROR_MSG("client info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataWriter * request_writer = client_info->request_writer;
  if (request_writer == nullptr) {
    RMW_SET_ERROR_MSG("request writer is null");
    return RMW_RET_ERROR;
  }

  const service_type_support_callbacks_t * callbacks = client_info->callbacks;
  if (callbacks == nullptr) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return RMW_RET_ERROR;
  }

  const message_type_support_callbacks_t * request_callbacks =
    static_cast<const message_type_support_callbacks_t *>(
    callbacks->request_callbacks->data);
  if (request_callbacks == nullptr) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return RMW_RET_ERROR;
  }

  // Some types such as string need to be converted
  void * dds_request = request_callbacks->alloc();
  if (!request_callbacks->convert_ros_to_dds(ros_request, dds_request)) {
    RMW_SET_ERROR_MSG("failed to convert message");
    return RMW_RET_ERROR;
  }

  int8_t temp_guid[16];
  memcpy(temp_guid, &client_info->writer_guid_0, sizeof(client_info->writer_guid_0));
  memcpy(
    temp_guid + sizeof(client_info->writer_guid_0),
    &client_info->writer_guid_1, sizeof(client_info->writer_guid_1));

  // Sequence number and guid are needed to match responses and requests
  callbacks->request_set_sequence_number(dds_request, ++(client_info->sequence_number));
  callbacks->request_set_guid(dds_request, temp_guid);

  if (dds_DataWriter_write(request_writer, dds_request, dds_HANDLE_NIL) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to publish data");
    request_callbacks->free(dds_request);
    return RMW_RET_ERROR;
  }

  *sequence_id = client_info->sequence_number;
  request_callbacks->free(dds_request);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_request(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken)
{
  if (service == nullptr) {
    RMW_SET_ERROR_MSG("service handle is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service handle,
    service->implementation_identifier, gurum_gurumdds_static_identifier,
    return RMW_RET_ERROR)

  if (ros_request == nullptr) {
    RMW_SET_ERROR_MSG("ros request handle is null");
    return RMW_RET_ERROR;
  }

  if (taken == nullptr) {
    RMW_SET_ERROR_MSG("boolean flag for taken is null");
    return RMW_RET_ERROR;
  }

  *taken = false;

  GurumddsServiceInfo * service_info = static_cast<GurumddsServiceInfo *>(service->data);
  if (service_info == nullptr) {
    RMW_SET_ERROR_MSG("service info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataReader * request_reader = service_info->request_reader;
  if (request_reader == nullptr) {
    RMW_SET_ERROR_MSG("request reader is null");
    return RMW_RET_ERROR;
  }

  const service_type_support_callbacks_t * callbacks = service_info->callbacks;
  if (callbacks == nullptr) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return RMW_RET_ERROR;
  }

  const message_type_support_callbacks_t * request_callbacks =
    static_cast<const message_type_support_callbacks_t *>(
    callbacks->request_callbacks->data);
  if (request_callbacks == nullptr) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return RMW_RET_ERROR;
  }

  dds_DataSeq * data_values = dds_DataSeq_create(1);
  if (data_values == nullptr) {
    RMW_SET_ERROR_MSG("failed to create data sequence");
    return RMW_RET_ERROR;
  }

  dds_SampleInfoSeq * sample_infos = dds_SampleInfoSeq_create(1);
  if (sample_infos == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample info sequence");
    dds_DataSeq_delete(data_values);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DataReader_take(
    request_reader, data_values, sample_infos, 1,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    dds_DataReader_return_loan(request_reader, data_values, sample_infos);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    *taken = false;
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    dds_DataReader_return_loan(request_reader, data_values, sample_infos);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);
  if (sample_info->valid_data) {
    void * sample = dds_DataSeq_get(data_values, 0);
    int64_t sequence_number = callbacks->request_get_sequence_number(sample);
    if (!request_callbacks->convert_dds_to_ros(sample, ros_request)) {
      RMW_SET_ERROR_MSG("failed to convert message");
      dds_DataReader_return_loan(request_reader, data_values, sample_infos);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      return RMW_RET_ERROR;
    }

    // Sequence number and guid are needed to match responses and requests
    request_header->sequence_number = sequence_number;
    callbacks->request_get_guid(sample, request_header->writer_guid);

    *taken = true;
  }

  dds_DataReader_return_loan(request_reader, data_values, sample_infos);
  dds_DataSeq_delete(data_values);
  dds_SampleInfoSeq_delete(sample_infos);

  return RMW_RET_OK;
}
}  // extern "C"

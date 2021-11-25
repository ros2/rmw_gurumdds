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

#include "rmw_gurumdds_cpp/identifier.hpp"
#include "rmw_gurumdds_cpp/types.hpp"

#include "./type_support_service.hpp"

extern "C"
{
rmw_ret_t
rmw_send_request(
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client handle,
    client->implementation_identifier, gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(sequence_id, RMW_RET_INVALID_ARGUMENT);

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

  auto type_support = client_info->service_typesupport;
  if (type_support == nullptr) {
    RMW_SET_ERROR_MSG("typesupport handle is null");
    return RMW_RET_ERROR;
  }

  size_t size = 0;

  void * dds_request = allocate_request(
    type_support->data,
    type_support->typesupport_identifier,
    ros_request,
    &size
  );

  if (dds_request == nullptr) {
    // Error message already set
    return RMW_RET_ERROR;
  }

  bool res = serialize_request(
    type_support->data,
    type_support->typesupport_identifier,
    ros_request,
    dds_request,
    size,
    ++client_info->sequence_number,
    client_info->writer_guid
  );

  if (!res) {
    RMW_SET_ERROR_MSG("failed to serialize message");
    free(dds_request);
    return RMW_RET_ERROR;
  }

  if (dds_DataWriter_raw_write(request_writer, dds_request, size) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to publish data");
    free(dds_request);
    return RMW_RET_ERROR;
  }

  *sequence_id = client_info->sequence_number;
  free(dds_request);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_request(
  const rmw_service_t * service,
  rmw_service_info_t * request_header,
  void * ros_request,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service handle,
    service->implementation_identifier, gurum_gurumdds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

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

  auto type_support = service_info->service_typesupport;
  if (type_support == nullptr) {
    RMW_SET_ERROR_MSG("typesupport handle is null");
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

  dds_UnsignedLongSeq * sample_sizes = dds_UnsignedLongSeq_create(1);
  if (sample_sizes == nullptr) {
    RMW_SET_ERROR_MSG("failed to create sample size sequence");
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DataReader_raw_take(
    request_reader, dds_HANDLE_NIL, data_values, sample_infos, sample_sizes, 1,
    dds_ANY_SAMPLE_STATE, dds_ANY_VIEW_STATE, dds_ANY_INSTANCE_STATE);

  if (ret == dds_RETCODE_NO_DATA) {
    dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_OK;
  }

  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to take data");
    dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
    dds_DataSeq_delete(data_values);
    dds_SampleInfoSeq_delete(sample_infos);
    dds_UnsignedLongSeq_delete(sample_sizes);
    return RMW_RET_ERROR;
  }

  dds_SampleInfo * sample_info = dds_SampleInfoSeq_get(sample_infos, 0);
  if (sample_info->valid_data) {
    void * sample = dds_DataSeq_get(data_values, 0);
    if (sample == nullptr) {
      dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }
    uint32_t size = dds_UnsignedLongSeq_get(sample_sizes, 0);
    int64_t sequence_number = 0;
    int8_t client_guid[16] = {0};
    bool res = deserialize_request(
      type_support->data,
      type_support->typesupport_identifier,
      ros_request,
      sample,
      static_cast<size_t>(size),
      &sequence_number,
      client_guid
    );

    if (!res) {
      // Error message already set
      dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
      dds_DataSeq_delete(data_values);
      dds_SampleInfoSeq_delete(sample_infos);
      dds_UnsignedLongSeq_delete(sample_sizes);
      return RMW_RET_ERROR;
    }

    request_header->source_timestamp =
      sample_info->source_timestamp.sec * static_cast<int64_t>(1000000000) +
      sample_info->source_timestamp.nanosec;
    // TODO(clemjh): SampleInfo doesn't contain received_timestamp
    request_header->received_timestamp = 0;
    request_header->request_id.sequence_number = sequence_number;
    memcpy(request_header->request_id.writer_guid, client_guid, 16);

    *taken = true;
  }

  dds_DataReader_raw_return_loan(request_reader, data_values, sample_infos, sample_sizes);
  dds_DataSeq_delete(data_values);
  dds_SampleInfoSeq_delete(sample_infos);
  dds_UnsignedLongSeq_delete(sample_sizes);

  return RMW_RET_OK;
}
}  // extern "C"

// Copyright 2026 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__RAII_HPP_
#define RMW_GURUMDDS_CPP__RAII_HPP_

#include <gurumdds/dcps.h>

#include <memory>
#include <utility>
#include <string>

//See the raii_hpp.txt document in the docs folder.

namespace raii
{

#define RAII_SEQUENCE_DEFINE(seq_type) \
  class seq_type { \
public: \
    using native_type = ::seq_type; \
 \
    seq_type() noexcept = default; \
    seq_type(const seq_type &) = delete; \
    seq_type & operator=(const seq_type &) = delete; \
 \
    seq_type(seq_type && other) noexcept \
    : seq(other.seq) {other.seq = nullptr;} \
 \
    seq_type & operator=(seq_type && other) noexcept { \
      if (this == &other) { \
        return *this; \
      } \
 \
      if (seq != nullptr) { \
        ::seq_type ## _delete(seq); \
      } \
 \
      seq = other.seq; \
      other.seq = nullptr; \
      return *this; \
    } \
 \
    operator native_type *() noexcept {return seq;} \
 \
    operator const native_type *() const noexcept {return seq;} \
 \
    explicit operator bool() const noexcept {return seq != nullptr;} \
 \
    bool operator==(std::nullptr_t) const noexcept {return seq == nullptr;} \
 \
    bool operator!=(std::nullptr_t) const noexcept {return seq != nullptr;} \
 \
    ~seq_type() noexcept { \
      if (seq != nullptr) { \
        ::seq_type ## _delete(seq); \
        seq = nullptr; \
      } \
    } \
 \
    friend raii::seq_type seq_type ## _create(uint32_t capacity); \
    friend void seq_type ## _delete(seq_type & seq); \
 \
private: \
    native_type * seq{nullptr}; \
  }; \
  inline raii::seq_type seq_type ## _create(uint32_t capacity) { \
    raii::seq_type obj; \
    obj.seq = ::seq_type ## _create(capacity); \
    return obj; \
  } \
  inline void seq_type ## _delete(raii::seq_type & seq) { \
    if (seq.seq == nullptr) return; \
    ::seq_type ## _delete(seq.seq); \
    seq.seq = nullptr; \
  }

RAII_SEQUENCE_DEFINE(dds_DataSeq);
RAII_SEQUENCE_DEFINE(dds_SampleInfoSeq);
RAII_SEQUENCE_DEFINE(dds_UnsignedLongSeq);
RAII_SEQUENCE_DEFINE(dds_InstanceHandleSeq);
RAII_SEQUENCE_DEFINE(dds_ConditionSeq);
RAII_SEQUENCE_DEFINE(dds_PropertySeq);

#undef RAII_SEQUENCE_DEFINE

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#define RAII_QOS_DEFINE(qos_type) \
  class qos_type { \
public: \
    using native_type = ::qos_type; \
 \
    qos_type() noexcept = default; \
    qos_type(const qos_type &) = delete; \
    qos_type & operator=(const qos_type &) = delete; \
    qos_type(qos_type &&) = delete; \
    qos_type & operator=(qos_type &&) = delete; \
 \
    operator native_type *() noexcept {return &qos_entity_;} \
 \
    operator const native_type *() const noexcept {return &qos_entity_;} \
 \
    native_type * operator->() noexcept {return &qos_entity_;} \
 \
    const native_type * operator->() const noexcept {return &qos_entity_;} \
 \
    ~qos_type() noexcept {(void)::qos_type ## _finalize(&qos_entity_);} \
 \
private: \
    native_type qos_entity_{}; \
  }; \
 \
  inline ::dds_ReturnCode_t qos_type ## _finalize(qos_type & qos) noexcept { \
    return ::qos_type ## _finalize(qos); \
  } \
 \
  inline ::dds_ReturnCode_t qos_type ## _copy(qos_type & dst, const qos_type & src) noexcept { \
    if (&dst == &src) { \
      return dds_RETCODE_OK; \
    } \
    (void)::qos_type ## _finalize(dst); \
    return ::qos_type ## _copy(dst, src); \
  } \
 \
  inline ::dds_ReturnCode_t qos_type ## _copy( \
    qos_type & dst, \
    const typename qos_type::native_type * src) noexcept { \
    if (src == nullptr) { \
      return dds_RETCODE_BAD_PARAMETER; \
    } \
 \
    const typename qos_type::native_type * dst_ptr = dst; \
    if (src == dst_ptr) { \
      return dds_RETCODE_OK; \
    } \
 \
    (void)::qos_type ## _finalize(dst); \
    return ::qos_type ## _copy(dst, src); \
  }

// RAII Qos 매크로 확장
RAII_QOS_DEFINE(dds_DomainParticipantFactoryQos)
RAII_QOS_DEFINE(dds_DomainParticipantQos)
RAII_QOS_DEFINE(dds_TopicQos)
RAII_QOS_DEFINE(dds_PublisherQos)
RAII_QOS_DEFINE(dds_SubscriberQos)
RAII_QOS_DEFINE(dds_DataWriterQos)
RAII_QOS_DEFINE(dds_DataReaderQos)

#undef RAII_QOS_DEFINE

#define RAII_QOS_OUT_API_DEFINE(c_api_name, entity_type, qos_type) \
  inline ::dds_ReturnCode_t c_api_name(::entity_type * self, qos_type & qos) noexcept { \
    (void)::qos_type ## _finalize(qos); \
    return ::c_api_name(self, qos); \
  }

RAII_QOS_OUT_API_DEFINE(
  dds_DomainParticipant_get_qos,
  dds_DomainParticipant,
  dds_DomainParticipantQos)

RAII_QOS_OUT_API_DEFINE(
  dds_DomainParticipant_get_default_publisher_qos,
  dds_DomainParticipant,
  dds_PublisherQos)

RAII_QOS_OUT_API_DEFINE(
  dds_DomainParticipant_get_default_subscriber_qos,
  dds_DomainParticipant,
  dds_SubscriberQos)

RAII_QOS_OUT_API_DEFINE(
  dds_DomainParticipant_get_default_topic_qos,
  dds_DomainParticipant,
  dds_TopicQos)

RAII_QOS_OUT_API_DEFINE(
  dds_DomainParticipantFactory_get_default_participant_qos,
  dds_DomainParticipantFactory,
  dds_DomainParticipantQos)

RAII_QOS_OUT_API_DEFINE(
  dds_DomainParticipantFactory_get_qos,
  dds_DomainParticipantFactory,
  dds_DomainParticipantFactoryQos)

RAII_QOS_OUT_API_DEFINE(dds_Topic_get_qos, dds_Topic, dds_TopicQos)

RAII_QOS_OUT_API_DEFINE(dds_Publisher_get_qos, dds_Publisher, dds_PublisherQos)

RAII_QOS_OUT_API_DEFINE(dds_Publisher_get_default_datawriter_qos, dds_Publisher, dds_DataWriterQos)

RAII_QOS_OUT_API_DEFINE(dds_DataWriter_get_qos, dds_DataWriter, dds_DataWriterQos)

RAII_QOS_OUT_API_DEFINE(dds_Subscriber_get_qos, dds_Subscriber, dds_SubscriberQos)

RAII_QOS_OUT_API_DEFINE(
  dds_Subscriber_get_default_datareader_qos,
  dds_Subscriber,
  dds_DataReaderQos)

RAII_QOS_OUT_API_DEFINE(dds_DataReader_get_qos, dds_DataReader, dds_DataReaderQos)

#undef RAII_QOS_OUT_API_DEFINE

// copy_qos 함수 래퍼 정의 매크로
// QoS 값을 가져오기 전 finalize()를 호출하여 기존에 들고 있는 자원을 정리함.
inline ::dds_ReturnCode_t dds_Publisher_copy_from_topic_qos(
  ::dds_Publisher * self,
  dds_DataWriterQos & datawriter_qos,
  const dds_TopicQos & topic_qos) noexcept
{
  (void)::dds_DataWriterQos_finalize(datawriter_qos);
  return ::dds_Publisher_copy_from_topic_qos(self, datawriter_qos, topic_qos);
}

inline ::dds_ReturnCode_t dds_Publisher_copy_from_topic_qos(
  ::dds_Publisher * self,
  dds_DataWriterQos & datawriter_qos,
  const ::dds_TopicQos * topic_qos) noexcept
{
  if (topic_qos == nullptr) {
    return dds_RETCODE_BAD_PARAMETER;
  }

  (void)::dds_DataWriterQos_finalize(datawriter_qos);
  return ::dds_Publisher_copy_from_topic_qos(self, datawriter_qos, topic_qos);
}

inline ::dds_ReturnCode_t dds_Subscriber_copy_from_topic_qos(
  ::dds_Subscriber * self,
  dds_DataReaderQos & datareader_qos,
  const dds_TopicQos & topic_qos) noexcept
{
  (void)::dds_DataReaderQos_finalize(datareader_qos);
  return ::dds_Subscriber_copy_from_topic_qos(self, datareader_qos, topic_qos);
}

inline ::dds_ReturnCode_t dds_Subscriber_copy_from_topic_qos(
  ::dds_Subscriber * self,
  dds_DataReaderQos & datareader_qos,
  const ::dds_TopicQos * topic_qos) noexcept
{
  if (topic_qos == nullptr) {
    return dds_RETCODE_BAD_PARAMETER;
  }

  (void)::dds_DataReaderQos_finalize(datareader_qos);
  return ::dds_Subscriber_copy_from_topic_qos(self, datareader_qos, topic_qos);
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// RAII dds_TypeSupport 래퍼
// 기존 GurumDDS C API dds_TypeSupport_create()함수로 자원을 할당함.
class dds_TypeSupport {
public:
  dds_TypeSupport() = default;
  dds_TypeSupport(dds_TypeSupport &) = delete;
  dds_TypeSupport & operator=(dds_TypeSupport &) = delete;
  dds_TypeSupport(dds_TypeSupport && other)
  : entity(std::exchange(other.entity, nullptr)) {}
  dds_TypeSupport & operator=(dds_TypeSupport && other)
  {
    if (this == &other) {
      return *this;
    }

    if (this->entity != nullptr) {
      dds_TypeSupport_delete(entity);
    }

    this->entity = std::exchange(other.entity, nullptr);

    return *this;
  }

  dds_TypeSupport(::dds_TypeSupport * typesupport)
  : entity(typesupport) {}
  dds_TypeSupport & operator=(::dds_TypeSupport * typesupport)
  {
    if (this->entity == typesupport) {
      return *this;
    }

    if (this->entity != nullptr) {
      dds_TypeSupport_delete(entity);
    }

    this->entity = typesupport;

    return *this;
  }

  bool operator==(std::nullptr_t) const noexcept {return entity == nullptr;}

  bool operator!=(std::nullptr_t) const noexcept {return entity != nullptr;}

  operator:: dds_TypeSupport * () noexcept {return entity;}

  operator const::dds_TypeSupport *() const noexcept {return entity;}

  ~dds_TypeSupport()
  {
    if (entity != nullptr) {
      dds_TypeSupport_delete(entity);
    }
  }

private:
  ::dds_TypeSupport * entity{nullptr};
};

inline void dds_TypeSupport_delete(raii::dds_TypeSupport & self)
{
  if (self == nullptr) {
    return;
  }

  ::dds_TypeSupport_delete(self);

  self = nullptr;
}

}  // namespace raii

#endif

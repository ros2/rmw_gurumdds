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


namespace raii{
//dds_*Seq 타입의 RAII Wrapper
//dds_*Seq는 dds_*Seq_create()로 만들어지므로 GurumDDS C API를 최대한 유지한 채 템플릿 클래스으로 RAII Wrapping하기가 어려움.
//따라서 매크로를 사용해 정의함.
#define RAII_SEQUENCE_DEFINE(seq_type)  \
    class seq_type {                     \
    public:                              \
        seq_type() = default;               \
        seq_type(seq_type&) = delete;       \
        seq_type& operator=(const seq_type&) = delete; \
        seq_type(seq_type&& other) : seq(other.seq) {         \
            other.seq = nullptr;             \
        }                                   \
        raii::seq_type& operator=(seq_type&& other){         \
            if(this->seq != nullptr)                          \
                ::seq_type##_delete(this->seq);    \
            this->seq = other.seq;          \
            other.seq = nullptr;             \
            return *this;                    \
        }                                   \
        ::seq_type* get() const noexcept {                                     \
            return this->seq;                                                      \
        }                                                                    \
        operator ::seq_type*() const noexcept {                                  \
            return this->seq;                                                         \
        }                                                                        \
        explicit operator bool() const noexcept {                                 \
            return this->seq != nullptr;                                           \
        }                                                                    \
        bool operator==(std::nullptr_t) const noexcept {                    \
            return this->seq == nullptr;                                    \
        }                                                                   \
        bool operator!=(std::nullptr_t) const noexcept {                    \
            return !(this->seq == nullptr);                                    \
        }                                                                   \
        ~ seq_type () {                 \
            if(this->seq != nullptr) {        \
                ::seq_type##_delete(this->seq);    \
                this->seq = nullptr;                \
            }                                   \
        }                                       \
        friend raii::seq_type seq_type##_create(uint32_t capacity); \
        friend void seq_type##_delete(seq_type& seq); \
    private:                    \
        ::seq_type * seq{nullptr};   \
    };                              \
    inline raii::seq_type seq_type##_create(uint32_t capacity) {    \
        raii::seq_type obj;                                       \
        obj.seq = ::seq_type##_create(capacity);                 \
        return obj;                                             \
    }                                                           \
    inline void seq_type##_delete(raii::seq_type& seq) {                 \
        if(seq.seq == nullptr)                                     \
            return;                                             \
        ::seq_type##_delete(seq.seq);                     \
        seq.seq = nullptr;                                      \
    }                                                           

RAII_SEQUENCE_DEFINE(dds_DataSeq);
RAII_SEQUENCE_DEFINE(dds_SampleInfoSeq);
RAII_SEQUENCE_DEFINE(dds_UnsignedLongSeq);
RAII_SEQUENCE_DEFINE(dds_InstanceHandleSeq);
RAII_SEQUENCE_DEFINE(dds_ConditionSeq);
    

}








#endif
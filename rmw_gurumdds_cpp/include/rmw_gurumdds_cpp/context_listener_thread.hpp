// Copyright 2022 GurumNetworks, Inc.
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

#ifndef RMW_GURUMDDS_CPP__CONTEXT_LISTENER_THREAD_HPP_
#define RMW_GURUMDDS_CPP__CONTEXT_LISTENER_THREAD_HPP_

namespace rmw_gurumdds_cpp
{
rmw_ret_t
run_listener_thread(rmw_context_t * ctx);

rmw_ret_t
stop_listener_thread(rmw_context_t * ctx);
}  // namespace rmw_gurumdds_cpp

#endif  // RMW_GURUMDDS_CPP__CONTEXT_LISTENER_THREAD_HPP_

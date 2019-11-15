# Copyright 2019 GurumNetworks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_package(gurumdds_cmake_module REQUIRED)
find_package(GurumDDS REQUIRED MODULE)

list(APPEND rmw_gurumdds_shared_cpp_INCLUDE_DIRS ${GurumDDS_INCLUDE_DIRS})
list(APPEND rmw_gurumdds_shared_cpp_LIBRARIES ${GurumDDS_LIBRARIES})

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

###############################################################################
#
# CMake module for finding GurumNetworks GurumDDS.
#
# Input variables:
#
# - GURUMDDS_HOME: Header files and libraries will be searched for in
#   `${GURUMDDS_HOME}/include` and ${GURUMDDS_HOME}/lib` respectively.
#
# Output variables:
#
# - GurumDDS_FOUND: flag indicating if the package was found
# - GurumDDS_INCLUDE_DIR: Path to the header files
# - GurumDDS_LIBRARIES: Path to the library
# - GurumDDS_GURUMIDL: Path to the idl2code generator
#
# Example usage:
#
#   find_package(gurumdds_cmake_module REQUIRED)
#   find_package(GurumDDS MODULE)
#   # use GurumDDS_* variables
#
###############################################################################

# lint_cmake: -convention/filename, -package/stdargs

if(DEFINED GurumDDS_FOUND)
  return()
endif()
set(GurumDDS_FOUND FALSE)

file(TO_CMAKE_PATH "$ENV{GURUMDDS_HOME}" _GURUMDDS_HOME)

if(NOT _GURUMDDS_HOME STREQUAL "")
  message(STATUS "Found GurumDDS:" ${_GURUMDDS_HOME})
  set(GurumDDS_HOME "${_GURUMDDS_HOME}")
  set(GurumDDS_INCLUDE_DIR "${_GURUMDDS_HOME}/include/")
  set(GurumDDS_LIBRARY_DIRS "${_GURUMDDS_HOME}/lib/")
  if(WIN32)
    set(ext "lib")
  elseif(APPLE)
    message(FATAL_ERROR "This operating system is not supported yet.")
    return()
  else() # Linux
    set(ext "so")
  endif()
  set(GurumDDS_LIBRARIES "${GurumDDS_LIBRARY_DIRS}libgurumdds.${ext}")

  file(GLOB library "${GurumDDS_LIBRARIES}")
  if(library)
    set(GurumDDS_FOUND TRUE)
  endif()

  if(WIN32)
    set(GurumDDS_GURUMIDL "${_GURUMDDS_HOME}/tools/gurumidl.exe")
    if(NOT EXISTS "${GurumDDS_GURUMIDL}")
      set(GurumDDS_GURUMIDL "${_GURUMDDS_HOME}/tool/gurumidl.exe")
      if(NOT EXISTS "${GurumDDS_GURUMIDL}")
        message(FATAL_ERROR "Could not find executable 'GurumIDL'")
      endif()
    endif()
  else()
    set(GurumDDS_GURUMIDL "${_GURUMDDS_HOME}/tools/gurumidl")
    if(NOT EXISTS "${GurumDDS_GURUMIDL}")
      set(GurumDDS_GURUMIDL "${_GURUMDDS_HOME}/tool/gurumidl")
      if(NOT EXISTS "${GurumDDS_GURUMIDL}")
        set(GurumDDS_GURUMIDL "${_GURUMDDS_HOME}/bin/gurumidl")
        if(NOT EXISTS "${GurumDDS_GURUMIDL}")
          message(FATAL_ERROR "Could not find executable 'GurumIDL'")
        endif()
      endif()
    endif()
  endif()
else()
  if(WIN32)
    set(GurumDDS_FOUND FALSE)
  else()
    find_package(gurumdds QUIET PATHS /usr /usr/lib /usr/local)
    if(gurumdds_FOUND)
      set(GurumDDS_HOME "${GURUMDDS_CONFIG_ROOT_DIR}")
      set(GurumDDS_INCLUDE_DIR ${GURUMDDS_INCLUDE_DIR})
      set(GurumDDS_LIBRARIES ${GURUMDDS_LIBRARY})
      set(GurumDDS_GURUMIDL ${GURUMDDS_GURUMIDL})
      set(GurumDDS_FOUND TRUE)
    else()
      set(GurumDDS_FOUND FALSE)
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GurumDDS
  FOUND_VAR GurumDDS_FOUND
  REQUIRED_VARS
  GurumDDS_INCLUDE_DIR
  GurumDDS_LIBRARIES
  GurumDDS_GURUMIDL
)

add_library(GurumDDS SHARED IMPORTED)
set_target_properties(GurumDDS PROPERTIES
        IMPORTED_LOCATION ${GurumDDS_LIBRARIES}
)

target_include_directories(GurumDDS INTERFACE ${GurumDDS_INCLUDE_DIR})

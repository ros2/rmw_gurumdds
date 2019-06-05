cmake_minimum_required(VERSION 3.5)
project(rmw_coredds_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(rcutils REQUIRED)
find_package(coredds_cmake_module REQUIRED)

find_package(CoreDDS QUIET MODULE)
if(CoreDDS_FOUND)
  find_package(CoreDDS MODULE)
endif()
if(NOT CoreDDS_FOUND OR COREDDS_STATIC_DISABLE)
  ament_package()
  if(NOT CoreDDS_FOUND)
    message(WARNING "Could not find CoreDDS - skipping '${PROJECT_NAME}'")
  else()
    message(STATUS "CoreDDS static rmw implementation explicitly disabled - skipping '${PROJECT_NAME}'")
  endif()
  return()
endif()

find_package(rosidl_typesupport_coredds_c)
if(NOT rosidl_typesupport_coredds_c_FOUND)
  ament_package()
  message(WARNING "Could not find ROSIDL TypeSupport for CoreDDS (rosidl_typesupport_coredds_c) - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rosidl_typesupport_coredds_cpp)
if(NOT rosidl_typesupport_coredds_cpp_FOUND)
  ament_package()
  message(WARNING "Could not find ROSIDL TypeSupport for CoreDDS (rosidl_typesupport_coredds_cpp) - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rmw REQUIRED)
find_package(rmw_coredds_shared_cpp REQUIRED)
find_package(rosidl_generator_c REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)

include_directories(include)

add_library(rmw_coredds_cpp
  SHARED
  src/identifier.cpp
  src/serialization_format.cpp
  src/rmw_client.cpp
  src/rmw_compare_gids_equal.cpp
  src/rmw_count.cpp
  src/rmw_get_implementation_identifier.cpp
  src/rmw_get_serialization_format.cpp
  src/rmw_guard_condition.cpp
  src/rmw_init.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_publisher.cpp
  src/rmw_request.cpp
  src/rmw_response.cpp
  src/rmw_serialize.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_subscription.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_wait.cpp
  src/type_support_common.hpp
  src/get_entities.cpp
)

ament_target_dependencies(rmw_coredds_cpp
  "rcutils"
  "rosidl_typesupport_coredds_c"
  "rosidl_typesupport_coredds_cpp"
  "rmw_coredds_shared_cpp"
  "rmw"
  "rosidl_generator_c"
  "rosidl_generator_cpp"
  "CoreDDS")

ament_export_include_directories(include)
ament_export_libraries(rmw_coredds_cpp)

ament_export_dependencies(
  rosidl_typesupport_coredds_cpp
  rosidl_typesupport_coredds_c
  rosidl_generator_c
  rosidl_generator_cpp
  rcutils
  rmw_coredds_shared_cpp
  rmw
)

register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_coredds_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_coredds_cpp")

configure_rmw_library(rmw_coredds_cpp)

# Causes the visibility macros to use dllexport rather than dllimport
# which is appropriate when building the library but not consuming it.
target_compile_definitions(rmw_coredds_cpp PRIVATE "RMW_COREDDS_CPP_BUILDING_LIBRARY")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS_POST "rmw_coredds_cpp-extras.cmake"
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS rmw_coredds_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

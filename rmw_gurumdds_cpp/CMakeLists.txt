cmake_minimum_required(VERSION 3.5)
project(rmw_gurumdds_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(rcutils REQUIRED)
find_package(gurumdds_cmake_module REQUIRED)

find_package(GurumDDS QUIET MODULE)
if(GurumDDS_FOUND)
  find_package(GurumDDS MODULE)
endif()
if(NOT GurumDDS_FOUND OR GURUMDDS_STATIC_DISABLE)
  ament_package()
  if(NOT GurumDDS_FOUND)
    message(WARNING "Could not find GurumDDS - skipping '${PROJECT_NAME}'")
  else()
    message(STATUS "GurumDDS static rmw implementation explicitly disabled - skipping '${PROJECT_NAME}'")
  endif()
  return()
endif()

find_package(rosidl_typesupport_introspection_c)
if(NOT rosidl_typesupport_introspection_c_FOUND)
  ament_package()
  message(WARNING "Could not find ROSIDL TypeSupport for GurumDDS (rosidl_typesupport_introspection_c) - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rosidl_typesupport_introspection_cpp)
if(NOT rosidl_typesupport_introspection_cpp_FOUND)
  ament_package()
  message(WARNING "Could not find ROSIDL TypeSupport for GurumDDS (rosidl_typesupport_introspection_cpp) - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rmw REQUIRED)
find_package(rmw_gurumdds_shared_cpp REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)

include_directories(include)

add_library(rmw_gurumdds_cpp
  SHARED
  src/identifier.cpp
  src/message_converter.cpp
  src/serialization_format.cpp
  src/rmw_client.cpp
  src/rmw_compare_gids_equal.cpp
  src/rmw_count.cpp
  src/rmw_event.cpp
  src/rmw_get_implementation_identifier.cpp
  src/rmw_get_serialization_format.cpp
  src/rmw_get_topic_endpoint_info.cpp
  src/rmw_guard_condition.cpp
  src/rmw_init.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_publisher.cpp
  src/rmw_qos.cpp
  src/rmw_request.cpp
  src/rmw_response.cpp
  src/rmw_serialize.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_subscription.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_wait.cpp
  src/type_support_common.hpp
  src/types.cpp
  src/get_entities.cpp
)

ament_target_dependencies(rmw_gurumdds_cpp
  "rcutils"
  "rosidl_typesupport_introspection_c"
  "rosidl_typesupport_introspection_cpp"
  "rmw_gurumdds_shared_cpp"
  "rmw"
  "rosidl_runtime_c"
  "rosidl_runtime_cpp"
  "GurumDDS")

ament_export_include_directories(include)
ament_export_libraries(rmw_gurumdds_cpp)

ament_export_dependencies(
  rosidl_typesupport_introspection_cpp
  rosidl_typesupport_introspection_c
  rosidl_runtime_c
  rosidl_runtime_cpp
  rcutils
  rmw_gurumdds_shared_cpp
  rmw
)

register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_introspection_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_introspection_cpp")

configure_rmw_library(rmw_gurumdds_cpp)

# Causes the visibility macros to use dllexport rather than dllimport
# which is appropriate when building the library but not consuming it.
target_compile_definitions(rmw_gurumdds_cpp PRIVATE "RMW_GURUMDDS_CPP_BUILDING_LIBRARY")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS_POST "rmw_gurumdds_cpp-extras.cmake"
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS rmw_gurumdds_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

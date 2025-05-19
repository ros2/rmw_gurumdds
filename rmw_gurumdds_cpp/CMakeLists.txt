cmake_minimum_required(VERSION 3.5)
project(rmw_gurumdds_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros_core REQUIRED)
find_package(rcpputils REQUIRED)
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
find_package(rmw_dds_common REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)

find_package(tracetools REQUIRED)
if(NOT tracetools_FOUND)
  ament_package()
  message(WARNING "Could not find tracetools for GurumDDS - skipping '${PROJECT_NAME}'")
  return()
endif()

include_directories(include)
include_directories(${GurumDDS_INCLUDE_DIR})
link_directories(${GurumDDS_LIBRARY_DIRS})

add_library(rmw_gurumdds_cpp
  SHARED
  src/cdr_buffer.cpp
  src/cdr_deser_buffer.cpp
  src/context_listener_thread.cpp
  src/demangle.cpp
  src/event_converter.cpp
  src/get_entities.cpp
  src/gid.cpp
  src/graph_cache.cpp
  src/identifier.cpp
  src/names_and_types_helpers.cpp
  src/namespace_prefix.cpp
  src/qos.cpp
  src/rmw_client.cpp
  src/rmw_compare_gids_equal.cpp
  src/rmw_context_impl.cpp
  src/rmw_count.cpp
  src/rmw_event.cpp
  src/rmw_features.cpp
  src/rmw_get_implementation_identifier.cpp
  src/rmw_get_network_flow_endpoints.cpp
  src/rmw_get_serialization_format.cpp
  src/rmw_get_topic_endpoint_info.cpp
  src/rmw_guard_condition.cpp
  src/rmw_init.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_publisher.cpp
  src/rmw_qos.cpp
  src/rmw_serialize.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_subscription.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_wait.cpp
  src/serialization_format.cpp
  src/event_info_common.cpp
  src/type_support.cpp
  src/type_support_common.cpp
  src/type_support_service.cpp
  src/wait.cpp
)
target_link_libraries(rmw_gurumdds_cpp PUBLIC
  ${rmw_dds_common_TARGETS}
  GurumDDS
  "rcpputils::rcpputils"
  "rcutils::rcutils"
  "rmw::rmw"
  "rmw_dds_common::rmw_dds_common_library"
  "rosidl_runtime_c::rosidl_runtime_c"
  "rosidl_runtime_cpp::rosidl_runtime_cpp"
  "rosidl_typesupport_introspection_c::rosidl_typesupport_introspection_c"
  "rosidl_typesupport_introspection_cpp::rosidl_typesupport_introspection_cpp"
  "tracetools::tracetools"
)
ament_export_libraries(rmw_gurumdds_cpp)

ament_export_dependencies(
  rcpputils
  rcutils
  rmw
  rmw_dds_common
  rosidl_runtime_c
  rosidl_runtime_cpp
  rosidl_typesupport_introspection_c
  rosidl_typesupport_introspection_cpp
  tracetools
)

register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_introspection_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_introspection_cpp"
)

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

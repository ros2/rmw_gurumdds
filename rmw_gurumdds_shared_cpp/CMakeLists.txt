cmake_minimum_required(VERSION 3.5)
project(rmw_gurumdds_shared_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(gurumdds_cmake_module REQUIRED)

find_package(GurumDDS QUIET MODULE)
if(GurumDDS_FOUND)
  find_package(GurumDDS MODULE)
endif()
if(NOT GurumDDS_FOUND)
  ament_package()
  message(WARNING "Could not find GurumDDS - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rcutils REQUIRED)
find_package(rmw REQUIRED)

include_directories(include)

ament_export_include_directories(include ${GurumDDS_INCLUDE_DIR})
ament_export_dependencies(rcutils)
ament_export_dependencies(rmw)

include_directories(${GurumDDS_INCLUDE_DIR})
link_directories(${GurumDDS_LIBRARY_DIRS})
add_library(
  rmw_gurumdds_shared_cpp
  SHARED
  src/demangle.cpp
  src/event_converter.cpp
  src/names_and_types_helpers.cpp
  src/namespace_prefix.cpp
  src/qos.cpp
  src/rmw_count.cpp
  src/rmw_event.cpp
  src/rmw_get_topic_endpoint_info.cpp
  src/rmw_guard_condition.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_wait.cpp
  src/types/custom_listener.cpp
)
#target_link_libraries(rmw_gurumdds_shared_cpp gurumdds)
ament_target_dependencies(rmw_gurumdds_shared_cpp
  "rcutils"
  "rmw"
  "GurumDDS")
ament_export_libraries(rmw_gurumdds_shared_cpp)

# Causes the visibility macros to use dllexport rather than dllimport
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(rmw_gurumdds_shared_cpp PRIVATE "RMW_GURUMDDS_SHARED_CPP_BUILDING_LIBRARY")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS "rmw_gurumdds_shared_cpp-extras.cmake"
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS rmw_gurumdds_shared_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

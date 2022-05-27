cmake_minimum_required(VERSION 3.5)
project(gurumdds_cmake_module)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

set(${PROJECT_NAME}_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
find_package(GurumDDS QUIET MODULE)
if(GurumDDS_FOUND)
  find_package(GurumDDS MODULE)
endif()
if(GurumDDS_HOME)
  file(TO_NATIVE_PATH "${GurumDDS_HOME}" GurumDDS_HOME)
  if(WIN32)
    ament_environment_hooks(env_hook/gurumdds.bat.in)
  else()
    ament_environment_hooks(env_hook/gurumdds.sh.in)
  endif()
endif()

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS "gurumdds_cmake_module-extras.cmake"
)

install(DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME})

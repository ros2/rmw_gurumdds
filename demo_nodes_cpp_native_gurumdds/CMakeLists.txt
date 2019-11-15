cmake_minimum_required(VERSION 3.5)
project(demo_nodes_cpp_native_gurumdds)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rmw REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rmw_gurumdds_cpp QUIET)

function(custom_add_executable target)
  add_executable(${target} src/${target}.cpp)
  ament_target_dependencies(${target}
    "rclcpp"
    "std_msgs"
    "rmw_gurumdds_cpp")
  install(TARGETS ${target}
    DESTINATION lib/${PROJECT_NAME})
endfunction()

find_package(GurumDDS QUIET MODULE)
if(GurumDDS_FOUND)
  find_package(GurumDDS MODULE)
endif()
if(NOT GurumDDS_FOUND)
  message(WARNING "Could not find GurumDDS - skipping '${PROJECT_NAME}'")
  ament_package()
  return()
endif()
if(rmw_gurumdds_cpp_FOUND)
  find_package(rmw_gurumdds_cpp REQUIRED)
  custom_add_executable(talker)
  custom_add_executable(listener)

  if(BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)
    ament_lint_auto_find_test_dependencies()

    find_package(ament_cmake_pytest REQUIRED)
    find_package(launch_testing_ament_cmake REQUIRED)

    set(executables "talker;listener")

    set(RMW_GURUMDDS_DEMO_EXPECTED_OUTPUT "")

    foreach(executable ${executables})
      list(APPEND RMW_GURUMDDS_DEMO_EXPECTED_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/test/${executable}")
    endforeach()

    set(RMW_GURUMDDS_DEMO_EXECUTABLE "")
    foreach(executable ${executables})
      list(APPEND RMW_GURUMDDS_DEMO_EXECUTABLE "$<TARGET_FILE:${executable}>")
    endforeach()

    string(REPLACE ";" "_" exe_list_underscore "${executables}")
    configure_file(
      test/test_executables.py.in
      test_${exe_list_underscore}.py.configured
      @ONLY
    )
    file(GENERATE
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_${exe_list_underscore}_$<CONFIG>.py"
      INPUT "${CMAKE_CURRENT_BINARY_DIR}/test_${exe_list_underscore}.py.configured"
    )

    add_launch_test(
      "${CMAKE_CURRENT_BINARY_DIR}/test_${exe_list_underscore}_$<CONFIG>.py"
      TARGET test_tutorial_${exe_list_underscore}
      TIMEOUT 30
      ENV
      RCL_ASSERT_RMW_ID_MATCHES=rmw_gurumdds_cpp
      RMW_IMPLEMENTATION=rmw_gurumdds_cpp
    )

    foreach(executable ${executables})
      set_property(
        TEST test_tutorial_${exe_list_underscore}
        APPEND PROPERTY DEPENDS ${executable})
    endforeach()
  endif()
endif()

ament_package()

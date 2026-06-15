find_package(ament_cmake_gmock REQUIRED)
find_package(ament_cmake_gtest REQUIRED)
find_package(osrf_testing_tools_cpp REQUIRED)

ament_add_gtest(test_buffer_backend_user_data test_buffer_backend_user_data.cpp)
if(TARGET test_buffer_backend_user_data)
  target_link_libraries(test_buffer_backend_user_data
    ${PROJECT_NAME}
    rmw::rmw
    rmw_dds_common::rmw_dds_common_library
    rosidl_runtime_c::rosidl_runtime_c
  )
endif()

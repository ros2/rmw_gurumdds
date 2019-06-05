find_package(coredds_cmake_module REQUIRED)
find_package(CoreDDS REQUIRED MODULE)

list(APPEND rmw_coredds_cpp_INCLUDE_DIRS ${CoreDDS_INCLUDE_DIRS})
list(APPEND rmw_coredds_cpp_LIBRARIES ${CoreDDS_LIBRARIES})

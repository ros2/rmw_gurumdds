#include "rmw/rmw.h"

#include "rmw_coredds_cpp/serialization_format.hpp"

extern "C"
{
const char *
rmw_get_serialization_format()
{
  return coredds_serialization_format;
}
}  // extern "C"

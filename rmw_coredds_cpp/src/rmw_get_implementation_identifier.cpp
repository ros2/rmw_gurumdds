#include "rmw/rmw.h"

#include "rmw_coredds_cpp/identifier.hpp"

extern "C"
{
const char *
rmw_get_implementation_identifier()
{
  return gurum_coredds_identifier;
}
}  // extern "C"

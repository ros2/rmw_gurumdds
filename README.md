# rmw_coredds
Implementation of the ROS middleware interface using [GurumNetworks CoreDDS](http://www.gurum.cc).

## Requirements
This project requires `rosidl_typesupport_coredds` to be built. For more information, see README.md of the [project](https://github.com/gurumnet/rosidl_typesupport_coredds).

## Packages
This project consists of three packages, `rmw_coredds_cpp`, `rmw_coredds_shared_cpp` and `demo_nodes_cpp_native_coredds`.  

### rmw_coredds_cpp
`rmw_coredds_cpp` is the rmw implementation. You can use this rmw implementation by setting environment variable `RMW_IMPLEMENTATION=rmw_coredds_cpp` after installation. For `rmw_coredds_cpp` to work properly, make sure to set environment variable `COREDDS_CONFIG=$COREDDS_HOME/coredds.yaml` and set `allow_loopback` variable in `coredds.yaml` to `true`. If you are not familiar with [YAML](https://yaml.org/), please note that YAML only supports spaces, not tabs, for indentation.  

```
DATA:
  allow_loopback: true
  dynamic_buffer: true
  mtu: auto # auto | number(1472)
  bitmap_capacity: 256
  buffer_capacity: 512
```

### rmw_coredds_shared_cpp
`rmw_coredds_shared_cpp` contains some functions used by `rmw_coredds_cpp`. `rmw_coredds_dynamic_cpp` is not implemented yet, but `rmw_coredds_shared_cpp` is separated for expandability.  

### demo_nodes_cpp_native_coredds
`demo_nodes_cpp_natvie_coredds` is similar to `demo_nodes_cpp_native` from ROS2 `demos`. This demo shows how to access the native handles of `rmw_coredds_cpp`.

## Implementation Status
Currently some features are not fully implemented.
- DDS Security
- Entity status

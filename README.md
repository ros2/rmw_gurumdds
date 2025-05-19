# rmw_gurumdds
Implementation of the ROS 2 middleware interface using [GurumNetworks GurumDDS](https://www.gurum.cc/index_eng).
Installation guide is available [here](https://docs.ros.org/en/humble/Installation/DDS-Implementations/Working-with-GurumNetworks-GurumDDS.html).

## Requirements
Required version of GurumDDS depends on the version of this project.
| rmw_gurumdds             | GurumDDS                    |
|--------------------------|-----------------------------|
| >= 6.0.0                 | >= 3.2.9                    |
| >= 5.0.0                 | >= 3.2.0                    |
| >= 3.6.0, < 5.0.0        | 3.1.x                       |
| >= 3.4.2, < 3.6.0        | 3.0.x                       |
| >= 3.3.0, < 3.4.2        | >= 2.8.3165, < 3.0.0        |
| 3.2.x                    | >= 2.8.3140, < 2.8.3165     |
| >= 3.1.2, < 3.2.0        | >= 2.8.0, < 2.8.3140        |
| <= 3.1.1                 | 2.7.x                       |

## Packages
### gurumdds_cmake_module
`gurumdds_cmake_module` looks for GurumDDS, and provides the information to other packages.

For `gurumdds_cmake_module` to work properly, you need to set `GURUMDDS_HOME` environment variable to where GurumDDS is located.

If GurumDDS is normally installed through the debian package, `GURUMDDS_HOME` will be set automatically.
For example, if `GURUMDDS_HOME=~/gurumdds` is set, the directory `~/gurumdds` will be:
```
gurumdds
├── include
│   └── gurumdds
│       ├── dcps.h
│       ├── dcpsx.h
│       ├── dds.h
│       ├── typesupport.h
│       └── xml.h
└── lib
    └── libgurumdds.so
```

### rmw_gurumdds_cpp
`rmw_gurumdds_cpp` is the rmw implementation. You can use this rmw implementation by setting environment variable `RMW_IMPLEMENTATION=rmw_gurumdds_cpp` after installation.

For `rmw_gurumdds_cpp` to work properly, make sure to set environment variable `GURUMDDS_CONFIG=~/gurumdds.yaml` and set `allow_loopback` variable in `gurumdds.yaml` to `true`.

`gurumdds.yaml` will be located in `/etc/gurumnet/gurumdds` if gurumdds is installed through the debian package.

Please note that YAML only supports spaces, not tabs, for indentation.
```
DATA:
  allow_loopback: true
  dynamic_buffer: true
  mtu: auto # auto | number(1472)
  bitmap_capacity: 256
  buffer_capacity: 512
```

### demo_nodes_cpp_native_gurumdds
`demo_nodes_cpp_natvie_gurumdds` is similar to `demo_nodes_cpp_native` from ROS2 `demos`. This demo shows how to access the native handles of `rmw_gurumdds_cpp`.

This package is disabled by default.

## Branches
There are three active branches in this project: `rolling`, `jazzy`, and `humble`.
New changes made in [ROS2 repository](https://github.com/ros2) will be applied to the `rolling` branch.

## Implementation Status
Following features are not fully implemented yet.
- DDS Security

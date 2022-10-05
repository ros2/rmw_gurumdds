# rmw_gurumdds
Implementation of the ROS middleware interface using [GurumNetworks GurumDDS](https://www.gurum.cc/index_eng).

## Requirements
Required version of GurumDDS depends on the version of this project.

| rmw_gurumdds            | GurumDDS                    |
|-------------------------|-----------------------------|
| 1.3.0 or higher         | higher than 2.8.3165        |
| 1.2.0 or higher         | 2.8.3140 or higher          |
| 1.1.1 or higher         | 2.8.x                       |
| 1.0.3 or higher         | 2.7.x                       |
| 1.0.2 or lower          | 2.6.x(deprecated)           |

## Packages
This project consists of three packages, `gurumdds_camke_module`, `rmw_gurumdds_cpp` and `demo_nodes_cpp_native_gurumdds`.

### gurumdds_cmake_module
`gurumdds_cmake_module` looks for GurumDDS, and provides the information to other packages.  
For `gurumdds_cmake_module` to work properly, you need to set `GURUMDDS_HOME` environment variable to where GurumDDS is located.  
For example, if you set `GURUMDDS_HOME=~/gurumdds`, the directory `~/gurumdds` should look like this:
```
gurumdds
├── gurumdds.lic
├── gurumdds.yaml
├── examples
│   └── ...
├── include
│   └── gurumdds
│       ├── dcps.h
│       ├── dcpsx.h
│       ├── typesupport.h
│       └── xml.h
├── lib
│   └── libgurumdds.so
├── Makefile
└── tool
    └── gurumidl
```

### rmw_gurumdds_cpp
`rmw_gurumdds_cpp` is the rmw implementation. You can use this rmw implementation by setting environment variable `RMW_IMPLEMENTATION=rmw_gurumdds_cpp` after installation. For `rmw_gurumdds_cpp` to work properly, make sure to set environment variable `GURUMDDS_CONFIG=$GURUMDDS_HOME/gurumdds.yaml` and set `allow_loopback` variable in `gurumdds.yaml` to `true`. If you are not familiar with [YAML](https://yaml.org/), please note that YAML only supports spaces, not tabs, for indentation.  

```
DATA:
  allow_loopback: true
  dynamic_buffer: true
  mtu: auto # auto | number(1472)
  bitmap_capacity: 256
  buffer_capacity: 512
```

### rmw_gurumdds_shared_cpp
~~`rmw_gurumdds_shared_cpp` contains some functions used by `rmw_gurumdds_cpp`.~~  
This package was integrated into `rmw_gurumdds_cpp`.

### demo_nodes_cpp_native_gurumdds
`demo_nodes_cpp_natvie_gurumdds` is similar to `demo_nodes_cpp_native` from ROS2 `demos`. This demo shows how to access the native handles of `rmw_gurumdds_cpp`.  
This package is disabled by default.

## Branches
There are four active branches in this project: master, humble, galactic and foxy.
New changes made in [ROS2 repository](https://github.com/ros2) will be applied to the master branch.
If you want to use this project with ROS2 Rolling Ridley, Humble Hawksbill or Galactic Geochelone, please use master, humble or galactic branch, respectively.

## Implementation Status
Currently some features are not fully implemented.
- DDS Security

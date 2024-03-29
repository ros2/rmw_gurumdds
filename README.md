# rmw_gurumdds
Implementation of the ROS middleware interface using [GurumNetworks GurumDDS](https://www.gurum.cc/index_eng).  
[Installation guide](https://docs.ros.org/en/rolling/Installation/DDS-Implementations/Working-with-GurumNetworks-GurumDDS.html) is available.

## Requirements
Required version of GurumDDS depends on the version of this project.

| rmw_gurumdds             | GurumDDS                    |
|--------------------------|-----------------------------|
| 4.1.0 or higher          | higher than 2.8.3165        |
| 4.0.0 or higher          | 2.8.3140 or higher          |

## Packages
This project consists of three packages, `gurumdds_camke_module`, `rmw_gurumdds_cpp` and `demo_nodes_cpp_native_gurumdds`.

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
If you are not familiar with [YAML](https://yaml.org/), please note that YAML only supports spaces, not tabs, for indentation.  

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
`demo_nodes_cpp_natvie_gurumdds` is similar to `demo_nodes_cpp_native` from ROS2 `demos`.  
This demo shows how to access the native handles of `rmw_gurumdds_cpp`.  
This package is disabled by default.

## Branches
There are three active branches in this project: master, humble and foxy.  
New changes made in [ROS2 repository](https://github.com/ros2) will be applied to the master branch, so this branch might be unstable.  
If you want to use this project with ROS2 Humble Hawksbill or Foxy Fitzroy, please use humble or foxy branch, respectively.

## Implementation Status
Currently some features are not fully implemented.
- DDS Security

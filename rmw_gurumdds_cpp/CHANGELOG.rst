^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.3.0 (2022-10-05)
------------------
* Apply graph cache
* Apply on_remote_changed callback
* Avoid string generation on func call
* Change the behavior of take response to a loop
* Fix typo
* Redefine rmw gurumdds identifier
* Pass extra include dirs to cppcheck explicitly
* Integrate rmw_gurumdds_shared_cpp into rmw_gurumdds_cpp
* Contributors: Youngjin Yun, donghee811

1.2.3 (2022-07-05)
------------------
* Add missing guid comparison conditional statement
* Contributors: Youngjin Yun

1.2.2 (2022-05-27)
------------------

1.2.1 (2022-05-25)
------------------
* Handle null string
* Fix rclcpp test(test_serialized_message) failure
* Contributors: Youngjin Yun, donghee811

1.2.0 (2022-04-26)
------------------
* Enhanced rpc with sampleinfoex
* Basic rpc
* Contributors: Youngjin Yun

1.1.6 (2022-04-26)
------------------
* Remove minimum dds version as raw typesupport removed
* Contributors: Youngjin Yun

1.1.5 (2022-03-23)
------------------
* Revert raw typesupport patch
* Contributors: Youngjin Yun

1.1.4 (2022-03-17)
------------------
* Remove dead store
* Adjust minimum dds version
* Contributors: Youngjin Yun

1.1.3 (2022-02-11)
------------------
* Use raw typesupport instead of typesupport
  * To reduce memory usage
* Contributors: hyeonwoo

1.1.2 (2022-02-11)
------------------
* Add omitted free
* Change to delete only the entities created by the user
* Contributors: Youngjin Yun

1.1.1 (2022-01-03)
------------------
* Update packages to use gurumdds-2.8 & Update README
* Contributors: Youngjin Yun

1.1.0 (2021-11-17)
------------------
* Remove dds_typesupport from Publisher/Subscriber Info
* Change the return time when destroying entities
* Add ommited memory manage code
* Modify unnecessary code
* Fix typo
* Update return value
* Contributors: Youngjin Yun

1.0.12 (2021-10-14)
-------------------

1.0.11 (2021-10-13)
-------------------

1.0.10 (2021-09-02)
-------------------
* Fix unbounded sequence size
* Contributors: Youngjin Yun

1.0.9 (2021-07-23)
------------------
* Revise for lint
* Contributors: Youngjin Yun

1.0.8 (2021-07-22)
------------------
* Remove datareader listener patch
* Remove unnecessary operation
* Contributors: Kumazuma, Youngjin Yun

1.0.7 (2021-07-14)
------------------
* Use variable attempt to take the number of times equal to count
* Check if the queue is empty before using it
* Contributors: Youngjin Yun

1.0.6 (2021-05-07)
------------------
* Update code about build error on windows
* Contributors: Youngjin Yun

1.0.5 (2021-04-12)
------------------
* Use dds_free instead of free for dll library
* Contributors: Youngjin Yun

1.0.4 (2021-03-10)
------------------
* Change maintainer
* Use DataReader listener for taking data samples
* Contributors: junho, youngjin

1.0.3 (2020-11-19)
------------------
* Update packages to use gurumdds-2.7
* Contributors: junho

1.0.2 (2020-07-29)
------------------
* Change maintainer
* Contributors: junho

1.0.1 (2020-07-06)
------------------
* Renamed rmw_gurumdds_dynamic_cpp to rmw_gurumdds_cpp
* Renamed rmw_gurumdds_cpp to rmw_gurumdds_static_cpp
* Contributors: junho

1.0.0 (2020-06-04)
------------------
* Fixed wrong package version
* MANUAL_BY_NODE liveliness is deprecated
* Updated packages to use gurumdds-2.6
* Replaced rosidl_message_bounds_t with rosidl_runtime_c__Sequence__bound
* Replaced rmw_request_id_t with rmw_service_info_t
* Added rmw_take_sequence()
* Fill timestamps in message info
* Fixed template specialization
* security_context is renamed to enclave
* Replaced rosidl_generator\_* with rosidl_runtime\_*
* Added incompatible qos support
* Apply one participant per context API changes
* Fixed serialization/deserialization errors
* Fixed some errors
  * added missing qos finalization
  * fixed issue that topic endpoint info was not handled correctly
  * added null check to builtin datareader callbacks
* Added qos finalization after creating publisher/subscriber
* Added event init functions
* Implemented rmw_serialize/rmw_deserialize
* Implemented client
* Implemented service
* Fixed code style divergence
* Implemented subscription
* Fixed some errors in cdr buffer
* Implemented publisher
* Implemented serialization/deserialization
* Suppress complie warnings
* Modified structures in types.hpp
* Implemented create_metastring()
* added rmw_gurumdds_cpp
* Contributors: junho

0.8.2 (2019-12-19)
------------------

0.8.1 (2019-11-15)
------------------

0.8.0 (2019-11-06)
------------------

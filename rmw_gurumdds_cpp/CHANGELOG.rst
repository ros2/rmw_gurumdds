^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

3.4.2 (2023-09-18)
------------------
* Update packages to use gurumdds-3.0 & Update README
* Contributors: Jaemin Jo

3.4.1 (2023-09-13)
------------------
* Fix interoperability errors: Remove buffer roundup at the end of messeage deserialize function
* Contributors: Jaemin Jo

3.4.0 (2023-03-08)
------------------
* Add maintainer
* Add null handling
* Apply loop to take sequence
* Contributors: Youngjin Yun

3.3.1 (2022-11-30)
------------------
* Remove sleep from entity creation
* Contributors: Youngjin Yun

3.3.0 (2022-10-05)
------------------
* Apply graph cache (`#17 <https://github.com/ros2/rmw_gurumdds/issues/17>`_)
  * graph_cache 0718
  * Add topic name creation
  * graph_cache 0722
  * Sync api
  * add context::finalize()
  * Wrap-up pub and sub
  * fix gid
  * Wrap-up client and service
  * fix segfault
  * Add set on callback empty stub
  * Add content filter topic feature empty stub
  * Change the behavior of take response to a loop
  * Add missing dependency
  * Fix bug and remove unnecessary struct
  * Synk for log
  * Amend log and scope exit for node
  * Apply on_remote_changed callback & fix typo
  * Cleanup log
  * Reposition message handling timing of response
  Co-authored-by: donghee811 <donghee@gurum.cc>
* Redefine rmw gurumdds identifier
* Integrate rmw_gurumdds_shared_cpp into rmw_gurumdds_cpp
* Contributors: Youngjin Yun

3.2.2 (2022-07-05)
------------------
* Add missing guid comparison conditional statement
* Add sequence numbers to message info structure
* Contributors: Youngjin Yun, donghee811

3.2.1 (2022-06-02)
------------------
* Handle null string
* Fix rclcpp test(test_serialized_message) failure
* Fix cast style and relative paths for cpplint
* Contributors: Youngjin Yun

3.2.0 (2022-04-26)
------------------
* Enhanced rpc with sampleinfoex
* Basic rpc
* Contributors: Youngjin Yun

3.1.6 (2022-04-26)
------------------
* Remove minimum dds version as raw typesupport removed
* Revert raw typesupport patch
* Contributors: Youngjin Yun

3.1.5 (2022-03-17)
------------------
* Remove dead store
* Adjust minimum dds version
* Contributors: Youngjin Yun

3.1.4 (2022-02-16)
------------------
* Use raw typesupport instead of typesupport
* Contributors: Youngjin Yun

3.1.3 (2022-02-16)
------------------
* Add omitted free
* Change to delete only the entities created by the user
* Contributors: Youngjin Yun

3.1.2 (2022-01-03)
------------------
* Update packages to use gurumdds-2.8 & Update README
* Contributors: Youngjin Yun

3.1.1 (2021-12-21)
------------------
* Add public to qos convert api& fix for uncrustify
* Contributors: Youngjin Yun

3.1.0 (2021-11-25)
------------------
* Use convert api for publisher/subscription Qos getters
* Add client/service Qos getters
* Remove dds_typesupport from Publisher/Subscriber Info
* Change the return time when destroying entities
* Add ommited memory manage code
* Modify unnecessary code
* Fix typo
* Update return value
* Contributors: Youngjin Yun

3.0.9 (2021-10-14)
------------------

3.0.8 (2021-10-14)
------------------

3.0.7 (2021-09-27)
------------------

3.0.6 (2021-09-23)
------------------
* Revise for lint
* Contributors: Youngjin Yun

3.0.5 (2021-09-23)
------------------
* Update rmw_context_impl_t definition
* Add rmw_publisher_wait_for_all_acked
* Contributors: Youngjin Yun

3.0.4 (2021-09-02)
------------------
* Fix unbounded sequence size
* Contributors: Youngjin Yun

3.0.3 (2021-08-19)
------------------
* Remove datareader listener patch
* Remove unnecessary operation
* Contributors: Youngjin Yun

3.0.2 (2021-07-14)
------------------

3.0.1 (2021-07-07)
------------------
* Use variable attempt to take the number of times equal to count
* Check if the queue is empty before using it
* Contributors: Youngjin Yun

3.0.0 (2021-04-29)
------------------
* Revise for lint
* Contributors: Youngjin Yun

2.1.4 (2021-04-22)
------------------
* Indicate missing support for unique network flows
* Contributors: Youngjin Yun

2.1.3 (2021-04-12)
------------------
* Use dds_free instead of free for dll library
* Contributors: Youngjin Yun

2.1.2 (2021-03-22)
------------------
* Update code about build error on windows
* Add RMW function to check QoS compatibility
* Contributors: Youngjin Yun, youngjin

2.1.1 (2021-03-12)
------------------
* Update packages to use gurumdds-2.7
* Contributors: youngjin

2.1.0 (2021-02-23)
------------------
* Change maintainer
* Handle typesupport errors on retrieval
* Set actual domain id into context
* Fix wrong error messages
* Use DataReader listener for taking data samples
* Contributors: junho, youngjin

2.0.1 (2020-07-29)
------------------
* Change maintainer
* Contributors: junho

2.0.0 (2020-07-09)
------------------
* Removed parameters domain_id and localhost_only from rmw_create_node()
* Updated init/shutdown/init option functions
* Contributors: junho

1.1.0 (2020-07-09)
------------------
* Finalize rmw context only if it's shutdown
* Added support for sample_lost event
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

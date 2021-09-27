^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.1.1 (2021-03-12)
------------------
* Update packages to use gurumdds-2.7
* Contributors: youngjin

2.1.10 (2021-09-27)
-------------------

2.1.9 (2021-09-02)
------------------
* Fix unbounded sequence size
* Contributors: Youngjin Yun

2.1.8 (2021-08-19)
------------------
* Remove datareader listener patch
* Remove unnecessary operation
* Contributors: Youngjin Yun

2.1.7 (2021-07-14)
------------------

2.1.6 (2021-07-08)
------------------
* Use variable attempt to take the number of times equal to count
* Check if the queue is empty before using it
* Contributors: Youngjin Yun

2.1.5 (2021-04-28)
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
* 2.1.1
* Update packages to use gurumdds-2.7
* Contributors: Youngjin Yun, youngjin

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

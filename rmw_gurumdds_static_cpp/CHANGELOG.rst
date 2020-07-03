^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_static_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.0.0 (2020-06-04)
------------------
* MANUAL_BY_NODE liveliness is deprecated
* Updated packages to use gurumdds-2.6
* Replaced rosidl_message_bounds_t with rosidl_runtime_c__Sequence__bound
* Replaced rmw_request_id_t with rmw_service_info_t
* Added rmw_take_sequence()
* Fill timestamps in message info
* security_context is renamed to enclave
* Replaced rosidl_generator\_* with rosidl_runtime\_*
* Added incompatible qos support
* Apply one participant per context API changes
* Fixed some errors
  * added missing qos finalization
  * fixed issue that topic endpoint info was not handled correctly
  * added null check to builtin datareader callbacks
* Added qos finalization after creating publisher/subscriber
* Added event init functions
* Implemented client
* Implemented service
* Implemented subscription
* Implemented publisher
* added rmw_gurumdds_dynamic_cpp
* Minor refactoring
* Fixed some errors
* Implemented rmw_get_publishers/subscriptions_info_by_topic()
* Contributors: junho

0.8.2 (2019-12-19)
------------------
* updated packages to use gurumdds-2.5
* Contributors: hyeonwoo

0.8.1 (2019-11-15)
------------------
* CoreDDS is renamed to GurumDDS
* Contributors: junho

0.8.0 (2019-11-06)
------------------
* added environment variable for init log message
* added gurumdds dependency to package.xml
* refactored error handling code
* added logs for publish/take functions
* wait for announcements after creating entities
* minor fixes
  * fixed incorrect null checks
  * fixed possible memory leaks
* added new zero copy api
* added localhost_only parameter to rmw_create_node()
* updated create_publisher/subscription API
* adjusted sleep time before discovery functions and fixed typos
* now rmw_wait() can handle events properly
* fixed indents
* Implemented rmw_get_client_names_and_types_by_node()
* changed content of PublisherGID
* fixed code style divergence
* fixed typos
* added rmw_subscription_get_actual_qos()
* fixed code style divergence
* updated cmake to fit new library paths
* updated rmw_publisher_get_actual_qos() to get all supported qos
* migration from gitlab
* Contributors: junho

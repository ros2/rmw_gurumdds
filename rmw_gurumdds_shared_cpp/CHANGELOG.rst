^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_shared_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.0.0 (2020-07-09)
------------------
* Removed parameters domain_id and localhost_only from rmw_create_node()
* Contributors: junho

1.1.0 (2020-07-09)
------------------
* Handle RMW_DEFAULT_DOMAIN_ID
* Added support for sample_lost event
* Set resource_limit explicitly
* Fixed compile warnings
* Contributors: junho

1.0.0 (2020-06-04)
------------------
* MANUAL_BY_NODE liveliness is deprecated
* Updated packages to use gurumdds-2.6
* security_context is renamed to enclave
* Added incompatible qos support
* Apply one participant per context API changes
* Fixed some errors
  * added missing qos finalization
  * fixed issue that topic endpoint info was not handled correctly
  * added null check to builtin datareader callbacks
* Fixed missing string array finalization
* Added event init functions
* Follow changes made to rmw_topic_endpoint_info_array
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
* added gurumdds dependency to package.xml
* refactored error handling code
* wait for announcements after creating entities
* fixed wrong return value
* implemented localhost_only feature
* added localhost_only parameter to rmw_create_node()
* adjusted sleep time before discovery functions and fixed typos
* now rmw_wait() can handle events properly
* rewrote rmw_node_info_and_types
* fixed indents
* Implemented rmw_get_client_names_and_types_by_node()
* fixed code style divergence
* fixed typos
* updated cmake to fit new library paths
* migration from gitlab
* Contributors: junho

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_shared_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

3.2.0 (2022-04-26)
------------------

3.1.6 (2022-04-26)
------------------
* Remove minimum dds version as raw typesupport removed
* Contributors: Youngjin Yun

3.1.5 (2022-03-17)
------------------
* Remove dead store
* Adjust minimum dds version
* Contributors: Youngjin Yun

3.1.4 (2022-02-16)
------------------

3.1.3 (2022-02-16)
------------------
* Wrap up unordered_map with shared_ptr
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
* Add pdp handling process
* Add client/service Qos getters
* Update return value
* Contributors: Youngjin Yun

3.0.9 (2021-10-14)
------------------
* Add missing return
* Contributors: Youngjin Yun

3.0.8 (2021-10-14)
------------------
* Fix bug: condition of dw/dr seq delete
* Support static discovery
* Contributors: Youngjin Yun

3.0.7 (2021-09-27)
------------------
* Remove sleep before fill tnat
* Contributors: Youngjin Yun

3.0.6 (2021-09-23)
------------------

3.0.5 (2021-09-23)
------------------
* Stop double-defining structs
* Add rmw_publisher_wait_for_all_acked
* Contributors: Youngjin Yun

3.0.4 (2021-09-02)
------------------

3.0.3 (2021-08-19)
------------------
* Wait for state change of topic cache
* Remove datareader listener patch
* Remove attached waitset conditions on destructor
* Remove unnecessary operation
* Contributors: Youngjin Yun

3.0.2 (2021-07-14)
------------------
* Move handle sequence delete into right place
* Contributors: Youngjin Yun

3.0.1 (2021-07-07)
------------------

3.0.0 (2021-04-29)
------------------
* Revise for lint
* Contributors: Youngjin Yun

2.1.4 (2021-04-22)
------------------
* Take and return new RMW_DURATION_INFINITE
* Contributors: Youngjin Yun

2.1.3 (2021-04-12)
------------------
* fix typo
* Contributors: Youngjin Yun

2.1.2 (2021-03-22)
------------------
* Update code about build error on windows
* Add RMW function to check QoS compatibility
* Contributors: Youngjin Yun, youngjin

2.1.1 (2021-03-12)
------------------
* Update packages to use gurumdds-2.7
* fix typo
* Contributors: youngjin

2.1.0 (2021-02-23)
------------------
* Change maintainer
* Set actual domain id into context
* Use DataReader listener for taking data samples
* Contributors: junho, youngjin

2.0.1 (2020-07-29)
------------------
* Change maintainer
* Contributors: junho

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

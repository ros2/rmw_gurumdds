^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_shared_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.2.0 (2022-04-26)
------------------

1.1.6 (2022-04-26)
------------------
* Remove minimum dds version as raw typesupport removed
* Contributors: Youngjin Yun

1.1.5 (2022-03-23)
------------------

1.1.4 (2022-03-17)
------------------
* Remove dead store
* Adjust minimum dds version
* Contributors: Youngjin Yun

1.1.3 (2022-02-11)
------------------

1.1.2 (2022-02-11)
------------------
* Wrap up unordered_map with shared_ptr
* Change to delete only the entities created by the user
* Contributors: Youngjin Yun

1.1.1 (2022-01-03)
------------------
* Update packages to use gurumdds-2.8 & Update README
* Contributors: Youngjin Yun

1.1.0 (2021-11-17)
------------------
* Modify unnecessary code
* Update return value
* Contributors: Youngjin Yun

1.0.12 (2021-10-14)
-------------------
* Fix bug: condition of dw/dr seq delete
* Contributors: Youngjin Yun

1.0.11 (2021-10-13)
-------------------
* Fix bug: condition of dw_seq delete
* Support static discovery
* Contributors: Youngjin Yun

1.0.10 (2021-09-02)
-------------------

1.0.9 (2021-07-23)
------------------
* Revise for lint
* Contributors: Youngjin Yun

1.0.8 (2021-07-22)
------------------
* Remove datareader listener patch
* Remove attached waitset conditions on destructor
* Remove unnecessary operation
* Contributors: Kumazuma, Youngjin Yun

1.0.7 (2021-07-14)
------------------
* Move handle sequence delete into right place
* Contributors: Youngjin Yun

1.0.6 (2021-05-07)
------------------
* fix typo: check namespace\_ allocate
* Update code about build error on windows
* Contributors: Youngjin Yun

1.0.5 (2021-04-12)
------------------
* fix typo
* Contributors: Youngjin Yun

1.0.4 (2021-03-10)
------------------
* Change maintainer
* fix typo
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

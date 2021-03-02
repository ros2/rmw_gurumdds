^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_shared_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.8.8 (2021-03-02)
------------------
* Use DataReader listener for taking data samples
* Delete contained entities before deleting domain participant
* Change maintainer
* Update packages to use gurumdds-2.7
* Contributors: youngjin

0.8.7 (2020-07-29)
------------------
* Change maintainer
* Contributors: junho

0.8.6 (2020-07-06)
------------------
* Set resource_limit explicitly
* Fixed compile warnings
* Contributors: junho

0.8.5 (2020-06-04)
------------------
* Updated packages to use gurumdds-2.6
* Contributors: junho

0.8.4 (2020-04-16)
------------------

0.8.3 (2020-04-01)
------------------
* Fixed some errors
  * added missing qos finalization
  * fixed issue that topic endpoint info was not handled correctly
  * added null check to builtin datareader callbacks
* Fixed missing string array finalization
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

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Forthcoming
-----------
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
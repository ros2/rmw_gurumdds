^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_shared_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

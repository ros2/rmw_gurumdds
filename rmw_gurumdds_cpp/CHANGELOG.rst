^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.7.11 (2021-04-12)
-------------------
* Use dds_free instead of free for dll library
* Contributors: Youngjin Yun

0.7.10 (2021-03-10)
-------------------
* Change maintainer
* Use DataReader listener for taking data samples
* Contributors: junho, youngjin

0.7.9 (2020-11-19)
------------------
* Update packages to use gurumdds-2.7
* Contributors: junho

0.7.8 (2020-07-29)
------------------
* Change maintainer
* Contributors: junho

0.7.7 (2020-07-06)
------------------
* Renamed rmw_gurumdds_dynamic_cpp to rmw_gurumdds_cpp
* Renamed rmw_gurumdds_cpp to rmw_gurumdds_static_cpp
* Contributors: junho

0.7.6 (2020-06-04)
------------------
* Updated packages to use gurumdds-2.6
* Contributors: junho

0.7.5 (2020-04-16)
------------------
* Fixed template specialization
* Contributors: junho

0.7.4 (2020-04-01)
------------------
* Fixed serialization/deserialization errors
* Fixed some errors
  * added missing qos finalization
  * fixed issue that topic endpoint info was not handled correctly
  * added null check to builtin datareader callbacks
* Added qos finalization after creating publisher/subscriber
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

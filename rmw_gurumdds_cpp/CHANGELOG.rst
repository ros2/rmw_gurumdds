^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_gurumdds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

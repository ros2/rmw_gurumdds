// Copyright 2019 GurumNetworks, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RMW_GURUMDDS_SHARED_CPP__TOPIC_CACHE_HPP_
#define RMW_GURUMDDS_SHARED_CPP__TOPIC_CACHE_HPP_

#include <utility>
#include <set>
#include <string>
#include <map>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <mutex>
#include "rcutils/logging_macros.h"

typedef std::map<std::string, std::set<std::string>> TopicsTypes;

template<typename GUID_t>
class TopicCache
{
public:
  struct TopicInfo
  {
    GUID_t participant_guid;
    GUID_t topic_guid;
    std::string name;
    std::string type;
  };

  typedef std::map<GUID_t, std::multiset<GUID_t>> ParticipantToTopicGuidMap;
  typedef std::map<GUID_t, TopicInfo> TopicGuidToInfo;

  const TopicGuidToInfo & get_topic_guid_to_info() const
  {
    return topic_guid_to_info_;
  }

  const ParticipantToTopicGuidMap & get_participant_to_topic_guid_map() const
  {
    return participant_to_topic_guids_;
  }

  bool add_topic(
    const GUID_t & participant_guid,
    const GUID_t & topic_guid,
    const std::string & topic_name,
    const std::string & type_name)
  {
    initialize_participant_map(participant_to_topic_guids_, participant_guid);
    if (rcutils_logging_logger_is_enabled_for(
        "rmw_gurumdds_shared_cpp", RCUTILS_LOG_SEVERITY_DEBUG))
    {
      std::stringstream guid_stream;
      guid_stream << participant_guid;
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "Adding topic '%s' with type '%s' for node '%s'",
        topic_name.c_str(), type_name.c_str(), guid_stream.str().c_str());
    }
    auto topic_info_it = topic_guid_to_info_.find(topic_guid);
    if (topic_info_it != topic_guid_to_info_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unique topic '%s' with type '%s' attempted to be added twice, ignoring",
        topic_name.c_str(), type_name.c_str());
      return false;
    }
    topic_guid_to_info_[topic_guid] =
      TopicInfo {participant_guid, topic_guid, topic_name, type_name};
    participant_to_topic_guids_[participant_guid].insert(topic_guid);
    return true;
  }

  bool get_topic(const GUID_t & topic_guid, TopicInfo & topic_info) const
  {
    auto topic_info_it = topic_guid_to_info_.find(topic_guid);
    if (topic_info_it == topic_guid_to_info_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "topic not available");
      return false;
    }
    return true;
  }

  bool remove_topic(const GUID_t & topic_guid)
  {
    auto topic_info_it = topic_guid_to_info_.find(topic_guid);
    if (topic_info_it == topic_guid_to_info_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unexpected topic removal");
      return false;
    }

    std::string topic_name = topic_info_it->second.name;
    std::string type_name = topic_info_it->second.type;

    auto participant_guid = topic_info_it->second.participant_guid;
    auto participant_to_topic_guid = participant_to_topic_guids_.find(participant_guid);
    if (participant_to_topic_guid == participant_to_topic_guids_.end()) {
      RCUTILS_LOG_WARN_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unable to remove topic, "
        "participant guid does not exist for topic name '%s' with type '%s'",
        topic_name.c_str(), type_name.c_str());
      return false;
    }

    auto topic_guid_to_remove = participant_to_topic_guid->second.find(topic_guid);
    if (topic_guid_to_remove == participant_to_topic_guid->second.end()) {
      RCUTILS_LOG_WARN_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unable to remove topic, "
        "topic guid does not exist in participant guid: topic name '%s' with type '%s'",
        topic_name.c_str(), type_name.c_str());
      return false;
    }

    topic_guid_to_info_.erase(topic_info_it);
    participant_to_topic_guid->second.erase(topic_guid_to_remove);
    if (participant_to_topic_guids_.empty()) {
      participant_to_topic_guids_.erase(participant_to_topic_guid);
    }

    return true;
  }

  TopicsTypes get_topic_types_by_guid(const GUID_t & participant_guid)
  {
    TopicsTypes topics_types;
    const auto participant_to_topic_guids = participant_to_topic_guids_.find(participant_guid);
    if (participant_to_topic_guids == participant_to_topic_guids_.end()) {
      return topics_types;
    }

    for (auto & topic_guid : participant_to_topic_guids->second) {
      auto topic_info = topic_guid_to_info_.find(topic_guid);
      if (topic_info == topic_guid_to_info_.end()) {
        continue;
      }
      auto topic_name = topic_info->second.name;
      auto topic_entry = topics_types.find(topic_name);
      if (topic_entry == topics_types.end()) {
        topics_types[topic_name] = std::set<std::string>();
      }
      topics_types[topic_name].insert(topic_info->second.type);
    }
    return topics_types;
  }

  void clear_cache(void)
  {
    topic_guid_to_info_.clear();
    participant_to_topic_guids_.clear();
  }

private:
  TopicGuidToInfo topic_guid_to_info_;
  ParticipantToTopicGuidMap participant_to_topic_guids_;

  void initialize_participant_map(ParticipantToTopicGuidMap & map, const GUID_t & participant_guid)
  {
    if (map.find(participant_guid) == map.end()) {
      map[participant_guid] = std::multiset<GUID_t>();
    }
  }
};

#endif  // RMW_GURUMDDS_SHARED_CPP__TOPIC_CACHE_HPP_

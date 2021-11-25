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

#include <algorithm>
#include <iterator>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "rcutils/logging_macros.h"

#include "rmw/types.h"

typedef std::map<std::string, std::set<std::string>> TopicsTypes;

template<typename GUID_t>
class TopicCache
{
public:
  struct TopicInfo
  {
    GUID_t participant_guid;
    GUID_t entity_guid;
    std::string name;
    std::string type;
    rmw_qos_profile_t qos;
  };

  typedef std::map<GUID_t, std::multiset<GUID_t>> ParticipantToEntityGuidMap;
  typedef std::map<GUID_t, TopicInfo> EntityGuidToInfo;
  typedef std::unordered_map<std::string, std::vector<TopicInfo>> TopicNameToInfo;

  const EntityGuidToInfo & get_entity_guid_to_info() const
  {
    return entity_guid_to_info_;
  }

  const ParticipantToEntityGuidMap & get_participant_to_entity_guid_map() const
  {
    return participant_to_entity_guids_;
  }

  const TopicNameToInfo get_topic_name_to_info() const
  {
    TopicNameToInfo tnti;
    for (auto & i : entity_guid_to_info_) {
      tnti[i.second.name].push_back(i.second);
    }
    return tnti;
  }

  bool add_topic(
    const GUID_t & participant_guid,
    const GUID_t & entity_guid,
    std::string && topic_name,
    std::string && type_name,
    rmw_qos_profile_t & qos)
  {
    initialize_participant_map(participant_to_entity_guids_, participant_guid);
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
    auto topic_info_it = entity_guid_to_info_.find(entity_guid);
    if (topic_info_it != entity_guid_to_info_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unique topic '%s' with type '%s' attempted to be added twice, ignoring",
        topic_name.c_str(), type_name.c_str());
      return false;
    }
    entity_guid_to_info_[entity_guid] =
      TopicInfo {participant_guid, entity_guid, std::move(topic_name), std::move(type_name), qos};
    participant_to_entity_guids_[participant_guid].insert(entity_guid);
    return true;
  }

  bool add_topic(
    const GUID_t & participant_guid,
    const GUID_t & entity_guid,
    const std::string & topic_name,
    const std::string & type_name,
    rmw_qos_profile_t & qos)
  {
    return add_topic(
      participant_guid, entity_guid, std::string(topic_name),
      std::string(type_name), qos);
  }

  bool get_topic(const GUID_t & entity_guid, TopicInfo & topic_info) const
  {
    auto topic_info_it = entity_guid_to_info_.find(entity_guid);
    if (topic_info_it == entity_guid_to_info_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "topic not available");
      return false;
    }
    topic_info = topic_info_it->second;
    return true;
  }

  int remove_topic_by_puid(const GUID_t & participant_guid)
  {
    int removed_topic_count = 0;
    auto participant_to_entity_guid = participant_to_entity_guids_.find(participant_guid);
    if (participant_to_entity_guid == participant_to_entity_guids_.end()) {
      return -1;
    }

    for (auto & entity_guid : participant_to_entity_guid->second) {
      auto topic_info_iter = entity_guid_to_info_.find(entity_guid);
      if (topic_info_iter == entity_guid_to_info_.end()) {
        continue;
      }
      entity_guid_to_info_.erase(topic_info_iter);
      removed_topic_count++;
    }

    participant_to_entity_guid->second.clear();
    if (participant_to_entity_guid->second.empty()) {
      participant_to_entity_guids_.erase(participant_to_entity_guid);
    }

    return removed_topic_count;
  }

  bool remove_topic(const GUID_t & entity_guid)
  {
    auto topic_info_it = entity_guid_to_info_.find(entity_guid);
    if (topic_info_it == entity_guid_to_info_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unexpected topic removal");
      return false;
    }

    std::string & topic_name = topic_info_it->second.name;
    std::string & type_name = topic_info_it->second.type;

    auto & participant_guid = topic_info_it->second.participant_guid;
    auto participant_to_entity_guid = participant_to_entity_guids_.find(participant_guid);
    if (participant_to_entity_guid == participant_to_entity_guids_.end()) {
      RCUTILS_LOG_WARN_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unable to remove topic, "
        "participant guid does not exist for topic name '%s' with type '%s'",
        topic_name.c_str(), type_name.c_str());
      return false;
    }

    auto entity_guid_to_remove = participant_to_entity_guid->second.find(entity_guid);
    if (entity_guid_to_remove == participant_to_entity_guid->second.end()) {
      RCUTILS_LOG_WARN_NAMED(
        "rmw_gurumdds_shared_cpp",
        "unable to remove topic, "
        "topic guid does not exist in participant guid: topic name '%s' with type '%s'",
        topic_name.c_str(), type_name.c_str());
      return false;
    }

    entity_guid_to_info_.erase(topic_info_it);
    participant_to_entity_guid->second.erase(entity_guid_to_remove);
    if (participant_to_entity_guid->second.empty()) {
      participant_to_entity_guids_.erase(participant_to_entity_guid);
    }

    return true;
  }

  TopicsTypes get_topic_types_by_guid(const GUID_t & participant_guid)
  {
    TopicsTypes topics_types;
    const auto participant_to_entity_guids = participant_to_entity_guids_.find(participant_guid);
    if (participant_to_entity_guids == participant_to_entity_guids_.end()) {
      return topics_types;
    }

    for (auto & entity_guid : participant_to_entity_guids->second) {
      auto topic_info = entity_guid_to_info_.find(entity_guid);
      if (topic_info == entity_guid_to_info_.end()) {
        continue;
      }
      auto & topic_name = topic_info->second.name;
      topics_types[topic_name].insert(topic_info->second.type);
    }
    return topics_types;
  }

  void clear_cache(void)
  {
    entity_guid_to_info_.clear();
    participant_to_entity_guids_.clear();
  }

private:
  EntityGuidToInfo entity_guid_to_info_;
  ParticipantToEntityGuidMap participant_to_entity_guids_;

  void initialize_participant_map(ParticipantToEntityGuidMap & map, const GUID_t & participant_guid)
  {
    if (map.find(participant_guid) == map.end()) {
      map[participant_guid] = std::multiset<GUID_t>();
    }
  }
};

#endif  // RMW_GURUMDDS_SHARED_CPP__TOPIC_CACHE_HPP_

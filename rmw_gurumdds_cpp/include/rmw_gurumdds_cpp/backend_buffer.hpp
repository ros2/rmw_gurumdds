// Copyright 2026 GurumNetworks, Inc.
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

#ifndef GURUMDDS_RMW__BACKEND_BUFFER_HPP_
#define GURUMDDS_RMW__BACKEND_BUFFER_HPP_

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "rmw_gurumdds_cpp/identifier.hpp"

#include "rmw/types.h"
#include "rmw/impl/cpp/key_value.hpp"

#include "rosidl_buffer_backend/buffer_backend.hpp"
#include "rosidl_buffer_backend_registry/buffer_backend_registry.hpp"
#include "rosidl_typesupport_fastrtps_cpp/buffer_serialization.hpp"

namespace rmw_gurumdds_cpp
{

constexpr const char * LOGGERNAME = "rmw_gurumdds_cpp.buffer_backend_loader";
constexpr const char * BUFFER_BACKEND_KEY = "bufbe";

/// Per-rmw_context bundle of buffer backend state.
/// Stored as a void* in rmw_context_impl_s::buffer_serialization_context.
struct BufferBackendContext
{
  rosidl_typesupport_fastrtps_cpp::BufferSerializationContext serialization_context;
  std::unique_ptr<rosidl_buffer_backend_registry::BufferBackendRegistry> registry;
  std::unordered_map<std::string, std::shared_ptr<rosidl::BufferBackend>> backend_instances;
};


/// Information about a discovered buffer-aware endpoint.
struct BufferEndpointInfo
{
  rmw_gid_t gid{};
  std::string topic_name;
  /// Backend type -> backend metadata string (empty map means CPU-only).
  std::unordered_map<std::string, std::string> backend_metadata;
};

/// Callback invoked when a buffer-aware endpoint is discovered.
using BufferEndpointDiscoveryCallback = std::function<void (const BufferEndpointInfo &)>;

/// Per-context registry for buffer-aware endpoint discovery callbacks.
///
/// When a buffer-aware publisher is created, it registers a "subscriber discovered"
/// callback on the topic so it can create per-subscriber DataWriters.  Symmetrically,
/// buffer-aware subscribers register a "publisher discovered" callback.
///
/// The DDS ParticipantListener discovery path feeds newly discovered endpoints
/// into the owning context's registry so the appropriate callbacks fire.
class BufferEndpointRegistry
{
public:
  BufferEndpointRegistry() = default;

  /// Register a callback for when a buffer-aware subscriber is discovered on a topic.
  void register_subscriber_discovery_callback(
    const std::string & topic_name,
    const rmw_gid_t & publisher_gid,
    BufferEndpointDiscoveryCallback callback);

  /// Register a callback for when a buffer-aware publisher is discovered on a topic.
  void register_publisher_discovery_callback(
    const std::string & topic_name,
    const rmw_gid_t & subscriber_gid,
    BufferEndpointDiscoveryCallback callback);

  /// Unregister all discovery callbacks associated with a GID.
  void unregister_callbacks(const rmw_gid_t & gid);

  /// Notify that a buffer-aware subscriber has been discovered.
  void notify_subscriber_discovered(const BufferEndpointInfo & info);

  /// Notify that a buffer-aware publisher has been discovered.
  void notify_publisher_discovered(const BufferEndpointInfo & info);

private:
  struct CallbackEntry
  {
    rmw_gid_t registrant_gid{};
    BufferEndpointDiscoveryCallback callback;
  };

  std::mutex mutex_;

  /// topic_name -> list of (publisher_gid, callback) entries that want subscriber notifications.
  std::unordered_map<std::string, std::vector<CallbackEntry>> subscriber_callbacks_;
  /// topic_name -> list of (subscriber_gid, callback) entries that want publisher notifications.
  std::unordered_map<std::string, std::vector<CallbackEntry>> publisher_callbacks_;

  /// Already-discovered buffer-aware endpoints, used to retroactively fire callbacks.
  std::vector<BufferEndpointInfo> known_subscribers_;
  std::vector<BufferEndpointInfo> known_publishers_;
};

/// Load buffer backend plugins and register them with FastCDR serialization.
/// Populates an RMW-context-local serialization map so multiple contexts in the
/// same process do not share mutable global descriptor state.
void initialize_buffer_backends(BufferBackendContext & context);

/// Clear context-local serialization maps.
void shutdown_buffer_backends(BufferBackendContext & context);


inline std::string
gid_to_hex(const rmw_gid_t & gid, size_t bytes = RMW_GID_STORAGE_SIZE)
{
  static const char hex_chars[] = "0123456789abcdef";
  std::string result;
  result.reserve(bytes * 2);
  for (size_t i = 0; i < bytes && i < RMW_GID_STORAGE_SIZE; ++i) {
    result += hex_chars[(gid.data[i] >> 4) & 0xF];
    result += hex_chars[gid.data[i] & 0xF];
  }
  return result;
}

}  // namespace rmw_gurumdds_cpp

#endif // GURUMDDS_RMW__BACKEND_BUFFER_HPP_

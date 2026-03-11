#pragma once

#include <string>
#include <vector>

#include "core/common/peer_endpoint.h"

namespace dss {

class PeerClient {
public:
  static void putChunk(const PeerEndpoint& peer,
                       const std::string& chunkId,
                       const std::vector<char>& data);

  static std::vector<char> getChunk(const PeerEndpoint& peer,
                                    const std::string& chunkId);

  // Lightweight existence probe; returns true if peer reports it has the chunk.
  static bool hasChunk(const PeerEndpoint& peer,
                       const std::string& chunkId);
};

} // namespace dss

#pragma once

#include <string>
#include <vector>

#include "core/tracker/tracker_client.h"

namespace dss {

class PeerClient {
public:
  static void putChunk(const PeerEndpoint& peer,
                       const std::string& chunkId,
                       const std::vector<char>& data);

  static std::vector<char> getChunk(const PeerEndpoint& peer,
                                    const std::string& chunkId);
};

} // namespace dss

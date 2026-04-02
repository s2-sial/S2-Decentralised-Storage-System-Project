#pragma once

#include <string>
#include <vector>
#include <utility>

#include "core/common/peer_endpoint.h"

namespace dss {

class TrackerClient {
public:
    TrackerClient(std::string tracker_ip, int tracker_port);

    // raw request helper (sends exactly one line ending in '\n')
    std::string requestLine(const std::string& line) const;

    std::vector<PeerEndpoint> getPeers() const;
    std::vector<PeerEndpoint> whereChunk(const std::string& chunk_id) const;

    // best-effort announce (your old code ignored response)
    void announceChunk(const std::string& chunk_id,
                       const std::string& peer_ip,
                       int peer_port) const;

    // matches your NEED_REPAIR parsing: returns chunk_ids only
    std::vector<std::string> needRepair(int desired, int limit) const;

private:
    std::string ip_;
    int port_{};
};

} // namespace dss

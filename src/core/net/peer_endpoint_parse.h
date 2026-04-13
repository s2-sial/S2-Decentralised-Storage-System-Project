#pragma once

#include "core/common/peer_endpoint.h"

#include <string>

namespace dss::net {

// Accepts "host:port" or "tcp://host:port" (e.g. ngrok forwarding lines).
PeerEndpoint parse_peer_entry(std::string item);

}  // namespace dss::net

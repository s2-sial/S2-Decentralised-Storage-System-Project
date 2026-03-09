#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "core/common/peer_endpoint.h"

namespace dss::dht {

// 160‑bit ID, like classic Kademlia
using Id = std::array<std::uint8_t, 20>;

// Construct an Id from raw bytes (truncating or zero‑padding as needed).
Id idFromBytes(const std::vector<std::uint8_t>& bytes);

// Construct a node Id from a peer endpoint (e.g. hash of "ip:port").
Id makeNodeId(const PeerEndpoint& peer);

// Construct a key Id from an arbitrary string (e.g. chunk id).
Id makeKeyId(const std::string& key);

// Return the XOR distance between two Ids as a big integer‑like value
// suitable for ordering (higher bits are more significant).
// For small networks you can just compare lexicographically on this.
std::array<std::uint8_t, 20> xorDistance(const Id& a, const Id& b);

// Convenience: return whether a is "closer" to target than b using XOR distance.
bool closerTo(const Id& a, const Id& b, const Id& target);

// Hex helpers for logging / debugging.
std::string toHex(const Id& id);
Id fromHex(const std::string& hex);

}  // namespace dss::dht


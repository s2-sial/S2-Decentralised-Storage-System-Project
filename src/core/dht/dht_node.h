#pragma once

#include <string>
#include <vector>

#include "core/common/peer_endpoint.h"
#include "core/dht/kademlia_id.h"
#include "core/dht/kademlia_routing_table.h"

namespace dss::dht {

// Simplified Kademlia node that knows about a fixed set of peers.
// It uses XOR distance over node IDs to decide which peers are
// responsible for storing / serving a given key (chunk hash).
class DhtNode {
public:
  // peers: initial known peers in the DHT (each becomes a node).
  explicit DhtNode(const std::vector<PeerEndpoint>& peers);

  // Kademlia-style find_node: return up to k nodes closest to target.
  std::vector<NodeInfo> findNode(const Id& target, std::size_t k) const;

  // store(key): return up to `replicas` peers that should store the key.
  std::vector<PeerEndpoint> store(const std::string& key, int replicas) const;

  // find_value(key): return candidate peers likely to have this key.
  // For now this mirrors `store` (deterministic placement).
  std::vector<PeerEndpoint> findValue(const std::string& key,
                                      int replicas) const;

private:
  Id selfId_;
  RoutingTable table_;
};

}  // namespace dss::dht


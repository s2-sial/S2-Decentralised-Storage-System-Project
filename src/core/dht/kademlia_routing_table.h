#pragma once

#include <cstddef>
#include <vector>

#include "core/common/peer_endpoint.h"
#include "core/dht/kademlia_id.h"

namespace dss::dht {

struct NodeInfo {
  Id id;
  PeerEndpoint endpoint;
};

// Minimal routing table for small networks.
// Internally just keeps a flat list of known nodes and can
// return the K closest nodes to a given Id using XOR distance.
class RoutingTable {
public:
  explicit RoutingTable(Id selfId);

  const Id& selfId() const { return selfId_; }

  // Add or update a node. No self‑insertion.
  void addNode(const NodeInfo& node);

  // Return up to k nodes closest to target (excluding self).
  std::vector<NodeInfo> closestNodes(const Id& target, std::size_t k) const;

  // Inspect all known nodes (for debugging / tests).
  const std::vector<NodeInfo>& allNodes() const { return nodes_; }

private:
  Id selfId_;
  std::vector<NodeInfo> nodes_;
};

}  // namespace dss::dht


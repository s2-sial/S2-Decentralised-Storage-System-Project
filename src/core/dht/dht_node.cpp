#include "core/dht/dht_node.h"

namespace dss::dht {

DhtNode::DhtNode(const std::vector<PeerEndpoint>& peers)
    : selfId_(makeKeyId("client")),  // arbitrary stable id for this node
      table_(selfId_) {
  for (const auto& p : peers) {
    NodeInfo n;
    n.id = makeNodeId(p);
    n.endpoint = p;
    table_.addNode(n);
  }
}

std::vector<NodeInfo> DhtNode::findNode(const Id& target, std::size_t k) const {
  const auto& all = table_.allNodes();
  if (all.empty() || k == 0) return {};
  if (k > all.size()) k = all.size();
  return table_.closestNodes(target, k);
}

std::vector<PeerEndpoint> DhtNode::store(const std::string& key,
                                         int replicas) const {
  if (replicas <= 0) replicas = 1;
  const auto& all = table_.allNodes();
  if (all.empty()) return {};
  std::size_t k = static_cast<std::size_t>(replicas);
  if (k > all.size()) k = all.size();

  Id keyId = makeKeyId(key);
  auto nodes = table_.closestNodes(keyId, k);

  std::vector<PeerEndpoint> out;
  out.reserve(nodes.size());
  for (const auto& n : nodes) {
    out.push_back(n.endpoint);
  }
  return out;
}

std::vector<PeerEndpoint> DhtNode::findValue(const std::string& key,
                                             int replicas) const {
  // In a full Kademlia implementation this would perform iterative
  // FIND_VALUE RPCs. For now we use deterministic placement, so the
  // nodes responsible for the key are the same as returned by store().
  return store(key, replicas);
}

}  // namespace dss::dht


#include "core/dht/kademlia_routing_table.h"

#include <algorithm>

namespace dss::dht {

RoutingTable::RoutingTable(Id selfId) : selfId_(selfId) {}

void RoutingTable::addNode(const NodeInfo& node) {
  // Do not add self.
  if (node.id == selfId_) return;

  // Replace existing entry if ID matches.
  for (auto& n : nodes_) {
    if (n.id == node.id) {
      n.endpoint = node.endpoint;
      return;
    }
  }
  nodes_.push_back(node);
}

std::vector<NodeInfo> RoutingTable::closestNodes(const Id& target,
                                                 std::size_t k) const {
  if (nodes_.empty() || k == 0) return {};

  std::vector<NodeInfo> tmp = nodes_;
  std::sort(tmp.begin(), tmp.end(),
            [&](const NodeInfo& a, const NodeInfo& b) {
              return closerTo(a.id, b.id, target);
            });

  if (tmp.size() > k) {
    tmp.resize(k);
  }
  return tmp;
}

}  // namespace dss::dht


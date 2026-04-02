#pragma once

#include <string>

namespace dss {

struct PeerEndpoint {
  std::string ip;
  int port{};
};

}  // namespace dss


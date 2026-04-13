#include "core/net/peer_endpoint_parse.h"

#include <cctype>
#include <stdexcept>

namespace dss::net {

namespace {

std::string trim_ws(std::string s) {
  const char* ws = " \t\r\n";
  const auto b = s.find_first_not_of(ws);
  if (b == std::string::npos) {
    return {};
  }
  const auto e = s.find_last_not_of(ws);
  return s.substr(b, e - b + 1);
}

}  // namespace

PeerEndpoint parse_peer_entry(std::string item) {
  item = trim_ws(std::move(item));
  if (item.size() >= 6) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(item.c_str());
    if (std::tolower(p[0]) == 't' && std::tolower(p[1]) == 'c' && std::tolower(p[2]) == 'p' &&
        p[3] == ':' && p[4] == '/' && p[5] == '/') {
      item = trim_ws(item.substr(6));
    }
  }
  const std::size_t colon = item.rfind(':');
  if (colon == std::string::npos || colon == 0) {
    throw std::runtime_error(
        "Invalid peer entry (expected host:port or tcp://host:port): " + item);
  }
  std::string host = trim_ws(item.substr(0, colon));
  std::string portStr = trim_ws(item.substr(colon + 1));
  if (host.empty() || portStr.empty()) {
    throw std::runtime_error("Invalid peer entry (empty host or port): " + item);
  }
  PeerEndpoint ep;
  ep.ip = std::move(host);
  try {
    ep.port = std::stoi(portStr);
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid peer entry (bad port): " + portStr);
  }
  return ep;
}

}  // namespace dss::net

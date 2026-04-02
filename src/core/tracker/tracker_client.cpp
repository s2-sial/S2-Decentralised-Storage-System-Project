#include "core/tracker/tracker_client.h"

#include "core/net/tcp.h"

#include <sstream>
#include <unistd.h>

namespace dss {

TrackerClient::TrackerClient(std::string tracker_ip, int tracker_port)
    : ip_(std::move(tracker_ip)), port_(tracker_port) {}

std::string TrackerClient::requestLine(const std::string& line) const {
  using namespace dss::net;

  int sock = connect_tcp_fatal_or_throw(ip_, port_);
  try {
    std::string toSend = line;
    if (toSend.empty() || toSend.back() != '\n') {
      toSend.push_back('\n');
    }
    send_all_or_throw(sock, toSend);
    std::string resp = recv_all_text(sock);
    ::close(sock);
    return resp;
  } catch (...) {
    ::close(sock);
    throw;
  }
}

std::vector<PeerEndpoint> TrackerClient::getPeers() const {
  std::vector<PeerEndpoint> out;
  std::string resp = requestLine("GET_PEERS\n");

  std::istringstream iss(resp);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.empty()) continue;
    std::istringstream l(line);
    std::string ipPort;
    l >> ipPort;
    if (ipPort.empty()) continue;

    auto pos = ipPort.find(':');
    if (pos == std::string::npos) continue;
    PeerEndpoint ep;
    ep.ip = ipPort.substr(0, pos);
    ep.port = std::stoi(ipPort.substr(pos + 1));
    out.push_back(std::move(ep));
  }
  return out;
}

std::vector<PeerEndpoint> TrackerClient::whereChunk(const std::string& chunk_id) const {
  std::vector<PeerEndpoint> out;
  std::string line = "WHERE " + chunk_id + "\n";
  std::string resp = requestLine(line);

  std::istringstream iss(resp);
  std::string l;
  while (std::getline(iss, l)) {
    if (l.empty()) continue;
    auto pos = l.find(':');
    if (pos == std::string::npos) continue;
    PeerEndpoint ep;
    ep.ip = l.substr(0, pos);
    ep.port = std::stoi(l.substr(pos + 1));
    out.push_back(std::move(ep));
  }
  return out;
}

void TrackerClient::announceChunk(const std::string& chunk_id,
                                  const std::string& peer_ip,
                                  int peer_port) const {
  std::ostringstream oss;
  oss << "ANNOUNCE " << chunk_id << " " << peer_ip << " " << peer_port << "\n";
  try {
    (void)requestLine(oss.str());
  } catch (...) {
  }
}

std::vector<std::string> TrackerClient::needRepair(int desired, int limit) const {
  std::ostringstream oss;
  oss << "NEED_REPAIR " << desired << " " << limit << "\n";
  std::string resp = requestLine(oss.str());

  std::vector<std::string> out;
  std::istringstream iss(resp);
  std::string cid;
  int count = 0;
  while (iss >> cid >> count) {
    if (!cid.empty()) out.push_back(cid);
  }
  return out;
}

} // namespace dss


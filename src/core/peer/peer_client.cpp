#include "core/peer/peer_client.h"

#include "core/net/tcp.h"

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>

namespace dss {

void PeerClient::putChunk(const PeerEndpoint& peer,
                          const std::string& chunkId,
                          const std::vector<char>& data) {
  using namespace dss::net;

  int sock = connect_tcp_fatal_or_throw(peer.ip, peer.port);
  try {
    std::string header =
        "PUT_CHUNK " + chunkId + " " + std::to_string(data.size()) + "\n";
    send_all_or_throw(sock, header);
    if (!data.empty()) {
      send_all_or_throw(sock, data.data(), data.size());
    }

    std::string resp = recv_all_text(sock);
    ::close(sock);

    if (resp.rfind("OK", 0) != 0) {
      throw std::runtime_error("Peer PUT_CHUNK failed for " + chunkId);
    }
  } catch (...) {
    ::close(sock);
    throw;
  }
}

std::vector<char> PeerClient::getChunk(const PeerEndpoint& peer,
                                       const std::string& chunkId) {
  using namespace dss::net;

  int sock = connect_tcp_fatal_or_throw(peer.ip, peer.port);

  try {
    std::string req = "GET_CHUNK " + chunkId + "\n";
    send_all_or_throw(sock, req);

    std::string header;
    if (!recv_line(sock, header)) {
      ::close(sock);
      throw std::runtime_error("Failed to read GET_CHUNK header");
    }

    std::istringstream iss(header);
    std::string status;
    size_t size = 0;
    iss >> status >> size;
    if (status != "OK") {
      ::close(sock);
      throw std::runtime_error("Peer returned non-OK for GET_CHUNK");
    }

    std::vector<char> out(size);
    size_t received = 0;
    while (received < size) {
      ssize_t n = ::recv(sock, out.data() + received, size - received, 0);
      if (n <= 0) break;
      received += static_cast<size_t>(n);
    }
    ::close(sock);

    if (received != size) {
      throw std::runtime_error("Incomplete GET_CHUNK payload");
    }
    return out;
  } catch (...) {
    ::close(sock);
    throw;
  }
}

bool PeerClient::hasChunk(const PeerEndpoint& peer,
                          const std::string& chunkId) {
  using namespace dss::net;

  int sock = connect_tcp_fatal_or_throw(peer.ip, peer.port);
  try {
    std::string req = "HAS_CHUNK " + chunkId + "\n";
    send_all_or_throw(sock, req);

    std::string line;
    if (!recv_line(sock, line)) {
      ::close(sock);
      return false;
    }
    ::close(sock);
    return line.rfind("OK", 0) == 0;
  } catch (...) {
    ::close(sock);
    return false;
  }
}

void PeerClient::putManifest(const PeerEndpoint& peer,
                             const std::string& manifestId,
                             const std::string& manifestText) {
  using namespace dss::net;
  int sock = connect_tcp_fatal_or_throw(peer.ip, peer.port);
  try {
    std::string header =
        "PUT_MANIFEST " + manifestId + " " + std::to_string(manifestText.size()) + "\n";
    send_all_or_throw(sock, header);
    if (!manifestText.empty()) {
      send_all_or_throw(sock, manifestText);
    }
    std::string resp = recv_all_text(sock);
    ::close(sock);
    if (resp.rfind("OK", 0) != 0) {
      throw std::runtime_error("Peer PUT_MANIFEST failed for " + manifestId);
    }
  } catch (...) {
    ::close(sock);
    throw;
  }
}

std::string PeerClient::getManifest(const PeerEndpoint& peer,
                                    const std::string& manifestId) {
  using namespace dss::net;
  int sock = connect_tcp_fatal_or_throw(peer.ip, peer.port);
  try {
    std::string req = "GET_MANIFEST " + manifestId + "\n";
    send_all_or_throw(sock, req);

    std::string header;
    if (!recv_line(sock, header)) {
      ::close(sock);
      throw std::runtime_error("Failed to read GET_MANIFEST header");
    }
    std::istringstream iss(header);
    std::string status;
    size_t size = 0;
    iss >> status >> size;
    if (status != "OK") {
      ::close(sock);
      throw std::runtime_error("Peer returned non-OK for GET_MANIFEST");
    }
    std::string out;
    out.resize(size);
    size_t received = 0;
    while (received < size) {
      ssize_t n = ::recv(sock, out.data() + received, size - received, 0);
      if (n <= 0) break;
      received += static_cast<size_t>(n);
    }
    ::close(sock);
    if (received != size) {
      throw std::runtime_error("Incomplete GET_MANIFEST payload");
    }
    return out;
  } catch (...) {
    ::close(sock);
    throw;
  }
}

bool PeerClient::hasManifest(const PeerEndpoint& peer,
                             const std::string& manifestId) {
  using namespace dss::net;
  int sock = connect_tcp_fatal_or_throw(peer.ip, peer.port);
  try {
    std::string req = "HAS_MANIFEST " + manifestId + "\n";
    send_all_or_throw(sock, req);
    std::string line;
    if (!recv_line(sock, line)) {
      ::close(sock);
      return false;
    }
    ::close(sock);
    return line.rfind("OK", 0) == 0;
  } catch (...) {
    ::close(sock);
    return false;
  }
}

} // namespace dss


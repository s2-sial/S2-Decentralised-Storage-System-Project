#include "core/peer/peer_service.h"

#include "core/net/tcp.h"
#include "core/storage/storage_manager.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace dss::peer {

PeerService::PeerService(std::string advertiseIp,
                         int peerPort,
                         std::string storageDir,
                         std::uint64_t maxBytes)
    : advertiseIp_(std::move(advertiseIp)),
      peerPort_(peerPort),
      storageDir_(std::move(storageDir)),
      maxBytes_(maxBytes) {}

void PeerService::runBlocking() {
  running_.store(true);
  runLoop();
}

void PeerService::start() {
  if (running_.load()) return;
  running_.store(true);
  worker_ = std::thread([this]() {
    try {
      runLoop();
    } catch (...) {
      running_.store(false);
    }
  });
}

void PeerService::stop() {
  if (!running_.load()) return;
  running_.store(false);
  if (listenFd_ >= 0) {
    ::shutdown(listenFd_, SHUT_RDWR);
    ::close(listenFd_);
    listenFd_ = -1;
  }
  if (worker_.joinable()) {
    worker_.join();
  }
}

void PeerService::runLoop() {
  (void)advertiseIp_;  // reserved for future DHT announce use

  dss::storage::StorageManager storage(storageDir_, maxBytes_);
  storage.init();

  std::cout << "Peer listening on port " << peerPort_ << "\n";

  int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    running_.store(false);
    throw std::runtime_error("PeerService socket() failed");
  }
  listenFd_ = listen_fd;

  int yes = 1;
  ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(static_cast<uint16_t>(peerPort_));

  if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    ::close(listen_fd);
    listenFd_ = -1;
    running_.store(false);
    throw std::runtime_error("PeerService bind() failed");
  }
  if (::listen(listen_fd, 16) < 0) {
    ::close(listen_fd);
    listenFd_ = -1;
    running_.store(false);
    throw std::runtime_error("PeerService listen() failed");
  }

  while (running_.load()) {
    int client = ::accept(listen_fd, nullptr, nullptr);
    if (client < 0) {
      if (!running_.load()) break;
      continue;
    }

    dss::net::set_timeouts(client, 10000, 10000);

    std::string line;
    if (!dss::net::recv_line(client, line)) {
      ::close(client);
      continue;
    }

    std::istringstream iss(line);
    std::string cmd, hash;
    size_t size = 0;
    iss >> cmd;

    if (cmd == "PUT_CHUNK") {
      iss >> hash >> size;
      std::vector<char> data(size);
      size_t remaining = size;
      size_t offset = 0;
      while (remaining > 0) {
        const size_t chunk = std::min<std::size_t>(remaining, 4096);
        ssize_t n = ::recv(client, data.data() + offset, chunk, 0);
        if (n <= 0) break;
        offset += static_cast<size_t>(n);
        remaining -= static_cast<size_t>(n);
      }
      if (remaining == 0) {
        try {
          storage.storeChunk(hash, data);
          dss::net::send_all_nothrow(client, "OK\n");
        } catch (const std::exception& e) {
          std::cerr << "PUT_CHUNK failed for " << hash << ": " << e.what() << "\n";
          dss::net::send_all_nothrow(client, "ERR\n");
        }
      } else {
        dss::net::send_all_nothrow(client, "ERR\n");
      }
    } else if (cmd == "GET_CHUNK") {
      iss >> hash;
      try {
        auto data = storage.loadChunk(hash);
        dss::net::send_all_nothrow(client, "OK " + std::to_string(data.size()) + "\n");
        if (!data.empty()) {
          dss::net::send_all_nothrow(
              client, std::string(data.data(), static_cast<std::streamsize>(data.size())));
        }
      } catch (...) {
        dss::net::send_all_nothrow(client, "ERR\n");
      }
    } else if (cmd == "HAS_CHUNK") {
      iss >> hash;
      dss::net::send_all_nothrow(client, storage.hasChunk(hash) ? "OK\n" : "ERR\n");
    } else if (cmd == "PUT_MANIFEST") {
      std::string manifestId;
      iss >> manifestId >> size;
      std::vector<char> data(size);
      size_t remaining = size;
      size_t offset = 0;
      while (remaining > 0) {
        const size_t chunk = std::min<std::size_t>(remaining, 4096);
        ssize_t n = ::recv(client, data.data() + offset, chunk, 0);
        if (n <= 0) break;
        offset += static_cast<size_t>(n);
        remaining -= static_cast<size_t>(n);
      }
      if (remaining == 0) {
        std::string path = storageDir_ + "/manifests/" + manifestId;
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (out) {
          out.write(data.data(), static_cast<std::streamsize>(data.size()));
          dss::net::send_all_nothrow(client, "OK\n");
        } else {
          dss::net::send_all_nothrow(client, "ERR\n");
        }
      } else {
        dss::net::send_all_nothrow(client, "ERR\n");
      }
    } else if (cmd == "GET_MANIFEST") {
      std::string manifestId;
      iss >> manifestId;
      std::string path = storageDir_ + "/manifests/" + manifestId;
      std::ifstream in(path, std::ios::binary);
      if (!in) {
        dss::net::send_all_nothrow(client, "ERR\n");
      } else {
        in.seekg(0, std::ios::end);
        size_t msize = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);
        std::string text(msize, '\0');
        if (msize > 0) in.read(text.data(), static_cast<std::streamsize>(msize));
        dss::net::send_all_nothrow(client, "OK " + std::to_string(msize) + "\n");
        if (msize > 0) dss::net::send_all_nothrow(client, text);
      }
    } else if (cmd == "HAS_MANIFEST") {
      std::string manifestId;
      iss >> manifestId;
      std::string path = storageDir_ + "/manifests/" + manifestId;
      std::ifstream in(path, std::ios::binary);
      dss::net::send_all_nothrow(client, in ? "OK\n" : "ERR\n");
    }

    ::close(client);
  }

  if (listenFd_ >= 0) {
    ::close(listenFd_);
    listenFd_ = -1;
  }
  running_.store(false);
}

}  // namespace dss::peer


#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace dss::peer {

class PeerService {
public:
  PeerService(std::string advertiseIp,
              int peerPort,
              std::string storageDir,
              std::uint64_t maxBytes);

  // Starts the peer server loop and blocks forever (until process exits).
  void runBlocking();

  // Start peer loop in background thread.
  void start();
  void stop();
  bool isRunning() const { return running_.load(); }

private:
  void runLoop();

  std::string advertiseIp_;
  int peerPort_{};
  std::string storageDir_;
  std::uint64_t maxBytes_{};
  std::atomic<bool> running_{false};
  int listenFd_{-1};
  std::thread worker_;
};

}  // namespace dss::peer


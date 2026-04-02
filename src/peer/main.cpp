#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#include "core/peer/peer_service.h"

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: peer <advertise_ip> <peer_port> <storage_dir> [max_bytes]\n";
    return 1;
  }

  std::string advertiseIp = argv[1];
  int peerPort = std::stoi(argv[2]);
  std::string storageDir = argv[3];
  std::uint64_t maxBytes = 10ull * 1024 * 1024 * 1024;  // 10 GiB default
  if (argc >= 5) {
    maxBytes = std::stoull(argv[4]);
  }

  std::cout << "Storage dir: " << std::filesystem::absolute(storageDir) << "\n";
  dss::peer::PeerService service(advertiseIp, peerPort, storageDir, maxBytes);
  service.runBlocking();
  return 0;
}
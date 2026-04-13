#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/dss/dss_client.h"
#include "core/net/peer_endpoint_parse.h"

static void usage(const char* prog) {
  std::cerr
      << "Usage:\n"
      << "  " << prog
      << " put <peers> <file_path> [chunk_size_bytes] [replicas] [rsa_public_key_pem]\n"
      << "  " << prog
      << " get <peers> <share_id_or_manifest_path> <output_file> [rsa_private_key_pem]\n"
      << "  " << prog
      << " repair (not supported without tracker)\n"
      << "\n"
      << "Environment:\n"
      << "  DECENT_STORE_MANIFEST_DIR  Directory for local *.manifest.txt after put (default: ./manifests).\n";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  std::string mode = argv[1];

  try {
    if (mode == "put") {
      if (argc < 4) {
        usage(argv[0]);
        return 1;
      }

      dss::ClientConfig cfg;
      // peers passed as comma-separated list: host:port or tcp://host:port,...
      std::string peersArg = argv[2];
      std::stringstream ss(peersArg);
      std::string item;
      while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        cfg.peers.push_back(dss::net::parse_peer_entry(std::move(item)));
      }

      if (cfg.peers.empty()) {
        throw std::runtime_error("No valid peers provided");
      }

      // put <peers> <file_path> [chunk_size] [replicas] [rsa_public.pem]
      // argv[3] = file_path, argv[4] = chunk_size, argv[5] = replicas, argv[6] = rsa
      cfg.chunkSize = (argc >= 5)
                          ? static_cast<size_t>(std::stoul(argv[4]))
                          : 1024 * 1024;
      cfg.desiredReplicas = (argc >= 6) ? std::stoi(argv[5]) : 2;
      if (argc >= 7) {
        cfg.rsaPublicKeyPath = argv[6];
      }
      if (const char* md = std::getenv("DECENT_STORE_MANIFEST_DIR")) {
        if (md[0] != '\0') {
          cfg.localManifestDir = md;
        }
      }

      dss::DssClient client(cfg);
      dss::PutFileResult result =
          client.putFile(argv[3], [](dss::Progress p) {
            std::cout << "[PUT] " << p.done << "/" << p.total << " "
                      << p.message << "\n";
          });
      std::cout << "DONE. Share ID: " << result.shareUri << "\n";
      std::cout << "Local manifest: " << result.localManifestPath << "\n";
      return 0;
    }

    if (mode == "get") {
      if (argc < 5) {
        usage(argv[0]);
        return 1;
      }

      dss::ClientConfig cfg;
      std::string peersArg = argv[2];
      std::stringstream ss(peersArg);
      std::string item;
      while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        cfg.peers.push_back(dss::net::parse_peer_entry(std::move(item)));
      }
      if (cfg.peers.empty()) {
        throw std::runtime_error("No valid peers provided");
      }

      cfg.chunkSize = 1024 * 1024;
      cfg.desiredReplicas = 2;
      if (argc >= 6) {
        cfg.rsaPrivateKeyPath = argv[5];
      }

      dss::DssClient client(cfg);
      client.getFile(argv[3], argv[4], [](dss::Progress p) {
        std::cout << "[GET] " << p.done << "/" << p.total << " " << p.message
                  << "\n";
      });
      std::cout << "Download complete: " << argv[4] << "\n";
      return 0;
    }

    if (mode == "repair") {
      std::cerr << "Repair is not supported in DHT mode (no tracker).\n";
      return 0;
    }

    usage(argv[0]);
    return 1;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}


#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>

#include "core/dss/dss_client.h"

static void usage(const char* prog) {
  std::cerr
      << "Usage:\n"
      << "  " << prog
      << " put <peers> <file_path> [chunk_size_bytes] [replicas]\n"
      << "  " << prog
      << " get <peers> <manifest_path> <output_file>\n"
      << "  " << prog
      << " repair (not supported without tracker)\n";
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
      // peers passed as comma-separated list: ip:port,ip:port,...
      std::string peersArg = argv[2];
      std::stringstream ss(peersArg);
      std::string item;
      while (std::getline(ss, item, ',')) {
        // trim whitespace around each entry
        auto trim = [](std::string s) {
          const char* ws = " \t\r\n";
          auto b = s.find_first_not_of(ws);
          if (b == std::string::npos) return std::string();
          auto e = s.find_last_not_of(ws);
          return s.substr(b, e - b + 1);
        };
        item = trim(item);
        if (item.empty()) continue;
        auto pos = item.find(':');
        if (pos == std::string::npos) {
          throw std::runtime_error("Invalid peer entry (expected ip:port): " + item);
        }
        dss::PeerEndpoint ep;
        std::string ipPart = trim(item.substr(0, pos));
        std::string portPart = trim(item.substr(pos + 1));
        if (ipPart.empty() || portPart.empty()) {
          throw std::runtime_error("Invalid peer entry (empty ip or port): " + item);
        }
        ep.ip = ipPart;
        ep.port = std::stoi(portPart);
        cfg.peers.push_back(ep);
      }

      if (cfg.peers.empty()) {
        throw std::runtime_error("No valid peers provided");
      }

      cfg.chunkSize = (argc >= 6)
                          ? static_cast<size_t>(std::stoul(argv[5]))
                          : 1024 * 1024;
      cfg.desiredReplicas = (argc >= 7) ? std::stoi(argv[6]) : 2;

      dss::DssClient client(cfg);
      std::string manifestPath =
          client.putFile(argv[4], [](dss::Progress p) {
            std::cout << "[PUT] " << p.done << "/" << p.total << " "
                      << p.message << "\n";
          });
      std::cout << "DONE. Manifest written to " << manifestPath << "\n";
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
        auto trim = [](std::string s) {
          const char* ws = " \t\r\n";
          auto b = s.find_first_not_of(ws);
          if (b == std::string::npos) return std::string();
          auto e = s.find_last_not_of(ws);
          return s.substr(b, e - b + 1);
        };
        item = trim(item);
        if (item.empty()) continue;
        auto pos = item.find(':');
        if (pos == std::string::npos) {
          throw std::runtime_error("Invalid peer entry (expected ip:port): " + item);
        }
        dss::PeerEndpoint ep;
        std::string ipPart = trim(item.substr(0, pos));
        std::string portPart = trim(item.substr(pos + 1));
        if (ipPart.empty() || portPart.empty()) {
          throw std::runtime_error("Invalid peer entry (empty ip or port): " + item);
        }
        ep.ip = ipPart;
        ep.port = std::stoi(portPart);
        cfg.peers.push_back(ep);
      }
      if (cfg.peers.empty()) {
        throw std::runtime_error("No valid peers provided");
      }

      cfg.chunkSize = 1024 * 1024;
      cfg.desiredReplicas = 2;

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


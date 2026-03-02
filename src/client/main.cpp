#include <iostream>
#include <string>
#include <stdexcept>

#include "core/dss/dss_client.h"

static void usage(const char* prog) {
  std::cerr
      << "Usage:\n"
      << "  " << prog
      << " put <tracker_ip> <tracker_port> <file_path> [chunk_size_bytes] [replicas]\n"
      << "  " << prog
      << " get <tracker_ip> <tracker_port> <manifest_path> <output_file>\n"
      << "  " << prog
      << " repair <tracker_ip> <tracker_port> [desired_replicas] [batch]\n";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  std::string mode = argv[1];

  try {
    if (mode == "put") {
      if (argc < 5) {
        usage(argv[0]);
        return 1;
      }

      dss::ClientConfig cfg;
      cfg.trackerIp = argv[2];
      cfg.trackerPort = std::stoi(argv[3]);
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
      if (argc < 6) {
        usage(argv[0]);
        return 1;
      }

      dss::ClientConfig cfg;
      cfg.trackerIp = argv[2];
      cfg.trackerPort = std::stoi(argv[3]);
      cfg.chunkSize = 1024 * 1024;
      cfg.desiredReplicas = 2;

      dss::DssClient client(cfg);
      client.getFile(argv[4], argv[5], [](dss::Progress p) {
        std::cout << "[GET] " << p.done << "/" << p.total << " " << p.message
                  << "\n";
      });
      std::cout << "Download complete: " << argv[5] << "\n";
      return 0;
    }

    if (mode == "repair") {
      if (argc < 4) {
        usage(argv[0]);
        return 1;
      }

      dss::ClientConfig cfg;
      cfg.trackerIp = argv[2];
      cfg.trackerPort = std::stoi(argv[3]);
      cfg.chunkSize = 1024 * 1024;
      cfg.desiredReplicas = (argc >= 5) ? std::stoi(argv[4]) : 2;

      int batch = (argc >= 6) ? std::stoi(argv[5]) : 50;

      dss::DssClient client(cfg);
      client.repair(batch, [](dss::Progress p) {
        std::cout << "[REPAIR] " << p.done << "/" << p.total << " "
                  << p.message << "\n";
      });
      std::cout << "Repair finished.\n";
      return 0;
    }

    usage(argv[0]);
    return 1;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}


#pragma once
#include <string>
#include <vector>

namespace dss {
  struct ManifestEntry { std::string chunkId; /* maybe size too */ };

  struct Manifest {
    std::string originalName;
    size_t originalSize{};
    size_t chunkSize{};
    std::vector<ManifestEntry> chunks;
  };

  void writeManifest(const std::string& path, const Manifest& m);
  Manifest readManifest(const std::string& path);
}

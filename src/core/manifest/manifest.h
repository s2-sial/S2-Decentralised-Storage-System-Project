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

    // Optional encryption metadata for hybrid encryption.
    bool encrypted{false};
    std::string encAlgo;        // e.g. "AES-256-GCM"
    std::string keyEncAlgo;     // e.g. "RSA"
    std::string encryptedKeyHex;  // hex(RSA(key||iv))
  };

  std::string serializeManifest(const Manifest& m);
  Manifest parseManifestText(const std::string& text);

  void writeManifest(const std::string& path, const Manifest& m);
  Manifest readManifest(const std::string& path);
}

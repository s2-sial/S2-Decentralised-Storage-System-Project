#include "core/dss/dss_client.h"

#include "core/chunk/chunker.h"
#include "core/crypto/sha256.h"
#include "core/manifest/manifest.h"
#include "core/peer/peer_client.h"
#include "core/dht/dht_node.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace dss {

DssClient::DssClient(ClientConfig cfg) : cfg_(std::move(cfg)) {}

std::string DssClient::putFile(const std::string& filePath,
                               std::function<void(Progress)> onProgress) {
  if (cfg_.peers.empty()) {
    throw std::runtime_error("No peers configured for DHT");
  }

  dss::dht::DhtNode dht(cfg_.peers);

  int replicas = cfg_.desiredReplicas;
  if (replicas <= 0) replicas = 1;
  if (replicas > static_cast<int>(cfg_.peers.size())) {
    replicas = static_cast<int>(cfg_.peers.size());
  }

  auto fsPath = std::filesystem::path(filePath);
  auto fileSize = std::filesystem::file_size(fsPath);

  Manifest manifest;
  manifest.originalName = fsPath.filename().string();
  manifest.originalSize = fileSize;
  manifest.chunkSize = cfg_.chunkSize;

  auto chunks = chunkFile(filePath, cfg_.chunkSize);
  int total = static_cast<int>(chunks.size());
  int done = 0;

  for (auto& c : chunks) {
    std::string cid = dss::crypto::sha256_hex(c.bytes);
    c.id = cid;
    manifest.chunks.push_back(ManifestEntry{cid});

    // Choose responsible peers for this chunk via DHT mapping.
    auto targets = dht.store(cid, replicas);
    for (const auto& peer : targets) {
      PeerClient::putChunk(peer, cid, c.bytes);
    }

    ++done;
    if (onProgress) {
      onProgress(Progress{done, total, "Uploading chunks"});
    }
  }

  std::string manifestPath = manifest.originalName + ".manifest.txt";
  writeManifest(manifestPath, manifest);
  return manifestPath;
}

void DssClient::getFile(const std::string& manifestHash,
                        const std::string& outPath) {
  // For now treat manifestHash as a manifest path. Later, when manifests
  // are themselves addressed by hash in the DHT, this can:
  //  1) resolve manifestHash to a manifest blob via DHT
  //  2) parse that blob into a Manifest
  //  3) reuse the chunk download logic below.
  getFile(manifestHash, outPath, {});
}

void DssClient::getFile(const std::string& manifestPath,
                        const std::string& outPath,
                        std::function<void(Progress)> onProgress) {
  if (cfg_.peers.empty()) {
    throw std::runtime_error("No peers configured for DHT");
  }

  dss::dht::DhtNode dht(cfg_.peers);

  Manifest manifest = readManifest(manifestPath);

  std::ofstream out(outPath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Cannot open output file: " + outPath);
  }

  int total = static_cast<int>(manifest.chunks.size());
  int done = 0;

  for (const auto& entry : manifest.chunks) {
    // Ask DHT which peers are responsible for this chunk hash,
    // then fall back to all peers if needed.
    int replicas = cfg_.desiredReplicas <= 0 ? 1 : cfg_.desiredReplicas;
    auto candidates = dht.findValue(entry.chunkId, replicas);
    // Ensure we eventually try every peer if DHT mapping fails.
    for (const auto& p : cfg_.peers) {
      bool already = false;
      for (const auto& c : candidates) {
        if (c.ip == p.ip && c.port == p.port) {
          already = true;
          break;
        }
      }
      if (!already) candidates.push_back(p);
    }

    bool found = false;
    for (const auto& peer : candidates) {
      try {
        auto data = PeerClient::getChunk(peer, entry.chunkId);
        if (dss::crypto::sha256_hex(data) != entry.chunkId) {
          continue;
        }
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        found = true;
        break;
      } catch (...) {
      }
    }

    if (!found) {
      throw std::runtime_error("Failed to retrieve chunk " + entry.chunkId);
    }

    ++done;
    if (onProgress) {
      onProgress(Progress{done, total, "Downloading chunks"});
    }
  }
}

void DssClient::repair(int batch, std::function<void(Progress)> onProgress) {
  (void)batch;
  (void)onProgress;
  throw std::runtime_error("Repair is not supported without a tracker/DHT index yet");
}

} // namespace dss


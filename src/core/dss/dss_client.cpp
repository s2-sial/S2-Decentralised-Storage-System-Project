#include "core/dss/dss_client.h"

#include "core/chunk/chunker.h"
#include "core/crypto/sha256.h"
#include "core/manifest/manifest.h"
#include "core/peer/peer_client.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace dss {

DssClient::DssClient(ClientConfig cfg) : cfg_(std::move(cfg)) {}

std::string DssClient::putFile(const std::string& filePath,
                               std::function<void(Progress)> onProgress) {
  if (cfg_.peers.empty()) {
    throw std::runtime_error("No peers configured for DHT");
  }

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

    // simple DHT-style placement: hash(chunk_id + replica_index) -> peer index
    std::unordered_set<size_t> used;
    for (int r = 0; r < replicas; ++r) {
      const std::string salt = cid + "#" + std::to_string(r);
      const std::string h = dss::crypto::sha256_hex(
          std::vector<char>(salt.begin(), salt.end()));

      std::uint64_t value = 0;
      const size_t hexLen = std::min<std::size_t>(16, h.size());
      for (size_t i = 0; i < hexLen; ++i) {
        char ch = h[i];
        std::uint64_t v = 0;
        if (ch >= '0' && ch <= '9') v = static_cast<std::uint64_t>(ch - '0');
        else if (ch >= 'a' && ch <= 'f') v = static_cast<std::uint64_t>(10 + ch - 'a');
        else if (ch >= 'A' && ch <= 'F') v = static_cast<std::uint64_t>(10 + ch - 'A');
        value = (value << 4) | v;
      }

      size_t idx = static_cast<size_t>(value % cfg_.peers.size());
      size_t guard = 0;
      while (used.count(idx) && guard < cfg_.peers.size()) {
        idx = (idx + 1) % cfg_.peers.size();
        ++guard;
      }
      used.insert(idx);
      const auto& peer = cfg_.peers[idx];

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

void DssClient::getFile(const std::string& manifestPath,
                        const std::string& outPath,
                        std::function<void(Progress)> onProgress) {
  if (cfg_.peers.empty()) {
    throw std::runtime_error("No peers configured for DHT");
  }

  Manifest manifest = readManifest(manifestPath);

  std::ofstream out(outPath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Cannot open output file: " + outPath);
  }

  int total = static_cast<int>(manifest.chunks.size());
  int done = 0;

  for (const auto& entry : manifest.chunks) {
    // try ideal DHT locations first (same mapping as in putFile)
    std::vector<PeerEndpoint> candidates;
    std::unordered_set<size_t> used;
    int replicas = cfg_.desiredReplicas <= 0 ? 1 : cfg_.desiredReplicas;
    if (replicas > static_cast<int>(cfg_.peers.size())) {
      replicas = static_cast<int>(cfg_.peers.size());
    }
    for (int r = 0; r < replicas; ++r) {
      const std::string salt = entry.chunkId + "#" + std::to_string(r);
      const std::string h = dss::crypto::sha256_hex(
          std::vector<char>(salt.begin(), salt.end()));
      std::uint64_t value = 0;
      const size_t hexLen = std::min<std::size_t>(16, h.size());
      for (size_t i = 0; i < hexLen; ++i) {
        char ch = h[i];
        std::uint64_t v = 0;
        if (ch >= '0' && ch <= '9') v = static_cast<std::uint64_t>(ch - '0');
        else if (ch >= 'a' && ch <= 'f') v = static_cast<std::uint64_t>(10 + ch - 'a');
        else if (ch >= 'A' && ch <= 'F') v = static_cast<std::uint64_t>(10 + ch - 'A');
        value = (value << 4) | v;
      }
      size_t idx = static_cast<size_t>(value % cfg_.peers.size());
      size_t guard = 0;
      while (used.count(idx) && guard < cfg_.peers.size()) {
        idx = (idx + 1) % cfg_.peers.size();
        ++guard;
      }
      used.insert(idx);
      candidates.push_back(cfg_.peers[idx]);
    }
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


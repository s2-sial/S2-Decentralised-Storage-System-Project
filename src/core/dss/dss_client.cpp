#include "core/dss/dss_client.h"

#include "core/chunk/chunker.h"
#include "core/crypto/sha256.h"
#include "core/manifest/manifest.h"
#include "core/peer/peer_client.h"
#include "core/tracker/tracker_client.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace dss {

DssClient::DssClient(ClientConfig cfg) : cfg_(std::move(cfg)) {}

std::string DssClient::putFile(const std::string& filePath,
                               std::function<void(Progress)> onProgress) {
  TrackerClient tracker(cfg_.trackerIp, cfg_.trackerPort);
  auto peers = tracker.getPeers();
  if (peers.empty()) {
    throw std::runtime_error("No peers available from tracker");
  }

  int replicas = cfg_.desiredReplicas;
  if (replicas <= 0) replicas = 1;
  if (replicas > static_cast<int>(peers.size())) {
    replicas = static_cast<int>(peers.size());
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

    size_t start = static_cast<size_t>(done % peers.size());
    for (int r = 0; r < replicas; ++r) {
      size_t idx = (start + static_cast<size_t>(r)) % peers.size();
      const auto& peer = peers[idx];

      PeerClient::putChunk(peer, cid, c.bytes);
      tracker.announceChunk(cid, peer.ip, peer.port);
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
  TrackerClient tracker(cfg_.trackerIp, cfg_.trackerPort);
  auto peers = tracker.getPeers();
  if (peers.empty()) {
    throw std::runtime_error("No peers available from tracker");
  }

  Manifest manifest = readManifest(manifestPath);

  std::ofstream out(outPath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Cannot open output file: " + outPath);
  }

  int total = static_cast<int>(manifest.chunks.size());
  int done = 0;

  for (const auto& entry : manifest.chunks) {
    auto locations = tracker.whereChunk(entry.chunkId);
    if (locations.empty()) {
      locations = peers;
    }

    bool found = false;
    for (const auto& peer : locations) {
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
  TrackerClient tracker(cfg_.trackerIp, cfg_.trackerPort);
  auto peers = tracker.getPeers();
  if (peers.empty()) {
    throw std::runtime_error("No peers available from tracker");
  }

  int desired = cfg_.desiredReplicas;
  if (desired <= 0) desired = 2;

  auto chunks = tracker.needRepair(desired, batch);
  int total = static_cast<int>(chunks.size());
  int done = 0;

  for (const auto& cid : chunks) {
    auto locations = tracker.whereChunk(cid);
    if (locations.empty()) continue;

    // download from first source
    std::vector<char> data;
    bool got = false;
    for (const auto& peer : locations) {
      try {
        data = PeerClient::getChunk(peer, cid);
        if (dss::crypto::sha256_hex(data) != cid) continue;
        got = true;
        break;
      } catch (...) {
      }
    }
    if (!got) continue;

    // upload to new peers
    std::unordered_set<std::string> existing;
    for (const auto& p : locations) {
      existing.insert(p.ip + ":" + std::to_string(p.port));
    }

    int repaired = 0;
    for (const auto& p : peers) {
      std::string key = p.ip + ":" + std::to_string(p.port);
      if (existing.count(key)) continue;

      try {
        PeerClient::putChunk(p, cid, data);
        tracker.announceChunk(cid, p.ip, p.port);
        existing.insert(key);
        if (++repaired >= desired - static_cast<int>(locations.size())) break;
      } catch (...) {
      }
    }

    ++done;
    if (onProgress) {
      onProgress(Progress{done, total, "Repairing chunks"});
    }
  }
}

} // namespace dss


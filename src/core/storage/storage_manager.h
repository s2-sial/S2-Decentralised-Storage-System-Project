#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace dss::storage {

struct ChunkInfo {
  std::string id;
  std::uint64_t size{};
  std::uint64_t lastAccess{};
};

// Simple on-disk chunk storage with:
//  - max storage limit in bytes
//  - LRU eviction based on last-access counter
//  - disk usage tracking
//
// Layout under baseDir:
//   baseDir/
//     chunks/<hash>
//     manifests/        (reserved for future use)
//     metadata.db
class StorageManager {
public:
  StorageManager(std::string baseDir, std::uint64_t maxBytes);

  // Create directories, load or rebuild metadata.
  void init();

  std::uint64_t maxBytes() const { return maxBytes_; }
  void setMaxBytes(std::uint64_t bytes);

  std::uint64_t usedBytes() const { return usedBytes_; }

  // Store or overwrite a chunk. May trigger eviction.
  void storeChunk(const std::string& hash,
                  const std::vector<char>& data);

  // Load a chunk into memory. Throws if missing.
  std::vector<char> loadChunk(const std::string& hash);

  // Delete a chunk (if it exists).
  void deleteChunk(const std::string& hash);

  // Snapshot of all known chunks.
  std::vector<ChunkInfo> listChunks() const;

  // Check whether a chunk exists (metadata only, no disk IO).
  bool hasChunk(const std::string& hash) const;

private:
  struct ChunkMeta {
    std::uint64_t size{};
    std::uint64_t lastAccess{};
  };

  std::string baseDir_;
  std::string chunksDir_;
  std::string manifestsDir_;
  std::string metaPath_;

  std::uint64_t maxBytes_{};
  std::uint64_t usedBytes_{};
  std::uint64_t accessCounter_{};

  std::unordered_map<std::string, ChunkMeta> meta_;

  void loadMetadata();
  void rebuildMetadataFromDisk();
  void persistMetadata() const;

  void touch(const std::string& hash);
  void enforceLimit();
  std::string chunkPath(const std::string& hash) const;
};

}  // namespace dss::storage


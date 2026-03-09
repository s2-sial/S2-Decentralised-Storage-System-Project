#include "core/storage/storage_manager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <unordered_map>

namespace fs = std::filesystem;

namespace dss::storage {

StorageManager::StorageManager(std::string baseDir, std::uint64_t maxBytes)
    : baseDir_(std::move(baseDir)),
      chunksDir_(baseDir_ + "/chunks"),
      manifestsDir_(baseDir_ + "/manifests"),
      metaPath_(baseDir_ + "/metadata.db"),
      maxBytes_(maxBytes) {}

void StorageManager::init() {
  std::error_code ec;
  fs::create_directories(baseDir_, ec);
  fs::create_directories(chunksDir_, ec);
  fs::create_directories(manifestsDir_, ec);

  loadMetadata();
}

void StorageManager::setMaxBytes(std::uint64_t bytes) {
  maxBytes_ = bytes;
  enforceLimit();
}

std::string StorageManager::chunkPath(const std::string& hash) const {
  return chunksDir_ + "/" + hash;
}

void StorageManager::loadMetadata() {
  meta_.clear();
  usedBytes_ = 0;
  accessCounter_ = 0;

  std::ifstream in(metaPath_);
  if (!in) {
    // No metadata yet; rebuild from disk.
    rebuildMetadataFromDisk();
    return;
  }

  std::uint64_t storedCounter = 0;
  std::uint64_t storedUsed = 0;
  if (!(in >> storedCounter >> storedUsed)) {
    // Malformed; fall back to disk scan.
    rebuildMetadataFromDisk();
    return;
  }

  accessCounter_ = storedCounter;
  usedBytes_ = storedUsed;

  std::string id;
  ChunkMeta m{};
  while (in >> id >> m.size >> m.lastAccess) {
    meta_[id] = m;
  }
}

void StorageManager::rebuildMetadataFromDisk() {
  meta_.clear();
  usedBytes_ = 0;
  accessCounter_ = 0;

  std::error_code ec;
  if (!fs::exists(chunksDir_, ec)) {
    return;
  }

  for (const auto& entry : fs::directory_iterator(chunksDir_, ec)) {
    if (!entry.is_regular_file()) continue;
    const auto& p = entry.path();
    const std::string id = p.filename().string();
    std::uint64_t size = static_cast<std::uint64_t>(fs::file_size(p, ec));
    if (ec) continue;

    ChunkMeta m;
    m.size = size;
    m.lastAccess = ++accessCounter_;
    meta_[id] = m;
    usedBytes_ += size;
  }

  persistMetadata();
}

void StorageManager::persistMetadata() const {
  std::ofstream out(metaPath_, std::ios::trunc);
  if (!out) {
    // Best-effort only; storage can still function without metadata file.
    return;
  }

  out << accessCounter_ << " " << usedBytes_ << "\n";
  for (const auto& kv : meta_) {
    out << kv.first << " " << kv.second.size << " " << kv.second.lastAccess << "\n";
  }
}

void StorageManager::touch(const std::string& hash) {
  auto it = meta_.find(hash);
  if (it == meta_.end()) return;
  it->second.lastAccess = ++accessCounter_;
}

void StorageManager::enforceLimit() {
  if (maxBytes_ == 0) return;

  while (usedBytes_ > maxBytes_ && !meta_.empty()) {
    // Find LRU chunk (minimum lastAccess).
    auto victimIt = meta_.begin();
    for (auto it = meta_.begin(); it != meta_.end(); ++it) {
      if (it->second.lastAccess < victimIt->second.lastAccess) {
        victimIt = it;
      }
    }

    const std::string victimId = victimIt->first;
    const std::uint64_t victimSize = victimIt->second.size;

    std::error_code ec;
    fs::remove(chunkPath(victimId), ec);

    usedBytes_ -= victimSize;
    meta_.erase(victimIt);
  }

  persistMetadata();
}

void StorageManager::storeChunk(const std::string& hash,
                                const std::vector<char>& data) {
  if (hash.empty()) {
    throw std::runtime_error("storeChunk: empty hash");
  }

  const std::uint64_t newSize = static_cast<std::uint64_t>(data.size());

  // Adjust usedBytes_ if overwriting.
  auto it = meta_.find(hash);
  if (it != meta_.end()) {
    if (usedBytes_ >= it->second.size) {
      usedBytes_ -= it->second.size;
    } else {
      usedBytes_ = 0;
    }
  }

  const std::string pathTmp = chunkPath(hash) + ".tmp";
  const std::string pathFin = chunkPath(hash);

  {
    std::ofstream out(pathTmp, std::ios::binary);
    if (!out) {
      throw std::runtime_error("storeChunk: failed to open temp file for " + hash);
    }
    if (newSize > 0) {
      out.write(data.data(), static_cast<std::streamsize>(newSize));
    }
  }

  std::error_code ec;
  fs::rename(pathTmp, pathFin, ec);
  if (ec) {
    fs::remove(pathTmp, ec);
    throw std::runtime_error("storeChunk: rename failed for " + hash + ": " + ec.message());
  }

  ChunkMeta m;
  m.size = newSize;
  m.lastAccess = ++accessCounter_;
  meta_[hash] = m;
  usedBytes_ += newSize;

  enforceLimit();
}

std::vector<char> StorageManager::loadChunk(const std::string& hash) {
  const std::string path = chunkPath(hash);

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("loadChunk: missing chunk " + hash);
  }

  in.seekg(0, std::ios::end);
  const auto size = static_cast<std::uint64_t>(in.tellg());
  in.seekg(0, std::ios::beg);

  std::vector<char> data;
  data.resize(static_cast<std::size_t>(size));
  if (size > 0) {
    in.read(data.data(), static_cast<std::streamsize>(size));
  }

  touch(hash);
  persistMetadata();
  return data;
}

void StorageManager::deleteChunk(const std::string& hash) {
  auto it = meta_.find(hash);
  if (it != meta_.end()) {
    if (usedBytes_ >= it->second.size) {
      usedBytes_ -= it->second.size;
    } else {
      usedBytes_ = 0;
    }
    meta_.erase(it);
  }

  std::error_code ec;
  fs::remove(chunkPath(hash), ec);

  persistMetadata();
}

std::vector<ChunkInfo> StorageManager::listChunks() const {
  std::vector<ChunkInfo> out;
  out.reserve(meta_.size());
  for (const auto& kv : meta_) {
    out.push_back(ChunkInfo{kv.first, kv.second.size, kv.second.lastAccess});
  }
  return out;
}

}  // namespace dss::storage


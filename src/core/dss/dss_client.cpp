#include "core/dss/dss_client.h"

#include "core/chunk/chunker.h"
#include "core/crypto/sha256.h"
#include "core/crypto/hybrid.h"
#include "core/manifest/manifest.h"
#include "core/peer/peer_client.h"
#include "core/dht/dht_node.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace dss {
namespace {

std::string normalize_manifest_id(const std::string& manifestRef) {
  const std::string prefix = "dss://file/";
  if (manifestRef.rfind(prefix, 0) == 0) {
    return manifestRef.substr(prefix.size());
  }
  return manifestRef;
}

bool looks_like_local_manifest_path(const std::string& manifestRef) {
  return std::filesystem::exists(manifestRef);
}

}  // namespace

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

  const bool useEncryption = !cfg_.rsaPublicKeyPath.empty();
  dss::crypto::FileEncryptionParams encParams{};
  if (useEncryption) {
    encParams = dss::crypto::generate_file_key();
    dss::crypto::EncryptedKey ek =
        dss::crypto::encrypt_file_key_rsa(encParams, cfg_.rsaPublicKeyPath);
    manifest.encrypted = true;
    manifest.encAlgo = "AES-256-GCM";
    manifest.keyEncAlgo = ek.alg;
    manifest.encryptedKeyHex = dss::crypto::to_hex(ek.blob);
  }

  auto chunks = chunkFile(filePath, cfg_.chunkSize);
  int total = static_cast<int>(chunks.size());
  int done = 0;
  std::uint64_t chunkIndex = 0;

  for (auto& c : chunks) {
    std::string cid = dss::crypto::sha256_hex(c.bytes);
    c.id = cid;
    manifest.chunks.push_back(ManifestEntry{cid});

    std::vector<char> toSend;
    if (useEncryption) {
      toSend = dss::crypto::aes_gcm_encrypt(c.bytes,
                                            encParams.key,
                                            encParams.iv,
                                            chunkIndex);
    } else {
      toSend = c.bytes;
    }

    auto targets = dht.store(cid, replicas);
    // Deduplication (only in plaintext mode): if a target peer already has
    // this chunk ID, don't upload it again. Ensure we still reach the desired
    // replica count by uploading only to peers that don't have it.
    if (!useEncryption) {
      int already = 0;
      std::vector<const PeerEndpoint*> missing;
      missing.reserve(targets.size());
      for (const auto& p : targets) {
        if (PeerClient::hasChunk(p, cid)) {
          ++already;
        } else {
          missing.push_back(&p);
        }
      }
      int needed = replicas - already;
      if (needed > 0) {
        for (const PeerEndpoint* p : missing) {
          PeerClient::putChunk(*p, cid, toSend);
          if (--needed <= 0) break;
        }
      }
    } else {
      // Encrypted chunks use per-file keys, so they cannot be safely
      // deduplicated across different uploads.
      for (const auto& peer : targets) {
        PeerClient::putChunk(peer, cid, toSend);
      }
    }

    ++done;
    ++chunkIndex;
    if (onProgress) {
      onProgress(Progress{done, total, "Uploading chunks"});
    }
  }

  // Store manifest as content-addressed object in the DHT.
  std::string manifestText = serializeManifest(manifest);
  std::vector<char> manifestBytes(manifestText.begin(), manifestText.end());
  std::string manifestHash = dss::crypto::sha256_hex(manifestBytes);

  auto mTargets = dht.store("manifest:" + manifestHash, replicas);
  for (const auto& p : mTargets) {
    PeerClient::putManifest(p, manifestHash, manifestText);
  }

  // Keep writing local manifest file for debugging/backward compatibility.
  std::string localManifestPath = manifest.originalName + ".manifest.txt";
  writeManifest(localManifestPath, manifest);

  return "dss://file/" + manifestHash;
}

void DssClient::getFile(const std::string& manifestHash,
                        const std::string& outPath) {
  getFile(manifestHash, outPath, {});
}

void DssClient::getFile(const std::string& manifestPath,
                        const std::string& outPath,
                        std::function<void(Progress)> onProgress) {
  if (cfg_.peers.empty()) {
    throw std::runtime_error("No peers configured for DHT");
  }

  dss::dht::DhtNode dht(cfg_.peers);

  Manifest manifest;
  const std::string manifestId = normalize_manifest_id(manifestPath);
  if (looks_like_local_manifest_path(manifestPath)) {
    manifest = readManifest(manifestPath);
  } else {
    // Resolve manifest by hash/share-id from network.
    int replicas = cfg_.desiredReplicas <= 0 ? 1 : cfg_.desiredReplicas;
    auto mCandidates = dht.findValue("manifest:" + manifestId, replicas);
    for (const auto& p : cfg_.peers) {
      bool already = false;
      for (const auto& c : mCandidates) {
        if (c.ip == p.ip && c.port == p.port) {
          already = true;
          break;
        }
      }
      if (!already) mCandidates.push_back(p);
    }

    bool foundManifest = false;
    for (const auto& peer : mCandidates) {
      try {
        std::string text = PeerClient::getManifest(peer, manifestId);
        std::vector<char> bytes(text.begin(), text.end());
        if (dss::crypto::sha256_hex(bytes) != manifestId) {
          continue;
        }
        manifest = parseManifestText(text);
        foundManifest = true;
        break;
      } catch (...) {
      }
    }
    if (!foundManifest) {
      throw std::runtime_error("Failed to retrieve manifest " + manifestId);
    }
  }

  dss::crypto::FileEncryptionParams encParams{};
  const bool encryptedManifest = manifest.encrypted && !manifest.encryptedKeyHex.empty();
  const bool canDecrypt = encryptedManifest && !cfg_.rsaPrivateKeyPath.empty();
  if (encryptedManifest && !canDecrypt) {
    throw std::runtime_error("Manifest is encrypted but no RSA private key configured");
  }
  if (canDecrypt) {
    dss::crypto::EncryptedKey ek;
    ek.alg = manifest.keyEncAlgo;
    ek.blob = dss::crypto::from_hex(manifest.encryptedKeyHex);
    encParams = dss::crypto::decrypt_file_key_rsa(ek, cfg_.rsaPrivateKeyPath);
  }

  std::ofstream out(outPath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Cannot open output file: " + outPath);
  }

  int total = static_cast<int>(manifest.chunks.size());
  int done = 0;
  std::uint64_t chunkIndex = 0;

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
        auto encData = PeerClient::getChunk(peer, entry.chunkId);
        std::vector<char> plain;
        if (canDecrypt) {
          plain = dss::crypto::aes_gcm_decrypt(encData,
                                               encParams.key,
                                               encParams.iv,
                                               chunkIndex);
        } else {
          plain = encData;
        }
        if (dss::crypto::sha256_hex(plain) != entry.chunkId) {
          continue;
        }
        out.write(plain.data(), static_cast<std::streamsize>(plain.size()));
        found = true;
        break;
      } catch (...) {
      }
    }

    if (!found) {
      throw std::runtime_error("Failed to retrieve chunk " + entry.chunkId);
    }

    ++done;
    ++chunkIndex;
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


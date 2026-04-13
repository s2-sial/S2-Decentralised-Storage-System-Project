#pragma once
#include <functional>
#include <string>
#include <vector>

#include "core/common/peer_endpoint.h"

namespace dss {

struct ClientConfig {
  std::vector<PeerEndpoint> peers;  // list of known peers (DHT nodes)
  size_t chunkSize{};
  int desiredReplicas{};

  // Optional hybrid encryption settings.
  // If rsaPublicKeyPath is non-empty, putFile() will encrypt data.
  // If rsaPrivateKeyPath is non-empty and manifest is encrypted,
  // getFile() will attempt decryption.
  std::string rsaPublicKeyPath;
  std::string rsaPrivateKeyPath;

  // Directory for local <filename>.manifest.txt copies after put (created if missing).
  // If empty, defaults to "manifests" under the current working directory.
  std::string localManifestDir;
};

struct Progress {
  int done{};
  int total{};
  std::string message;
};

struct PutFileResult {
  std::string shareUri;         // e.g. dss://file/<sha256(manifest_text)>
  std::string localManifestPath; // absolute path under localManifestDir
};

class DssClient {
public:
  explicit DssClient(ClientConfig cfg);

  PutFileResult putFile(const std::string& filePath,
                        std::function<void(Progress)> onProgress = {});

  // Resolves by local .manifest.txt path, bare SHA-256 hex, or dss://file/<hex>.
  void getFile(const std::string& manifestHash,
               const std::string& outPath);

  void getFile(const std::string& manifestPath,
               const std::string& outPath,
               std::function<void(Progress)> onProgress = {});

  void repair(int batch,
              std::function<void(Progress)> onProgress = {});

private:
  ClientConfig cfg_;
};

}

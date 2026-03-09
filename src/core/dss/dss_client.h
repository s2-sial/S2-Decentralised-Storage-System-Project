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
};

struct Progress {
  int done{};
  int total{};
  std::string message;
};

class DssClient {
public:
  explicit DssClient(ClientConfig cfg);

  // returns path to created manifest
  std::string putFile(const std::string& filePath,
                      std::function<void(Progress)> onProgress = {});

  // Convenience: resolve and download by manifest identifier.
  // For now this expects manifestHash to be a local manifest path;
  // once manifests are stored in the DHT, this can fetch them by hash.
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

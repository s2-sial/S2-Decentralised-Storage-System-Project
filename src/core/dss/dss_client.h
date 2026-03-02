#pragma once
#include <functional>
#include <string>

namespace dss {

struct ClientConfig {
  std::string trackerIp;
  int trackerPort{};
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

  void getFile(const std::string& manifestPath,
               const std::string& outPath,
               std::function<void(Progress)> onProgress = {});

  void repair(int batch,
              std::function<void(Progress)> onProgress = {});

private:
  ClientConfig cfg_;
};

}

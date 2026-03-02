#pragma once
#include <string>
#include <vector>

namespace dss {
  struct Chunk { std::string id; std::vector<char> bytes; };
  std::vector<Chunk> chunkFile(const std::string& filePath, size_t chunkSize);
}

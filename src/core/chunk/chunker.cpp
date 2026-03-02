#include "core/chunk/chunker.h"

#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace dss {

std::vector<Chunk> chunkFile(const std::string& filePath, size_t chunkSize) {
  if (chunkSize == 0) {
    throw std::invalid_argument("chunkSize must be > 0");
  }

  std::ifstream in(filePath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open file for chunking: " + filePath);
  }

  std::vector<Chunk> chunks;
  std::vector<char> buffer(chunkSize);
  std::uint64_t index = 0;

  while (true) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    std::streamsize got = in.gcount();
    if (got <= 0) break;

    std::vector<char> bytes(buffer.begin(), buffer.begin() + got);

    Chunk c;
    c.id = ""; // caller is expected to fill id (e.g. sha256) if needed
    c.bytes = std::move(bytes);
    chunks.push_back(std::move(c));

    ++index;
  }

  return chunks;
}

} // namespace dss


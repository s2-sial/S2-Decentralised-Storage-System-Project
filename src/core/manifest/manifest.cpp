#include "core/manifest/manifest.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace dss {

void writeManifest(const std::string& path, const Manifest& m) {
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("Failed to open manifest for write: " + path);
  }

  out << "filename " << m.originalName << "\n";
  out << "size " << m.originalSize << "\n";
  out << "chunk_size " << m.chunkSize << "\n";
  out << "chunks " << m.chunks.size() << "\n";
  for (const auto& e : m.chunks) {
    out << e.chunkId << "\n";
  }

  if (m.encrypted) {
    out << "encrypted 1\n";
    out << "enc_algo " << m.encAlgo << "\n";
    out << "key_enc_algo " << m.keyEncAlgo << "\n";
    out << "enc_key " << m.encryptedKeyHex << "\n";
  } else {
    out << "encrypted 0\n";
  }
}

Manifest readManifest(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Failed to open manifest for read: " + path);
  }

  Manifest m;
  std::string label;
  size_t chunkCount = 0;

  in >> label >> m.originalName;
  if (!in || label != "filename") {
    throw std::runtime_error("Malformed manifest (filename): " + path);
  }

  in >> label >> m.originalSize;
  if (!in || label != "size") {
    throw std::runtime_error("Malformed manifest (size): " + path);
  }

  in >> label >> m.chunkSize;
  if (!in || label != "chunk_size") {
    throw std::runtime_error("Malformed manifest (chunk_size): " + path);
  }

  in >> label >> chunkCount;
  if (!in || label != "chunks") {
    throw std::runtime_error("Malformed manifest (chunks): " + path);
  }

  m.chunks.clear();
  m.chunks.reserve(chunkCount);
  for (size_t i = 0; i < chunkCount; ++i) {
    ManifestEntry e;
    if (!(in >> e.chunkId)) {
      throw std::runtime_error("Malformed manifest (chunk id): " + path);
    }
    m.chunks.push_back(std::move(e));
  }

  // Optional encryption metadata.
  int encFlag = 0;
  if (in >> label >> encFlag) {
    if (label == "encrypted" && encFlag == 1) {
      m.encrypted = true;
      if (!(in >> label >> m.encAlgo) || label != "enc_algo") {
        throw std::runtime_error("Malformed manifest (enc_algo): " + path);
      }
      if (!(in >> label >> m.keyEncAlgo) || label != "key_enc_algo") {
        throw std::runtime_error("Malformed manifest (key_enc_algo): " + path);
      }
      if (!(in >> label >> m.encryptedKeyHex) || label != "enc_key") {
        throw std::runtime_error("Malformed manifest (enc_key): " + path);
      }
    }
  }

  return m;
}

} // namespace dss


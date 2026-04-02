#include "core/dht/kademlia_id.h"

#include "core/crypto/sha256.h"

#include <algorithm>
#include <cctype>

namespace dss::dht {

namespace {

std::vector<std::uint8_t> hexToBytes(const std::string& hex) {
  std::vector<std::uint8_t> out;
  if (hex.size() % 2 != 0) return out;
  out.reserve(hex.size() / 2);
  for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
    auto hexByte = hex.substr(i, 2);
    std::uint8_t value = static_cast<std::uint8_t>(std::stoi(hexByte, nullptr, 16));
    out.push_back(value);
  }
  return out;
}

std::string bytesToHex(const std::uint8_t* data, std::size_t len) {
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.resize(len * 2);
  for (std::size_t i = 0; i < len; ++i) {
    out[2 * i]     = kHex[(data[i] >> 4) & 0xF];
    out[2 * i + 1] = kHex[data[i] & 0xF];
  }
  return out;
}

}  // namespace

Id idFromBytes(const std::vector<std::uint8_t>& bytes) {
  Id id{};
  const std::size_t n = std::min<std::size_t>(bytes.size(), id.size());
  if (n > 0) {
    std::copy(bytes.begin(), bytes.begin() + n, id.begin());
  }
  // rest stays zero‑padded
  return id;
}

Id makeNodeId(const PeerEndpoint& peer) {
  const std::string s = peer.ip + ":" + std::to_string(peer.port);
  const std::vector<char> data(s.begin(), s.end());
  const std::string hex = dss::crypto::sha256_hex(data);
  auto bytes = hexToBytes(hex);
  return idFromBytes(bytes);
}

Id makeKeyId(const std::string& key) {
  const std::vector<char> data(key.begin(), key.end());
  const std::string hex = dss::crypto::sha256_hex(data);
  auto bytes = hexToBytes(hex);
  return idFromBytes(bytes);
}

std::array<std::uint8_t, 20> xorDistance(const Id& a, const Id& b) {
  std::array<std::uint8_t, 20> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint8_t>(a[i] ^ b[i]);
  }
  return out;
}

bool closerTo(const Id& a, const Id& b, const Id& target) {
  auto da = xorDistance(a, target);
  auto db = xorDistance(b, target);
  return da < db;  // lexicographic compare
}

std::string toHex(const Id& id) {
  return bytesToHex(id.data(), id.size());
}

Id fromHex(const std::string& hex) {
  Id id{};
  auto bytes = hexToBytes(hex);
  const std::size_t n = std::min<std::size_t>(bytes.size(), id.size());
  if (n > 0) {
    std::copy(bytes.begin(), bytes.begin() + n, id.begin());
  }
  return id;
}

}  // namespace dss::dht


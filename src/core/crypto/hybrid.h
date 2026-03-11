#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace dss::crypto {

struct FileEncryptionParams {
  std::vector<unsigned char> key;  // 32 bytes AES-256 key
  std::vector<unsigned char> iv;   // 12 bytes base IV
};

struct EncryptedKey {
  std::string alg;                  // e.g. "RSA-2048"
  std::vector<unsigned char> blob;  // RSA-encrypted (key || iv)
};

// Generate random AES-256-GCM key and base IV.
FileEncryptionParams generate_file_key();

// AES-256-GCM encrypt/decrypt.
// IV for each chunk is derived from base iv and chunkIndex.
// Ciphertext layout: [cipher || tag] (16-byte tag appended).
std::vector<char> aes_gcm_encrypt(const std::vector<char>& plaintext,
                                  const std::vector<unsigned char>& key,
                                  const std::vector<unsigned char>& baseIv,
                                  std::uint64_t chunkIndex);

std::vector<char> aes_gcm_decrypt(const std::vector<char>& ciphertext,
                                  const std::vector<unsigned char>& key,
                                  const std::vector<unsigned char>& baseIv,
                                  std::uint64_t chunkIndex);

// Encrypt/decrypt file key+IV with RSA public/private key in PEM files.
EncryptedKey encrypt_file_key_rsa(const FileEncryptionParams& params,
                                  const std::string& publicKeyPemPath);

FileEncryptionParams decrypt_file_key_rsa(const EncryptedKey& ek,
                                          const std::string& privateKeyPemPath);

// Simple hex helpers for binary blobs.
std::string to_hex(const std::vector<unsigned char>& data);
std::vector<unsigned char> from_hex(const std::string& hex);

}  // namespace dss::crypto


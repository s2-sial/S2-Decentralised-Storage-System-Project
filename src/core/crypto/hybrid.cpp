#include "core/crypto/hybrid.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <stdexcept>
#include <sstream>
#include <cstring>

namespace dss::crypto {

namespace {

void ensure_ok(int rc, const char* what) {
  if (rc != 1) {
    throw std::runtime_error(std::string("OpenSSL error in ") + what);
  }
}

std::vector<unsigned char> derive_chunk_iv(const std::vector<unsigned char>& baseIv,
                                           std::uint64_t idx) {
  if (baseIv.size() != 12) {
    throw std::runtime_error("base IV must be 12 bytes");
  }
  std::vector<unsigned char> iv = baseIv;
  // use last 4 bytes as chunk counter (big endian)
  std::uint32_t ctr = static_cast<std::uint32_t>(idx & 0xffffffffu);
  iv[8]  = static_cast<unsigned char>((ctr >> 24) & 0xff);
  iv[9]  = static_cast<unsigned char>((ctr >> 16) & 0xff);
  iv[10] = static_cast<unsigned char>((ctr >> 8) & 0xff);
  iv[11] = static_cast<unsigned char>(ctr & 0xff);
  return iv;
}

}  // namespace

FileEncryptionParams generate_file_key() {
  FileEncryptionParams p;
  p.key.resize(32);
  p.iv.resize(12);
  if (RAND_bytes(p.key.data(), static_cast<int>(p.key.size())) != 1 ||
      RAND_bytes(p.iv.data(), static_cast<int>(p.iv.size())) != 1) {
    throw std::runtime_error("RAND_bytes failed");
  }
  return p;
}

std::vector<char> aes_gcm_encrypt(const std::vector<char>& plaintext,
                                  const std::vector<unsigned char>& key,
                                  const std::vector<unsigned char>& baseIv,
                                  std::uint64_t chunkIndex) {
  auto iv = derive_chunk_iv(baseIv, chunkIndex);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

  std::vector<unsigned char> out;
  out.resize(plaintext.size() + 16);  // ciphertext + tag

  int len = 0;
  int ciphertext_len = 0;

  try {
    ensure_ok(EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr),
              "EVP_EncryptInit_ex");
    ensure_ok(EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()),
              "EVP_EncryptInit_ex (key/iv)");

    if (!plaintext.empty()) {
      ensure_ok(EVP_EncryptUpdate(ctx,
                                  out.data(),
                                  &len,
                                  reinterpret_cast<const unsigned char*>(plaintext.data()),
                                  static_cast<int>(plaintext.size())),
                "EVP_EncryptUpdate");
      ciphertext_len = len;
    }

    ensure_ok(EVP_EncryptFinal_ex(ctx, out.data() + ciphertext_len, &len),
              "EVP_EncryptFinal_ex");
    ciphertext_len += len;

    unsigned char tag[16];
    ensure_ok(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag),
              "EVP_CIPHER_CTX_ctrl(GET_TAG)");

    out.resize(ciphertext_len + 16);
    std::memcpy(out.data() + ciphertext_len, tag, 16);

    EVP_CIPHER_CTX_free(ctx);
    return std::vector<char>(out.begin(), out.end());
  } catch (...) {
    EVP_CIPHER_CTX_free(ctx);
    throw;
  }
}

std::vector<char> aes_gcm_decrypt(const std::vector<char>& ciphertext,
                                  const std::vector<unsigned char>& key,
                                  const std::vector<unsigned char>& baseIv,
                                  std::uint64_t chunkIndex) {
  if (ciphertext.size() < 16) {
    throw std::runtime_error("ciphertext too short");
  }
  auto iv = derive_chunk_iv(baseIv, chunkIndex);

  const std::size_t tagPos = ciphertext.size() - 16;
  const unsigned char* tag =
      reinterpret_cast<const unsigned char*>(ciphertext.data() + tagPos);
  const unsigned char* ctext =
      reinterpret_cast<const unsigned char*>(ciphertext.data());
  const int ctextLen = static_cast<int>(tagPos);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

  std::vector<char> out;
  out.resize(ctextLen);

  int len = 0;
  int plain_len = 0;

  try {
    ensure_ok(EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr),
              "EVP_DecryptInit_ex");
    ensure_ok(EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()),
              "EVP_DecryptInit_ex (key/iv)");

    if (ctextLen > 0) {
      ensure_ok(EVP_DecryptUpdate(ctx,
                                  reinterpret_cast<unsigned char*>(out.data()),
                                  &len,
                                  ctext,
                                  ctextLen),
                "EVP_DecryptUpdate");
      plain_len = len;
    }

    ensure_ok(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                                  const_cast<unsigned char*>(tag)),
              "EVP_CIPHER_CTX_ctrl(SET_TAG)");

    int ret = EVP_DecryptFinal_ex(ctx,
                                  reinterpret_cast<unsigned char*>(out.data()) + plain_len,
                                  &len);
    if (ret <= 0) {
      EVP_CIPHER_CTX_free(ctx);
      throw std::runtime_error("AES-GCM tag verification failed");
    }
    plain_len += len;
    out.resize(plain_len);

    EVP_CIPHER_CTX_free(ctx);
    return out;
  } catch (...) {
    EVP_CIPHER_CTX_free(ctx);
    throw;
  }
}

EncryptedKey encrypt_file_key_rsa(const FileEncryptionParams& params,
                                  const std::string& publicKeyPemPath) {
  FILE* f = std::fopen(publicKeyPemPath.c_str(), "r");
  if (!f) throw std::runtime_error("Failed to open RSA public key: " + publicKeyPemPath);

  EVP_PKEY* pkey = PEM_read_PUBKEY(f, nullptr, nullptr, nullptr);
  std::fclose(f);
  if (!pkey) throw std::runtime_error("Failed to read RSA public key");

  std::vector<unsigned char> blob;
  blob.reserve(params.key.size() + params.iv.size());
  blob.insert(blob.end(), params.key.begin(), params.key.end());
  blob.insert(blob.end(), params.iv.begin(), params.iv.end());

  EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
  if (!ctx) {
    EVP_PKEY_free(pkey);
    throw std::runtime_error("EVP_PKEY_CTX_new failed");
  }

  EncryptedKey ek;
  ek.alg = "RSA";

  try {
    ensure_ok(EVP_PKEY_encrypt_init(ctx), "EVP_PKEY_encrypt_init");
    ensure_ok(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING),
              "EVP_PKEY_CTX_set_rsa_padding");

    size_t outLen = 0;
    ensure_ok(EVP_PKEY_encrypt(ctx, nullptr, &outLen, blob.data(), blob.size()),
              "EVP_PKEY_encrypt (size)");

    ek.blob.resize(outLen);
    ensure_ok(EVP_PKEY_encrypt(ctx, ek.blob.data(), &outLen, blob.data(), blob.size()),
              "EVP_PKEY_encrypt");
    ek.blob.resize(outLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ek;
  } catch (...) {
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    throw;
  }
}

FileEncryptionParams decrypt_file_key_rsa(const EncryptedKey& ek,
                                          const std::string& privateKeyPemPath) {
  (void)ek.alg;  // currently unused; could be used to switch algorithms

  FILE* f = std::fopen(privateKeyPemPath.c_str(), "r");
  if (!f) throw std::runtime_error("Failed to open RSA private key: " + privateKeyPemPath);

  EVP_PKEY* pkey = PEM_read_PrivateKey(f, nullptr, nullptr, nullptr);
  std::fclose(f);
  if (!pkey) throw std::runtime_error("Failed to read RSA private key");

  EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
  if (!ctx) {
    EVP_PKEY_free(pkey);
    throw std::runtime_error("EVP_PKEY_CTX_new failed");
  }

  try {
    ensure_ok(EVP_PKEY_decrypt_init(ctx), "EVP_PKEY_decrypt_init");
    ensure_ok(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING),
              "EVP_PKEY_CTX_set_rsa_padding");

    size_t outLen = 0;
    ensure_ok(EVP_PKEY_decrypt(ctx, nullptr, &outLen,
                               ek.blob.data(), ek.blob.size()),
              "EVP_PKEY_decrypt (size)");

    std::vector<unsigned char> out;
    out.resize(outLen);
    ensure_ok(EVP_PKEY_decrypt(ctx, out.data(), &outLen,
                               ek.blob.data(), ek.blob.size()),
              "EVP_PKEY_decrypt");
    out.resize(outLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    if (out.size() < 32 + 12) {
      throw std::runtime_error("Decrypted key blob too short");
    }
    FileEncryptionParams p;
    p.key.assign(out.begin(), out.begin() + 32);
    p.iv.assign(out.begin() + 32, out.begin() + 44);
    return p;
  } catch (...) {
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    throw;
  }
}

std::string to_hex(const std::vector<unsigned char>& data) {
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.resize(data.size() * 2);
  for (std::size_t i = 0; i < data.size(); ++i) {
    out[2 * i]     = kHex[(data[i] >> 4) & 0xF];
    out[2 * i + 1] = kHex[data[i] & 0xF];
  }
  return out;
}

std::vector<unsigned char> from_hex(const std::string& hex) {
  std::vector<unsigned char> out;
  if (hex.size() % 2 != 0) return out;
  out.reserve(hex.size() / 2);
  for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
    std::string byteStr = hex.substr(i, 2);
    unsigned char v =
        static_cast<unsigned char>(std::stoi(byteStr, nullptr, 16));
    out.push_back(v);
  }
  return out;
}

}  // namespace dss::crypto


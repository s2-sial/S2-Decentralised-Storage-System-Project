#include "core/crypto/sha256.h"

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace dss::crypto {

std::string sha256_hex(const std::vector<char>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(hash, &ctx);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }
    return oss.str();
}

}

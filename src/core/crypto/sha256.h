#pragma once
#include <string>
#include <vector>

namespace dss::crypto {

// returns lowercase hex sha256 like your current function
std::string sha256_hex(const std::vector<char>& data);

}

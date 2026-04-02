#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace dss::net {

bool set_timeouts(int sock, int recv_ms, int send_ms);
bool is_timeout_errno();

// Send helpers
bool send_all_nothrow(int fd, const void* buf, size_t len);
bool send_all_nothrow(int fd, const std::string& s);

// Same semantics as your old client "die": throws on failure.
void send_all_or_throw(int fd, const void* buf, size_t len);
void send_all_or_throw(int fd, const std::string& s);

// Receive helpers
std::string recv_all_text(int fd);              // reads until close or timeout
bool recv_line(int fd, std::string& out_line);  // reads until '\n' (max 8192)

// Connect helpers
int connect_tcp_fatal_or_throw(const std::string& ip, int port);
int connect_tcp_try(const std::string& ip, int port, int timeout_ms = 8000);

} // namespace dss::net

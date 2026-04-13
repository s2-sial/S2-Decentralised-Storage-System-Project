#include "core/net/tcp.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/time.h>

namespace dss::net {

namespace {

// IPv4 literal (inet_pton) or hostname (getaddrinfo), e.g. ngrok TCP endpoints.
bool fill_sockaddr_in_v4(sockaddr_in* addr, const std::string& host, int port) {
  addr->sin_family = AF_INET;
  addr->sin_port = htons(static_cast<uint16_t>(port));
  if (::inet_pton(AF_INET, host.c_str(), &addr->sin_addr) == 1) {
    return true;
  }
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* res = nullptr;
  int gai = ::getaddrinfo(host.c_str(), nullptr, &hints, &res);
  if (gai != 0 || res == nullptr) {
    return false;
  }
  const auto* sa = reinterpret_cast<const sockaddr_in*>(res->ai_addr);
  addr->sin_addr = sa->sin_addr;
  ::freeaddrinfo(res);
  return true;
}

} // namespace

static std::runtime_error sys_err(const char* what) {
    return std::runtime_error(std::string(what) + ": " + std::strerror(errno));
}

bool set_timeouts(int sock, int recv_ms, int send_ms) {
    timeval tv{};

    tv.tv_sec  = recv_ms / 1000;
    tv.tv_usec = (recv_ms % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        return false;

    tv.tv_sec  = send_ms / 1000;
    tv.tv_usec = (send_ms % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        return false;

    return true;
}

bool is_timeout_errno() {
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

bool send_all_nothrow(int fd, const void* buf, size_t len) {
    const char* p = static_cast<const char*>(buf);
    while (len > 0) {
        ssize_t n = ::send(fd, p, len, 0);
        if (n <= 0) return false;
        p += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

bool send_all_nothrow(int fd, const std::string& s) {
    return send_all_nothrow(fd, s.data(), s.size());
}

void send_all_or_throw(int fd, const void* buf, size_t len) {
    if (!send_all_nothrow(fd, buf, len))
        throw sys_err("send");
}

void send_all_or_throw(int fd, const std::string& s) {
    send_all_or_throw(fd, s.data(), s.size());
}

// Same as your client recv_all_text() behavior:
// - keeps reading until peer closes
// - if timeout happens, stops waiting and returns what it has
std::string recv_all_text(int fd) {
    std::string out;
    char buf[4096];
    while (true) {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n == 0) break;
        if (n < 0) {
            if (is_timeout_errno()) break;
            throw sys_err("recv");
        }
        out.append(buf, buf + n);
    }
    return out;
}

// Based on your peer recv_line() (with safety cap)
bool recv_line(int fd, std::string& out_line) {
    out_line.clear();
    char ch;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n <= 0) return false;
        if (n < 0) {
            if (is_timeout_errno()) return false;
            return false;
        }
        if (ch == '\n') break;
        if (ch != '\r') out_line.push_back(ch);
        if (out_line.size() > 8192) return false;
    }
    return true;
}

int connect_tcp_fatal_or_throw(const std::string& ip, int port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw sys_err("socket");

    sockaddr_in addr{};
    if (!fill_sockaddr_in_v4(&addr, ip, port)) {
        ::close(sock);
        throw std::runtime_error("Could not resolve host: " + ip);
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(sock);
        throw sys_err("connect");
    }

    return sock;
}

// Same as your client connect_tcp_try():
// - returns -1 on failure
// - sets recv/send timeouts
int connect_tcp_try(const std::string& ip, int port, int timeout_ms) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    set_timeouts(sock, timeout_ms, timeout_ms);

    sockaddr_in addr{};
    if (!fill_sockaddr_in_v4(&addr, ip, port)) {
        ::close(sock);
        return -1;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(sock);
        return -1;
    }

    return sock;
}

} // namespace dss::net

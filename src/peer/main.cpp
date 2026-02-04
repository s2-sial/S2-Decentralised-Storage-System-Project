#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h> // timeval

static bool set_timeouts(int sock, int recv_ms, int send_ms) {
    timeval tv{};

    tv.tv_sec = recv_ms / 1000;
    tv.tv_usec = (recv_ms % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    return false;

    tv.tv_sec = send_ms / 1000;
    tv.tv_usec = (send_ms % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
    return false;

    return true;
}

static void die(const std::string& msg) {
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(1);
}

static void ensure_dir(const std::string& path) {
    mkdir(path.c_str(), 0755); //ok if already exists
}

static bool is_timeout_errno() {
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

static bool recv_line(int fd, std::string& out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return false;
        if (n < 0) {
            if (is_timeout_errno()) return false; //timed out
            return false;
        }

        if (ch == '\n') break;
        if (ch != '\r') out.push_back(ch);

        //protection for long lines
        if (out.size() > 8192) return false;
    }

    return true;
}

static bool send_all(int fd, const char* buf, size_t len) {
    while (len > 0) {
        ssize_t n = ::send(fd, buf, len, 0);
        if (n <= 0) return false;
        buf += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: peer <peer_port> <tracker_ip> <tracker_port> <storage_dir>\n";
        return 1;
    } 

    int peer_port = std::stoi(argv[1]);
    std::string tracker_ip = argv[2];
    int tracker_port = std::stoi(argv[3]);
    std::string storage_dir = argv[4];

    ensure_dir(storage_dir);

    // Register with tracker
    {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);

        set_timeouts(sock, 3000, 3000);

        if (sock < 0) die("socket");

        sockaddr_in tracker{};
        tracker.sin_family = AF_INET;
        tracker.sin_port = htons(tracker_port);
        inet_pton(AF_INET, tracker_ip.c_str(), &tracker.sin_addr);

        if (::connect(sock, (sockaddr*)&tracker, sizeof(tracker)) < 0)
        die("connect tracker");

        std::string msg = "REGISTER 127.0.0.1 " + std::to_string(peer_port) + "\n";
        send_all(sock, msg.c_str(), msg.size());
        close(sock);
    }

    std::cout << "Peer listening on port " << peer_port << "\n";

    // Storage server
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) die("socket");

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(peer_port);

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(listen_fd, 16) < 0) die("listen");

    while (true) {
        int client = accept(listen_fd, nullptr, nullptr);
        if (client < 0) continue;

        if (!set_timeouts(client, /*recv_ms*/ 10000, /*send_ms*/ 10000)) {
            //not fatal; but log
        }

        std::string line;
        if (!recv_line(client, line)) {
            close(client);
            continue;
        }
        
        std::istringstream iss(line);
        std::string cmd, hash;
        size_t size;

        iss >> cmd;

        if (cmd == "PUT_CHUNK") {
            iss >> hash >> size;

            std::string path = storage_dir + "/" + hash;
            std::ofstream out(path, std::ios::binary);

            char buf[4096];
            size_t remaining = size;

            while (remaining > 0) {
                ssize_t n = recv(client, buf, std::min(sizeof(buf), remaining), 0);
                if (n <= 0) break;
                out.write(buf, n);
                remaining -= n;
            }

            out.close();
            send_all(client, "OK\n", 3);
        }
        else if (cmd == "GET_CHUNK") {
            iss >> hash;
            std::string path = storage_dir + "/" + hash;

            std::ifstream in(path, std::ios::binary);
            if (!in) {
                send_all(client, "ERR\n", 4);
            } else {
                in.seekg(0, std::ios::end);
                size_t size = in.tellg();
                in.seekg(0);

                std::string header = "OK " + std::to_string(size) + "\n";
                send_all(client, header.c_str(), header.size());

                char buf[4096];
                while (in.read(buf, sizeof(buf)))
                    send_all(client, buf, sizeof(buf));
                send_all(client, buf, in.gcount());
            }
        }

        close(client);
    }
    
}
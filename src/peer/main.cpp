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
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "core/net/tcp.h"
#include "core/storage/storage_manager.h"

using dss::net::set_timeouts;
using dss::net::recv_line;
using dss::net::send_all_nothrow;

//helpers 
static void die(const std::string& msg) {
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(1);
}

static void ensure_dir(const std::string& path) {
    mkdir(path.c_str(), 0755); //ok if already exists
}

static void send_to_tracker(const std::string& tracker_ip, int tracker_port, const std::string& line) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    set_timeouts(sock, 3000, 3000);

    sockaddr_in tracker{};
    tracker.sin_family = AF_INET;
    tracker.sin_port = htons(tracker_port);
    inet_pton(AF_INET, tracker_ip.c_str(), &tracker.sin_addr);

    if (::connect(sock, (sockaddr*)&tracker, sizeof(tracker)) < 0) {
        close(sock);
        return;
    }

    send_all_nothrow(sock, line);
    close(sock);
    return;
}

//end of helpers
//main program
int main(int argc, char** argv) {

    if (argc < 4) {
        std::cerr << "Usage: peer <advertise_ip> <peer_port> <storage_dir> [max_bytes]\n";
        return 1;
    } 

    std::string advertise_ip = argv[1];
    int peer_port = std::stoi(argv[2]);
    std::string storage_dir = argv[3];
    std::uint64_t max_bytes = 10ull * 1024 * 1024 * 1024; // 10 GiB default
    if (argc >= 5) {
        max_bytes = std::stoull(argv[4]);
    }

    //prints the storage directory for testing purposes
    std::cout << "Storage dir: " << std::filesystem::absolute(storage_dir) << "\n";

    ensure_dir(storage_dir);

    dss::storage::StorageManager storage(storage_dir, max_bytes);
    storage.init();

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

            std::vector<char> data;
            data.resize(size);

            size_t remaining = size;
            size_t offset = 0;
            while (remaining > 0) {
                const size_t chunk = std::min<std::size_t>(remaining, 4096);
                ssize_t n = ::recv(client, data.data() + offset, chunk, 0);
                if (n <= 0) break;
                offset += static_cast<std::size_t>(n);
                remaining -= static_cast<std::size_t>(n);
            }

            if (remaining == 0) {
                try {
                    storage.storeChunk(hash, data);
                    send_all_nothrow(client, "OK\n");
                } catch (const std::exception& e) {
                    std::cerr << "PUT_CHUNK failed for " << hash << ": " << e.what() << "\n";
                    send_all_nothrow(client, "ERR\n");
                }
            } else {
                std::cerr << "PUT_CHUNK incomplete for " << hash
                          << " remaining=" << remaining << "\n";
                send_all_nothrow(client, "ERR\n");
            }
        }
        else if (cmd == "GET_CHUNK") {
            iss >> hash;
            try {
                auto data = storage.loadChunk(hash);
                const size_t size = data.size();

                std::string header = "OK " + std::to_string(size) + "\n";
                send_all_nothrow(client, header);

                if (size > 0) {
                    send_all_nothrow(client,
                                     std::string(data.data(), static_cast<std::streamsize>(size)));
                }
            } catch (const std::exception&) {
                send_all_nothrow(client, "ERR\n");
            }
        }
        else if (cmd == "HAS_CHUNK") {
            iss >> hash;
            if (storage.hasChunk(hash)) {
                send_all_nothrow(client, "OK\n");
            } else {
                send_all_nothrow(client, "ERR\n");
            }
        }
        else if (cmd == "PUT_MANIFEST") {
            std::string manifestId;
            iss >> manifestId >> size;
            std::vector<char> data;
            data.resize(size);
            size_t remaining = size;
            size_t offset = 0;
            while (remaining > 0) {
                const size_t chunk = std::min<std::size_t>(remaining, 4096);
                ssize_t n = ::recv(client, data.data() + offset, chunk, 0);
                if (n <= 0) break;
                offset += static_cast<std::size_t>(n);
                remaining -= static_cast<std::size_t>(n);
            }
            if (remaining == 0) {
                std::string path = storage_dir + "/manifests/" + manifestId;
                std::ofstream out(path, std::ios::binary | std::ios::trunc);
                if (!out) {
                    send_all_nothrow(client, "ERR\n");
                } else {
                    out.write(data.data(), static_cast<std::streamsize>(data.size()));
                    send_all_nothrow(client, "OK\n");
                }
            } else {
                send_all_nothrow(client, "ERR\n");
            }
        }
        else if (cmd == "GET_MANIFEST") {
            std::string manifestId;
            iss >> manifestId;
            std::string path = storage_dir + "/manifests/" + manifestId;
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                send_all_nothrow(client, "ERR\n");
            } else {
                in.seekg(0, std::ios::end);
                size_t msize = static_cast<size_t>(in.tellg());
                in.seekg(0, std::ios::beg);
                std::string text;
                text.resize(msize);
                if (msize > 0) {
                    in.read(text.data(), static_cast<std::streamsize>(msize));
                }
                send_all_nothrow(client, "OK " + std::to_string(msize) + "\n");
                if (msize > 0) send_all_nothrow(client, text);
            }
        }
        else if (cmd == "HAS_MANIFEST") {
            std::string manifestId;
            iss >> manifestId;
            std::string path = storage_dir + "/manifests/" + manifestId;
            std::ifstream in(path, std::ios::binary);
            if (in) send_all_nothrow(client, "OK\n");
            else send_all_nothrow(client, "ERR\n");
        }

        close(client);
    }
    
}
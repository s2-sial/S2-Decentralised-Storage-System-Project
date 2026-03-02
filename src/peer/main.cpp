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
#include "core/net/tcp.h"

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

    if (argc != 6) {
        std::cerr << "Usage: peer <advertise_ip> <peer_port> <tracker_ip> <tracker_port> <storage_dir>\n";
        return 1;
    } 

    std::string advertise_ip = argv[1];
    int peer_port = std::stoi(argv[2]);
    std::string tracker_ip = argv[3];
    int tracker_port = std::stoi(argv[4]);
    std::string storage_dir = argv[5];

    //prints the storage directory for testing purposes
    std::cout << "Storage dir: " << std::filesystem::absolute(storage_dir) << "\n";

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

        std::string msg = "REGISTER " + advertise_ip + " " + std::to_string(peer_port) + "\n";
        send_all_nothrow(sock, msg);
        close(sock);
    }

    std::cout << "Peer listening on port " << peer_port << "\n";

    //heartbeat thread
    std::atomic<bool> running{true};

    std::thread hb([&]{
        while (running.load()) {
            std::string msg = "HEARTBEAT " + advertise_ip + " " + std::to_string(peer_port) + "\n";
            send_to_tracker(tracker_ip, tracker_port, msg);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });
    hb.detach(); //simplest (or join on shutdown)

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

            std::string path_tmp = storage_dir + "/" + hash + ".tmp";
            std::string path_fin = storage_dir + "/" + hash;

            std::ofstream out(path_tmp, std::ios::binary);
            if (!out) {
                send_all_nothrow(client, "ERR\n");
                close(client);
                continue;
            }

            char buf[4096];
            size_t remaining = size;

            while (remaining > 0) {
                ssize_t n = ::recv(client, buf, std::min(sizeof(buf), remaining), 0);
                if (n <= 0) break;
                out.write(buf, n);
                remaining -= static_cast<size_t>(n);
            }
            
            out.close();

            if (remaining == 0) {
                //atomic replace
                ::rename(path_tmp.c_str(), path_fin.c_str());
                send_all_nothrow(client, "OK\n");
            } else {
                ::unlink(path_tmp.c_str());
                send_all_nothrow(client, "ERR\n");
                std::cerr << "PUT_CHUNK incomplete for " << hash
                << " remaining=" << remaining << "\n";
            }
        }
        else if (cmd == "GET_CHUNK") {
            iss >> hash;
            std::string path = storage_dir + "/" + hash;

            std::ifstream in(path, std::ios::binary);
            if (!in) {
                send_all_nothrow(client, "ERR\n");
            } else {
                in.seekg(0, std::ios::end);
                size_t size = in.tellg();
                in.seekg(0);

                std::string header = "OK " + std::to_string(size) + "\n";
                send_all_nothrow(client, header);

                char buf[4096];
                while (in.read(buf, sizeof(buf)))
                    send_all_nothrow(client, std::string(buf, sizeof(buf)));
                send_all_nothrow(client, std::string(buf, in.gcount()));
            }
        }

        close(client);
    }
    
}
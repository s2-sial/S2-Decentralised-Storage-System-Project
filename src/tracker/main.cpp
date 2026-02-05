#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <chrono>

static void die(const std::string& msg) {
    std::cerr << msg << ":" << std::strerror(errno) << "\n";
    std::exit(1);
}

//read until "\n" (very simple line based protocol)
static bool recv_line(int fd, std::string& out_line) {
    out_line.clear();
    char ch;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n == 0) return false;               //connection closed
        if (n < 0) return false;                //error (keep mvp simple)
        if (ch== '\n') break;
        if (ch == '\n') break;
        if (ch != '\r') out_line.push_back(ch); //ignore /r for windows clients
        if (out_line.size() > 4096) return false;   //basic safety capp
    }
    return true;
}

static void send_all(int fd, const std::string& s) {
    const char* p = s.c_str();
    size_t left = s.size();
    while (left > 0) {
        ssize_t n = ::send(fd, p, left, 0);
        if (n <=0) return; //MVP: ignore detailed error handling
        p += n;
        left -= static_cast<size_t>(n);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: tracker <port>\n";
        return 1;
    }
    int port = std::stoi(argv[1]);

    // 1) Create listening socket
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) die("socket");

    //allow quick restart on same port
    int yes = 1;
    if (::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        die("setsockopt(SO_REUSEADDR)");
    }

    //2) bind to 0.0.0.0:<port>
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        die("bind");
    }

    //3)listen
    if (::listen(listen_fd, 64) < 0) die("listen");

    std::cout << "Tracker listening on port " << port << "\n";

    //peers now replaced by map storing timestamps
    struct PeerInfo {
        std::chrono::steady_clock::time_point last_seen;
    };

    static std::unordered_map<std::string, PeerInfo> peers;
    //key format: "ip:port"

    while (true) {
        //4) accept a connection
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            //accept can fail transiently; keep running
            continue;
        }

        //determine the connecting client's IP (useful default if REGISTER omits IP later)
        char ipbuf[INET_ADDRSTRLEN]{};
        const char* ipstr = ::inet_ntop(AF_INET, &client_addr.sin_addr, ipbuf, sizeof(ipbuf));
        std::string remote_ip = ipstr ? ipstr : "unknown";

        //5) Read one command line
        std::string line;
        if (!recv_line(client_fd, line)) {
            ::close(client_fd);
            continue;
        }

        //6) parse and respond
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "REGISTER") {
            std::string ip;
            int p = 0;
            iss >> ip >> p;

            if (ip.empty() || p <= 0 || p > 65535) {
                send_all(client_fd, "ERR invalid REGISTER. Use: REGISTER <ip> <port>\n");
            } else {
                std::string entry = ip + ":" + std::to_string(p);
                peers[entry].last_seen = std::chrono::steady_clock::now();

                std::cout << "Registered peer " << entry << "\n";
                send_all(client_fd, "OK\n");
            }
        }
        else if (cmd == "HEARTBEAT") {
            std::string ip;
            int p = 0;
            iss >> ip >> p;

            if (ip.empty() || p <= 0 || p > 65535) {
                send_all(client_fd, "ERR invalid HEARTBEAT. Use: HEARTBEAT <ip> <port>\n");
            } else {
                std::string entry = ip + ":" + std::to_string(p);
                peers[entry].last_seen = std::chrono::steady_clock::now();
                send_all(client_fd, "OK\n");
            }
        }

        else if (cmd == "GET_PEERS") {
            auto now = std::chrono::steady_clock::now();
            const auto TTL = std::chrono::seconds(30);

            //prune stale peers
            for (auto it = peers.begin(); it != peers.end(); ) {
                if (now - it->second.last_seen > TTL) it = peers.erase(it);
                else ++it;
            }

            //output alive peers
            std::string out;
            for (const auto& kv : peers) {
                out += kv.first + "\n"; //kv.first is "ip:port"
            }
            if (out.empty()) out = "\n";
            send_all(client_fd, out);
        } else {
            send_all(client_fd, "ERR unknown command\n");
        }
        
        ::close(client_fd);
    }

}
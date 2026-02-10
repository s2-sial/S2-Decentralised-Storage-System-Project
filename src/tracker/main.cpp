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

//peers now replaced by map storing timestamps
struct PeerInfo {
    std::chrono::steady_clock::time_point last_seen;
};

static void prune_stale(
    std::unordered_map<std::string, PeerInfo>& peers,
    std::chrono::seconds ttl
) {
    auto now = std::chrono::steady_clock::now();
    for (auto it = peers.begin(); it != peers.end(); ) {
        if (now - it->second.last_seen > ttl) it = peers.erase(it);
        else ++it;
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


    static std::unordered_map<std::string, std::unordered_set<std::string>> chunk_index;
    // chunk_id -> set of "ip:port"

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

        std::cout << "[TRACKER] line='" << line << "'\n";

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

        else if (cmd == "ANNOUNCE") {
            std::string chunk_id, ip;
            int p = 0;
            iss >> chunk_id >> ip >> p;
            auto now = std::chrono::steady_clock::now();

            std::string entry = ip + ":" + std::to_string(p);
            std::cout << "[TRACKER] ANNOUNCE chunk=" << chunk_id << " peer=" << entry << "\n";

            if (chunk_id.empty() || ip.empty() || p <= 0 || p > 65535) {
                send_all(client_fd, "ERR invalid ANNOUNCE. Use: ANNOUNCE <chunk_id> <ip> <port>\n");
            } else {
                const auto TTL = std::chrono::seconds(30);
                auto pit = peers.find(entry);
                if (pit == peers.end() || (now - pit->second.last_seen > TTL)) {
                    std::cerr << "[TRACKER] ANNOUNCE rejected(peer not alive)\n";
                    chunk_index[chunk_id].insert(entry); //temporary accept announces
                    send_all(client_fd, "OK\n");
                } else {
                    chunk_index[chunk_id].insert(entry);
                    std::cout << "[TRACKER] ANNOUNCE accepted\n";
                    send_all(client_fd, "OK\n");
                }
                
            }
        }
        else if(cmd == "WHERE") {
            std::string chunk_id;
            iss >> chunk_id;
            std::cout << "[TRACKER] WHERE chunk=" << chunk_id << "\n";

            if (chunk_id.empty()) {
                send_all(client_fd, "ERR invalid WHERE. Use: WHERE <chunk_id>\n");
            } else {
                const auto TTL = std::chrono::seconds(30);
                auto now = std::chrono::steady_clock::now();

                std::string out;

                auto it = chunk_index.find(chunk_id);
                if (it != chunk_index.end()) {
                    //return only alive peers
                    for (const auto& entry : it->second) {
                        auto pit = peers.find(entry);
                        if (pit != peers.end() && (now - pit->second.last_seen <= TTL)) {
                            out += entry + "\n";
                        }
                    }
                }

                if (out.empty()) out = "\n";
                send_all(client_fd, out);
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
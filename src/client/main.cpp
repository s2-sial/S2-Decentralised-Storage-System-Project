#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/sha.h>
#include <iomanip>
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

static bool is_timeout_errno() {
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

static void die(const std::string& msg) {
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(1);
}

struct ClientConfig {
    std::string tracker_ip = "127.0.0.1";
    int tracker_port = 9000;
    int replica_count = 2;
    size_t chunk_size = 1024 * 1024; // 1mb
};





static void send_all(int fd, const char* buf, size_t len) {
    while (len > 0) {
        ssize_t n = ::send(fd, buf, len, 0);
        if (n <= 0) die("send");
        buf += n;
        len -= static_cast<size_t>(n);
    }
}

static std::string recv_all_text(int fd) {
    std::string out;
    char buf[4096];
    while (true) {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n == 0) break;
        if (n < 0) {
            if(is_timeout_errno()) break; // stop waiting forever
            die("recv");
        }
        out.append(buf, buf + n);
    }
    return out;
}

static int connect_tcp_fatal(const std::string& ip, int port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die("socket");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
        die("connect");

    return sock;
}

static int connect_tcp_try(const std::string& ip, int port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    set_timeouts(sock, 8000, 8000);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}


static std::vector<std::pair<std::string,int>> get_peers(const std::string& tracker_ip, int tracker_port) {
    int sock = connect_tcp_fatal(tracker_ip, tracker_port);
    std::string msg = "GET_PEERS\n";
    send_all(sock, msg.c_str(), msg.size());
    std::string resp = recv_all_text(sock);
    ::close(sock);

    std::vector<std::pair<std::string,int>> peers;
    std::istringstream iss(resp);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string ip = line.substr(0, pos);
        int port = std::stoi(line.substr(pos + 1));
        peers.push_back({ip, port});
    }
    return peers;
}


static void put_chunk_to_peer(const std::string& peer_ip, int peer_port,
                              const std::string& chunk_id,
                              const std::vector<char>& data) {
    int sock = connect_tcp_try(peer_ip, peer_port);
    if (sock < 0) {
        std::cerr << "Connect failed to " << peer_ip << ":" << peer_port << "\n";
        return;
    }

    if (!data.empty()) send_all(sock, data.data(), data.size());

    // Read small response (OK/ERR). Peer sends "OK\n" in your code.
    char buf[16]{};
    ssize_t n = ::recv(sock, buf, sizeof(buf)-1, 0);
    ::close(sock);

    if (n <= 0 || std::string(buf).rfind("OK", 0) != 0) {
        std::cerr << "Upload failed to " << peer_ip << ":" << peer_port << " for " << chunk_id << "\n";
        // MVP: just warn. Later: retry different peer.
    }
}

static std::string sha256(const std::vector<char>& data) {
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

static bool get_chunk_from_peer(const std::string& peer_ip, int peer_port,
                                const std::string& chunk_id,
                                std::vector<char>& out_data) {
    int sock = connect_tcp_try(peer_ip, peer_port);
    if (sock < 0) return false;


    std::string req = "GET_CHUNK " + chunk_id + "\n";
    send_all(sock, req.c_str(), req.size());

    // Read header line
    std::string header;
    char ch;
    while (true) {
        ssize_t n = recv(sock, &ch, 1, 0);
        if (n <= 0) { close(sock); return false; }
        if (ch == '\n') break;
        if (ch != '\r') header.push_back(ch);
    }

    std::istringstream iss(header);
    std::string status;
    iss >> status;

    if (status != "OK") {
        close(sock);
        return false;
    }

    size_t size;
    iss >> size;

    out_data.resize(size);
    size_t received = 0;

    while (received < size) {
        ssize_t n = recv(sock, out_data.data() + received,
                         size - received, 0);
        if (n <= 0) break;
        received += n;
    }

    close(sock);
    return received == size;
}

static void usage(const char* prog) {
    std::cerr
    << "Usage:\n"
    << " " << prog << "put <tracker_ip> <tracker_port> <file_path> [chunk_size_bytes] [replicas]\n"
    << " " << prog << "get <tracker_ip <tracker_port> <manifest> <output_file>\n";
}


int cmd_put(const ClientConfig& cfg, const std::string& file_path) {
    auto peers = get_peers(cfg.tracker_ip, cfg.tracker_port);
    if (peers.empty()) {
        std::cerr << "No peers available from tracker.\n";
        return 1;
    }

    int replicas = cfg.replica_count;
    if (replicas > (int)peers.size()) replicas = (int)peers.size();

    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file: " << file_path << "\n";
        return 1;
    }

    std::vector<std::string> chunk_ids;
    uint64_t chunk_index = 0;

    while (true) {
        std::vector<char> buf(cfg.chunk_size);
        in.read(buf.data(), buf.size());
        std::streamsize got = in.gcount();
        if (got <= 0) break;
        buf.resize(static_cast<size_t>(got));

        std::string cid = sha256(buf);

        chunk_ids.push_back(cid);

        // Deterministic peer selection (simple ring): start = chunk_index % peers.size()
        size_t start = static_cast<size_t>(chunk_index % peers.size());

        std::cout << "Uploading " << cid << " (" << buf.size() << " bytes) to " << replicas << " peers...\n";
        for (int r = 0; r < replicas; r++) {
            size_t idx = (start + (size_t)r) % peers.size();
            put_chunk_to_peer(peers[idx].first, peers[idx].second, cid, buf);
        }

        chunk_index++;
    }

    // Write manifest next to the file
    std::filesystem::path p(file_path);
    std::string manifest_path = p.filename().string() + ".manifest.txt";

    std::ofstream man(manifest_path);
    if (!man) {
        std::cerr << "Cannot write manifest:" << manifest_path << "\n";
        return 1;
    }

    man << "filename " << p.filename().string() << "\n";
    man << "chunk_size " << cfg.chunk_size << "\n";
    man << "chunks " << chunk_ids.size() << "\n";
    for (const auto& cid : chunk_ids) man << cid << "\n";

    std::cout << "DONE. Manifest written to " << manifest_path << "\n";
    return 0;
}

int cmd_get(const ClientConfig& cfg, const std::string& manifest_path, const std::string& out_path) {
    auto peers = get_peers(cfg.tracker_ip, cfg.tracker_port);
    if (peers.empty()) {
        std::cerr << "No peers available\n";
        return 1;
    }

    std::ifstream man(manifest_path);
    if (!man) {
        std::cerr << "Cannot open manifest\n";
        return 1;
    }

    std::string filename, label;
    size_t chunk_size, chunk_count;

    man >> label >> filename;
    man >> label >> chunk_size;
    man >> label >> chunk_count;

    std::vector<std::string> chunks(chunk_count);
    for (size_t i = 0; i < chunk_count; i++)
        man >> chunks[i];


    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot open output file\n";
        return 1;
    }

    for (const auto& cid : chunks) {
        bool found = false;

        for (const auto& peer : peers) {
            std::vector<char> data;
            if (!get_chunk_from_peer(peer.first, peer.second, cid, data))
                continue;

            // Verify integrity
            std::string computed = sha256(data);
            if (computed != cid) {
                std::cerr << "Hash mismatch for chunk " << cid << "\n";
                continue;
            }

            out.write(data.data(), data.size());
            found = true;
            break;
        }

        if (!found) {
            std::cerr << "Failed to retrieve chunk " << cid << "\n";
            return 1;
        }
    }

    std::cout << "Download complete: " << out_path << "\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }
    std::string mode = argv[1];
    try {
        if (mode == "put") {
            if (argc < 5) { usage(argv[0]); return 1; }

            ClientConfig cfg;
            cfg.tracker_ip = argv[2];
            cfg.tracker_port = std::stoi(argv[3]);
            std::string file_path = argv[4];

            if (argc>= 6) cfg.chunk_size = static_cast<size_t>(std::stoul(argv[5]));
            if (argc >= 7) cfg.replica_count = std::stoi(argv[6]);
            if (cfg.replica_count < 1) cfg.replica_count = 1;

            return cmd_put(cfg, file_path);
        }
    
        if (mode == "get") {
            if (argc < 6) { usage(argv[0]); return 1; }

            ClientConfig cfg;
            cfg.tracker_ip = argv[2];
            cfg.tracker_port = std::stoi(argv[3]);

            std::string manifest_path = argv[4];
            std::string out_path = argv[5];

            return cmd_get(cfg, manifest_path, out_path);
        }

        //usage(argv[0]);
        //return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    if (mode == "put") {
    // existing put code (unchanged)
}

else {
    std::cerr << "Unknown command\n";
    return 1;
}

}

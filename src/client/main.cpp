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

static void die(const std::string& msg) {
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(1);
}

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
        if (n < 0) die("recv");
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

    std::string header = "PUT_CHUNK " + chunk_id + " " + std::to_string(data.size()) + "\n";
    send_all(sock, header.c_str(), header.size());
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


int main(int argc, char** argv) {
    // Usage:
    // client put <tracker_ip> <tracker_port> <file_path> [chunk_size] [replicas]
    if (argc < 5) {
        std::cerr << "Usage:\n"
                  << "  client put <tracker_ip> <tracker_port> <file_path> [chunk_size_bytes] [replicas]\n";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "put") {
    // existing put code (unchanged)
}
else if (mode == "get") {
    // Usage:
    // client get <tracker_ip> <tracker_port> <manifest> <output_file>

    if (argc < 6) {
        std::cerr << "Usage: client get <tracker_ip> <tracker_port> <manifest> <output_file>\n";
        return 1;
    }

    std::string tracker_ip = argv[2];
    int tracker_port = std::stoi(argv[3]);
    std::string manifest_path = argv[4];
    std::string out_path = argv[5];

    auto peers = get_peers(tracker_ip, tracker_port);
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

    man.close();

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

    out.close();
    std::cout << "Download complete: " << out_path << "\n";
    return 0;
}
else {
    std::cerr << "Unknown command\n";
    return 1;
}


    std::string tracker_ip = argv[2];
    int tracker_port = std::stoi(argv[3]);
    std::string file_path = argv[4];

    size_t chunk_size = (argc >= 6) ? static_cast<size_t>(std::stoul(argv[5])) : (1024 * 1024);
    int replicas = (argc >= 7) ? std::stoi(argv[6]) : 2;

    if (replicas < 1) replicas = 1;

    auto peers = get_peers(tracker_ip, tracker_port);
    if (peers.empty()) {
        std::cerr << "No peers available from tracker.\n";
        return 1;
    }
    if (replicas > (int)peers.size()) replicas = (int)peers.size();

    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file: " << file_path << "\n";
        return 1;
    }

    std::vector<std::string> chunk_ids;
    uint64_t chunk_index = 0;

    while (true) {
        std::vector<char> buf(chunk_size);
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
    man << "filename " << p.filename().string() << "\n";
    man << "chunk_size " << chunk_size << "\n";
    man << "chunks " << chunk_ids.size() << "\n";
    for (const auto& cid : chunk_ids) man << cid << "\n";
    man.close();

    std::cout << "DONE. Manifest written to " << manifest_path << "\n";
    return 0;
}

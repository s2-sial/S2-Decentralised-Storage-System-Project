#include <iostream>

int main(int argc, char** argv) {
    if (argc !=5) {
        std::cerr << "Usage: peer <peer_port> <tracker_ip> <tracker_port> <storage_dir>\n";
        return 1;
    }
    std::cout << "Peer starting on port " << argv[1]
    << " tracker=" << argv[2] << ":" << argv[3]
    << " store=" << argv[4] << "\n";
    // finish: register with tracker + listen for PUT/GET
    return 0;
}
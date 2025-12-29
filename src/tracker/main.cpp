#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: tracker <port>\n";
        return 1;
    }
    std::cout << "Tracker starting on port " << argv[1] << "\n";
    // TODO: listen socket + REGISTER/GET_PEERS
    return 0;
}
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
        << " client put <tracker_ip> <tracker_port> <file_path>\n"
        << " client get <tracker_ip> <tracker_port> <manifest> <out_path>\n";
        return 1;
    }

    std::string cmd = argv[1];
    std::cout << "Client cmd: " << cmd << "\n";
    // finish: implement put/get
    return 0;
}
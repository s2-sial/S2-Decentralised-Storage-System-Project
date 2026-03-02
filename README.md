How to use 
1. Open ubuntu WSL and open the root directory using cd 
2. build the program with command cmake --build build -j
3. open another terminal for how many peers you want e.g 1, 2, 3
4. in that terminal run the peer program with command ./build/peer 9101 ./store1
5. for uploading and downloading open up another terminal for the client 
6. for uploading use the command ./build/client put 127.0.0.1:9101,127.0.0.1:9102 <location for file> 1024 <replicas> . the output for the file will be printed, you can use that to download
7. for downloading use the command ./build/client get 127.0.0.1:9101,127.0.0.1:9102 <manifest file> <new file name>

Repair
- Tracker-based automatic repair is disabled in DHT mode; `client repair` will print a message and exit.
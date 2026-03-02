How to use 
1. Open ubuntu WSL and open the root directory using cd 
2. build the program with command cmake --build build -j
3. start the tracker using ./build/tracker 9000(port)
4. open another terminal for how many peers you want e.g 1, 2, 3
5. in that terminal run the peer program with command ./build/peer 9101(port example) 127.0.0.1 9000 ./store1
6. for uploading and downloading open up another terminal for the client 
7. for uploading use the command ./build/client put 127.0.0.1 9000 <location for file> 1024 <how many peers> . the output for the file will be printed, you can use that to download
8. for downloading use the command ./build/client get 127.0.0.1 9000 <output file/file youre downloading> <new file name>

How to run repair rereplication
1. Start tracker + 2 peers
2. client put ... replicas 2 
3. kill peer 2, wait > TTL (until the time that a heartbeat is sent)
4. start a new peer 3 on a different port and storage directory and let it register
5. run ./client repair 127.0.0.1 9000 2 50
what happens:
-client will copy chunks from the remaining peer to the new peer
-tracker chunk index updates via announce
need_repair eventually returns empty
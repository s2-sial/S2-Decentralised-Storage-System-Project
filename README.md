How to use 
1. Open ubuntu WSL and open the root directory using cd 
2. build the program with command cmake --build build -j
3. start the tracker using ./build/tracker 9000(port)
4. open another terminal for how many peers you want e.g 1, 2, 3
5. in that terminal run the peer program with command ./build/peer 9101(port example) 127.0.0.1 9000 ./store1
6. for uploading and downloading open up another terminal for the client 
7. for uploading use the command ./build/client put 127.0.0.1 9000 <location for file> 1024 <how many peers> . the output for the file will be printed, you can use that to download
8. for downloading use the command ./build/client get 127.0.0.1 9000 <output file/file youre downloading> <new file name>
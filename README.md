# shared-memory-processes
Processes share different buffers with use of semaphores

## Prerequisites
You must have `g++` compiler installed.

## How to run
Move into folder where files are in and firstly compile and run `server.cpp`:
```
g++ server.cpp -o server -lrt -pthread
./server
```

Then compile and run `client.cpp`:
```
g++ client.cpp -o client -lrt -pthread
./client
```

## Authors
Gianpio Sozzo
Gianluca Bonifazi
Mattia Campeggi

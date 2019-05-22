# udp-client-server

A simple UDP client/server system for server to pick top 90% of shortest messages from all messages from a client.

## Summary:

Server runs in multi-threaded mode, where the main thread spawns off
all the worker thread, and wait them to terminate. In test mode, the
threads will terminate. In UDP server mode, the server and its threads
never terminate until forced by `ctl-c`.

The worker threads totaled 11, one of which is the reader of UDP
socket. All others are message processer.

## Usage:

To compile:
```
make all
```
Then first run the server:
```
./server 9090
```
Afterward, run any number of clients:
```
./client localhost 9090 6
```
which will read from a file `client-data/messages-6.txt`, and send line
by line to the server at localhost:9090.

## System testing:

For convenience of testing, a script is provided:

```
./multi-client-test.sh
```

will create multiple simultanerous clients to send UDP traffic to the
server.

Each line is prefixed with the client ID, e.g. <6:> as above. Any
empty line such as "6:" will indicate the end of the line stream.

The server is unlikely to terminate, unless user intervene. Client
will terminate quickly at each run once finish sending the file.

## Unit test:

The server can be unit tested if compiled with -DTEST=1. In that case,
the server can ran using the internal test cases, and produce the same
results in `server-data/`.

## Stress test:

`./stress-test.sh` will create 4000 clients to send traffic at sporatic
random intervals between [0, 1] millisecond. Performance could be
limited also by file system IOs.


# Included files

## Client
- chatclient.c (source file for client code)
- Makefile (run "make" to generate the executable, "chatclient")
- pg3lib.h (includes encryption library)

## Server
- chatserver.c (source file for server code)
- Makefile (run "make" to generate the executable, "chatserver")
- pg3lib.h (includes encryption library)
- users.txt (list of users and their passwords)
- activeusers.txt (list of active users, their sockets, and their public keys, filled during runtime)

# Commands to run

## Client
$ make
$ ./chatclient <server> <port> <username>

## Server
$ make
$ ./chatserver <port>

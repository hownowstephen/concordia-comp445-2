# Assignment 2 Comp 445

Author: Stephen Young, st_youn at encs.concordia.ca
Student ID: 9736247

The application is a routed FTP client/server application over UDP/winsock

Included are the following files:

client.cpp - The FTP client
server.cpp - The FTP server
protocol.cpp - A custom file transfer protocol library
Makefile - The makefile, optimized for MinGW/GNU

as well as the necessary router.h, router.cpp - Packet-dropping router application

The application was made to be built using MinGW on windows, and was built and
developed under Cygwin. It should work as well with Visual Studio, but is best
compiled using the GCC c++ compiler (g++) and GNU Make

Basic build: make clean && make all
Server application: ./server.exe
Client application: ./client.exe

The server assumes the following:

* Buffer size of 2048 bytes
* Hostname does not exceed 11 characters
* Filename and Username must be <= 128 characters
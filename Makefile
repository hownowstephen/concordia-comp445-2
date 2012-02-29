# Project: server

CPP  = g++.exe
CC   = gcc.exe
LINKCLI = client.o
LINKSRV  = server.o
OBJ = $(LINKCLI) $(LINKSRV) protocol.o
LIBS = -lwsock32
BINSRV = server
BINCLI = client
RM = rm -f

all: server client

clean:
	${RM} $(OBJ) $(BINSRV) $(BINCLI)

server: server.o protocol.o
	$(CPP) $(LINKSRV) -o $(BINSRV) $(LIBS)

client: client.o protocol.o
	$(CPP) $(LINKCLI) -o $(BINCLI) $(LIBS)

client.o: client.cpp
	$(CPP) -c client.cpp -o $(LINKCLI) $(LIBS)

server.o: server.cpp
	$(CPP) -c server.cpp -o $(LINKSRV) $(LIBS)

protocol.o: protocol.cpp
	$(CPP) -c protocol.cpp -o protocol.o $(LIBS)
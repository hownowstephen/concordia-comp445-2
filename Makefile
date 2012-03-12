# Project: server

CPP  = g++.exe
CC   = gcc.exe
LINKCLI = client.o
LINKSRV  = server.o
LINKRT = router.o
OBJ = $(LINKCLI) $(LINKSRV) $(LINKRT) protocol.o
LIBS = -lwsock32
BINSRV = server
BINCLI = client
BINRT = router
RM = rm -f

all: clean refresh update

clean:
	${RM} $(OBJ) $(BINSRV) $(BINCLI)

refresh:
	git pull origin master

update: server client router

server: server.o protocol.o
	$(CPP) $(LINKSRV) -o $(BINSRV) $(LIBS)

client: client.o protocol.o
	$(CPP) $(LINKCLI) -o $(BINCLI) $(LIBS)

router: router.o
	$(CPP) $(LINKRT) -o $(BINRT) $(LIBS)

client.o: client.cpp
	$(CPP) -c client.cpp -o $(LINKCLI) $(LIBS)

server.o: server.cpp
	$(CPP) -c server.cpp -o $(LINKSRV) $(LIBS)

protocol.o: protocol.cpp
	$(CPP) -c protocol.cpp -o protocol.o $(LIBS)

router.o: router.cpp
	$(CPP) -c router.cpp -o router.o $(LIBS)
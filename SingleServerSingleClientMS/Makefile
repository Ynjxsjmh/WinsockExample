CC = g++
CFLAGS = -std=c++11
LIBS =  -lwsock32 -lWs2_32

all: client server

server: server.cpp
	$(CC) $(CFLAGS) server.cpp  server.o -o server $(LIBS)

client: client.cpp
	$(CC) $(CFLAGS) client.cpp client.o -o client $(LIBS)

clean:
	rm -f *.o server client
CPPFLAGS=-g -pthread -std=c++0x -DTEST=1 -I./

all: server client

clean:
	rm client server *~

server: server.cpp server.hpp
	g++ -o server $(CPPFLAGS) server.cpp

client: client.cpp client.hpp
	g++ -o client $(CPPFLAGS) client.cpp

cscope:
	rscan all

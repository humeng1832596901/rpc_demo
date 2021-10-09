CXX := g++
CFLAGS := -pthread -O3 -Wall -g -DBOOST_BIND_GLOBAL_PLACEHOLDERS -std=c++14
.PHONY : clean

all: server client

server: server.o
	${CXX} -o server server.o ${CFLAGS}

client: client.o
	${CXX} -o client client.o ${CFLAGS}

server.o: server.cpp
	${CXX} -c server.cpp -o server.o ${CFLAGS}

client.o: client.cpp
	${CXX} -c client.cpp -o client.o ${CFLAGS}

clean:
	rm server.o
	rm client.o
	rm server
	rm client

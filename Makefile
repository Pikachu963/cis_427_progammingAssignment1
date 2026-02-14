# Compiler and flags [cite: 173-178]
CXX = g++
CXXFLAGS = -lpthread -ldl

# Targets
all: server client

server: server.cpp sqlite3.o
	$(CXX) server.cpp sqlite3.o $(CXXFLAGS) -o server

client: client.cpp
	$(CXX) client.cpp -o client

# Rule to compile sqlite3 object if needed [cite: 173-178]
sqlite3.o: sqlite3.c
	gcc -c sqlite3.c -o sqlite3.o

clean:
	rm -f server client sqlite3.o stocks.db
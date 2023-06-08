ALL=server client
#ALL=server #client
all: $(ALL)
server: server.cpp socket.cpp database.cpp pugi.cpp pugixml/pugixml.cpp request.cpp
	g++ -std=gnu++11 -g -o $@ $^ -lpqxx -lpq -lpthread

client: client.cpp socket.cpp database.cpp pugi.cpp pugixml/pugixml.cpp request.cpp
	g++ -std=gnu++11 -g -o $@ $^ -lpqxx -lpq -lpthread

# client: client.cpp socket.cpp
# 	g++ -std=gnu++11 -g -o $@ $^ -lpqxx -lpq -lpthread

.PHONY: clean
clean:
	rm -f *~ $(ALL)
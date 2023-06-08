ALL=client
all: $(ALL)

client: client.cpp socket.cpp pugixml/pugixml.cpp request.cpp
	g++ -std=gnu++11 -g -o $@ $^ -lpqxx -lpq -lpthread

.PHONY: clean
clean:
	rm -f *~ $(ALL)
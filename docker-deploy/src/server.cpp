// sudo docker-compose up

#include "socket.hpp"
#include "database.hpp"
#include "pugi.hpp"
#define PORT "12345"

using namespace std;
using namespace pqxx;


int main() {
    try{
        // initiate database
        Database DB;
        DB.init();

        // initiate server
        Server s = Server(PORT);

        while (1){
            // accept client connection
            string client_host;
            int client_connection_fd = s.serverAccept(&client_host);
            if (client_connection_fd == -1) {
                std::cout<< "ERROR: cannot accept connection" << endl;
                continue;
            }

            cout<<"client_connection_fd: "<<client_connection_fd<<endl;

            thread([&] {recv_request(client_connection_fd, std::ref(DB));}).detach();

        }
    }
    catch (const std::exception &e) {
        cerr << e.what() << std::endl;
    }

    return 0;
}





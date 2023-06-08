#include "socket.hpp"
#include "pugixml/pugixml.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "pugi.hpp"
using namespace std;


int main(int argc, char *argv[]){
    // cout << "client running...\n";
    const char * hostname = argv[1];
    Client c = Client(hostname,"12345");
    int master_fd = c.socketfd;

    // send xml request to server
    // cout << "send xml request to server\n";
    std::string f_name = "xml_test/request_create.xml";
    send_xml(f_name, master_fd);

    // receive response from the server
    // receive the size of the response

    std::string server_response = recv_response_xml(master_fd);

    //check
    // cout << "server respond : " << endl;
    // std::cout << server_response << std::endl;

    // send xml request to server
    // cout << "send xml request to server\n";
    std::string f_name1 = "xml_test/request_tran.xml";
    send_xml(f_name1, master_fd);

    std::string server_response1 = recv_response_xml(master_fd);

    //check
    // cout << "server respond 1: " << endl;
    // std::cout << server_response1 << std::endl;

    return 0;
}
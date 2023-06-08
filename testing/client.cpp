#include "socket.hpp"
#include "pugixml/pugixml.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "pugi.hpp"
#include <ctime>
#include <chrono>
#include <ratio>
using namespace std;

int main(int argc, char *argv[]){
    // 1. get initial time
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    if (argc == 1){
        cout<<"Invalid input, should be: ./client requestNum\n";
        return 0;
    }

    // get the value of request number
    long ReqstNum = atol(argv[1]); 
    stringstream create;
    size_t f_size = 0;

    try{
        //2. connect to the server
        const char * hostname = "vcm-32428.vm.duke.edu";//"127.0.0.1";//"vcm-32426.vm.duke.edu";////argv[2];
        Client c = Client(hostname,"12345");
        cout << "client connected...\n";
        int master_fd = c.socketfd;
        
        //create account
        std::string f_name = "xml_test/sca-create.xml";
        send_xml(f_name, master_fd);
        std::string server_response = recv_response_xml(master_fd);        
        // std::cout << server_response << std::endl;

        // Test for single thread with different core
        for (long i = 1; i<=ReqstNum;i++){
            //3. send xml request to server
            f_name = "xml_test/sca-trans.xml";
            send_xml(f_name, master_fd);

            //4. receive the size and the response from the server
            std::string server_response = recv_response_xml(master_fd);
            // std::cout << server_response << std::endl;
        }

        // 5. get final time and output
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        cout<<"The whole process took "<<time_span.count()<<"seconds."<<endl;
        
    }catch(const exception e){
        cerr<<e.what()<<endl;
    }

    return 0;
}
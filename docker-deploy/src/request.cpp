#include "pugi.hpp"

std::string recv_xml(int socket_fd){
    // receive the buffer data
    std::string buffer_str = "";
    // const char * buffer_res;
    // delete[]
    char * buffer = new char[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int req_len = 0, tp = 0;

    tp = recv(socket_fd, buffer, BUFFER_SIZE, 0);
    while (tp > 0) {
        buffer_str.append(buffer, tp);
        req_len += tp;
        // reset the buffer
        memset(buffer, 0, BUFFER_SIZE);

        if (buffer_str.find("</") != std::string::npos){
            break;
        }
    }

    // buffer_res = buffer_str.c_str();
    
    delete[] buffer;

    size_t at = buffer_str.find('\n');

    return buffer_str.substr(at + 1, buffer_str.length() - at);
}

std::string recv_response_xml(int socket_fd){
    // receive the buffer data
    std::string buffer_str = "";
    // const char * buffer_res;
    // delete[]
    char * buffer = new char[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int req_len = 0, tp = 0;

    tp = recv(socket_fd, buffer, BUFFER_SIZE, 0);
    while (tp > 0) {
        buffer_str.append(buffer, tp);
        req_len += tp;
        // reset the buffer
        memset(buffer, 0, BUFFER_SIZE);

        if (buffer_str.find("</") != std::string::npos){
            break;
        }
    }

    // buffer_res = buffer_str.c_str();
    
    delete[] buffer;

    // size_t at = buffer_str.find('\n');

    return buffer_str;
}

void send_xml(std::string f_name, int master_fd){
    // load the xml file
    std::ifstream infile(f_name); // std::ios::binary
    if (!infile){
        std::cout << "Cannot load the file" << std::endl;
        return;
    }
    // get the size of the file
    infile.seekg(0, std::ios::end);
    size_t f_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::string buffer((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    buffer = to_string(f_size) + '\n' + buffer;
    size_t buffer_size = buffer.size();

    //check
    // cout << "client sent" << endl;
    // cout << buffer << endl;

    // send the file
    if (send(master_fd, buffer.c_str(), buffer_size, 0) < 0){
        std::cout << "Cannot send" << std::endl;
        return;
    }
}

void send_string(std::string msg_string, int master_fd){
    size_t msg_size = msg_string.size();

    // send the string
    if (send(master_fd, msg_string.c_str(), msg_size, 0) < 0){
        std::cout << "Cannot send" << std::endl;
        return;
    }
}

void recv_request(int client_connection_fd, Database & DB){
    while (1){
        // receive the buffer data
        std::string re_xml = recv_xml(client_connection_fd);

        if (re_xml.empty()){
            return;
        }
        // check
        // cout << "Server received: " << endl;
        // std::cout << re_xml << std::endl;

        // load the xml file
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(re_xml.c_str());

        // check whether parsed successfully
        if (!result){
            // send error xml back to the client
            pugi::xml_document error_response;
            pugi::xml_node results = error_response.append_child("results");
            pugi::xml_node error_msg = results.append_child("error");
            error_msg.text() = "XML parsed with errors";
            std::stringstream strstream;
            error_response.save(strstream, "\t", pugi::format_default);
            send_string(strstream.str(), client_connection_fd);
        }
        else{
            pugi::xml_node create = doc.child("create");
            pugi::xml_node transactions = doc.child("transactions");
            if (create){
                std::string create_response = pugi_create(doc, DB);
                // send response back to client
                send_string(create_response, client_connection_fd);
            }
            else if (transactions){
                std::string tran_response = pugi_transaction(doc, DB);
                // send response back to client
                send_string(tran_response, client_connection_fd);
            }
        }
    }
}
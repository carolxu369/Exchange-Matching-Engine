#ifndef __PUGI__H__
#define __PUGI__H__

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>
#include <mutex>

#include "pugixml/pugixml.hpp"
#include "database.hpp"
#define BUFFER_SIZE 65536
#define MAX_RES_HEADER_LEN 8192

std::string pugi_create(pugi::xml_document & doc, Database & DB);
std::string pugi_transaction(pugi::xml_document & doc, Database & DB);
std::string recv_xml(int socket_fd);
void send_xml(std::string f_name, int master_fd);
void send_string(std::string msg_string, int master_fd);
void recv_request(int client_connection_fd, Database & DB);
std::string recv_response_xml(int socket_fd);


#endif
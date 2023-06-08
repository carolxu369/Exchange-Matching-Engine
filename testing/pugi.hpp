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
#include <vector>
#include "pugixml/pugixml.hpp"
#define BUFFER_SIZE 65536
#define MAX_RES_HEADER_LEN 8192

std::string recv_xml(int socket_fd);
void send_xml(std::string f_name, int master_fd);
void send_string(std::string msg_string, int master_fd);
std::string recv_response_xml(int socket_fd);

#endif
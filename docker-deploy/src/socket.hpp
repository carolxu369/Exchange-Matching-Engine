#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <vector>
#include <algorithm>
#include <exception>
using namespace std;
// 0. socket let apps attach to local network ar different port
class Socket{
public:
    int status;
    int socketfd;
    const char * host;// e.g. host name or IP
    const char * port;// e.g. port number
    struct addrinfo hostInfo;
    struct addrinfo * hostInfo_list;// will point to the results
    Socket():status(0),socketfd(0),host(NULL),port(NULL),hostInfo_list(NULL){}
    Socket(const char * _h, const char * _p):status(0),socketfd(0),host(_h),port(_p),hostInfo_list(NULL){}
    ~Socket(){
      if (hostInfo_list!=NULL){
        freeaddrinfo(hostInfo_list);
      }
      if (socketfd!=0){
        close(socketfd);
      }
    }
    void printInfo();
};
// 1.server proccess
// Variable: const char * port
// 1.1 socket() -> 1.2 bind() -> 1.3 listen() 
// -> 1.4 accept() -> connect() -> 1.5 read() -> 1.6 write() ->close()
class Server: public Socket{
public:
    Server(const char * _p):Socket(NULL,_p){
        serverSetUp();
        // std::cout<<"server created successfully."<<std::endl;
    }
    void serverSetUp();
    int serverAccept(std::string *ip);
};

// 2.client process 
// Varaible: const char * host, port
// 2.1 connect() 
// 2.2 write() // 2.3 read
class Client: public Socket{
public:
    Client(const char * _h,const char * _p):Socket(_h,_p){
        clientSetUp();
        // std::cout<<"client created successfully."<<std::endl;
    }
    void clientSetUp();
};

class ErrMsg : public std::exception {
 private:
  std::string msg;
 public:
  ErrMsg(std::string _errMsg) : msg(_errMsg) {}
  virtual ~ErrMsg() throw() {}
  virtual const char * what() const throw() { return msg.c_str(); }
};
#endif
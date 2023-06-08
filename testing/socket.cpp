#include "socket.hpp"
void Socket::printInfo(){
//  Test:
    std::cout<<"status:  "<<status<<std::endl;
    std::cout<<"socketfd:"<<socketfd<<std::endl;
    std::cout<<"host:    "<<host<<std::endl;
    std::cout<<"port:    "<<port<<std::endl;
}

// for Server
void Server::serverSetUp(){
    // make sure the struct is empty
    memset(&hostInfo, 0, sizeof(hostInfo));
    // don't care IPv4 or IPv6
    hostInfo.ai_family = AF_UNSPEC;  
    // TCP stream sockets
    hostInfo.ai_socktype = SOCK_STREAM;
    // fill in my IP for me
    hostInfo.ai_flags = AI_PASSIVE;  
    // get status and error check
    status = getaddrinfo(host, port, &hostInfo, &hostInfo_list);
    if (status != 0) {
        // cerr << "ERROR cannot get address info for host" << endl;
        // cerr << "  (" << host << "," << port << ")" << endl;
        // exit(EXIT_FAILURE);
        throw(ErrMsg("ERROR getaddrinfo of server\n"));
    }
    // get the File Descriptor and error check
    socketfd = socket(hostInfo_list->ai_family,
                            hostInfo_list->ai_socktype,
                            hostInfo_list->ai_protocol);

    if (socketfd == -1) {
        throw(ErrMsg("ERROR in socket of server\n"));
    }
    // allow to reuse the port
    int yes=1;
    status = setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    if (status == -1){
        throw(ErrMsg("ERROR in setsockopt of server\n"));
    }
    // bind: decide on which port
    status = bind(socketfd, hostInfo_list->ai_addr, hostInfo_list->ai_addrlen);
    if (status == -1){
        throw(ErrMsg("ERROR in bind of server\n"));
    }    
    // listen: seconf argument is backlog which default limit is 20
    status = listen(socketfd, 10);
    if (status == -1){
        throw(ErrMsg("ERROR in listen of server\n"));
    }
}
int Server::serverAccept(std::string *ip){
    struct sockaddr_storage socket_addr;
    socklen_t addr_size;
    int new_fd;
    // now accept an incoming connection:
    addr_size = sizeof(socket_addr);
    new_fd = accept(socketfd, (struct sockaddr *)&socket_addr, &addr_size);
    if (new_fd == -1){
        throw(ErrMsg("ERROR in accept of server\n"));
    }
    // ready to communicate on socket descriptor new_fd!
    //convert binary ip to decimal ip
    struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
    *ip = inet_ntoa(addr->sin_addr);
    return new_fd;
}

// for Client
void Client::clientSetUp(){
    // make sure the struct is empty
    memset(&hostInfo, 0, sizeof(hostInfo));
    // don't care IPv4 or IPv6
    hostInfo.ai_family = AF_UNSPEC;  
    // TCP stream sockets
    hostInfo.ai_socktype = SOCK_STREAM;
    // get ready to connect
    status = getaddrinfo(host, port, &hostInfo, &hostInfo_list);
    //cout the client info about host and port
    // cout << "  (" << host << "," << port << ")" << endl;
    if (status != 0) {
        // throw(ErrMsg("ERROR in getaddrinfo of client\n"));
        cerr << "ERROR cannot get address info for Client host" << endl;
        cerr << "  (" << host << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    }
    socketfd = socket(hostInfo_list->ai_family,
                    hostInfo_list->ai_socktype,
                    hostInfo_list->ai_protocol);
    if (socketfd == -1) {
        throw(ErrMsg("ERROR in socket of client\n"));
    }
    status = connect(socketfd,hostInfo_list->ai_addr,hostInfo_list->ai_addrlen);
    if (status == -1) {
        cerr << "ERROR in connect of client\n" << endl;
        cerr << "  (" << host << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
        // throw(ErrMsg("ERROR in connect of client\n"));
    }
}
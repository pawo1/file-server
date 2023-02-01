#include <stdio.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>

#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>


#include <unordered_set>
#include <signal.h>

#include <nlohmann/json.hpp>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#include "../shared_lib/jsonTree.h"
#include "../shared_lib/utils.h"
#include "../shared_lib/protocolHandlerServer.h"
#include "../shared_lib/protocolSenderServer.h"


namespace fs = std::filesystem;
using json = nlohmann::json;

class Client;

int servFd;
int epollFd;
json fileSystemTree;
std::string root;
std::unordered_set<Client*> clients;

void ctrl_c(int);

void sendToAllBut(int fd, char * buffer, int count);

uint16_t readPort(char * txt);

void setReuseAddr(int sock);

void sendToAll(int fd, std::string name, char operation) ;

struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client : public Handler {
private:
    int _fd;
    ProtocolHandlerServer _protocolHandler;
    ProtocolSenderServer _protocolSender;
public:
    Client(int fd) : _fd(fd), _protocolHandler(&fileSystemTree, fd, root), _protocolSender(fd, root) {
        epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
        epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
        _protocolHandler.sendToAllPointer = &sendToAll;
    }
    virtual ~Client(){
        _protocolHandler.free();
        epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
    }
    int fd() const {return _fd;}

    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN) {
            if(!_protocolHandler.read(_fd))
                events |= EPOLLERR;
        }
        if(events & ~EPOLLIN){
            remove();
        }
    }
    void write(std::string name, char operation){
        if(!_protocolSender.send_message(name, operation))
            remove();
        
    }
    void remove() {
        printf("removing %d\n", _fd);
        clients.erase(this);
        delete this;
    }
};

class : Handler {
    public:
    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN){
            sockaddr_in clientAddr{};
            socklen_t clientAddrSize = sizeof(clientAddr);
            
            auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
            if(clientFd == -1) error(1, errno, "accept failed");
            
            printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
            
            clients.insert(new Client(clientFd));
        }
        if(events & ~EPOLLIN){
            error(0, errno, "Event %x on server socket", events);
            ctrl_c(SIGINT);
        }
    }
} servHandler;


int main()
{
    json mockup_configuration = json::parse(R"({"host": "localhost", "port": 1235, "path":"/home/pmarc/Dokumenty/"})");
    std::ifstream ifconf("config.json");
    //json configuration = mockup_configuration;
    json configuration = json::parse(ifconf);

    std::string host = configuration["host"];
    root =  configuration["path"];
    uint16_t port = configuration["port"]; 
    if(!fs::is_directory(root)) {
        std::cout << "Wrong path: " << root << std::endl;
        return 1;
    }
    
    fileSystemTree = parseDirectoryToTree(root);

    
    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if(servFd == -1) error(1, errno, "socket failed");
    
    signal(SIGINT, ctrl_c);
    signal(SIGPIPE, SIG_IGN);
    
    setReuseAddr(servFd);
    
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
    int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res) error(1, errno, "bind failed");
    
    res = listen(servFd, 1);
    if(res) error(1, errno, "listen failed");

    epollFd = epoll_create1(0);
    
    epoll_event ee {EPOLLIN, {.ptr=&servHandler}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, servFd, &ee);
    
    while(true){
        if(-1 == epoll_wait(epollFd, &ee, 1, -1)) {
            error(0,errno,"epoll_wait failed");
            ctrl_c(SIGINT);
        }
        ((Handler*)ee.data.ptr)->handleEvent(ee.events);
    }
    
    
    return 0;
}

void setReuseAddr(int sock){
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(res) error(1,errno, "setsockopt failed");
}

void ctrl_c(int){
    for(Client * client : clients)
        delete client;
    close(servFd);
    printf("Closing server\n");
    exit(0);
}

void sendToAll(int fd, std::string name, char operation) {
    std::cout << "SEND TO ALL WITH: fd: "<<fd<<" name: "<<name<<", op: "<<operation << std::endl;
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->fd()!=fd)
            client->write(name, operation);
    }
}



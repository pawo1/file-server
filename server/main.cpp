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

#include <linux/inotify.h>
#include <nlohmann/json.hpp>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#include "../shared_lib/jsonTree.h"
#include "../shared_lib/utils.h"

#define CLIENT_BUFFER 1024

namespace fs = std::filesystem;
using json = nlohmann::json;

class Client;

int servFd;
int epollFd;
json fileSystemTree;
std::unordered_set<Client*> clients;

void ctrl_c(int);

void sendToAllBut(int fd, char * buffer, int count);

uint16_t readPort(char * txt);

void setReuseAddr(int sock);

struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client : public Handler {
private:
    int _fd;


    char const_buffer[CLIENT_BUFFER];
    uint32_t const_head;

    uint32_t trans_size;
    uint32_t read_bytes;

    char *trans_buffer;


    void _moveBuffer(uint32_t offset) {
        if(const_head == 0) return;

        memcpy(trans_buffer+read_bytes, const_buffer+offset, const_head-offset);
        read_bytes += (uint32_t)(const_head - offset);
        const_head = 0;
    }

    void completeTransmission() {
        char msg_type = trans_buffer[0];
        switch(msg_type) {
            case 'D':
                break;
            case 'U':
                break;
            case 'B':
                
                break;
            default:
                std::cout << "Corrupted data from client " << _fd << std::endl;
                break;
        }
    }

public:
    Client(int fd) : _fd(fd) {
        epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
        epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
        read_bytes = 0;
        trans_size = 0;
        const_head = 0;
 
    }
    virtual ~Client(){
        if(trans_buffer != nullptr)
            delete [] trans_buffer;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
    }
    int fd() const {return _fd;}

    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN) {
            ssize_t count;
           

            count = read(_fd, 
                        const_buffer+const_head, 
                        std::min(CLIENT_BUFFER-const_head, 
                                (trans_size-(read_bytes+const_head))
                                )
                        );
            if(count > 0) {
                
                const_head += (uint32_t)count;
                if(trans_size == 0) {
                    if(const_head >= sizeof(uint32_t) ) {
                        trans_size = *(uint32_t*)const_buffer;
                        trans_buffer = new char[trans_size]();
                        _moveBuffer(sizeof(uint32_t));
                        
                    }
                } else {
                    if(const_head == CLIENT_BUFFER) 
                        _moveBuffer(0);

                    if(trans_size == (read_bytes+const_head)) {
                        _moveBuffer(0);
                        completeTransmission();
                        delete [] trans_buffer;
                        read_bytes = 0;
                        trans_size = 0;
                    }
                }
            } else {
                events |= EPOLLERR;
            }
        }
        if(events & ~EPOLLIN){
            remove();
        }
    }
    void write(char * buffer, int count){
        if(count != ::write(_fd, buffer, count))
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
    json mockup_configuration = json::parse(R"({"host": "localhost", "port": 1235, "path":"/home/pawo/Dokumenty/server-folder/"})");
    
    std::string host = mockup_configuration["host"];
    std::string path =  mockup_configuration["path"];
    uint16_t port = mockup_configuration["port"]; 
    if(!fs::is_directory(mockup_configuration["path"])) {
        std::cout << "Wrong path: " << mockup_configuration["path"] << std::endl;
        return 1;
    }
    
    fileSystemTree = parseDirectoryToTree(mockup_configuration["path"]);
    
    
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

void sendToAllBut(int fd, char * buffer, int count){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->fd()!=fd)
            client->write(buffer, count);
    }
}

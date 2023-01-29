#include "../shared_lib/protocolSender.h"
#include "../shared_lib/utils.h"

#include <stdio.h>
#include <cstdlib>
#include <sys/inotify.h>
#include <sys/select.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string>

#include <errno.h>
#include <error.h>

#include <signal.h>
#include <cstring>

#include <sys/epoll.h>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>

#include <openssl/md5.h>

#include <fstream>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <sys/stat.h>     

#include "../shared_lib/protocolHandlerClient.h"

std::string root;
int sock;

ProtocolSender *proto_sender;
ProtocolHandlerClient *proto_handler;

void ctrl_c(int);
uint16_t readPort(char * txt);
ssize_t readData(int, char * , ssize_t );
void writeData(int, char * , ssize_t );
int connect_to_address(char* address, char* port);

struct Handler {
    virtual int handleEvent(uint32_t event) = 0;
};

struct InotifyHandler :  Handler {
public:
    InotifyHandler(std::string root, ProtocolSender *proto_sender){
        init_notify(root);
        this->proto_sender = proto_sender;
    }
    ~InotifyHandler(){
        close(this->fd);
    }
    int getFd(){
        return this->fd;
    }
    virtual int handleEvent(uint32_t ee) override {       
        
        if(ee & EPOLLIN){
            
            char buf[const_buflen];
            int len, i = 0;

            len = read (this->fd, buf, const_buflen);
            if (len < 0) {
                perror ("read inotify_event");
                return 1;
            }
            while (i < len) {
                struct inotify_event *event = (struct inotify_event *) &buf[i];

                std::string name = event->name;
                // ignoruj pliki z rozszerzeniem .fstmp
                if (name.find(".fstmp") != std::string::npos) {
                    i += const_event_size + event->len;
                    continue;
                }

//                if(!last_name.empty() && (event->mask != IN_MOVED_TO) && != event->cookie){
//                    // move_from bez następnika move_to
//                    // oznacza usunięcie pliku
//                    
//                    operation_delete(name, event->wd);
//                    
//                    last_name = "";
//                }
                
                switch (event->mask){
                    case IN_MOVED_FROM:
                        printf("- IN_MOVED_FROM\n");
                        // to jeszcze nic nie znaczy
                        if (event->len){ 
                            cookie = event->cookie;
                            last_name = name; 
                            last_wd = event->wd;
                            operation_delete(name, event->wd);
                        }
                        break;
                    case IN_MOVED_TO:
                        printf("- IN_MOVED_TO\n");
                        if (event->len){ 
//                            if(event->cookie == cookie){
//                                // niepusta nazwa poprzednika i teraz nowa
//                                // wykrycie zmiany nazwy 
//                                
//                                operation_rename(last_name, name, event->wd);
//                            }
//                            else {
                                // pusta nazwa poprzednika i teraz nowa
                                // wykrycie utworzenia
                                operation_create(name, event->wd);
//                            }
                        }
                        cookie = 0;
                        break;
                    case IN_CLOSE_WRITE:
                        printf("- IN_CLOSE_WRITE\n");
                        if (event->len){                             
                            // wykrycie modyfikacji
                            operation_create(name, event->wd);
                        }
                        cookie = 0;
                        break;
                    case IN_DELETE:
                        printf("- IN_DELETE\n");
                        if (event->len){                             
                            // wykrycie usunięcia
                            operation_delete(name, event->wd);
                        }
                        cookie = 0;
                        break;
                    case IN_CREATE:
                        printf("- IN_CREATE\n");
                        if (event->len){                             
                            // wykrycie usunięcia
                            operation_create(name, event->wd);
                        }
                        cookie = 0;
                        break;
                }
                
                
//                if(last_wd != event->wd && cookie != event->cookie && cookie != 0){
//                    // move_from bez następnika move_to
//                    // oznacza usunięcie pliku
//                    
//                    operation_delete(last_name, last_wd);
//                    
//                    last_name = "";
//                }
                
//                printf ("wd=%d mask=%u cookie=%u len=%u\n",
//                        event->wd, event->mask,
//                        event->cookie, event->len);


//                if (event->len){
//                        printf("name=%s\n", name);
//                }

                i += const_event_size + event->len;
                
            }
            
        }  
        return 0;
    }
private:
    int fd;
    
    std::string last_name = "";
    uint32_t cookie = 0;
    int last_wd = 0;

    std::map<int, std::string> inotify_dirs;
    ProtocolSender *proto_sender;

    static const long const_event_size = sizeof(struct inotify_event);
    static const long const_buflen = 1024 * (const_event_size + 16);

    int add_notify_dir(std::string name){
        uint32_t flags = /*IN_MOVED_FROM|IN_MOVED_TO|*/ IN_CLOSE_WRITE|IN_DELETE|IN_CREATE;
        int wd = inotify_add_watch(this->fd, name.c_str(), flags);
        if (wd < 0) {
            perror ("inotify_add_watch");
            return 1;
        }
        
        this->inotify_dirs[wd] = name;
        
        printf("added inotify wd: %d to %s\n", wd, name.c_str());
        
        return wd;
    }
    std::string get_inotify_fullpath(int wd, std::string name){
        return this->inotify_dirs[wd] + "/" + name;
    }
    void init_notify(std::string root){
        this->fd = inotify_init();
        if (this->fd < 0)
            perror ("inotify_init");
        printf("inotify fd: %d\n", this->fd);
        
        add_notify_dir(root);
        
        // TODO: add subnodes
    }  
    void operation_create(std::string name, int wd) {
        this->proto_sender->send_message(get_inotify_fullpath(wd, name), 'U');
        // send_to_server(get_inotify_fullpath(wd, name), 'B');   // TODO: ONLY FOR TESTING! 
        std::cout << "\t=> ("<<wd<<")CREATE: " << get_inotify_fullpath(wd, name) << std::endl;
    }
    void operation_delete(std::string name, int wd) {
        this->proto_sender->send_message(get_inotify_fullpath(wd, name), 'D');
        std::cout << "\t=> ("<<wd<<")DELETE: " << get_inotify_fullpath(wd, name) << std::endl;
    }
    // void operation_rename(std::string name1, std::string name2, int wd) {
    //     // todo
    //     std::cout << "\t=> ("<<wd<<")RENAME: " << inotify_dirs[wd] << "/" << name1 << " -> " << << inotify_dirs[wd] << "/" << name2 << std::endl;
    // }
};

struct NetworkHandler : Handler {

    virtual int handleEvent(uint32_t ee) override {
        
        
        if(ee & EPOLLIN){
            proto_handler->read(sock);
        }

        return 0;
    }
};




int main(int argc, char **argv)
{
    if (argc == 3){
        root = ".";
    }
    else if (argc == 4){
        root = argv[3];
    } else {
        printf("Składnia: client <ip> <port> [ścieżka źródłowa]\n");
        return 1;
    }
    
    signal(SIGINT, ctrl_c);
    signal(SIGPIPE, SIG_IGN);
    
    sock = connect_to_address(argv[1], argv[2]);

    proto_sender = new ProtocolSender(sock, root);

    printf("Wysyłanie wiadomości do serwera...\n");
    proto_sender->send_message("", 'T');
    proto_sender->send_message("/home/pmarc/test/123456.txt", 'B');

    proto_handler = new ProtocolHandlerClient(proto_sender);
    
    struct InotifyHandler inotify = InotifyHandler(root, proto_sender);
    printf(("Połączenie na %s:%s, oczekiwanie na zmiany w: " + root + "\n").c_str(), argv[1], argv[2] );
    
    struct NetworkHandler networkHandler = NetworkHandler();

    int epollfd = epoll_create1(0);
    
    epoll_event ee {EPOLLIN, { .ptr=&inotify }};
    epoll_ctl(epollfd, EPOLL_CTL_ADD, inotify.getFd(), &ee);
    
    epoll_event ee1 {EPOLLIN, { .ptr=&networkHandler }};
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ee1);
    
        
    while(1){
        printf("Waiting for epoll...\n");
        if(-1 == epoll_wait(epollfd, &ee, 1, -1)) {
            //shutdown(sock, SHUT_RDWR);
            //close(sock);
            error(1,errno,"epoll_wait failed");
        }
        printf("=============epoll success\n");
        
        Handler * handler = (Handler*)(ee.data.ptr);
        handler->handleEvent(ee.events);
    }

    
	return 0;
}


void ctrl_c(int){
    close(sock);
    delete proto_sender;
    delete proto_handler;
    printf("\nClosing client\n");
    exit(0);
}

ssize_t readData(int fd, char * buffer, ssize_t buffsize){
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd, char * buffer, ssize_t count){
    auto ret = write(fd, buffer, count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}
uint16_t readPort(char * txt){
    char * ptr;
    auto port = strtol(txt, &ptr, 10);
    if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
    return port;
}
int connect_to_address(char* address, char* port){
    // Resolve arguments to IPv4 address with a port number
    //addrinfo *resolved, hints;
    addrinfo *resolved;
    addrinfo hints={.ai_flags=0, .ai_family=AF_INET, .ai_socktype=SOCK_STREAM,
    .ai_protocol=0, .ai_addrlen=0, .ai_addr=0, .ai_canonname=0, .ai_next=0};

    int res = getaddrinfo(address, port, &hints, &resolved);
    if(res || !resolved) error(1, 0, "getaddrinfo: %s", gai_strerror(res));
    
    // create socket
    int sock = socket(resolved->ai_family, resolved->ai_socktype, 0);
    if(sock == -1) error(1, errno, "socket failed");
    
    // attept to connect
    res = connect(sock, resolved->ai_addr, resolved->ai_addrlen);
    if(res) error(1, errno, "connect failed");
    
    // free memory
    freeaddrinfo(resolved);

    return sock;
}

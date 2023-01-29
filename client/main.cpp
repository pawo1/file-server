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
#include <queue>
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

#include "../shared_lib/jsonTree.h"
#include "../shared_lib/utils.h"

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

using namespace std::chrono_literals;
using json = nlohmann::json;
namespace fs = std::filesystem;

std::string root;
int sock;
int inotfd = 0;


std::string last_name = "";
uint32_t cookie = 0;
int last_wd = 0;

std::map<int, std::string> inotify_dirs;

void ctrl_c(int);
ssize_t readData(int, char * , ssize_t );
void writeData(int, char * , ssize_t );


int add_notify_dir(std::string name);

std::string get_inotify_fullpath(int wd, std::string name);

int readAndSendFile(std::string pathname);

void init_notify(std::string root);


int add_filename(char *buffer, std::string name){
    strcpy(buffer, name.c_str());
    return name.size() + sizeof(char);
}

int add_timestamp(char *buffer, std::string name, char operation){
    size_t seco_size = sizeof(int64_t);
    int64_t timestamp = 0;
    if (operation != 'D') {
        //  timestamp = std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(name).time_since_epoch()).count();
        // timestamp = std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(name).time_since_epoch()).count() + 6437750400;
        struct stat attr;
        stat(name.c_str(), &attr);
        timestamp = attr.st_mtime;
        //printf("Last modified time: %s", ctime(&attr.st_mtime));
    }
    else {
        timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    memcpy(buffer, &timestamp, seco_size);

    return seco_size;
}

int get_file_descriptor(std::string name, long *size){
    
    int fd = open(name.c_str(), O_RDONLY|O_NOATIME);
    if (fd < 0){
        perror("Otwieranie pliku");
        return fd;
    }

    *size = lseek(fd, 0, SEEK_END); // seek to end of file - read file size
    lseek(fd, 0, SEEK_SET);

    return fd;
}

int add_md5(char *buffer, std::string name){
    std::string md5 = calcMD5Sum(name);
    strcpy(buffer, md5.c_str());
    return md5.size() + sizeof(char);
}

std::string get_json(){
    json json_object = parseDirectoryToTree(root);
    return json_object.dump();
}

int send_file_content(int sock, int fd){
    ssize_t bufsize = 1024, received;
    char read_buffer[bufsize];
    
    long bytesRead = 0;
    while(1){
        received = readData(fd, read_buffer, bufsize);
        if(received <= 0){
            close(fd);
            break;
        }
        bytesRead += received;
        writeData(sock, read_buffer, received);
    }

    return bytesRead;
}

int send_string_content(int sock, std::string string_content){

    const ssize_t bufsize = 1024;

    // gwartantowane \0 na końcu od C++11
    char *str_buffer = string_content.data();
    long size = string_content.size() + sizeof(char);

    long bytesRead = 0;
    long offset = 0;
    while(1){
        bytesRead = std::min(bufsize, size - offset);
        if (bytesRead <= 0){
            break;
        }
        writeData(sock, str_buffer+offset, bytesRead);
        offset += bytesRead;
    }

    return offset;
}

void send_to_server(std::string name, char operation){

    const size_t ui32_size = sizeof(uint32_t);
    const size_t char_size = sizeof(char);

    long length = 0;

    int fd = -1;
    std::string json_content = "";

    ssize_t bufsize = 1024;
    char buffer[bufsize];

    uint32_t offset = ui32_size; // also size of the entire message
    uint32_t size_to_send = ui32_size;

    // add operation char
    memcpy(buffer+offset, &operation, char_size);
    offset += char_size;

    switch (operation)
    {
        case 'U':
            length = add_timestamp(buffer+offset, name, operation);
            offset += length;
            length = add_filename(buffer+offset, name);
            offset += length;
            length = add_md5(buffer+offset, name);
            offset += length;
            size_to_send = offset;
            break;
        case 'D':
            length = add_timestamp(buffer+offset, name, operation);
            offset += length;
            length = add_filename(buffer+offset, name);
            offset += length;
            size_to_send = offset;
            break;
        case 'B':
            length = add_timestamp(buffer+offset, name, operation);
            offset += length;
            length = add_filename(buffer+offset, name);
            offset += length;
            size_to_send = offset;
            // send in the next message
            fd = get_file_descriptor(name, &length); // opens fd
            offset += length;
            break;
        case 'T':
            // send in the next message
            size_to_send = offset;
            json_content = get_json();
            offset += json_content.size() + char_size;
            break;
    }

    // add message size
    memcpy(buffer, (char*)&offset, ui32_size);

    // send message without file content
    writeData(sock, buffer, size_to_send);

    // the next message is now
    if(fd >= 0){
        send_file_content(sock, fd); // closes fd
    }
    else if(json_content.size() > 0){
        send_string_content(sock, json_content);
    }
}

void operation_create(std::string name, int wd) {
    send_to_server(get_inotify_fullpath(wd, name), 'U');
    // send_to_server(get_inotify_fullpath(wd, name), 'B');   // TODO: ONLY FOR TESTING! 
    std::cout << "\t=> ("<<wd<<")CREATE: " << get_inotify_fullpath(wd, name) << std::endl;
}

void operation_delete(std::string name, int wd) {
    send_to_server(get_inotify_fullpath(wd, name), 'D');
    std::cout << "\t=> ("<<wd<<")DELETE: " << get_inotify_fullpath(wd, name) << std::endl;
}

//void operation_rename(std::string name1, std::string name2, int wd) {
//    // todo
//    std::cout << "\t=> ("<<wd<<")RENAME: " << inotify_dirs[wd] << "/" << name1 << " -> " << << inotify_dirs[wd] << "/" << name2 << std::endl;
//}

struct Handler {
    virtual int handleEvent(uint32_t event) = 0;
};

struct InotifyHandler :  Handler {
    virtual int handleEvent(uint32_t ee) override {
        
        
        if(ee & EPOLLIN){
            
            char buf[BUF_LEN];
            int len, i = 0;

            len = read (inotfd, buf, BUF_LEN);
            if (len < 0) {
                perror ("read");
                return 1;
            }

            

            while (i < len) {
                struct inotify_event *event;

                event = (struct inotify_event *) &buf[i];
                
                
//                if(!last_name.empty() && (event->mask != IN_MOVED_TO) && != event->cookie){
//                    // move_from bez następnika move_to
//                    // oznacza usunięcie pliku
//                    
//                    operation_delete(event->name, event->wd);
//                    
//                    last_name = "";
//                }
                
                
                switch (event->mask){
                    case IN_MOVED_FROM:
                        printf("- IN_MOVED_FROM\n");
                        // to jeszcze nic nie znaczy
                        if (event->len){ 
                            cookie = event->cookie;
                            last_name = event->name; 
                            last_wd = event->wd;
                            
                            
                            operation_delete(event->name, event->wd);
                        }
                        break;
                    case IN_MOVED_TO:
                        printf("- IN_MOVED_TO\n");
                        if (event->len){ 
//                            if(event->cookie == cookie){
//                                // niepusta nazwa poprzednika i teraz nowa
//                                // wykrycie zmiany nazwy 
//                                
//                                operation_rename(last_name, event->name, event->wd);
//                            }
//                            else {
                                // pusta nazwa poprzednika i teraz nowa
                                // wykrycie utworzenia
                                
                                
                                operation_create(event->name, event->wd);
//                            }
                        }
                        cookie = 0;
                        break;
                    case IN_CLOSE_WRITE:
                        printf("- IN_CLOSE_WRITE\n");
                        if (event->len){                             
                            // wykrycie modyfikacji
                            
                            
                            operation_create(event->name, event->wd);
                        }
                        cookie = 0;
                        break;
                    case IN_DELETE:
                        printf("- IN_DELETE\n");
                        if (event->len){                             
                            // wykrycie usunięcia
                            
                            
                            operation_delete(event->name, event->wd);
                        }
                        cookie = 0;
                        break;
                    case IN_CREATE:
                        printf("- IN_CREATE\n");
                        if (event->len){                             
                            // wykrycie usunięcia
                            
                            
                            operation_create(event->name, event->wd);
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
//                        printf("name=%s\n", event->name);
//                }

                i += EVENT_SIZE + event->len;
                
            }
            
        }
        // ...
        
        return 0;
    }
    // ...
};

struct NetworkHandler : Handler {
    virtual int handleEvent(uint32_t ee) override {
        
        
        if(ee & EPOLLIN){

        }
    }
};


uint16_t readPort(char * txt);


int main(int argc, char **argv)
{
//	printf("Czytanie pliku\n");
//    
//    
//    readAndSendFile("/home/pmarc/test/kalafior.png", 2);
//    
//	printf("\nOdczytano plik\n");

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
    
    init_notify(root);
    
    
    // Resolve arguments to IPv4 address with a port number
    //addrinfo *resolved, hints;
    addrinfo *resolved;
    addrinfo hints={.ai_flags=0, .ai_family=AF_INET, .ai_socktype=SOCK_STREAM,
    .ai_protocol=0, .ai_addrlen=0, .ai_addr=0, .ai_canonname=0, .ai_next=0};

    int res = getaddrinfo(argv[1], argv[2], &hints, &resolved);
    if(res || !resolved) error(1, 0, "getaddrinfo: %s", gai_strerror(res));
    
    // create socket
    sock = socket(resolved->ai_family, resolved->ai_socktype, 0);
    if(sock == -1) error(1, errno, "socket failed");
    
    // attept to connect
    res = connect(sock, resolved->ai_addr, resolved->ai_addrlen);
    if(res) error(1, errno, "connect failed");
    
    // free memory
    freeaddrinfo(resolved);
    
    printf("Wysyłanie wiadomości do serwera...\n");
    // send_to_server("", 'T');
    send_to_server("/home/pmarc/test/123456.txt", 'B');
    
    printf(("Połączenie na %s:%s, oczekiwanie na zmiany w: " + root + "\n").c_str(), argv[1], argv[2] );
    
    int epollfd = epoll_create1(0);
    
    struct InotifyHandler inotify;
    
    epoll_event ee {EPOLLIN, { .ptr=&inotify }};
    epoll_ctl(epollfd, EPOLL_CTL_ADD, inotfd, &ee);
    
        
    while(1){
        printf("Waiting for epoll...\n");
        if(-1 == epoll_wait(epollfd, &ee, 1, -1)) {
            //shutdown(sock, SHUT_RDWR);
            //close(sock);
            error(1,errno,"epoll_wait failed");
        }
        printf("=============epoll success\n");
        
        Handler * handler = (Handler*)(ee.data.ptr);
//        if(!
        handler->handleEvent(ee.events);
//        ){
            
//            
//            printf("Successfull handler!\n");
            //break;
        
//        }
    }

    
	return 0;
}


void ctrl_c(int){
    close(inotfd);
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
int add_notify_dir(std::string name){
    uint32_t flags = /*IN_MOVED_FROM|IN_MOVED_TO|*/ IN_CLOSE_WRITE|IN_DELETE|IN_CREATE;
    int wd = inotify_add_watch(inotfd, name.c_str(), flags);
    if (wd < 0) {
        perror ("inotify_add_watch");
        return 1;
    }
    
    inotify_dirs[wd] = name;
    
    printf("added inotify wd: %d to %s\n", wd, name.c_str());
    
    return wd;
}
std::string get_inotify_fullpath(int wd, std::string name){
    return inotify_dirs[wd] + "/" + name;
}
void init_notify(std::string root){
    inotfd = inotify_init();
    if (inotfd < 0)
        perror ("inotify_init");
    printf("inotify fd: %d\n", inotfd);
    
    add_notify_dir(root);
    
    // TODO: add subnodes
}

int readAndSendFile(std::string pathname){
    int fd = open(pathname.c_str(), O_RDONLY|O_NOATIME);
    if (fd < 0){
        perror("Otwieranie pliku");
        return 1;
    }
//    
//    int fd2 = open(pathname, O_WRONLY|O_CREAT|O_TRUNC, 0755);
    ssize_t bufsize = 1024, received;
    char buffer[bufsize];
    
    strcpy(buffer, "N");
    
    writeData(sock, buffer, 1);
    
    uint32_t size = pathname.size();
    writeData(sock, (char*)&size, sizeof(uint32_t));
    
    
    size = lseek(fd, 0, SEEK_END); // seek to end of file - read file size
    lseek(fd, 0, SEEK_SET);
    
    strcpy(buffer, "B");
    char * ptrSize = reinterpret_cast<char *>(&size);
    memcpy(buffer+1, ptrSize, sizeof(uint32_t));
    
    writeData(sock, buffer, (sizeof(uint32_t) + 1));
    
    long bytesRead = 0;
    while(1){
        // read from socket, write to stdout
        
        received = readData(fd, buffer, bufsize);
        if(received <= 0){
            //shutdown(sock, SHUT_RDWR);
            close(fd);
//            exit(0);
            break;
        }
        bytesRead += received;
        writeData(sock, buffer, received);
        //printf("%s", buffer);
    }
    
    printf("Expected size: %d, actual: %ld\n", size, bytesRead);
    
    return 0;
}

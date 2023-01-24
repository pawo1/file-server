#include <stdio.h>
#include <cstdlib>
#include <sys/inotify.h>
#include <sys/select.h>
#include <iostream>
//#include <sys/time.h>
//#include <sys/types.h>
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



/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

int inotfd = 0;

std::map<int, std::string> inotify_dirs;

int add_notify_dir(const char * name){
    int wd = inotify_add_watch(inotfd, name, IN_MOVED_FROM|IN_MOVED_TO|IN_CLOSE_WRITE|IN_DELETE|IN_CREATE);
    if (wd < 0) {
        perror ("inotify_add_watch");
        return 1;
    }
    
    inotify_dirs[wd] = name;
    
    return wd;
}

std::string get_inotify_fullpath(int wd, std::string name){
    return inotify_dirs[wd] + "/" + name;
}


void ctrl_c(int){
    close(inotfd);
    printf("Closing client\n");
    exit(0);
}

void operation_create(std::string name, int wd) {
    
    std::cout << "\t=> ("<<wd<<")CREATE: " << get_inotify_fullpath(wd, name) << std::endl;
}

void operation_delete(std::string name, int wd) {
    
    std::cout << "\t=> ("<<wd<<")DELETE: " << get_inotify_fullpath(wd, name) << std::endl;
}

//void operation_rename(std::string name1, std::string name2, int wd) {
//    // todo
//    std::cout << "\t=> ("<<wd<<")RENAME: " << inotify_dirs[wd] << "/" << name1 << " -> " << << inotify_dirs[wd] << "/" << name2 << std::endl;
//}

struct Handler {
    virtual int handleEvent(uint32_t event) = 0;
};


std::string last_name = "";
uint32_t cookie = 0;
int last_wd = 0;
 
struct Inotify :  Handler {
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

int readAndSendFile(const char* pathname, int sock){
    int fd = open(pathname, O_RDONLY|O_NOATIME);
    if (fd < 0){
        perror("Otwieranie pliku");
        return 1;
    }
    
    int fd2 = open("/home/pmarc/test/kalafior2.png", O_WRONLY|O_CREAT|O_TRUNC, 0755);
    
    long size = lseek(fd, 0, SEEK_END); // seek to end of file - read file size
    lseek(fd, 0, SEEK_SET);
    
    long bytesRead = 0;
    while(1){
        // read from socket, write to stdout
        ssize_t bufsize = 255, received;
        char buffer[bufsize];
        received = readData(fd, buffer, bufsize);
        if(received <= 0){
            //shutdown(sock, SHUT_RDWR);
            close(fd);
//            exit(0);
            break;
        }
        bytesRead += received;
        writeData(fd2, buffer, received);
        //printf("%s", buffer);
    }
    
    printf("Expected size: %ld, actual: %ld\n", size, bytesRead);
    
    return 0;
}


int main(int argc, char **argv)
{
	printf("Czytanie pliku\n");
    
    
    readAndSendFile("/home/pmarc/test/kalafior.png", 2);
    
	printf("\nOdczytano plik\n");
    
    
    inotfd = inotify_init();
    if (inotfd < 0)
        perror ("inotify_init");

    add_notify_dir("/home/pmarc/test");
    add_notify_dir("/home/pmarc/test/abcdef");
    
    int epollfd = epoll_create1(0);
    
    struct Inotify inotify;
    
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

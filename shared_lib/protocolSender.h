#ifndef PROTOCOL_SENDER_H
#define PROTOCOL_SENDER_H

#include "../shared_lib/jsonTree.h"

#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>
#include <error.h>

class ProtocolSender {

protected:
    int sock;
    std::string root;
    int add_filename(char *buffer, std::string name);
    int add_timestamp(char *buffer, std::string name, char operation);
    int get_file_descriptor(std::string name, long *size);
    int add_md5(char *buffer, std::string name);
    std::string get_json();
    int send_file_content(int fd);
    int send_string_content(std::string string_content);
    
    ssize_t readData(int, char * , ssize_t );
    bool writeData(int, char * , ssize_t );
public:
    ProtocolSender(int sock, std::string root);
    virtual bool send_message(std::string name, char operation);
};

inline ProtocolSender::ProtocolSender(int sock, std::string root){
    this->sock = sock;
    this->root = root;
}

inline int ProtocolSender::add_filename(char *buffer, std::string name){
    strcpy(buffer, name.c_str());
    return name.size() + sizeof(char);
}

inline int ProtocolSender::add_timestamp(char *buffer, std::string name, char operation){
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

inline int ProtocolSender::get_file_descriptor(std::string name, long *size){
        
    int fd = open(name.c_str(), O_RDONLY|O_NOATIME);
    if (fd < 0){
        perror("Otwieranie pliku");
        return fd;
    }

    *size = lseek(fd, 0, SEEK_END); // seek to end of file - read file size
    lseek(fd, 0, SEEK_SET);

    return fd;
}

inline int ProtocolSender::add_md5(char *buffer, std::string name){
    std::string md5 = calcMD5Sum(name);
    strcpy(buffer, md5.c_str());
    return md5.size() + sizeof(char);
}

inline std::string ProtocolSender::get_json(){
    json json_object = parseDirectoryToTree(root);
    return json_object.dump();
}

inline int ProtocolSender::send_file_content(int fd){
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
        writeData(this->sock, read_buffer, received);
    }

    return bytesRead;
}
inline int ProtocolSender::send_string_content(std::string string_content){

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
        writeData(this->sock, str_buffer+offset, bytesRead);
        offset += bytesRead;
    }

    return offset;
}

inline bool ProtocolSender::send_message(std::string name, char operation){

    const size_t ui32_size = sizeof(uint32_t);
    const size_t char_size = sizeof(char);

    long length = 0;

    int fd = -1;
    std::string json_content = "";
    bool abort = false;

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
            name = this->root + "/" + name;
            fd = get_file_descriptor(name, &length); // opens fd
            if(fd == -1){
                abort = true;
                break;
            }
            offset += length;
            break;
        case 'T':
            length = add_timestamp(buffer+offset, name, operation);
            offset += length;
            // send in the next message
            size_to_send = offset;
            json_content = get_json();
            offset += json_content.size() + char_size;
            break;
    }

    if (!abort){
        // add message size
        memcpy(buffer, (char*)&offset, ui32_size);

        // send message without file content
        if(!writeData(this->sock, buffer, size_to_send))
            return false;

        // the next message is now
        if(fd >= 0){
            send_file_content(fd); // closes fd
        }
        else if(json_content.size() > 0){
            send_string_content(json_content);
        }
    }
    else {
        printf("Wystąpił błąd. Nie wysłano wiadomości.\n");
        return false;
    }
    return true;
}

inline ssize_t ProtocolSender::readData(int fd, char * buffer, ssize_t buffsize){
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

inline bool ProtocolSender::writeData(int fd, char * buffer, ssize_t count){
    auto ret = write(fd, buffer, count);
    if(ret==-1) { error(1, errno, "write failed on descriptor %d", fd); return false; }
    if(ret!=count) { error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret); return false; }
    return true;
}

#endif // PROTOCOL_SENDER_H
#ifndef PROTOCOL_SENDER_SERVER_H
#define PROTOCOL_SENDER_SERVER_H

#include "../shared_lib/protocolSender.h"


class ProtocolSenderServer : public ProtocolSender {
public:
    ProtocolSenderServer(int sock, std::string root);
    virtual void send_message(std::string name, char operation) override;
};

inline ProtocolSenderServer::ProtocolSenderServer(int sock, std::string root) : ProtocolSender(sock, root) {

}

inline void ProtocolSenderServer::send_message(std::string name, char operation){

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
        case 'A':
            length = add_timestamp(buffer+offset, name, operation);
            offset += length;
            length = add_filename(buffer+offset, name);
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
    }

    // add message size
    memcpy(buffer, (char*)&offset, ui32_size);

    // send message without file content
    writeData(this->sock, buffer, size_to_send);

    // the next message is now
    if(fd >= 0){
        send_file_content(fd); // closes fd
    }
    else if(json_content.size() > 0){
        send_string_content(json_content);
    }
}

#endif // PROTOCOL_SENDER_SERVER_H
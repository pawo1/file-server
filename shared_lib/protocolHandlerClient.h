#ifndef PROTOCOL_HANDLER_CLIENT_H
#define PROTOCOL_HANDLER_CLIENT_H

#include "protocolHandler.h"
#include "protocolSender.h"

class ProtocolHandlerClient : public ProtocolHandler{
private:
    char *trans_buffer;

public:
    ProtocolHandlerClient(int sock, std::string root);
protected:
    ProtocolSender proto_sender;
    virtual void _completeTransmission() override;
};

inline ProtocolHandlerClient::ProtocolHandlerClient(int sock, std::string root) : ProtocolHandler((json*)nullptr), proto_sender(sock, root) {
    read_bytes = 0;
    trans_size = 0;
    const_head = 0;
    trans_buffer = nullptr;
}

inline void ProtocolHandlerClient::_completeTransmission() {
    switch(msg_type) {
        case 'D':
            {
                printf("operacja D\n");
                std::string str(trans_buffer);
                filename = str;
                fs::remove(filename);
                break;
            }
        case 'A':
            {
                printf("operacja A\n");
                std::string str(trans_buffer);
                filename = str;
                proto_sender.send_message(filename, 'B');
                break;
            }
        case 'B':
            {
                printf("operacja B\n");
                fs::remove(filename);
                fs::rename(filename+".fstmp", filename);
                file.close();
                break;
            }
        default:
            {  
            std::cout << "Corrupted data from client " << std::endl;
            return;
            }
    }
    if(trans_buffer != nullptr)
        delete [] trans_buffer;
    trans_buffer = nullptr;            
    read_bytes = 0;
    trans_size = 0;
}


#endif

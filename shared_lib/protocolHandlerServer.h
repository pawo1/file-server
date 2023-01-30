#ifndef PROTOCOL_HANDLER_SERVER_H
#define PROTOCOL_HANDLER_SERVER_H

#include "../shared_lib/protocolHandler.h"
#include "../shared_lib/protocolSenderServer.h"

class ProtocolHandlerServer : public ProtocolHandler {
public:
    ProtocolHandlerServer(json *json_ptr, int sock, std::string root);

protected:
    ProtocolSenderServer _protocolSender;
    virtual void _completeTransmission() override;
};

inline ProtocolHandlerServer::ProtocolHandlerServer(json *json_ptr, int sock, std::string root) : ProtocolHandler(json_ptr), _protocolSender(sock, root) {

}

inline void ProtocolHandlerServer::_completeTransmission() {
    switch(msg_type) {
        case 'D':
            {
                std::string str(trans_buffer);
                filename = str;
                int64_t timestamp = *(int64_t*)(trans_buffer + filename.length());
                json node = findNodeByPath(*fileSystemTree, filename);
                if(node.empty()) std::cout << "No file server site\n";
                if((int64_t)node["write_time"] < timestamp) {
                    std::cout << "Deleting file\n";
                } else {
                    std::cout << "Server has newer version of file. Sending to client...\n";
                }
                break;
            }
        case 'U':
            {
                break;
            }
        case 'T':
            {
                break;
            }
        case 'B':
            {
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
                
    read_bytes = 0;
    trans_size = 0;
}

#endif // PROTOCOL_HANDLER_SERVER_h
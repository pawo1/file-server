#ifndef PROTOCOL_HANDLER_SERVER_H
#define PROTOCOL_HANDLER_SERVER_H

#include "../shared_lib/protocolHandler.h"
#include "../shared_lib/protocolSenderServer.h"

class ProtocolHandlerServer : public ProtocolHandler {
public:
    ProtocolHandlerServer(json *json_ptr, int sock, std::string root);

protected:
    std::string root_path;
    ProtocolSenderServer _protocolSender;
    virtual void _completeTransmission() override;
};

inline ProtocolHandlerServer::ProtocolHandlerServer(json *json_ptr, int sock, std::string root) : ProtocolHandler(json_ptr), _protocolSender(sock, root) {
    roto_path = root;
}

inline void ProtocolHandlerServer::_completeTransmission() {
    switch(msg_type) {
        case 'D':
            {
                std::string str(trans_buffer);
                filename = str;
                fs::path filepath(root_path);
                filepath /= filename;
                int64_t timestamp = *(int64_t*)(trans_buffer + filename.length());
                json node = findNodeByPath(*fileSystemTree, filename);
                if(node.empty()) {
                    std::cout << "No file server site cannot delete\n";
                } else if((int64_t)node["write_time"] < timestamp) {
                    std::cout << "Deleting file\n";
                    fs::remove(filepath);
                } else {
                    std::cout << "Server has newer version of file. Sending to client...\n";
                    _protocolSender.send_message(filename, 'B');
                }
                break;
            }
        case 'U':
            {
                std::string str(trans_buffer);
                filename = str;
                fs::path filepath(root_path);
                filepath /= filename;
                int64_t timestamp = *(int64_t*)(trans_buffer + filename.length());
                std::ostringstream ss;
                ss << timestamp;
                fs::path newfilepath(root_path);
                newfilepath /= filename+"_"+ss.str();

                json node = findNodeByPath(*fileSystemTree, filename);
                if(node.empty()) {
                    std::cout << "Accepting new file\n";
                     _protocolSender.send_message(filename, 'A');
                } else if((int64_t)node["write_time"] > timestamp) {
                    std::cout << "Discarding older change\n";
                } else {
                    std::cout << "Renaming old version\n";
                    fs::rename(filepath, newfilepath);
                    std::cout << "Accepting newer version\n";
                    _protocolSender.send_message(filename, 'A');
                }
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
    trans_buffer = nullptr;
    read_bytes = 0;
    trans_size = 0;
}

#endif // PROTOCOL_HANDLER_SERVER_h
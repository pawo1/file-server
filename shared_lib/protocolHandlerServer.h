#ifndef PROTOCOL_HANDLER_SERVER_H
#define PROTOCOL_HANDLER_SERVER_H

#include "../shared_lib/protocolHandler.h"
#include "../shared_lib/protocolSenderServer.h"

class ProtocolHandlerServer : public ProtocolHandler {
// private:
//     char *trans_buffer;

public:
    ProtocolHandlerServer(json *json_ptr, int sock, std::string root);

protected:
    std::string root_path;
    ProtocolSenderServer _protocolSender;
    virtual void _completeTransmission() override;
    void _sendToAll(int fd, std::string name, char operation);
};

inline ProtocolHandlerServer::ProtocolHandlerServer(json *json_ptr, int sock, std::string root) : ProtocolHandler(json_ptr), _protocolSender(sock, root) {
    root_path = root;
    read_bytes = 0;
    trans_size = 0;
    const_head = 0;
    trans_buffer = nullptr;
}

inline void ProtocolHandlerServer::_completeTransmission() {
    switch(msg_type) {
        case 'D':
            {
                std::string str(trans_buffer);
                filename = str;
                fs::path filepath(root_path);
                filepath /= filename;
                json node = findNodeByPath(*fileSystemTree, filename);
                if(node.empty()) {
                    std::cout << "No file server site cannot delete\n";
                } else if((int64_t)node["write_time"] < msg_timestamp) {
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
                std::ostringstream ss;
                ss << msg_timestamp;
                fs::path newfilepath(root_path);
                newfilepath /= filename+"_"+ss.str();
                std::string MD5sum = (trans_buffer + filename.length());

                json node = findNodeByPath(*fileSystemTree, filename);
                if(node.empty()) {
                    std::cout << "Accepting new file\n";
                     _protocolSender.send_message(filename, 'A');
                } else if((int64_t)node["write_time"] > msg_timestamp) {
                    std::cout << "Discarding older change\n";
                } else if(node["MD5"] == MD5sum) {
                    std::cout << "Same MD5 discarding...\n";
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
                std::string str(trans_buffer);
                json target = json::parse(str);
                json source = *fileSystemTree;

                json patch = json::diff(source, target);
                json patched_tree = source.patch(patch);
                for(auto it: patched_tree["children"]) {
                    if(findNodeByPath(target, it["path"]).empty()) 
                        _protocolSender.send_message(it["path"].dump(), 'A');
                    else
                        _protocolSender.send_message(it["path"].dump(), 'B');
                }
                break;
            }
        case 'B':
            {
                file.close();
                fs::remove(filename);
                fs::rename(filename+".fstmp", filename);
                break;
            }
        default:
            {  
                std::cout << "Corrupted data from client " << std::endl;
                break;
            }
    }
    if(trans_buffer != nullptr)
        delete [] trans_buffer;
    trans_buffer = nullptr;
    read_bytes = 0;
    trans_size = 0;
}


void _sendToAll(int fd, std::string name, char operation) {
/*    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->fd()!=fd)
            client->write(name, operation);
    } */
}

#endif // PROTOCOL_HANDLER_SERVER_h
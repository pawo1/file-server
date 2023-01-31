#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#define CLIENT_BUFFER 1024

class ProtocolHandler {
protected:
    json *fileSystemTree;
    
    char const_buffer[CLIENT_BUFFER];
    uint32_t const_head;

    uint32_t trans_size;
    uint32_t read_bytes;

    char *trans_buffer;
    std::string filename;
    std::fstream file;
    char msg_type;
    int64_t msg_timestamp;
    void _moveBuffer(char type, uint32_t offset=0);
    void _createStream();
    virtual void _completeTransmission();

public:
    ProtocolHandler(json *json_ptr);
    ~ProtocolHandler();
    void free();
    bool read(int fd);

};

inline ProtocolHandler::ProtocolHandler(json *json_ptr) : fileSystemTree(json_ptr) {
    read_bytes = 0;
    trans_size = 0;
    const_head = 0;
    trans_buffer = nullptr;
}

inline ProtocolHandler::~ProtocolHandler() {
    if(trans_buffer != nullptr)
        delete [] trans_buffer;
    trans_buffer = nullptr;
}

inline void ProtocolHandler::free() {
    if(trans_buffer != nullptr)
        delete [] trans_buffer;
    trans_buffer = nullptr;
}

inline void ProtocolHandler::_moveBuffer(char type, uint32_t offset) {
    if(const_head == 0) return;
        
    int len = const_head-offset;
    if(len) {
        if(type=='B')
            file.write(const_buffer+offset, len);
        else
            memcpy(trans_buffer+read_bytes, const_buffer+offset, len);
    }
    memset(const_buffer, ' ', CLIENT_BUFFER); // filename search by '\0' so buffer must be fullfilled with different value
    read_bytes += (uint32_t)len;
    const_head = 0;

    if(read_bytes == trans_size)
        _completeTransmission();
}

inline void ProtocolHandler::_createStream() {
    file.open( (filename+".fstmp"), std::ios::out | std::ios::binary | std::ios::trunc);
        if(!file.good()) std::cout << "Could not create file" << std::endl;
    }


inline void ProtocolHandler::_completeTransmission() {}

inline bool ProtocolHandler::read(int fd) {


    ssize_t count;
    

    count = ::read(fd, 
                const_buffer+const_head, 
                std::min(CLIENT_BUFFER-const_head, 
                        trans_size > 0 ? (trans_size-(read_bytes+const_head)) : CLIENT_BUFFER
                        )
                );
    if(count > 0) {
        
        const_head += (uint32_t)count;
        uint32_t message_header = (uint32_t)(sizeof(uint32_t) + sizeof(char) + sizeof(int64_t));
        if(trans_size == 0) {
            if(const_head >= ( message_header ) ) {
                msg_type = (const_buffer+sizeof(uint32_t))[0];
                msg_timestamp = *(int64_t*)(const_buffer+sizeof(uint32_t)+sizeof(char));
                if( msg_type == 'B' && findNullTerminator(const_buffer, CLIENT_BUFFER)) {
                    trans_size = *(uint32_t*)const_buffer;
                    trans_size -= message_header;
                    std::string str(const_buffer+message_header);
                    filename = getRoot(*fileSystemTree) + str;
                    read_bytes += filename.length() + 1;
                    _createStream();
                    _moveBuffer(msg_type, message_header + filename.length() + 1);

                } else if(msg_type != 'B') {
                    trans_size = *(uint32_t*)const_buffer;
                    trans_size -= message_header;
                    trans_buffer = new char[trans_size]();
                    _moveBuffer(msg_type, message_header);
                }
            }
        } else {
            if(const_head == CLIENT_BUFFER || (trans_size == (read_bytes+const_head))) 
                _moveBuffer(msg_type);
        }
        return true;
    } 
    
    return false;
}

#endif // PROTOCOL_HANDLER_H
#ifndef UTILS_H
#define UTILS_H

#include <sstream>
#include <fstream>
#include <string>
#include <openssl/md5.h>

inline std::string getBasename(std::string filename) {
    std::string::size_type pos = filename.rfind("/");
    if(pos != std::string::npos)
        return filename.substr(pos + 1);
    return filename;
}


inline std::string calcMD5Sum(std::string filename) {
    
    std::ifstream file(filename, std::ifstream::binary);
    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    char buf[1024 * 16];
    while (file.good()) {
        file.read(buf, sizeof(buf));
        MD5_Update(&md5Context, buf, file.gcount());
    }
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5_Final(result, &md5Context);
    
    std::stringstream md5string;
    md5string << std::hex << std::uppercase << std::setfill('0');
    for (const auto &byte: result)
        md5string << std::setw(2) << (int)byte;
    return md5string.str();
}

#endif // UTILS_H
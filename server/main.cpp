#include <stdio.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <vector>

#include <linux/inotify.h>
#include <nlohmann/json.hpp>
#include <openssl/md5.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string getBasename(std::string filename) {
    std::string::size_type pos = filename.rfind("/");
    if(pos != std::string::npos)
        return filename.substr(pos + 1);
    return filename;
}

std::string calcMD5Sum(std::string filename) {
    
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


json parseDirectoryToTree(std::string path) {
    json tree;
    tree["node"] = "directory";
    json children;
    
    
    for(const auto & entry : fs::directory_iterator(path)) {
        if(fs::is_regular_file(entry.path())) {
            json record;
            record["node"] = "file";
            record["path"] = entry.path();
            record["MD5"] = calcMD5Sum(entry.path());
            record["write_time"] = std::chrono::duration_cast<std::chrono::seconds>(entry.last_write_time().time_since_epoch()).count() ;
            children.push_back(record);
        } else if(fs::is_directory(entry.path())) {
            children.push_back(parseDirectoryToTree(entry.path()));
        }

    }

    tree["children"] = children;
    return tree;
}

int main(int argc, char **argv)
{
    json mockup_configuration = json::parse(R"({"host": "localhost", "port": 80, "path":"/home/pawo/Dokumenty/server-folder/"})");
    std::cout << mockup_configuration["host"] << ":" << mockup_configuration["port"] << std::endl;
    std::cout << mockup_configuration["path"] << std::endl;
    std::cout << parseDirectoryToTree(mockup_configuration["path"]).dump(1) << std::endl;
    return 0;
}

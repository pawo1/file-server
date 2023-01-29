#ifndef JSON_TREE_H
#define JSON_TREE_H

#include <nlohmann/json.hpp>
#include <sys/stat.h>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "../shared_lib/utils.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

inline json parseDirectoryToTree(std::string path) {
    json tree;

    if(path.back() != '/') path += '/';
    tree["node"] = "directory";
    tree["path"] = path;
    json children;
    
    for(const auto & entry : fs::directory_iterator(path)) {
        if(fs::is_regular_file(entry.path())) {
            json record;
            struct stat attr;
            stat(entry.path().c_str(), &attr);

            record["node"] = "file";
            record["path"] = entry.path();
            record["MD5"] = calcMD5Sum(entry.path());
            record["write_time"] = (int64_t)attr.st_mtime;
            children.push_back(record);
        } else if(fs::is_directory(entry.path())) {
            children.push_back(parseDirectoryToTree(entry.path()));
        }

    }

    tree["children"] = children;
    return tree;
}

inline json findNodeByPath(json tree, std::string path) {
    json node = tree;
    json null;
    std::istringstream f(path);
    std::string part;
    std::string pwd = tree["path"];
    while(getline(f, part, '/')) {
        bool found = false;
        for(auto it: node["children"]) {
            if(it["path"] == (pwd + part) || it["path"] == (pwd + part + '/')) {
                node = it;
                found = true;
                if(it["node"] == "directory") {
                    pwd = it["path"];
                }
                break;
            }
        }
        if(!found) return null;
    }
    
    return node; 
}

#endif // JSON_TREE_H
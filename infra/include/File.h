#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>

namespace infra {

class File {
public:
    static bool createPath(const char *file, unsigned int mod);
    static std::string loadFile(const std::string &path);
    static std::string loadFile(const char* path);
}; 
}
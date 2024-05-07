#include <string>
#include "infra/include/File.h"
#include "infra/include/Logger.h"
#if defined(_WIN32)
#include <io.h>   
#include <direct.h>
#else
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#endif // WIN32

#if !defined(_WIN32)
#define    _unlink    unlink
#define    _rmdir     rmdir
#define    _access    access
#endif

namespace infra {

#if defined(_WIN32)
int mkdir(const char *path, int mode) {
    return _mkdir(path);
}
#endif // defined(_WIN32)

bool File::createPath(const char *file, unsigned int mod) {
    std::string path = file;
    std::string dir;
    size_t index = 1;
    while (1) {
        index = path.find('/', index) + 1;
        dir = path.substr(0, index);
        if (dir.length() == 0) {
            break;
        }
        if (_access(dir.c_str(), 0) == -1) {      //access函数是查看是不是存在
            if (mkdir(dir.c_str(), mod) == -1) {  //如果不存在就用mkdir函数来创建
                errorf("mkdir %s failed\n", dir.c_str());
                return false;
            }
        }
    }
    return true;    
}

std::string File::loadFile(const std::string &path) {
    FILE *fp = fopen(path.data(), "rb");
    if (!fp) {
        return {};
    }
    fseek(fp, 0, SEEK_END);
    auto len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::string content(len, '\0');
    if (len != (decltype(len))fread((char *)content.data(), sizeof(char), content.size(), fp)) {
        warnf("fread %s failed\n", path.data());
    }
    fclose(fp);
    return content;
}

std::string File::loadFile(const char* path) {
    std::string path_str(path);
    return loadFile(path_str);
}

}
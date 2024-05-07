#include "infra/include/network/Network.h"
#include "infra/include/network/Defines.h"

namespace infra {

bool network_init() {
#ifdef _WIN32
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
#endif
    return true;
}

bool network_deinit() {
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

}

#include "infra/include/network/Network.h"
#include "infra/include/network/Defines.h"
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif

namespace infra {

bool network_init() {
#ifdef _WIN32
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    return true;
}

bool network_deinit() {
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

#ifdef _WIN32
template<typename FUN>
static void for_each_netAdapter_win32(FUN && fun) { //type: PIP_ADAPTER_INFO
    unsigned long nSize = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO adapterList = (PIP_ADAPTER_INFO)new char[nSize];
    int nRet = GetAdaptersInfo(adapterList, &nSize);
    if (ERROR_BUFFER_OVERFLOW == nRet) {
        delete[] adapterList;
        adapterList = (PIP_ADAPTER_INFO)new char[nSize];
        nRet = GetAdaptersInfo(adapterList, &nSize);
    }
    auto adapterPtr = adapterList;
    while (adapterPtr && ERROR_SUCCESS == nRet) {
        if (fun(adapterPtr)) {
            break;
        }
        adapterPtr = adapterPtr->Next;
    }
    //释放内存空间
    delete[] adapterList;
}
#else
template<typename FUN>
static void for_each_netAdapter_posix(FUN &&fun) { //type: struct ifaddrs *
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *adapter = nullptr;
    if (getifaddrs(&interfaces) == 0) {
        adapter = interfaces;
        while (adapter) {
            if (adapter->ifa_addr->sa_family == AF_INET) {
                if (fun(adapter)) {
                    break;
                }
            }
            adapter = adapter->ifa_next;
        }
        freeifaddrs(interfaces);
    }
}
#endif

std::vector<Interface> getInterfaceList() {
    std::vector<Interface> result;

#ifdef _WIN32
#else
    for_each_netAdapter_posix([&](struct ifaddrs *adapter) {
        if (IFF_LOOPBACK == (adapter->ifa_flags & IFF_LOOPBACK)) {
            //skip loopback
            return false;
        }
        char buffer[64] = {0};
        Interface interface;
        interface.name = adapter->ifa_name;

        int32_t family = adapter->ifa_addr->sa_family;
        if (family == AF_INET) {
            memset(buffer, 0x00, sizeof(buffer));
            struct sockaddr_in *pstIPv4Addr = (struct sockaddr_in *)adapter->ifa_addr;
            inet_ntop(family, &pstIPv4Addr->sin_addr, buffer, INET_ADDRSTRLEN);
            interface.ip = buffer;

            memset(buffer, 0x00, sizeof(buffer));
			pstIPv4Addr = (struct sockaddr_in *)adapter->ifa_netmask;
			inet_ntop(family, &pstIPv4Addr->sin_addr, buffer, INET_ADDRSTRLEN);
            interface.netmask = buffer;

            memset(buffer, 0x00, sizeof(buffer));
			pstIPv4Addr = (struct sockaddr_in *)adapter->ifa_broadaddr;
			inet_ntop(family, &pstIPv4Addr->sin_addr, buffer, INET_ADDRSTRLEN);
            interface.broadaddr = buffer;
        } else if (family == AF_INET6) {
            memset(buffer, 0x00, sizeof(buffer));
			struct sockaddr_in6 *pstIPv6Addr = (struct sockaddr_in6 *)adapter->ifa_addr;
			inet_ntop(family, &pstIPv6Addr->sin6_addr, buffer, INET6_ADDRSTRLEN);
            interface.ip = buffer;

            memset(buffer, 0x00, sizeof(buffer));
			pstIPv6Addr = (struct sockaddr_in6 *)adapter->ifa_netmask;
			inet_ntop(family, &pstIPv6Addr->sin6_addr, buffer, INET6_ADDRSTRLEN);
            interface.netmask = buffer;
        }
        result.push_back(std::move(interface));
        return false;
    });
#endif
    return result;
}

}

/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  network.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-19 22:12:10
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include <string>

namespace infra {
bool network_init();
bool network_deinit();

typedef struct {
    std::string name;
    std::string ip;
    std::string netmask;
    std::string broadaddr;
    std::string mac;
} Interface;

std::vector<Interface> getInterfaceList();
}

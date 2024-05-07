/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MimeTypes.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-03 11:39:36
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <map>

extern const std::map<std::string, std::string> g_mime_type_map;

static inline std::string mimeType(std::string extension) {
    auto it = g_mime_type_map.find(std::string(extension.data(), extension.size()));
    if (it == g_mime_type_map.end()) {
        return "application/octet-stream";
    }
    return it->second;
}

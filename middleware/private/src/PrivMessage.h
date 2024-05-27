/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivMessage.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-27 19:25:07
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "Defs.h"

std::shared_ptr<Message> parseBuffer(const char* buffer, int32_t len, int32_t &usedLen);
/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaSource.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 12:39:45
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "MediaSource.h"

 MediaSource* MediaSource::instance() {
    static MediaSource s_mediasource;
    return &s_mediasource;
 }

 
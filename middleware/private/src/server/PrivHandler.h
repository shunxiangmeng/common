/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivHandler.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-06 12:02:29
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "PrivSessionBase.h"

class PrivHandler {
public:
    bool get_video_format(MessagePtr &msg);
    bool set_video_format(MessagePtr &msg);
    bool get_video_config(MessagePtr &msg);
    bool set_video_config(MessagePtr &msg);
};

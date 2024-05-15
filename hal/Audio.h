/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Audio.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-16 00:00:50
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include "common/mediaframe/MediaDefine.h"
#include "common/mediaframe/MediaFrame.h"
#include "infra/include/Optional.h"
#include "infra/include/Signal.h"

namespace hal {

typedef struct {
    infra::optional<AudioCodecType> codec;  // 编码格式
    infra::optional<int32_t> channel_count; // 通道数量
    infra::optional<int32_t> sample_rate;
    infra::optional<int32_t> bit_per_sample;
} AudioEncodeParams;

}
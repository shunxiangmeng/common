/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaFrame.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 12:35:13
 * Description :  媒体帧，主要用户存放编码后的音视频帧
 * Note        : 
 ************************************************************************/
#pragma once
#include "infra/include/Buffer.h"
#include "MediaDefine.h"

class MediaFrame : public infra::Buffer {
public:
    /**
     * @brief 构造一个空对象
     */
    MediaFrame();

    /**
     * @brief 带参数的构造函数
     * @param[in]   capacity  帧的存储内存大小
     */
    MediaFrame(int32_t capacity);

    /**
     * @brief 拷贝构造函数
     * @param[in]   other
     */
    MediaFrame(const MediaFrame &other);

    /**
     * @brief 移动构造函数
     * @param[in]   other
     */
    MediaFrame(MediaFrame &&other);

    /**
     * @brief 赋值函数
     * @param[in]   other
     */
    void operator=(const MediaFrame &other);

    /**
     * @brief 析构函数
     */
    virtual ~MediaFrame();

    /**
     * @brief 帧是否有效
     */
    bool empty();

    /**
     * @brief 清空帧引用的数据
     */
    void reset();

    /**
     * @brief 重新设置容量
     */
    bool ensureCapacity(int32_t capacity);

    /**
     * @brief 是否为I帧
     * @return @
     */
    bool isKeyFrame() const;

    /**
     * @brief 获取帧类型
     * @return @MediaFrameType
     */
    MediaFrameType getMediaFrameType() const;

    /**
     * @brief 设置帧类型
     * @param[in] type @MediaFrameType
     */
    MediaFrame& setMediaFrameType(MediaFrameType type);

    /**
     * @brief 获取存放格式
     * @return @PlacementType
     */
    PlacementType placementType() const;

    /**
     * @brief 设置存放格式
     * @param[in] placement @PlacementType
     */
    MediaFrame& setPlacementType(PlacementType placement);

    /**
     * @brief 转换存放格式，如果已经是对应的转换格式，则没有任何操作
     * @param[in] placement @PlacementType
     */
    MediaFrame& convertPlacementTypeToAnnexb();

    /**
     * @brief 设置video信息
     * @param[in] info @VideoFrameInfo
     */
    MediaFrame& setVideoFrameInfo(VideoFrameInfo &info);

    /**
     * @brief 获取video信息
     * @param[in] info @VideoFrameInfo
     */
    bool getVideoFrameInfo(VideoFrameInfo &info);

    /**
     * @brief 设置audio信息
     * @param[in] info @AudioFrameInfo
     */
    MediaFrame& setAudioFrameInfo(AudioFrameInfo &info);

    /**
     * @brief 设置audio信息
     * @param[in] info @AudioFrameInfo
     */
    bool getAudioFrameInfo(AudioFrameInfo &info);

    /**
     * @brief 获取时间戳
     * @return 编码出来的pts
     */
    int64_t pts() const;

    /**
     * @brief 设置时间戳
     * @param[in] pts 直接将编码出来的 pts 设置进去
     * @return
     */
    MediaFrame& setPts(int64_t pts);

    /**
     * @brief 获取时间戳
     * @return 编码出来的dts
     */
    int64_t dts() const;

    /**
     * @brief 设置时间戳
     * @param[in] pts 直接将编码出来的 dts 设置进去
     * @return
     */
    MediaFrame& setDts(int64_t dts);

    /**
     * @brief 获取帧序号
     * @return
     */
    int32_t sequence() const;

    /**
     * @brief 设置帧序号
     * @param[in] sequence 帧序号，视频和音频的序号单独计算
     */
    MediaFrame& setSequence(int32_t sequence);

private:
    struct Internal {
        MediaFrameType type;
        PlacementType placement;
        int64_t pts;
        int64_t dts;
        int32_t channel;
        int32_t sequence;     //帧序号
        VideoFrameInfo video_info;
        AudioFrameInfo audio_info;
        Internal() : type(InvalidFrameType), placement(Annexb), pts(-1), dts(-1), channel(-1), sequence(-1) {}
    };
    std::shared_ptr<Internal> minternal_;
    infra::ObjectStatistic<MediaFrame> statistic_;
};

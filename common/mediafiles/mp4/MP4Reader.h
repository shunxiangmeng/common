/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MP4Reader.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-30 22:11:49
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <queue>
#include "common/libmov/include/mp4-writer.h"
#include "common/libmov/include/mov-writer.h"
#include "common/libmov/include/mov-reader.h"
#include "common/libmov/include/mov-buffer.h"
#include "common/libmov/include/mov-format.h"
#include "common/libmpeg/include/mpeg-ps.h"
#include "common/libmpeg/include/mpeg-types.h"
#include "common/mediaframe/MediaFrame.h"

#if defined(_WIN32) || defined(_WIN64)
    #define fseek64 _fseeki64
    #define ftell64 _ftelli64
#else
    #define fseek64 fseek
    #define ftell64 ftell
#endif

class MP4Reader {
public:
    MP4Reader();
    ~MP4Reader();

    bool open(const char* filename);
    bool open(std::string filename);
    void close();

    MediaFrame getFrame();

    int32_t onRead(void *data, size_t bytes);
    int32_t onWrite(const void *data, size_t bytes);
    int32_t onSeek(uint64_t offset);
    uint64_t onTell();

    void getVideoInfo(VideoFrameInfo &videoinfo);
    void getAudioInfo(AudioFrameInfo &audioinfo);

    int seek(int64_t* timestamp);

private:
    int getAllTracks();
    void onVideoTrack(int32_t track_id, uint8_t object, int width, int height, const void *extra, size_t bytes);
    void onAudioTrack(int32_t track_id, uint8_t object, int channel_count, int bit_per_sample, int sample_rate, const void *extra, size_t bytes);

    void addADTSHeader(uint8_t *buffer, int audio_object_type, int frequence, int channel_config, int aac_len);

    typedef struct {
        std::vector<uint8_t> sps;
        std::vector<uint8_t> pps;
        std::vector<uint8_t> vps;
    } H26xParamSets;

    H26xParamSets h26x_param_sets_;
    int32_t nal_length_size_;
    int32_t all_ps_len_;
    void DecodeH264Extradata(const void *extra, size_t bytes, H26xParamSets&, int32_t& nal_length_size);

    bool tryOpenMP4File();
    bool tryOpenPSFile();
    MediaFrame getFrameFromPS();

private:
    typedef enum {
        FileMP4 = 0,
        FilePS,
    } FileType;

    FileType file_type_;
    std::shared_ptr<FILE> fp_;
    std::shared_ptr<mov_reader_t> mov_reader_;
    int64_t duration_ms_;
    int32_t video_track_index_;
    int32_t audio_track_index_;

    std::vector<uint8_t> video_extra_data_;
    std::vector<uint8_t> audio_extra_data_;

    ps_demuxer_t* ps_demuxer_ = nullptr;
    infra::Buffer ps_buffer_;
public:
    std::queue<MediaFrame> ps_mediaframe_list_;
    VideoFrameInfo video_info_;
    AudioFrameInfo audio_info_;
    int32_t video_sequence_;
    int32_t audio_sequence_;
};

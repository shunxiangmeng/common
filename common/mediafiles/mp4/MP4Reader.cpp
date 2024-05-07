/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MP4Reader.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-30 22:16:57
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <string.h>
#include "MP4Reader.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"
#include "infra/include/ByteIO.h"

static struct mov_buffer_t s_mov_buffer = {
    [](void *ctx, void *data, uint64_t bytes) {
        MP4Reader *thiz = (MP4Reader *)ctx;
        return thiz->onRead(data, bytes);
    },
    [](void *ctx, const void *data, uint64_t bytes) {
        MP4Reader *thiz = (MP4Reader *)ctx;
        return thiz->onWrite(data, bytes);
    },
    [](void *ctx, int64_t offset) {
        MP4Reader *thiz = (MP4Reader *)ctx;
        return thiz->onSeek(offset);
    },
    [](void *ctx) {
        MP4Reader *thiz = (MP4Reader *)ctx;
        return (int64_t)thiz->onTell();
    }
};

MP4Reader::MP4Reader() : video_track_index_(-1), audio_track_index_(-1) {
    video_sequence_ = 0;
    audio_sequence_ = 0;
}

MP4Reader::~MP4Reader() {
}

bool MP4Reader::open(std::string filename) {
    close();
    fp_.reset(fopen(filename.data(), "rb"), [](FILE *fp) {
        if (fp) {
            fclose(fp);
        }
    });
    if (!fp_) {
        errorf("open file %s error\n", filename.data());
        return false;
    }

    mov_reader_.reset(mov_reader_create(&s_mov_buffer, this), [](mov_reader_t *ptr) {
        if (ptr) {
            mov_reader_destroy(ptr);
        }
    });
    if (!mov_reader_) {
        errorf("create mov_reader error\n");
        fp_.reset();
        return false;
    }

    getAllTracks();
    duration_ms_ = mov_reader_getduration(mov_reader_.get());
    return true;
}

bool MP4Reader::open(const char* filename) {
    return open(std::string(filename));
}

void MP4Reader::close() {
    mov_reader_.reset();
    fp_.reset();
}

int MP4Reader::getAllTracks() {
    static mov_reader_trackinfo_t s_on_track = {
            [](void *param, uint32_t track, uint8_t object, int width, int height, const void *extra, size_t bytes) {
                //onvideo
                MP4Reader *thiz = (MP4Reader *)param;
                thiz->onVideoTrack(track,object,width,height,extra,bytes);
            },
            [](void *param, uint32_t track, uint8_t object, int channel_count, int bit_per_sample, int sample_rate, const void *extra, size_t bytes) {
                //onaudio
                MP4Reader *thiz = (MP4Reader *)param;
                thiz->onAudioTrack(track,object,channel_count,bit_per_sample,sample_rate,extra,bytes);
            },
            [](void *param, uint32_t track, uint8_t object, const void *extra, size_t bytes) {
                //onsubtitle, do nothing
            }
    };
    return mov_reader_getinfo(mov_reader_.get(), &s_on_track, this);
}

void MP4Reader::onVideoTrack(int32_t track_id, uint8_t object, int width, int height, const void *extra, size_t bytes) {
    infof("onVideoTrack track_id:%d, object:%d, width:%d, height:%d, extra_size:%d\n", track_id, object, width, height, bytes);
    video_track_index_ = track_id;
    video_info_.width = width;
    video_info_.height = height;
    switch (object) {
        case MOV_OBJECT_H264: video_info_.codec = H264;
            DecodeH264Extradata(extra, bytes, h26x_param_sets_, nal_length_size_);
            break;
        case MOV_OBJECT_H265: video_info_.codec = H265; break;
        default:
            errorf("unknown video codec %d\n", object);
            video_info_.codec = VideoCodecInvalid;
            break;
    }
    video_extra_data_.reserve(bytes);
    video_extra_data_.resize(bytes);
    memcpy(&(video_extra_data_[0]), extra, bytes);

    all_ps_len_ = 0;
    if (h26x_param_sets_.sps.size() > 0) {
        all_ps_len_ += (int32_t)h26x_param_sets_.sps.size() + 4;
    }
    if (h26x_param_sets_.pps.size() > 0) {
        all_ps_len_ += (int32_t)h26x_param_sets_.pps.size() + 4;
    }
    if (h26x_param_sets_.vps.size() > 0) {
        all_ps_len_ += (int32_t)h26x_param_sets_.vps.size() + 4;
    }
}

// see @ffmpeg ff_h264_decode_extradata()
void MP4Reader::DecodeH264Extradata(const void *extra, size_t size, H26xParamSets& ps, int32_t &nal_length_size) {
    const uint8_t *data = (const uint8_t *)extra;
    bool is_avc = false;
    nal_length_size = 0;
    if (data[0] == 1) {
        int i, cnt, nalsize;
        const uint8_t *p = data;
        is_avc = true;
        if (size < 7) {
            errorf("avcC %d too short\n", size);
            return;
        }
        // Decode sps from avcC
        cnt = *(p + 5) & 0x1f; // Number of sps
        p  += 6;
        for (i = 0; i < cnt; i++) {
            nalsize = p[0]*256 + p[1];
            p += 2;
            if (nalsize > size - (p - data)) {
                errorf("\n");
                return;
            }
            ps.sps.reserve(nalsize);
            ps.sps.resize(nalsize);
            memcpy(&(ps.sps[0]), p, nalsize);
            p += nalsize;
        }

        // Decode pps from avcC
        cnt = *(p++); // Number of pps
        for (i = 0; i < cnt; i++) {
            nalsize = p[0]*256 + p[1];
            p += 2;
            if (nalsize > size - (p - data)) {
                errorf("\n");
            }

            ps.pps.reserve(nalsize);
            ps.pps.resize(nalsize);
            memcpy(&(ps.pps[0]), p, nalsize);
            
            p += nalsize;
        }
        nal_length_size = (data[4] & 0x03) + 1;

        // ext
        int32_t profile_idc = data[1];
        if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 144) {
            int32_t chrome_format = p[0] & 0x02;
            int32_t bit_depth_luma_minus8 = p[1] & 0x07;
            int32_t bit_depth_chroma_minus8 = p[2] & 0x07;
            int32_t numOfSequenceParameterSetExt = p[3];
            if (numOfSequenceParameterSetExt > 0) {
                int32_t sequenceParameterSetExtLength = p[5] * 256 + p[6];
                warnf("need parser sequenceParameterSetExtLength:%d\n", sequenceParameterSetExtLength);
            }
        }
    } else {
        errorf("need todo\n");
    }
}

void MP4Reader::onAudioTrack(int32_t track_id, uint8_t object, int channel_count, int bit_per_sample, int sample_rate, const void *extra, size_t bytes) {
    infof("onAudioTrack track_id:%d, object:%d, channel:%d, bit_per_sample:%d, rate:%d, extra_size:%d\n", track_id, object, channel_count, bit_per_sample, sample_rate, bytes);
    audio_track_index_ = track_id;
    switch (object) {
        case MOV_OBJECT_AAC: audio_info_.codec = AAC; break;
        case MOV_OBJECT_G711a: audio_info_.codec = G711a; break;
        case MOV_OBJECT_G711u: audio_info_.codec = G711u; break;
        default:
            errorf("unknown audio codec %d\n", object);
            audio_info_.codec = AudioCodecInvalid;
            break;
    }
    audio_info_.sample_rate = sample_rate;
    audio_info_.bit_per_sample = bit_per_sample;
    audio_info_.channel_count = channel_count;

    audio_extra_data_.resize(bytes);
    audio_extra_data_.reserve(bytes);
    memcpy(&(audio_extra_data_[0]), extra, bytes);
}

void MP4Reader::addADTSHeader(uint8_t *buffer, int audio_object_type, int frequence, int channel_config, int aac_len) {
    int32_t samping_frequence_index = 0;
    switch (frequence) {
        case 96000: samping_frequence_index = 0; break;
        case 88200: samping_frequence_index = 1; break;
        case 64000: samping_frequence_index = 2; break;
        case 48000: samping_frequence_index = 3; break;
        case 44100: samping_frequence_index = 4; break;
        case 32000: samping_frequence_index = 5; break;
        case 24000: samping_frequence_index = 6; break;
        case 22050: samping_frequence_index = 7; break;
        case 16000: samping_frequence_index = 8; break;
        case 12000: samping_frequence_index = 9; break;
        case 11025: samping_frequence_index = 10; break;
        case 8000: samping_frequence_index = 11; break;
        case 7350: samping_frequence_index = 12; break;
        default: samping_frequence_index = 11; break;
    }

    int adts_len = aac_len + 7;
    buffer[0] = 0xff;  //syncuword 0xfff
    buffer[1] = 0xf0;
    buffer[1] |= (0 << 3);  //MPEG version 0 for MPEG4, 1 for MPEG2
    buffer[1] |= (0 << 1);  //layer 0
    buffer[1] |= 1;         //protection absent 1

    buffer[2] = (audio_object_type - 1) << 6;   //profile
    buffer[2] |= (samping_frequence_index & 0x0f) << 2;
    buffer[2] |= (0 << 1); //private bit 0
    buffer[2] |= (channel_config & 0x04) >> 2;

    buffer[3] = (channel_config & 0x03) << 6;
    buffer[3] |= (0 << 5); //original 0
    buffer[3] |= (0 << 4); //home 0
    buffer[3] |= (0 << 3); //copyright id bit
    buffer[3] |= (0 << 2); //copyright id start

    buffer[4] = (adts_len & 0x7f8) >> 3;
    buffer[5] = (adts_len & 0x07) << 5;
    buffer[5] |= 0x1f;
    buffer[6] = 0xfc;
}

MediaFrame MP4Reader::getFrame() {
    if (!mov_reader_) {
        return MediaFrame();
    }

    if (video_track_index_ == -1 && audio_track_index_ == -1) {
        return MediaFrame();
    }

    struct Context {
        MediaFrame *frame = nullptr;
        int32_t track_id = 0;
        size_t bytes = 0;
        int64_t pts = 0;
        int64_t dts = 0;
        int flags = 0;
        // for frame size
        int32_t video_track_index;
        int32_t nal_length_size;
        int32_t all_ps_len;
        H26xParamSets *ps;
        Context(MediaFrame *param, int32_t index, int32_t nal, int32_t ps_len, H26xParamSets* sets) :
            frame(param), video_track_index(index), nal_length_size(nal), all_ps_len(ps_len), ps(sets) { }
    };
    
    static mov_reader_onread2 mov_onalloc = [](void *param, uint32_t track_id, size_t bytes, int64_t pts, int64_t dts, int flags) -> void * {
        Context *context = (Context *)param;
        context->track_id = track_id;
        context->bytes = bytes;
        context->pts = pts;
        context->dts = dts;
        context->flags = flags;

        static int64_t pts_offset = infra::getCurrentTimeMs();
        int32_t ps_len = 0;
        if (context->video_track_index == track_id) {
            if (flags & MOV_AV_FLAG_KEYFREAME) {
                ps_len += context->all_ps_len;
            }
            if (context->nal_length_size != 4) {
                errorf("nal_length_size(%d) is not 4\n", context->nal_length_size);
            }
        } else {
            ps_len = 7; //for adts
        }

        if (!context->frame->ensureCapacity(int32_t(ps_len + bytes))) {
            return nullptr;
        }

        if (context->video_track_index == track_id) {
            if (flags & MOV_AV_FLAG_KEYFREAME) {
                auto write_special_nalu = [&](auto& nalu) {
                    uint32_t nalu_size = (uint32_t)nalu.size();
                    if (nalu_size > 0) {
                        infra::ByteWriter<uint32_t>::WriteBigEndian((uint8_t*)&nalu_size, nalu_size);
                        context->frame->putData((const char*)&nalu_size, sizeof(nalu_size));
                        context->frame->putData((const char*)&(nalu[0]), (int32_t)nalu.size());
                    }
                };
                write_special_nalu(context->ps->sps);
                write_special_nalu(context->ps->pps);
                write_special_nalu(context->ps->vps);
            }
        }

        int64_t real_pts = pts_offset + pts;
        //infof("get one frame track_id:%d, size:%d, pts:%lld, dts:%lld, flags:0x%x\n", track_id, bytes, real_pts, dts, flags);
        context->frame->setPts(real_pts).setDts(pts_offset + dts);
        context->frame->setSize(int32_t(ps_len + bytes));
        return context->frame->data() + ps_len;
    };

    MediaFrame frame;
    Context context(&frame, video_track_index_, nal_length_size_, all_ps_len_, &h26x_param_sets_);
    auto ret = mov_reader_read2(mov_reader_.get(), mov_onalloc, &context);
    if (ret == 0) {
        //file eof
        warnf("mp4 file end\n");
        return MediaFrame();
    } else if (ret == 1) {
        if (context.track_id == video_track_index_) {
            VideoFrameInfo info = video_info_;
            char* startcode = frame.data();
            if (context.flags & MOV_AV_FLAG_KEYFREAME) {
                info.type = VideoFrame_I;
                startcode += all_ps_len_;
            }
            frame.setVideoFrameInfo(info);
            frame.setMediaFrameType(Video);
            frame.setPlacementType(AvccHvcc);
            frame.setSequence(video_sequence_++);
        } else {
            addADTSHeader((uint8_t*)frame.data(), 2 /*LC*/, audio_info_.sample_rate, audio_info_.channel_count, frame.size() - 7);
            AudioFrameInfo info = audio_info_;
            frame.setAudioFrameInfo(info);
            frame.setMediaFrameType(Audio);
            frame.setSequence(audio_sequence_++);
        }
        return frame;
    } else {
        errorf("读取mp4文件数据失败:%d\n", ret);
        return MediaFrame();
    }
}

int32_t MP4Reader::onRead(void *data, size_t bytes) {
    if (bytes == fread(data, sizeof(char), bytes, fp_.get())) {
        return 0;
    }
    return 0 != ferror(fp_.get()) ? ferror(fp_.get()) : -1 /*EOF*/;
}

int32_t MP4Reader::onWrite(const void *data, size_t bytes) {
    return bytes == fwrite(data, sizeof(char), bytes, fp_.get()) ? 0 : ferror(fp_.get());
}

int MP4Reader::onSeek(uint64_t offset) {
    return fseek64(fp_.get(), offset, SEEK_SET);
}

uint64_t MP4Reader::onTell() {
    return ftell64(fp_.get());
}

int MP4Reader::seek(int64_t* timestamp) {
    return mov_reader_ ? mov_reader_seek(mov_reader_.get(), timestamp) : -1;
}

void MP4Reader::getVideoInfo(VideoFrameInfo &videoinfo) {
    videoinfo = video_info_;
}

void MP4Reader::getAudioInfo(AudioFrameInfo &audioinfo) {
    audioinfo = audio_info_;
}

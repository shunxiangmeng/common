/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ImageManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 20:17:02
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "ImageManager.h"
#include "infra/include/Logger.h"

namespace oac {

ImageManager::ImageManager(Role role) : role_(role) {
}

ImageManager::~ImageManager() {
}

bool ImageManager::init(SharedImageInfo& info) {
    if (info.count > PICTURE_MAX) {
        errorf("picture_count(%d) > PICTURE_MAX(%d)\n", info.count, PICTURE_MAX);
        return false;
    }

    int32_t picture_size = info.width * info.height * 3 + sizeof(SharedMemoryPictureHead);
    int32_t shared_memory_total_size =  sizeof(SharedMemoryHead) + (picture_size * info.count);
    tracef("image:%dx%d, sizeof(SharedMemoryHead):%d, sizeof(SharedMemoryPictureHead):%d, shared_memory_total_size:%d\n", info.width, info.height, 
        sizeof(SharedMemoryHead), sizeof(SharedMemoryPictureHead), shared_memory_total_size);

    shared_memory_path_ = info.shared_memory_path;
    shared_image_sem_prefix_ = info.shared_image_sem_prefix;

    shared_memory_ = std::make_shared<infra::SharedMemory>(shared_memory_path_, shared_memory_total_size, role_ == Role::server);
    if (!shared_memory_->open()) {
        errorf("create oac shared memory failed\n");
        return false;
    }

    uint8_t *data = shared_memory_->data();
    uint8_t *picture_addr = data + sizeof(SharedMemoryHead);
    // server 初始化共享内存
    if (role_ == Role::server) {
        shared_memory_data_ = (SharedMemoryHead*)data;
        shared_memory_data_->tag[0] = '@';
        shared_memory_data_->tag[1] = '@';
        shared_memory_data_->tag[2] = '@';
        shared_memory_data_->tag[3] = '@';
        shared_memory_data_->total_size = (int32_t)shared_memory_->size();
        shared_memory_data_->picture_sum = info.count;
        shared_memory_data_->picture_count = 0;
        shared_memory_data_->write_index = 0;
        shared_memory_data_->read_index = 0;

        for (auto i = 0; i < info.count; i++) {
            SharedMemoryPictureHead* picture = (SharedMemoryPictureHead*)picture_addr;
            picture->empty = true;
            picture->busy = false;
            picture->index = i;
            picture->format = (IMAGE_PIXEL_FORMAT)info.format;
            picture->width = info.width;
            picture->height = info.height;
            picture->stride = info.width;
            picture->buffer_size = info.width * info.height * 3;
            picture->size = 0;
            picture->timestamp = 0;
            picture->frame_number = 0;
            picture->data = picture_addr + sizeof(SharedMemoryPictureHead);

            picture_addr += picture_size;
        }
    }

    for (auto i = 0; i < info.count; i++) {
        SharedMemoryPictureHead* picture = (SharedMemoryPictureHead*)picture_addr;
        std::string sem_name = shared_image_sem_prefix_ + std::to_string(i);
        auto p = std::make_shared<SharedImage>(sem_name);
        p->sem_name = sem_name;
        p->shared_picture = picture;
        shared_images_.push_back(std::move(p));
        picture_addr += picture_size;
    }

    //infof("role:%d, write_index:%d, read_index:%d\n", role_, shared_memory_data_->write_index, shared_memory_data_->read_index);

    return true;
}

std::shared_ptr<SharedImage> ImageManager::getWriteSharedImage() {
    auto image = shared_images_[shared_memory_data_->write_index];
    //tracef("get write index:%d, shared_index:%d\n", shared_memory_data_->write_index, image->shared_picture->index);
    shared_memory_data_->write_index++;
    if (shared_memory_data_->write_index >= shared_memory_data_->picture_sum) {
        shared_memory_data_->write_index = 0;
    }
    
    return image;
}

std::shared_ptr<SharedImage> ImageManager::getReadSharedImage() {
    auto image = shared_images_[shared_memory_data_->read_index];
    //infof("get read index:%d, shared_index:%d\n", shared_memory_data_->read_index, image->shared_picture->index);
    shared_memory_data_->read_index++;
    if (shared_memory_data_->read_index >= shared_memory_data_->picture_sum) {
        shared_memory_data_->read_index = 0;
    }
    return image;
}

std::shared_ptr<SharedImage> ImageManager::getSharedImageByIndex(int32_t index) {
    return shared_images_[index];
}

std::vector<std::string> ImageManager::imageSemNames() const {
    std::vector<std::string> names;
    for (auto &it : shared_images_) {
        names.push_back(it->sem_name);
    }
    return names;
}

std::string ImageManager::sharedMemoryPath() const {
    return shared_memory_path_;
}

}
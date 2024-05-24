/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ImageManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 20:10:09
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <vector>
#include "infra/include/SharedMemory.h"
#include "Oac.h"

namespace oac {

class ImageManager {
public:
    typedef enum {
        server = 0,
        client
    } Role;

    ImageManager(Role role);
    ~ImageManager();

    bool init(int32_t width, int32_t height, IMAGE_PIXEL_FORMAT format, int32_t picture_count);

    std::shared_ptr<SharedImage> getWriteSharedImage();
    std::shared_ptr<SharedImage> getReadSharedImage();

    std::shared_ptr<SharedImage> getSharedImageByIndex(int32_t index);

    std::string sharedMemoryPath() const;
    std::vector<std::string> imageSemNames() const;

private:
    Role role_;
    std::string shared_memory_path_;
    std::shared_ptr<infra::SharedMemory> shared_memory_;
    SharedMemoryHead* shared_memory_data_;
    sem_t* shared_memory_head_sem_;

    std::vector<std::shared_ptr<SharedImage>> shared_images_;

};

}
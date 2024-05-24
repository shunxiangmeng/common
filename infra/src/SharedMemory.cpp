/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  SharedMemory.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 15:38:35
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/SharedMemory.h"
#include "infra/include/Logger.h"
#ifdef _WIN32
#include <io.h>  // CreateFileMappingA, OpenFileMappingA, etc.
#include <assert.h>
#else
#include <fcntl.h>     // for O_* constants
#include <sys/mman.h>  // mmap, munmap
#include <sys/stat.h>  // for mode constants
#include <unistd.h>    // unlink
#endif

namespace infra {

SharedMemory::SharedMemory(const std::string& path, int64_t size, bool create) noexcept
    : create_(create), path_(path), size_(size) {
#ifndef _WIN32
    int page_size = sysconf(_SC_PAGESIZE);
    tracef("memory page_size:%d\n", page_size);
    size_ = (size + page_size - 1) / page_size * page_size;
#endif
    infof("shared_memory %s, size:%lld(%d)\n", path_.data(), size_, size);
}

SharedMemory::~SharedMemory() noexcept {
    close();
}

#ifdef _WIN32
bool SharedMemory::createOrOpen() noexcept {
    if (path_.empty()) {
        return false;
    }
    if (create_) {
        DWORD sizeLowOrder = static_cast<DWORD>(size_);
        handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     sizeLowOrder,
                                     path_.c_str());
        if (!handle_) {
            errorf("Veigar: Error: Create file mapping failed, name:%s, size:%d, error:%d.\n", path_.c_str(), sizeLowOrder, GetLastError());
            return false;
        }
    } else {
        handle_ = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, path_.c_str());
        if (!handle_) {
            errorf("Veigar: Error: Open file mapping failed, name:%s, error:%d\n", path_.c_str(), GetLastError());
            return false;
        }
    }

    DWORD access = create_ ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ | FILE_MAP_WRITE;
    data_ = static_cast<uint8_t*>(MapViewOfFile(handle_, access, 0, 0, 0));
    if (!data_) {
        errorf("Veigar: Error: Map file view failed, name: %s, gle: %d.\n", path_.c_str(), GetLastError());
        if (handle_) {
            CloseHandle(handle_);
            handle_ = NULL;
        }
        return false;
    }
    return true;
}

bool SharedMemory::valid() const noexcept {
    return !!handle_;
}

void SharedMemory::close() noexcept {
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (handle_) {
        CloseHandle(handle_);
        handle_ = NULL;
    }
}

#else

bool SharedMemory::createOrOpen() noexcept {
    if (path_.empty()) {
        return false;
    }
    int flags = create_ ? (O_CREAT | O_RDWR) : O_RDWR;
    fd_ = shm_open(path_.c_str(), flags, 0666);
    if (fd_ < 0) {
        int err = errno;
        errorf("%s shared memory failed, err: %d\n", create_ ? "Create" : "Open", err);
        return false;
    }

    if (create_) {
        // this is the only way to specify the size of a newly-created POSIX shared memory object
        
        int ret = ftruncate(fd_, size_);
        if (ret != 0) {
            int err = errno;
            errorf("ftruncate shm failed, size: %lld, err: %d\n", size_, err);
            ::close(fd_);
            fd_ = -1;
            if (create_) {
                shm_unlink(path_.c_str());
            }
            return false;
        }
    }

    void* memory = mmap(nullptr,                 // addr
                        size_,                   // length
                        PROT_READ | PROT_WRITE,  // prot
                        MAP_SHARED,              // flags
                        fd_,                     // fd
                        0                        // offset
    );
    if (memory == MAP_FAILED) {
        int err = errno;
        errorf("mmap shm failed, size: %lld, err: %d.\n", size_, err);
        ::close(fd_);
        fd_ = -1;
        if (create_) {
            shm_unlink(path_.c_str());
        }
        return false;
    }
    data_ = static_cast<uint8_t*>(memory);
    if (!data_) {
        ::close(fd_);
        fd_ = -1;
        if (create_) {
            shm_unlink(path_.c_str());
        }
        return false;
    }

    infof("%s shared memory %s success, fd: %d\n", create_ ? "Create" : "Open", path_.c_str(), fd_);
    return true;
}

bool SharedMemory::valid() const noexcept {
    return fd_ != -1;
}

void SharedMemory::close() noexcept {
    if (fd_ != -1) {
        infof("Veigar: Close fd: %d.\n", fd_);
        if (data_) {
            munmap(data_, size_);
            data_ = nullptr;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        if (create_) {
            if (!path_.empty()) {
                shm_unlink(path_.c_str());
            }
        }
    }
}

#endif

}
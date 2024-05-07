#include "SocketHandler.h"
#include "NetworkThreadPool.h"

namespace infra {

SocketHandler::~SocketHandler() {
    if (bind_fd_ > 0) {
        NetworkThreadPool::instance()->delSocketEvent(bind_fd_, shared_from_this());
    }
}

bool SocketHandler::addEvent(Socket& socket, EventType event, int32_t timeout) {
    bind_fd_ = socket.getHandle();
    return NetworkThreadPool::instance()->addSocketEvent(bind_fd_, event, shared_from_this());
}

bool SocketHandler::delEvent(Socket& socket) {
    bind_fd_ = -1;
    return NetworkThreadPool::instance()->delSocketEvent(socket.getHandle(), shared_from_this());
}

bool SocketHandler::modifyEvent(Socket& socket, EventType event) {
    bind_fd_ = socket.getHandle();
    return NetworkThreadPool::instance()->modifySocketEvent(bind_fd_, event, shared_from_this());
}

}
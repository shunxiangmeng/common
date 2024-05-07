#pragma once

enum class ContentType {
    string,
    multipart,
    urlencoded,
    chunked,
    octet_stream,
    websocket,
    unknown,
};

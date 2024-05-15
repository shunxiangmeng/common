file(GLOB_RECURSE LIVE_SOURCE_FILES 
    application/*.cpp 
    stream/*.cpp 
    rtsp/*.cpp 
    private/*.cpp 
    http/*.cpp 
    http/*.c 
    api/*.cpp
    configManager/*.cpp)

file(GLOB_RECURSE LIVE_SOURCE_HEADER_FILES 
    application/*.h 
    stream/*.h 
    rtsp/*.h 
    private/*.h 
    http/*.h 
    api/*.h
    configManager/*.h)

set(APP_SOURCE_FILES ${APP_SOURCE_FILES} ${LIVE_SOURCE_FILES} PARENT_SCOPE)
set(APP_SOURCE_HEADER_FILES ${APP_SOURCE_HEADER_FILES} ${LIVE_SOURCE_HEADER_FILES} PARENT_SCOPE)

set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/common/stream 
    PARENT_SCOPE)
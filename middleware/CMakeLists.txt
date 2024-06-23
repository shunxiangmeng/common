file(GLOB_RECURSE LIVE_SOURCE_FILES 
    application/*.cpp 
    stream/*.cpp 
    rtsp/*.cpp 
    private/*.cpp 
    http/*.cpp 
    http/*.c 
    api/*.cpp
    configManager/*.cpp
    oac/*.cpp
    ulucu/*.cpp ulucu/*.c)

file(GLOB_RECURSE LIVE_SOURCE_HEADER_FILES 
    application/*.h 
    stream/*.h 
    rtsp/*.h 
    private/*.h 
    http/*.h 
    api/*.h
    configManager/*.h
    oac/*.h
    ulucu/*.h)

file(GLOB_RECURSE ULU_PLATFORM_SOURCE_FILES platform/*.cpp platform/*.c)
file(GLOB_RECURSE ULU_PLATFORM_HEADER_FILES platform/*.h)

set(APP_SOURCE_FILES ${APP_SOURCE_FILES} ${LIVE_SOURCE_FILES} ${ULU_PLATFORM_SOURCE_FILES} PARENT_SCOPE)
set(APP_SOURCE_HEADER_FILES ${APP_SOURCE_HEADER_FILES} ${LIVE_SOURCE_HEADER_FILES} ${ULU_PLATFORM_HEADER_FILES} PARENT_SCOPE)

set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/ulucu/src/anyan/src/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ulucu/src/anyan/src/inc/pthreads-w
    ${CMAKE_CURRENT_SOURCE_DIR}/ulucu/src/anyan/src/devmng
    ${CMAKE_CURRENT_SOURCE_DIR}/ulucu/src/anyan/src/ulu_protocol
    PARENT_SCOPE)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/ulucu/src/anyan/lib)

file(GLOB_RECURSE LIVE_SOURCE_FILES application/*.cpp stream/*.cpp private/*.cpp http/*.cpp http/*.c api/*.cpp)
file(GLOB_RECURSE LIVE_SOURCE_HEADER_FILES application/*.h stream/*.h private/*.h http/*.h api/*.h)

set(APP_SOURCE_FILES ${APP_SOURCE_FILES} ${LIVE_SOURCE_FILES} PARENT_SCOPE)
set(APP_SOURCE_HEADER_FILES ${APP_SOURCE_HEADER_FILES} ${LIVE_SOURCE_HEADER_FILES} PARENT_SCOPE)
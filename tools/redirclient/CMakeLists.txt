cmake_minimum_required(VERSION 3.10.0)

set(REDIR_SOURCE_FILES)
file(GLOB_RECURSE REDIR_SOURCE_FILES src/*.cpp)

add_executable(redir ${REDIR_SOURCE_FILES})
target_link_libraries(redir pthread)

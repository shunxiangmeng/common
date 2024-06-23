cmake_minimum_required(VERSION 3.10.0)

project(anyan)
set(Tartget anyan)

#源文件
set(LIB_SOURCE_FILES)
set(LIB_HEADER_FILES)

file(GLOB_RECURSE LIB_SOURCE_FILES src/*.cpp src/*.c)
file(GLOB_RECURSE LIB_HEADER_FILES src/*.h)

if(WIN32)
function(set_file_group prefix)
    message(STATUS "set_file_group " ${prefix} " " ${ARGC})
    foreach(FILE IN LISTS ARGN 1)
        # Get the directory of the source file
        get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
        # Remove common directory prefix to make the group
        string(REPLACE "${prefix}" "" GROUP "${PARENT_DIR}")
        # Make sure we are using windows slashes
        string(REPLACE "/" "\\" GROUP "${GROUP}")
        source_group("${GROUP}" FILES "${FILE}")
    endforeach()
endfunction()
set_file_group(${CMAKE_SOURCE_DIR} ${LIB_SOURCE_FILES} ${LIB_HEADER_FILES})
endif()

if(MSVC)
#禁止编译警告：warning C4819：该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
add_compile_options(/wd4819)
add_compile_options(/wd4200)  #禁止C4200警告
add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)  # 禁止C4996警告
endif()

#file(COPY ${PROJECT_SOURCE_DIR}/../../infra DESTINATION ${CMAKE_BINARY_DIR}/infra)
add_subdirectory(${PROJECT_SOURCE_DIR}/../../infra ${CMAKE_BINARY_DIR}/infra)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/inc)
include_directories(${PROJECT_SOURCE_DIR}/src/ulu_protocol)
include_directories(${PROJECT_SOURCE_DIR}/src/inc/pthreads-w)
include_directories(${PROJECT_SOURCE_DIR}/../..)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(${Tartget} ${LIB_SOURCE_FILES} ${LIB_HEADER_FILES})
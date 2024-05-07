cmake_minimum_required(VERSION 3.10)
project(test)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES ./*.cpp ../src/*.cpp)

include_directories(${PROJECT_SOURCE_DIR}/../src)

set(DEPEND_LIBS pthread)

add_executable(test ${SOURCES})
target_link_libraries(test ${DEPEND_LIBS})

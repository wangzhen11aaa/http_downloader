cmake_minimum_required(VERSION 3.16)

project(multithreadDownloader VERSION 1.0)

set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
#include_directories(${PROJECT_SOURCE_DIR})

set(SRC ${PROJECT_SOURCE_DIR}/MUtil.cpp ${PROJECT_SOURCE_DIR}/Task.cpp ${PROJECT_SOURCE_DIR}/HTTPDownLoadHelper.cpp ${PROJECT_SOURCE_DIR}/MultithreadDownloader.cpp )

set(TARGET multithreadDownloader)

add_subdirectory(lib)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Werror -Wunused-variable -Wpedantic -g")

add_executable(${TARGET} ${SRC})

target_include_directories(${TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/)

target_link_libraries(${TARGET} -lcurl)
target_link_libraries(${TARGET} -lpthread)
target_link_libraries(${TARGET} glog)

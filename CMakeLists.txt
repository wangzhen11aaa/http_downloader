cmake_minimum_required(VERSION 3.16)

project(multithreadDownloader VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
#set (CMAKE_CXX_FLAGS "-g")
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB SRC ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)
set(TARGET multithreadDownloader)

add_subdirectory(lib)
include_directories(lib/log)

add_executable(${TARGET} ${SRC})
target_link_libraries(${TARGET} -lcurl)
target_link_libraries(${TARGET} glog)

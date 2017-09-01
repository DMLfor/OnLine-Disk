cmake_minimum_required(VERSION 3.8)

project(chunk)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp chunk.h chunk.cpp codec.h jsonHandle.h)
add_executable(chunk ${SOURCE_FILES})

add_definitions(-DMUDUO_STD_STRING)
add_compile_options(-g)
find_package(Boost REQUIRED COMPONENTS system)

target_link_libraries(chunk ${Boost_LIBRARIES})

target_link_libraries(chunk muduo_base_cpp11 muduo_net_cpp11 pthread jsoncpp)
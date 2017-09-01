cmake_minimum_required(VERSION 3.8)

project(master)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp master.h master.cpp codec.h jsonHandle.h)

add_executable(master ${SOURCE_FILES})

add_definitions(-DMUDUO_STD_STRING)

find_package(Boost REQUIRED COMPONENTS system)

target_link_libraries(master ${Boost_LIBRARIES})

target_link_libraries(master muduo_base_cpp11 muduo_net_cpp11 pthread jsoncpp )

include_directories(include)

link_libraries(lib)


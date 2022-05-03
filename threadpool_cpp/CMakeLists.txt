cmake_minimum_required(VERSION 3.10)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 17)

find_package( Threads )
add_executable(ThreadPool main.cpp)
target_link_libraries( ThreadPool ${CMAKE_THREAD_LIBS_INIT} )

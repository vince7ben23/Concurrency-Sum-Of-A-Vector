cmake_minimum_required(VERSION 3.5.0)
project(concurrency_sum_of_a_vector)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(main.out src/main.cpp)

message("CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")
target_link_libraries(main.out ${CMAKE_THREAD_LIBS_INIT})


cmake_minimum_required(VERSION 3.20)
project(ThreadPool)

include_directories(
        include
)

set(CMAKE_CXX_STANDARD 11)

add_executable(threadpool_test threadpool_test.cpp src/ThreadPool.cpp)
add_executable(server server.cpp src/MyHttp.cpp src/ThreadPool.cpp)
add_executable(client_test client_test.cpp)

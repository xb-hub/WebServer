cmake_minimum_required(VERSION 3.20)
project(ThreadPool)

find_package(Threads REQUIRED)

include_directories(
        include
)

set(CMAKE_CXX_STANDARD 11)

#add_executable(threadpool_test threadpool_test.cpp src/ThreadPool.cpp)
#target_link_libraries(threadpool_test Threads::Threads)
#add_executable(client_test client_test.cpp)
add_executable(server server.cpp src/MyHttp.cpp src/ThreadPool.cpp)
target_link_libraries(server Threads::Threads)

cmake_minimum_required(VERSION 3.20)
project(ThreadPool)

find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem program_options system)

include_directories(
        include
        ${Boost_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIRS}
)

set(CMAKE_CXX_STANDARD 11)

#add_executable(threadpool_test threadpool_test.cpp src/ThreadPool.cpp)
#target_link_libraries(threadpool_test Threads::Threads)
#add_executable(client_test client_test.cpp)
add_executable(server server.cpp src/MyHttp.cpp src/ThreadPool.cpp)
target_link_libraries(server Threads::Threads ${Boost_LIBRARIES})

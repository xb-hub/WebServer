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
add_executable(server server.cpp src/MyHttp.cpp src/ThreadPool.cpp src/Thread.cpp src/Log.cpp src/util.cpp)
target_link_libraries(server Threads::Threads ${Boost_LIBRARIES} yaml-cpp)

add_executable(test test/test.cpp)
target_link_libraries(test Threads::Threads ${Boost_LIBRARIES})

add_executable(log_test test/log_test.cpp src/Log.cpp src/util.cpp)
target_link_libraries(log_test Threads::Threads yaml-cpp)

add_executable(yaml_test test/yaml_test.cpp)
target_link_libraries(yaml_test yaml-cpp)
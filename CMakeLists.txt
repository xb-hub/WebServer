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

add_compile_options(-std=c++14)

set(SOURCE_LIST server.cpp src/MyHttp.cpp src/ThreadPool.cpp src/Thread.cpp src/Scheduler.cpp src/Coroutine.cpp src/Log.cpp src/util.cpp)

#add_executable(threadpool_test threadpool_test.cpp src/ThreadPool.cpp)
#target_link_libraries(threadpool_test Threads::Threads)
#add_executable(client_test client_test.cpp)
add_executable(server ${SOURCE_LIST})
target_link_libraries(server Threads::Threads ${Boost_LIBRARIES} yaml-cpp)

add_executable(test test/test.cpp)
target_link_libraries(test Threads::Threads ${Boost_LIBRARIES})

add_executable(log_test test/log_test.cpp src/Log.cpp src/util.cpp)
target_link_libraries(log_test Threads::Threads yaml-cpp)

add_executable(yaml_test test/yaml_test.cpp)
target_link_libraries(yaml_test yaml-cpp)

add_executable(fiber_test test/fiber_test.cpp src/Scheduler.cpp src/Coroutine.cpp src/Log.cpp src/util.cpp  src/Thread.cpp)
target_link_libraries(fiber_test Threads::Threads yaml-cpp)

add_executable(schedule_test test/schedule_test.cpp src/Scheduler.cpp src/Coroutine.cpp src/Thread.cpp src/Log.cpp src/util.cpp)
target_link_libraries(schedule_test Threads::Threads yaml-cpp)

add_executable(io_test test/iomanager_test.cpp src/Timer.cpp src/Scheduler.cpp src/IOManager.cpp src/Coroutine.cpp src/Thread.cpp src/Log.cpp src/util.cpp)
target_link_libraries(io_test Threads::Threads yaml-cpp)
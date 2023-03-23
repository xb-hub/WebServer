cmake_minimum_required(VERSION 3.20)
project(ThreadPool)

find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem program_options system)

include_directories(
        include
        ${Boost_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIRS}
)

add_compile_options(-std=c++17)

set(TEST_LIST src/Coroutine.cpp src/Log.cpp
src/Scheduler.cpp src/Thread.cpp src/Timer.cpp src/util.cpp 
src/ThreadPool.cpp)

set(SOURCE_LIST src/Server.cpp src/ThreadPool.cpp 
src/Timer.cpp src/Thread.cpp src/HttpConn.cpp
src/Scheduler.cpp src/Coroutine.cpp src/Buffer.cpp src/epoller.cpp 
src/Log.cpp src/util.cpp src/HttpResponse.cpp src/HttpRequest.cpp)
#add_executable(threadpool_test threadpool_test.cpp src/ThreadPool.cpp)
#target_link_libraries(threadpool_test Threads::Threads)
#add_executable(client_test client_test.cpp)
add_executable(server main.cpp ${SOURCE_LIST})
target_link_libraries(server Threads::Threads ${Boost_LIBRARIES} yaml-cpp dl)

# add_executable(test test/test.cpp)
# target_link_libraries(test Threads::Threads ${Boost_LIBRARIES})

add_executable(log_test test/log_test.cpp src/Log.cpp src/util.cpp)
target_link_libraries(log_test Threads::Threads yaml-cpp)

add_executable(yaml_test test/yaml_test.cpp)
target_link_libraries(yaml_test yaml-cpp)

add_executable(coroutine_test test/coroutine_test.cpp ${TEST_LIST})
target_link_libraries(coroutine_test Threads::Threads yaml-cpp dl ${OPENSSL_LIBRARIES})

add_executable(schedule_test test/schedule_test.cpp ${TEST_LIST})
target_link_libraries(schedule_test Threads::Threads yaml-cpp dl ${OPENSSL_LIBRARIES})

# add_executable(io_test test/iomanager_test.cpp ${TEST_LIST})
# target_link_libraries(io_test Threads::Threads yaml-cpp dl ${OPENSSL_LIBRARIES})
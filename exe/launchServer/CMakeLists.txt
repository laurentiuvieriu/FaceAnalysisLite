cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/exe/launchServer)

set(SOURCE_FILES launchServer.cpp)
add_executable(launchServer ${SOURCE_FILES})
target_link_libraries(launchServer zmq ${Boost_LIBRARIES})

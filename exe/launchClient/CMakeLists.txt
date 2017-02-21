cmake_minimum_required(VERSION 3.2)

find_package( OpenCV REQUIRED )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/exe/launchClient)
set(SOURCE_FILES launchClient.cpp)
add_executable(launchClient ${SOURCE_FILES})
target_link_libraries(launchClient zmq)

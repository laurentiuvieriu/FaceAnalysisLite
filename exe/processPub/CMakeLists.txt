cmake_minimum_required(VERSION 3.2)

find_package( OpenCV REQUIRED )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES processPub.cpp)
add_executable(processPub ${SOURCE_FILES})
target_link_libraries(processPub zmq ${OpenCV_LIBS} boost_system boost_filesystem)

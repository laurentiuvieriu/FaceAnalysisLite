cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES launcher.cpp ../temp/faceSub.cpp)
add_executable(launcher ${SOURCE_FILES})
target_link_libraries(launcher zmq ${Boost_LIBRARIES})

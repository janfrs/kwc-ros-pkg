cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(dorylus)
add_definitions(-Wall)
add_definitions(-I`rospack find opencv_latest`/opencv-cvs/cxcore/include)
rospack_add_executable(dorylus_node src/dorylus_node.cpp src/dorylus.cpp)
target_link_libraries(dorylus_node profiler)		 




cmake_minimum_required(VERSION 2.6)
include(rosbuild)
add_definitions(-Wall)
rospack(orcollision)
add_executable(orcollision orcollision.cpp)
target_link_libraries (orcollision openrave-core)

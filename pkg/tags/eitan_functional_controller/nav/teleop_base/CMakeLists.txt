cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(teleop_base)
rospack_add_executable(teleop_base teleop_base.cpp)

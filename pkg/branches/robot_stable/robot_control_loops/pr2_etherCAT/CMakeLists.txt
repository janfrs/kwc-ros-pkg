cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(pr2_etherCAT)
rospack_add_executable(pr2_etherCAT src/main.cpp)

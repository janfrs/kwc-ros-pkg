cmake_minimum_required(VERSION 2.6)
include(rosbuild)
add_definitions(-Wall)
rospack(robot_kinematics)
rospack_add_library(robot_kinematics src/robot_kinematics.cpp src/serial_chain.cpp)

rospack_add_executable(bin/robot_kinematics_test test/robot_kinematics_test.cpp)
target_link_libraries(bin/robot_kinematics_test robot_kinematics)



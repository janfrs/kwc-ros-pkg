cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(mechanism_model)
rospack_add_library( mechanism_model
  src/simple_transmission.cpp
  src/gripper_transmission.cpp
  src/joint.cpp
  src/robot.cpp
  src/link.cpp)

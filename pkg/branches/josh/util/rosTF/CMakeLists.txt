cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(rosTF)
genmsg()

rospack_add_library(rosTF src/rosTF.cpp)
rospack_add_executable(testServer src/testServer.cc)
target_link_libraries(testServer rosTF)
rospack_add_executable(testClient src/testClient.cc)
target_link_libraries(testClient rosTF)

rospack_add_executable(frameServer src/frameServer.cc)
target_link_libraries(frameServer rosTF)

rospack_add_executable(viewTF src/viewTF.cpp)
target_link_libraries(viewTF rosTF)

/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#include "ros/node.h"
#include "robot_srvs/SelfTest.h"

#include <string>

class TestHokuyo : public ros::node
{
public:
  TestHokuyo() : ros::node("test_hokuyo")
  {
  }

  bool doTest(std::string name)
  {
    robot_srvs::SelfTest::request  req;
    robot_srvs::SelfTest::response res;
    if (ros::service::call(name + "/self_test", req, res))
    {
      printf("Self test completed:\n");
      if (res.passed)
        printf("Test passed!\n");
      else
        printf("Test failed!\n");

      for (size_t i = 0; i < res.get_status_size(); i++)
      {
        printf("%2d) %s\n", i + 1, res.status[i].name.c_str());
        if (res.status[i].level == 0)
          printf("     [OK]: ");
        else if (res.status[i].level == 1)
          printf("     [WARNING]: ");
        else
          printf("     [ERROR]: ");

        printf("%s\n\n", res.status[i].message.c_str());
      }
      return true;
    }
    else
    {
      printf("Failed to call service.\n");
      return false;
    }
  }

};

int main(int argc, char **argv)
{
  ros::init(argc, argv);
  if (argc != 2)
  {
    printf("usage: test_hokuyo name\n");
    return 1;
  }
  TestHokuyo h;

  h.doTest(std::string(argv[1]));

  ros::fini();
  return 0;
}


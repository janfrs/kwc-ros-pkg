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

#include "imu_node/ImuData.h"
#include <string>
#include "logging/LogPlayer.h"

void imu_callback(std::string name, imu_node::ImuData* imu, ros::Time t, void* f)
{
  FILE* file = (FILE*)f;

  fprintf(file, "%.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n",
          t.to_double(),
          imu->header.stamp.to_double(),
          imu->accel.x, imu->accel.y, imu->accel.z,
          imu->angrate.x, imu->angrate.y, imu->angrate.z);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("usage: imu_extract LOG\n");
    return 1;
  }

  LogPlayer player;

  player.open(std::string(argv[1]), ros::Time(0));

  int count;

  FILE* file = fopen("imu.txt", "w");

  player.addHandler<imu_node::ImuData>(std::string("*"), &imu_callback, file);

  while(player.nextMsg())  {}

  fclose(file);

  return 0;
}

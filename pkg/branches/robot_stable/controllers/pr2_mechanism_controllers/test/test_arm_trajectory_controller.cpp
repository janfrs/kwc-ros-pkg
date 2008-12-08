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

#include <ros/node.h>
#include <pr2_mechanism_controllers/JointTraj.h>

static int done = 0;

void finalize(int donecare)
{
  done = 1;
}

int main( int argc, char** argv )
{

  /*********** Initialize ROS  ****************/
  ros::init(argc,argv);
  ros::node *node = new ros::node("test_arm_trajectory_controller"); 

  signal(SIGINT,  finalize);
  signal(SIGQUIT, finalize);
  signal(SIGTERM, finalize);


  /*********** Start moving the robot ************/
  pr2_mechanism_controllers::JointTraj cmd;

  int num_points = 2;
  int num_joints = 5;

  cmd.set_points_size(num_points);

  for(int i=0; i<num_points; i++)
    cmd.points[i].set_positions_size(num_joints);

  cmd.points[0].positions[0] = 0.0;
  cmd.points[0].positions[1] = 0.0;
  cmd.points[0].positions[2] = 0.0;
  cmd.points[0].positions[3] = 0.0;
  cmd.points[0].positions[4] = 0.0;
  cmd.points[0].time = 0.0;

  cmd.points[1].positions[0] = 1.0;
  cmd.points[1].positions[1] = 1.0;
  cmd.points[1].positions[2] = 0.2;
  cmd.points[1].positions[3] = 0.5;
  cmd.points[1].positions[4] = 0.4;
  cmd.points[1].time = 10.0;

  node->advertise<pr2_mechanism_controllers::JointTraj>("arm_trajectory_command",1);
  sleep(1);
  node->publish("arm_trajectory_command",cmd);
  sleep(4);

  ros::Time start_time = ros::Time::now();
  ros::Duration sleep_time = ros::Duration().fromSec(0.01);

/*  while(!done)
  {
    ros::Duration delta_time = ros::Time::now() - start_time;
    sleep_time.sleep();
  }
*/
  ros::fini();
}

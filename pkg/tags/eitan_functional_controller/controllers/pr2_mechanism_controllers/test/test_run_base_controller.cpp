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

#include <libTF/libTF.h>
#include <ros/node.h>
#include <std_msgs/TransformWithRateStamped.h>
#include <std_msgs/BaseVel.h>
#include <std_msgs/RobotBase2DOdom.h>
#include <std_msgs/Quaternion.h>

static int done = 0;

void finalize(int donecare)
{
  done = 1;
}


////////////////////////////////////////////////////////////////////////////////
// Return the rotation in Euler angles
libTF::Vector GetAsEuler(std_msgs::Quaternion quat)
{
  libTF::Vector vec;

  double squ;
  double sqx;
  double sqy;
  double sqz;

//  this->Normalize();

  squ = quat.w * quat.w;
  sqx = quat.x * quat.x;
  sqy = quat.y * quat.y;
  sqz = quat.z * quat.z;

  // Roll
  vec.x = atan2(2 * (quat.y*quat.z + quat.w*quat.x), squ - sqx - sqy + sqz);

  // Pitch
  vec.y = asin(-2 * (quat.x*quat.z - quat.w * quat.y));

  // Yaw
  vec.z = atan2(2 * (quat.x*quat.y + quat.w*quat.z), squ + sqx - sqy - sqz);

  return vec;
}


class test_run_base
{
  public:

    test_run_base():subscriber_connected(0){}; 

    ~test_run_base() {}

    std_msgs::TransformWithRateStamped ground_truth;

    std_msgs::RobotBase2DOdom odom;

    int subscriber_connected;

    void subscriber_connect()
    {
      subscriber_connected = 1;
    }

    void odomMsgReceived()
    {
//       cout << "Odom:: (" << ground_truth.transform.translation.x << "), (" <<  ground_truth.transform.rotation.x << ")" << std::endl;  
    };

    void groundTruthMsgReceived()
    {
//       cout << "Odom:: (" << ground_truth.transform.translation.x << "), (" <<  ground_truth.transform.rotation.x << ")" << std::endl;  
    };
};

int main( int argc, char** argv )
{

  /*********** Initialize ROS  ****************/
  ros::init(argc,argv);
  ros::node *node = new ros::node("test_run_base_controller"); 


  // receive messages from 2dnav stack
  std_msgs::TransformWithRateStamped ground_truth;

  test_run_base tb;

  node->subscribe("base_pose_ground_truth",tb.ground_truth,&test_run_base::odomMsgReceived,&tb,10);

  node->subscribe("odom",tb.odom,&test_run_base::groundTruthMsgReceived,&tb,10);

  signal(SIGINT,  finalize);
  signal(SIGQUIT, finalize);
  signal(SIGTERM, finalize);


  /*********** Start moving the robot ************/
  std_msgs::BaseVel cmd;
  cmd.vx = 0;
  cmd.vy = 0;
  cmd.vw = 0;

  if(argc >= 2)
    cmd.vx = atof(argv[1]);

  if(argc >= 3)
    cmd.vy = atof(argv[2]);

  if(argc == 4)
    cmd.vw = atof(argv[3]);

  node->advertise<std_msgs::BaseVel>("cmd_vel",10);
  sleep(1);
  node->publish("cmd_vel",cmd);
  sleep(1);

  libTF::Vector ang_rates;

  while(!done)
  {
    //   ang_rates = GetAsEuler(tb.ground_truth.rate.rotation);
    cout << "g:: " << tb.ground_truth.rate.translation.x <<  " " << tb.ground_truth.rate.translation.y << " "  << tb.ground_truth.rate.rotation.z  << " " <<   tb.ground_truth.header.stamp.sec + tb.ground_truth.header.stamp.nsec/1.0e9 << std::endl;
    cout << "o:: " << tb.odom.vel.x <<  " " << tb.odom.vel.y << " " << tb.odom.vel.th << " " << tb.odom.header.stamp.sec + tb.odom.header.stamp.nsec/1.0e9 << std::endl;
    node->publish("cmd_vel",cmd);
    sleep(1);
  }
}

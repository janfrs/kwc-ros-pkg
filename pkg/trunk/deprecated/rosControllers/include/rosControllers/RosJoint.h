/*
 *  ROS WRAPPER FOR JOINT CONTROLLER
 *
 *  stubbed, to be filled in
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// #include <genericControllers/RosJoint.h>

// roscpp
#include <ros/node.h>
// roscpp - laser
#include <std_msgs/LaserScan.h>
// roscpp - laser image (point cloud)
#include <std_msgs/PointCloudFloat32.h>
#include <std_msgs/Point3DFloat32.h>
#include <std_msgs/ChannelFloat32.h>
// roscpp - used for shutter message right now
#include <std_msgs/Empty.h>
// roscpp - used for broadcasting time over ros
#include <rostools/Time.h>
// roscpp - base
#include <std_msgs/RobotBase2DOdom.h>
#include <std_msgs/BaseVel.h>
// roscpp - arm
#include <std_msgs/PR2Arm.h>
// roscpp - camera
#include <std_msgs/Image.h>

// for frame transforms
#include <rosTF/rosTF.h>

#include <time.h>
#include <iostream>
#include <cassert>
// Header created from the message
#include <rosControllers/JointState.h>
#include <mechanism_model/joint.h>
#include <libpr2HW/pr2HW.h>

// Our node
// TODO: add documentation

namespace controller
{
  
class RosJoint
{
  private:
    // This node only sends information about the current joint
    // You need a controller if you want to pass some torque commands
    rosControllers::JointState jointStateMsg;

    // A mutex to lock access to fields that are used in message callbacks
    ros::thread::mutex lock;


  public:
    // You should not use this constructor
    // The name should be given by the joint controller directly
    RosJoint(mechanism::Joint *joint, std::string jointName);
    //For old API...
    RosJoint(PR2::PR2HW * hw, int joint_id, std::string jointName);
    // The constructor that should be used
    // It assumes the name of the joint controller is properly initialized
    ~RosJoint();
    
    // Reigsters a joint controller to this class
    // The constructor should be used instead
//     void init(controller::RosJoint *jc);
    // advertise / subscribe models
    // TODO: return significant return value?
    int AdvertiseSubscribeMessages();

//     void init(controller::RosJoint *jc);
    // Do one update of the simulator.  May pause if the next update time
    // has not yet arrived.
    void Update();

    //Keep track of controllers
    mechanism::Joint* joint;
    
    std::string busName(void) const { return mStateBusName; }
  
  private:
    std::string mJointName; //we keep the name of the joint
    std::string mCmdBusName; //The name of the bus that will accept commands
    std::string mStateBusName; //The name of the bus that will send state
    // Since we cannot publish messages on our node, we have to rely on an external publisher
    ros::node * publisherNode;
    
    
    PR2::PR2HW * hw_;
    int joint_id_;
};


}


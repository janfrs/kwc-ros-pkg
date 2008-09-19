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

#pragma once

/***************************************************/
/*! \class controller::GripperTest1
    \brief Gripper Test1 Controller

    This tests the hysteresis of the gripper using a
    velocity controller.

*/
/***************************************************/

#include <std_msgs/ChannelFloat32.h>
#include <ros/node.h>
#include <math.h>
#include <robot_msgs/DiagnosticMessage.h>
#include <misc_utils/realtime_publisher.h>
#include <generic_controllers/controller.h>
#include <generic_controllers/joint_velocity_controller.h>

namespace controller
{

class GripperTest1 : public Controller
{

public:
  /*!
   * \brief Default Constructor of the GripperTest1 class.
   *
   */
  GripperTest1();

  /*!
   * \brief Destructor of the GripperTest1 class.
   */
  ~GripperTest1();

  /*!
   * \brief Functional way to initialize.
   * \param duration The duration in seconds from start to finish.
   * \param time The current hardware time.
   * \param *robot The robot that is being controlled.
   */
  void init( double velocity, double max_effort, double time, std::string name ,mechanism::RobotState *robot);
  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

  
  /*!
   * \brief Perform the test analysis
   */
  void analysis();

  /*!
   * \brief Issues commands to the joint. Should be called at regular intervals
   */

  virtual void update();

private:

  mechanism::JointState *joint_;                        /**< Joint we're controlling. */
  Actuator *actuator_;                              /**< Actuator we're controlling. */
  mechanism::RobotState *robot_;                        /**< Pointer to robot structure. */
  controller::JointVelocityController *velocity_controller_;    /**< The sine sweep. */
  double velocity_;                                    /**< Torque applied during the test. */
  double max_effort_;
  double initial_time_;                                 /**< Start time of the test. */
  int count_;
  int loop_count_;
  bool complete;
  bool start;
  misc_utils::RealtimePublisher<robot_msgs::DiagnosticMessage> publisher_;
  misc_utils::RealtimePublisher<std_msgs::ChannelFloat32> data_publisher_;

  robot_msgs::DiagnosticMessage diagnostic_message_;
   
  std_msgs::ChannelFloat32 test_effort_;
  std_msgs::ChannelFloat32 test_velocity_;
  std_msgs::ChannelFloat32 test_cmd_;
  std_msgs::ChannelFloat32 test_position_;
  std_msgs::ChannelFloat32 test_time_;
  
  
};

/***************************************************/
/*! \class controller::GripperTest1Node
    \brief Motor Test1 Controller

    This tests the hysteresis of the gripper using a
    velocity controller.

*/
/***************************************************/

class GripperTest1Node : public Controller
{
public:
  /*!
   * \brief Default Constructor
   *
   */
  GripperTest1Node();

  /*!
   * \brief Destructor
   */
  ~GripperTest1Node();

  void update();

  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

private:
  GripperTest1 *c_;
};
}



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
/*! \class controller::SineSweepController
    \brief Sine Sweep Controller

    This class basically applies a sine sweep to the joint.
*/
/***************************************************/

#include <std_msgs/ChannelFloat32.h>
#include <ros/node.h>
#include <math.h>
#include <robot_msgs/DiagnosticMessage.h>
#include <misc_utils/realtime_publisher.h>
#include <mechanism_model/controller.h>
#include <control_toolbox/sine_sweep.h>


namespace controller
{

class SineSweepController : public Controller
{
public:
  /*!
   * \brief Default Constructor of the SineSweepController class.
   *
   */
  SineSweepController();

  /*!
   * \brief Destructor of the SineSweepController class.
   */
  ~SineSweepController();

  /*!
   * \brief Functional way to initialize.
   * \param start_freq The start value of the sweep (Hz).
   * \param end_freq  The end value of the sweep (Hz).
   * \param amplitude The amplitude of the sweep (N).
   * \param duration The duration in seconds from start to finish (s).
   * \param time The current hardware time.
   * \param *robot The robot that is being controlled.
   */
  void init(double start_freq, double end_freq, double duration, double amplitude, double time,std::string name,mechanism::RobotState *robot);
  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

  void analysis();
  virtual void update();

private:
  mechanism::JointState *joint_state_;      /**< Joint we're controlling. */
  mechanism::RobotState *robot_;            /**< Pointer to robot structure. */
  control_toolbox::SineSweep *sweep_;       /**< Sine sweep. */
  double duration_;                         /**< Duration of the sweep. */
  double initial_time_;                     /**< Start time of the sweep. */
  int count_;
  bool done_;
  
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
/*! \class controller::SineSweepControllerNode
    \brief Sine Sweep Controller ROS Node

*/
/***************************************************/

class SineSweepControllerNode : public Controller
{
public:
 
  SineSweepControllerNode();
  ~SineSweepControllerNode();

  void update();
  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

private:
  SineSweepController *c_;
};
}



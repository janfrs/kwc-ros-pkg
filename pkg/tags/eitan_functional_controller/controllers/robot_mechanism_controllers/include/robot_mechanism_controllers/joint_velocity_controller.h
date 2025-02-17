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
/*! \class controller::JointVelocityController
    \brief Joint Velocity Controller

    This class closes the loop around velocity using
    a pid loop.

    Example config:

    <controller type="JointVelocityController" name="controller_name">
      <joint name="joint_to_control">
        <pid p="1.0" i="2.0" d="3.0" iClamp="4.0" />
      </joint>
    </controller>
*/
/***************************************************/

#include <ros/node.h>

#include <mechanism_model/controller.h>
#include <control_toolbox/pid.h>
#include "misc_utils/advertised_service_guard.h"

// Services
#include <robot_mechanism_controllers/SetCommand.h>
#include <robot_mechanism_controllers/GetActual.h>

namespace controller
{

//TODO: dpcument smoothing
class JointVelocityController : public Controller
{
public:
  /*!
   * \brief Default Constructor of the JointController class.
   *
   */
  JointVelocityController();

  /*!
   * \brief Destructor of the JointController class.
   */
  ~JointVelocityController();

  bool init(mechanism::RobotState *robot_state, const std::string &joint_name, const control_toolbox::Pid &pid);
  bool initXml(mechanism::RobotState *robot_state, TiXmlElement *config);

  /*!
   * \brief Give set position of the joint for next update: revolute (angle) and prismatic (position)
   *
   * \param double pos Velocity command to issue
   */
  void setCommand(double command);

  /*!
   * \brief Get latest position command to the joint: revolute (angle) and prismatic (position).
   */
  double getCommand();

  /*!
   * \brief Get latest time..
   */
  double getTime();

  /*!
   * \brief Read the torque of the motor
   */
  double getMeasuredVelocity();

  /*!
   * \brief Issues commands to the joint. Should be called at regular intervals
   */

  virtual void update();

  void getGains(double &p, double &i, double &d, double &i_max, double &i_min);

  void setGains(const double &p, const double &i, const double &d, const double &i_max, const double &i_min);

  std::string getJointName();

private:
  mechanism::JointState *joint_state_;
  mechanism::RobotState *robot_state_; /**< Pointer to robot structure. */
  control_toolbox::Pid pid_;
  double last_time_;        /**< Last time stamp of update. */
  double command_;          /**< Last commanded position. */

  double smoothed_velocity_; /** */
  double smoothing_factor_;

};

/***************************************************/
/*! \class controller::JointVelocityControllerNode
    \brief Joint Velocity Controller ROS Node

    This class closes the loop around velocity using
    a pid loop.

    The xml config is the same as for JointVelocityController except
    the addition of a "topic" attribute, which determines the
    namespace over which messages are published and services are
    offered.
*/
/***************************************************/

class JointVelocityControllerNode : public Controller
{
public:
  /*!
   * \brief Default Constructor
   *
   */
  JointVelocityControllerNode();

  /*!
   * \brief Destructor
   */
  ~JointVelocityControllerNode();

  void update();

  bool initXml(mechanism::RobotState *robot_state, TiXmlElement *config);

  // Services
  bool setCommand(robot_mechanism_controllers::SetCommand::request &req,
                  robot_mechanism_controllers::SetCommand::response &resp);

  bool getActual(robot_mechanism_controllers::GetActual::request &req,
                  robot_mechanism_controllers::GetActual::response &resp);

private:
  JointVelocityController *c_;
  AdvertisedServiceGuard guard_set_command_, guard_get_actual_;
};
}


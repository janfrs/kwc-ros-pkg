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
/*! \class controller::JointEffortController
    \brief Joint Torque Controller

    This class basically passes the commanded effort
    down through the transmissions and safety code.

    <controller type="JointEffortController" name="controller_name">
      <joint name="joint_to_control" />
    </controller>

*/
/***************************************************/


#include <ros/node.h>
#include <mechanism_model/controller.h>
#include "misc_utils/advertised_service_guard.h"

// Services
#include <robot_mechanism_controllers/SetCommand.h>
#include <robot_mechanism_controllers/GetActual.h>

namespace controller
{

class JointEffortController : public Controller
{
public:
  /*!
   * \brief Default Constructor of the JointEffortController class.
   *
   */
  JointEffortController();

  /*!
   * \brief Destructor of the JointEffortController class.
   */
  ~JointEffortController();

  /*!
   * \brief Functional way to initialize limits and gains.
   *
   */
  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

  /*!
   * \brief Give set position of the joint for next update: revolute (angle) and prismatic (position)
   *
   * \param command
   */
  void setCommand(double command);

  /*!
   * \brief Get latest position command to the joint: revolute (angle) and prismatic (position).
   */
  double getCommand();

  /*!
   * \brief Read the effort of the joint
   */
  double getMeasuredEffort();

  /*!
   * \brief Get latest time..
   */
  double getTime();

  /*!
   * \brief Issues commands to the joint. Should be called at regular intervals
   */

  virtual void update();

  std::string getJointName();

  mechanism::JointState *joint_; /**< Joint we're controlling. */

private:
  mechanism::RobotState *robot_; /**< Pointer to robot structure. */
  double command_;          /**< Last commanded position. */
};

/***************************************************/
/*! \class controller::JointEffortControllerNode
    \brief Joint Torque Controller ROS Node

    This class basically passes the commanded effort
    down through the transmissions and safety code.

*/
/***************************************************/

class JointEffortControllerNode : public Controller
{
public:
  /*!
   * \brief Default Constructor
   *
   */
  JointEffortControllerNode();

  /*!
   * \brief Destructor
   */
  ~JointEffortControllerNode();

  void update();

  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

  // Services
  bool setCommand(robot_mechanism_controllers::SetCommand::request &req,
                  robot_mechanism_controllers::SetCommand::response &resp);

  bool getActual(robot_mechanism_controllers::GetActual::request &req,
                  robot_mechanism_controllers::GetActual::response &resp);

private:
  JointEffortController *c_;
  AdvertisedServiceGuard guard_set_command_, guard_get_actual_;
};
}



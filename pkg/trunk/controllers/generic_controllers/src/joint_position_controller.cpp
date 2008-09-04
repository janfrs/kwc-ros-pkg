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
#include <generic_controllers/joint_position_controller.h>
#include <math_utils/angles.h>

using namespace std;
using namespace controller;

ROS_REGISTER_CONTROLLER(JointPositionController)

JointPositionController::JointPositionController()
: joint_state_(NULL), robot_(NULL)
{
  // Initialize PID class
  pid_controller_.initPid(0, 0, 0, 0, 0);
  command_ = 0;
  last_time_ = 0;
}

JointPositionController::~JointPositionController()
{
}

bool JointPositionController::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  assert(robot);
  robot_ = robot;
  last_time_ = robot->hw_->current_time_;

  TiXmlElement *j = config->FirstChildElement("joint");
  if (!j)
  {
    fprintf(stderr, "JointPositionController was not given a joint\n");
    return false;
  }

  const char *joint_name = j->Attribute("name");
  int index = joint_name ? robot->model_->getJointIndex(joint_name) : -1;
  if (index < 0)
  {
    fprintf(stderr, "JointPositionController could not find joint named \"%s\"\n", joint_name);
    return false;
  }
  joint_state_ = &robot->joint_states_[index];

  TiXmlElement *p = j->FirstChildElement("pid");
  if (p)
    pid_controller_.initXml(p);
  else
    fprintf(stderr, "JointPositionController's config did not specify the default pid parameters.\n");

  return true;
}

void JointPositionController::setGains(const double &p, const double &i, const double &d, const double &i_max, const double &i_min)
{
  pid_controller_.setGains(p,i,d,i_max,i_min);
}

void JointPositionController::getGains(double &p, double &i, double &d, double &i_max, double &i_min)
{
  pid_controller_.getGains(p,i,d,i_max,i_min);
}

// Set the joint position command
void JointPositionController::setCommand(double command)
{
  command_ = command;
}

std::string JointPositionController::getJointName()
{
  return joint_state_->joint_->name_;
}

// Return the current position command
double JointPositionController::getCommand()
{
  return command_;
}

// Return the measured joint position
double JointPositionController::getMeasuredPosition()
{
  return joint_state_->position_;
}

double JointPositionController::getTime()
{
  return robot_->hw_->current_time_;
}

void JointPositionController::update()
{
  assert(robot_ != NULL);
  double error(0);
  double time = robot_->hw_->current_time_;

  if(joint_state_->joint_)
  {
    if(joint_state_->joint_->type_ == mechanism::JOINT_ROTARY ||
       joint_state_->joint_->type_ == mechanism::JOINT_CONTINUOUS)
    {
      error = math_utils::shortest_angular_distance(command_, joint_state_->position_);
    }
    else
    {
      error = joint_state_->position_ - command_;
    }

    joint_state_->commanded_effort_ = pid_controller_.updatePid(error, time - last_time_);
  }
  last_time_ = time;
}

ROS_REGISTER_CONTROLLER(JointPositionControllerNode)
JointPositionControllerNode::JointPositionControllerNode()
{
  c_ = new JointPositionController();
}

JointPositionControllerNode::~JointPositionControllerNode()
{
  delete c_;
}

void JointPositionControllerNode::update()
{
  c_->update();
}

bool JointPositionControllerNode::setCommand(
  generic_controllers::SetCommand::request &req,
  generic_controllers::SetCommand::response &resp)
{
  c_->setCommand(req.command);
  resp.command = c_->getCommand();

  return true;
}

void JointPositionControllerNode::setCommand(double command)
{
  c_->setCommand(command);
}


// Return the current position command
double JointPositionControllerNode::getCommand()
{
  return c_->getCommand();
}


bool JointPositionControllerNode::getActual(
  generic_controllers::GetActual::request &req,
  generic_controllers::GetActual::response &resp)
{
  resp.command = c_->getMeasuredPosition();
  resp.time = c_->getTime();
  return true;
}

double JointPositionControllerNode::getMeasuredPosition()
{
  return c_->getMeasuredPosition();
}

bool JointPositionControllerNode::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  ros::node *node = ros::node::instance();
  assert(node);
  string prefix = config->Attribute("name");

  std::string topic = config->Attribute("topic") ? config->Attribute("topic") : "";
  if (topic == "")
  {
    fprintf(stderr, "No topic given to JointPositionControllerNode\n");
    return false;
  }

  if (!c_->initXml(robot, config))
    return false;
  node->advertise_service(prefix + "/set_command", &JointPositionControllerNode::setCommand, this);
  node->advertise_service(prefix + "/get_actual", &JointPositionControllerNode::getActual, this);
  return true;
}


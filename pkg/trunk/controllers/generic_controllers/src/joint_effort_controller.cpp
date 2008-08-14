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
#include <algorithm>
#include <generic_controllers/joint_effort_controller.h>


using namespace std;
using namespace controller;

ROS_REGISTER_CONTROLLER(JointEffortController)

JointEffortController::JointEffortController()
{
  command_ = 0;
}

JointEffortController::~JointEffortController()
{
}

void JointEffortController::init(mechanism::Robot *robot,mechanism::Joint *joint)
{
  command_= 0;
  robot_ = robot;
  joint_ = joint;
}

void JointEffortController::init(double p_gain, double i_gain, double d_gain, double windup, double time,mechanism::Robot *robot, mechanism::Joint *joint)
{
}


void JointEffortController::initXml(mechanism::Robot *robot, TiXmlElement *config)
{
   
  TiXmlElement *elt = config->FirstChildElement("joint");
  if (elt) 
  {
    init(robot,robot->getJoint(elt->Attribute("name")));
  }
    
}

// Set the joint position command
void JointEffortController::setCommand(double command)
{
  command_ = command;
}

// Return the current position command
double JointEffortController::getCommand()
{
  return command_;
}

// Return the measured joint position
double JointEffortController::getActual()
{
  return joint_->applied_effort_;
}

double JointEffortController::getTime()
{
  return robot_->hw_->current_time_;
}

void JointEffortController::update()
{

  setJointEffort(command_);
}

void JointEffortController::setJointEffort(double effort)
{
  joint_->commanded_effort_ = min(max(effort, -joint_->effort_limit_), joint_->effort_limit_);
}

ROS_REGISTER_CONTROLLER(JointEffortControllerNode)
JointEffortControllerNode::JointEffortControllerNode() 
{
  c_ = new JointEffortController();
}

JointEffortControllerNode::~JointEffortControllerNode()
{
  delete c_;
}

void JointEffortControllerNode::update()
{
  c_->update();
}

bool JointEffortControllerNode::setCommand(
  generic_controllers::SetCommand::request &req,
  generic_controllers::SetCommand::response &resp)
{
  c_->setCommand(req.command);
  resp.command = c_->getCommand();

  return true;
}

bool JointEffortControllerNode::getActual(
  generic_controllers::GetActual::request &req,
  generic_controllers::GetActual::response &resp)
{
  resp.command = c_->getActual();
  resp.time = c_->getTime();
  return true;
}

void JointEffortControllerNode::init(double p_gain, double i_gain, double d_gain, double windup, double time,mechanism::Robot *robot, mechanism::Joint *joint)
{
  ros::node *node = ros::node::instance();
  string prefix = joint->name_;
  
  c_->init(p_gain, i_gain, d_gain, windup, time,robot, joint);
  node->advertise_service(prefix + "/set_command", &JointEffortControllerNode::setCommand, this);
  node->advertise_service(prefix + "/get_actual", &JointEffortControllerNode::getActual, this);
}


void JointEffortControllerNode::initXml(mechanism::Robot *robot, TiXmlElement *config)
{
  ros::node *node = ros::node::instance();
  string prefix = config->Attribute("name");
  
  c_->initXml(robot, config);
  node->advertise_service(prefix + "/set_command", &JointEffortControllerNode::setCommand, this);
  node->advertise_service(prefix + "/get_actual", &JointEffortControllerNode::getActual, this);
}


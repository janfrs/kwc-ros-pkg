
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2008, Eric Berger
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////

/*
 * Author: Stuart Glaser
 */
#ifndef MECHANISM_CONTROL_H
#define MECHANISM_CONTROL_H

#include <pthread.h>
#include <map>
#include <string>
#include <vector>
#include "ros/node.h"
#include "rostools/Time.h"
#include <tinyxml/tinyxml.h>
#include <hardware_interface/hardware_interface.h>
#include <mechanism_model/robot.h>
#include <rosthread/mutex.h>
#include <mechanism_model/controller.h>
#include <misc_utils/realtime_publisher.h>

#include "mechanism_control/ListControllerTypes.h"
#include "mechanism_control/ListControllers.h"
#include "mechanism_control/SpawnController.h"
#include "mechanism_control/KillController.h"
#include "mechanism_control/MechanismState.h"
#include "rosTF/TransformArray.h"


typedef controller::Controller* (*ControllerAllocator)();

class MechanismControl {
public:
  MechanismControl(HardwareInterface *hw);
  virtual ~MechanismControl();

  // Real-time functions
  void update();

  // Non real-time functions
  bool initXml(TiXmlElement* config);
  void getControllerNames(std::vector<std::string> &v);
  bool addController(controller::Controller *c, const std::string &name);
  bool spawnController(const std::string &type, const std::string &name, TiXmlElement *config);
  bool killController(const std::string &name);
  controller::Controller* getControllerByName(std::string name);

  mechanism::Robot model_;
  mechanism::RobotState *state_;
  HardwareInterface *hw_;

private:
  bool initialized_;

  const static int MAX_NUM_CONTROLLERS = 100;
  ros::thread::mutex controllers_lock_;
  controller::Controller* controllers_[MAX_NUM_CONTROLLERS];
  std::string controller_names_[MAX_NUM_CONTROLLERS];

  // Killing a controller:
  // 1. Non-realtime thread places the index of the controller into please_remove_
  // 2. Realtime thread moves the controller out of the array and into removed_
  // 3. Non-realtime thread can now delete the controller
  // The non-realtime thread will hold controllers_lock_ throughout the process.
  // please_remove_ must be -1 when not removing
  int please_remove_;
  controller::Controller* removed_;
};

/*
 * Exposes MechanismControl's interface over ROS
 */
class MechanismControlNode
{
public:
  MechanismControlNode(MechanismControl *mc);
  virtual ~MechanismControlNode();

  bool initXml(TiXmlElement *config);

  void update();  // Must be realtime safe

  bool listControllerTypes(mechanism_control::ListControllerTypes::request &req,
                           mechanism_control::ListControllerTypes::response &resp);
  bool listControllers(mechanism_control::ListControllers::request &req,
                       mechanism_control::ListControllers::response &resp);
  bool spawnController(mechanism_control::SpawnController::request &req,
                       mechanism_control::SpawnController::response &resp);

private:
  ros::node *node_;

  bool killController(mechanism_control::KillController::request &req,
                      mechanism_control::KillController::response &resp);

  MechanismControl *mc_;

  static const double STATE_PUBLISHING_PERIOD = 0.01;  // this translates to about 100Hz

  mechanism_control::MechanismState mechanism_state_;
  const char* const mechanism_state_topic_;
  misc_utils::RealtimePublisher<mechanism_control::MechanismState> publisher_;

  rosTF::TransformArray transform_array_msg_;
  misc_utils::RealtimePublisher<rosTF::TransformArray> transform_array_publisher_;

  rostools::Time time_msg_;
  misc_utils::RealtimePublisher<rostools::Time> time_publisher_;
};

#endif /* MECHANISM_CONTROL_H */

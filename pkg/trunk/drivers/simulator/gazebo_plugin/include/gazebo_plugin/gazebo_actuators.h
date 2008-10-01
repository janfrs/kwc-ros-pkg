/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GAZEBO_ACTUATORS_H
#define GAZEBO_ACTUATORS_H

#include <vector>
#include <map>
#include <gazebo/Controller.hh>
#include <gazebo/Entity.hh>
#include <gazebo/Model.hh>
#include "hardware_interface/hardware_interface.h"
#include "mechanism_control/mechanism_control.h"
#include "mechanism_model/robot.h"
#include "tinyxml/tinyxml.h"


namespace gazebo
{
class HingeJoint;
class XMLConfigNode;

/// @addtogroup gazebo_dynamic_plugins Gazebo ROS Dynamic Plugins
/// @{
/** \defgroup gazebo_actuators GazeboActuators class

  \brief GazeboActuators Plugin
  
  This is a controller that provides interface between simulator and the Robot Mechanism Control.
  GazeboActuators requires model as its parent.

  \verbatim
  <model:physical name="ray_model">
    <!-- GazeboActuators -->
    <controller:gazebo_actuators name="gazebo_actuators" plugin="libgazebo_actuators.so">
      <alwaysOn>true</alwaysOn>
      <updateRate>1000.0</updateRate>
      <robot filename="pr2.xml" /> <!-- gazebo_actuators use this file to extract mechanism model -->
      <gazebo_physics filename="gazebo_joints.xml" /> <!-- for simulator/physics specific settigs, currently just damping -->
      <interface:audio name="gazebo_actuators_dummy_iface" />
    </controller:gazebo_actuators>
  </model:phyiscal>
  \endverbatim
 
\{


*/

/**
 * Gazebo simulator provides joint level control for mechanisms.  In order to work with mechanisms in real life
 * at the level of actuators, a plugin is required.
 * As implemented here in GazeboActuators, this plugin abstracts the definitions of
 * actuators and transmissions.  It parses the \e robot.xml, \e actuators.xml
 * and \e transmissions.xml, then sets up an abstract layer of actuators.  The entire chain of command from
 * controllers to actuators to simulated mechanism joints and back are implemented in this plugin.
 *
 * - On the software/controller side:
 *   -# The plugin maintians a list of \c fake-actuators as described by \e actuators.xml, from which
 *      the actuator's \b encoder-value is transmitted to \b joint-state via \e transmissions.xml
 *   -# The controller reads \b joint-state from \c Mechanism-State and sends \b joint-error-value
 *      to the PID controller, then issues the resulting \b joint-torque-command to \c Mechanism-Model
 *   -# \b joint-torque-command is converted to \b actuator-current-command
 *      via transmission definition from \e transmissions.xml
 * - On the Hardware side in the simulator
 *   -# The plugin maintains a list of \c fake-actuators as described by \e actuators.xml,
 *      from which the simulator reads the \b actuator-current-command, reverse maps to \b joint-torque-command
 *      and stores in a set of \c fake-mechanism-states
 *   -# The \b Joint-torque-command is sent to simulated joint in ODE
 *   -# \b Simulator-joint-state is obtained from ODE and stored in \c fake-mechanism-states.
 *   -# \c Fake-mechanism-state's \b joint-state is converted to
 *      \b actuator-encoder values and stored in \c fake-actuators as defined by \e transmissions.xml
 * - On the software/controller side:
 *   -# [loops around] \b Actuator-encoder-value is transmitted to \b joint-state via \e transmissions.xml
 *   -# Controller reads \b joint-state and issues a \b joint-torque-command
 * .
 *
 * @image html "http://pr.willowgarage.com/wiki/gazebo_plugin?action=AttachFile&do=get&target=gazebo_mcn.jpg" "Gazebo Mechanism Control Model"
 *
**/


class GazeboActuators : public gazebo::Controller
{
public:
  GazeboActuators(Entity *parent);
  virtual ~GazeboActuators();

protected:
  // Inherited from gazebo::Controller
  virtual void LoadChild(XMLConfigNode *node);
  virtual void InitChild();
  virtual void UpdateChild();
  virtual void FiniChild();

private:

  Model *parent_model_;
  HardwareInterface hw_;
  MechanismControl mc_;
  MechanismControlNode mcn_;

  TiXmlDocument config_;

  /// @todo The fake model helps Gazebo run the transmissions backwards, so
  ///       that it can figure out what its joints should do based on the
  ///       actuator values.
  /// TODO  mechanism::Robot fake_model_;
  mechanism::RobotState *fake_state_;
  std::vector<gazebo::Joint*>  joints_;

  // added for joint damping coefficients
  std::vector<double>          joints_damping_;
  std::map<std::string,double> joints_damping_map_;

  /*
   * \brief read pr2.xml for actuators, and pass tinyxml node to mechanism control node's initXml.
   */
  void ReadPr2Xml(XMLConfigNode *node);

  /*
   * \brief read gazebo_joints.xml for joint damping and additional simulation parameters for joints
   */
  void ReadGazeboPhysics(XMLConfigNode *node);
};

/** \} */
/// @}

}

#endif


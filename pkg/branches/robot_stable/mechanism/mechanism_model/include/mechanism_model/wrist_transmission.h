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
/*
 * Author: Melonee Wise

   example xml:
    <robot name="wrist_trans">
      <joint name="right_wrist_flex_joint" type="revolute">
        <limit min="-0.157" max="2.409" effort="5" velocity="5" />
        <axis xyz="0 0 1" />
      </joint>

      <joint name="right_wrist_roll_joint" type="continuous">
        <limit min="0.0" max="0.0" effort="5" velocity="5" />
        <axis xyz="0 0 1" />
      </joint>

      <transmission type="WristTransmission" name="wrist_trans">
        <rightActuator name="right_wrist_r_motor"/>
        <leftActuator name="right_wrist_l_motor"/>
        <flexJoint name="wrist_right_flex_joint" mechanicalReduction="60.17"/>
        <rollJoint name="wrist_right_roll_joint" mechanicalReduction="60.17"/>
      </transmission>
    </robot>
 */
#ifndef WRIST_TRANSMISSION_H
#define WRIST_TRANSMISSION_H

#include <tinyxml/tinyxml.h>
#include "mechanism_model/transmission.h"
#include "mechanism_model/joint.h"
#include "hardware_interface/hardware_interface.h"

namespace mechanism {

class WristTransmission : public Transmission
{
public:
  WristTransmission() {}
  ~WristTransmission() {}

  bool initXml(TiXmlElement *config, Robot *robot);

  std::vector<double> mechanical_reduction_;

  // Describes the order of the actuators and the joints in the arrays
  // of names and of those passed to propagate*
  enum { RIGHT_MOTOR, LEFT_MOTOR };
  enum { FLEX_JOINT, ROLL_JOINT };

  void propagatePosition(std::vector<Actuator*>&, std::vector<JointState*>&);
  void propagatePositionBackwards(std::vector<JointState*>&, std::vector<Actuator*>&);
  void propagateEffort(std::vector<JointState*>&, std::vector<Actuator*>&);
  void propagateEffortBackwards(std::vector<Actuator*>&, std::vector<JointState*>&);
};

} // namespace mechanism

#endif

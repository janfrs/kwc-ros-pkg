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
 *
 */
#ifndef JOINT_H
#define JOINT_H

#include <tinyxml/tinyxml.h>

namespace mechanism {

class JointState;

class Joint{
public:
  Joint() : reference_position_(0), has_safety_limits_(false), safety_length_min_(0), safety_length_max_(0){}
  ~Joint() {}

  void enforceLimits(JointState *s);
  bool initXml(TiXmlElement *elt);

  std::string name_;
  int type_;

  // Constants
  double joint_limit_min_;  // In radians
  double joint_limit_max_;  // In radians
  double effort_limit_;
  double velocity_limit_;

  // Calibration parameters
  double reference_position_; // The reading of the reference position, in radians
  /** \brief indicates wether the offset calibration value has been determined and wether the position safety parameters should be enforced.
   * \note The velocity and effort safety parameters are always enforced.
   */
  // Safety parameters
  /** Indicates if some safety limit parameters have been provided. Should be deprecated soon.*/
  bool has_safety_limits_;
  double spring_constant_min_;
  double damping_constant_min_;
  double safety_length_min_;

  double spring_constant_max_;
  double damping_constant_max_;
  double safety_length_max_;

  // Axis of motion (x,y,z).  Axis of rotation for revolute/continuous
  // joints, axis of translation for prismatic joints.
  double axis_[3];
};


class JointState
{
public:
  Joint *joint_;

  // State
  double position_;  // In radians
  double velocity_;
  double applied_effort_;

  // Command
  double commanded_effort_;

  bool calibrated_;

  JointState() : joint_(NULL), commanded_effort_(0), calibrated_(false) {}
  JointState(const JointState &s)
    : joint_(s.joint_), position_(s.position_), velocity_(s.velocity_),
      applied_effort_(s.applied_effort_), commanded_effort_(s.commanded_effort_), calibrated_(s.calibrated_)
  {}

};

enum
{
  JOINT_NONE,
  JOINT_ROTARY,
  JOINT_CONTINUOUS,
  JOINT_PRISMATIC,
  JOINT_FIXED,
  JOINT_PLANAR,
  JOINT_TYPES_MAX
};

/** Some joints do not advertise their minimum and maximum values, or do not advertise safety limit infomrations. When set to true, this behaviour is considered illegal and generates an assertion failure. */
static const bool SAFETY_LIMS_STRICTLY_ENFORCED = false;


}

#endif /* JOINT_H */

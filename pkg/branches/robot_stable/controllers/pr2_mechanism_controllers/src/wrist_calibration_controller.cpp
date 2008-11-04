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

/*
 * Author: Stuart Glaser
 */

#include "pr2_mechanism_controllers/wrist_calibration_controller.h"

namespace controller {

WristCalibrationController::WristCalibrationController()
  : state_(INITIALIZED)
{
}

WristCalibrationController::~WristCalibrationController()
{
}

bool WristCalibrationController::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  assert(robot);
  assert(config);

  TiXmlElement *cal = config->FirstChildElement("calibrate");
  if (!cal)
  {
    std::cerr<<"WristCalibrationController was not given calibration parameters"<<std::endl;
    return false;
  }

  if(cal->QueryDoubleAttribute("velocity", &search_velocity_) != TIXML_SUCCESS)
  {
    std::cerr<<"Velocity value was not specified\n";
    return false;
  }

  const char *flex_joint_name = cal->Attribute("flex_joint");
  flex_joint_ = flex_joint_name ? robot->getJointState(flex_joint_name) : NULL;
  if (!flex_joint_)
  {
    fprintf(stderr, "Error: WristCalibrationController could not find flex joint \"%s\"\n",
            flex_joint_name);
    return false;
  }

  const char *roll_joint_name = cal->Attribute("roll_joint");
  roll_joint_ = roll_joint_name ? robot->getJointState(roll_joint_name) : NULL;
  if (!roll_joint_)
  {
    fprintf(stderr, "Error: WristCalibrationController could not find roll_joint \"%s\"\n",
            roll_joint_name);
    return false;
  }

  const char *actuator_l_name = cal->Attribute("actuator_l");
  actuator_l_ = actuator_l_name ? robot->model_->getActuator(actuator_l_name) : NULL;
  if (!actuator_l_)
  {
    fprintf(stderr, "Error: WristCalibrationController could not find actuator_l \"%s\"\n",
            actuator_l_name);
    return false;
  }

  const char *actuator_r_name = cal->Attribute("actuator_r");
  actuator_r_ = actuator_r_name ? robot->model_->getActuator(actuator_r_name) : NULL;
  if (!actuator_r_)
  {
    fprintf(stderr, "Error: WristCalibrationController could not find actuator_r \"%s\"\n",
            actuator_r_name);
    return false;
  }

  const char *transmission_name = cal->Attribute("transmission");
  transmission_ = transmission_name ? robot->model_->getTransmission(transmission_name) : NULL;
  if (!transmission_)
  {
    fprintf(stderr, "Error: WristCalibrationController could not find transmission \"%s\"\n",
            transmission_name);
    return false;
  }

  control_toolbox::Pid pid;
  TiXmlElement *p = config->FirstChildElement("pid");
  if (p)
    pid.initXml(p);
  else
  {
    fprintf(stderr, "WristCalibrationController's config did not specify the default pid parameters.\n");
    return false;
  }

  if (!vc_flex_.init(robot, flex_joint_name, pid))
    return false;
  if (!vc_roll_.init(robot, roll_joint_name, pid))
    return false;

  fprintf(stderr, "WristCalibrationController initialized!\n");

  return true;
}

void WristCalibrationController::update()
{
  // Flex optical switch is connected to actuator_l
  // Roll optical switch is connected to actuator_r

  switch(state_)
  {
  case INITIALIZED:
    actuator_l_->state_.zero_offset_ = 0;
    actuator_r_->state_.zero_offset_ = 0;
    vc_flex_.setCommand(0);
    vc_roll_.setCommand(0);
    state_ = BEGINNING;
    break;
  case BEGINNING:
    original_switch_state_ = actuator_l_->state_.calibration_reading_;
    vc_flex_.setCommand(original_switch_state_ ? -search_velocity_ : search_velocity_);
    vc_roll_.setCommand(0);
    state_ = MOVING_FLEX;
    break;
  case MOVING_FLEX: {
    bool switch_state_ = actuator_l_->state_.calibration_reading_;
    if (switch_state_ != original_switch_state_)
    {
      if (switch_state_ == true)
        flex_switch_l_ = actuator_l_->state_.last_calibration_rising_edge_;
      else
        flex_switch_l_ = actuator_l_->state_.last_calibration_falling_edge_;

      // But where was actuator_r at the transition?  Unfortunately,
      // actuator_r is not connected to the flex joint's optical
      // switch, so we don't know directly.  Instead, we estimate
      // actuator_r's position based on the switch position of
      // actuator_l.
      double dl = actuator_l_->state_.position_ - prev_actuator_l_position_;
      double dr = actuator_r_->state_.position_ - prev_actuator_r_position_;
      double k = (flex_switch_l_ - prev_actuator_l_position_) / dl;
      assert(0 <= k && k <= 1);
      flex_switch_r_ = k * dr + prev_actuator_r_position_;

      original_switch_state_ = actuator_r_->state_.calibration_reading_;
      vc_flex_.setCommand(0);
      vc_roll_.setCommand(original_switch_state_ ? -search_velocity_ : search_velocity_);
      state_ = MOVING_ROLL;
    }
    break;
  }
  case MOVING_ROLL: {
    bool switch_state_ = actuator_r_->state_.calibration_reading_;
    if (switch_state_ != original_switch_state_)
    {
      if (switch_state_ == true)
        roll_switch_r_ = actuator_r_->state_.last_calibration_rising_edge_;
      else
        roll_switch_r_ = actuator_r_->state_.last_calibration_falling_edge_;

      // See corresponding comment above.
      double dl = actuator_l_->state_.position_ - prev_actuator_l_position_;
      double dr = actuator_r_->state_.position_ - prev_actuator_r_position_;
      double k = (roll_switch_r_ - prev_actuator_r_position_) / dr;
      assert(0 <= k && k <= 1);
      roll_switch_l_ =  k * dl + prev_actuator_l_position_;


      //----------------------------------------------------------------------
      //       Calibration computation
      //----------------------------------------------------------------------

      // At this point, we know the actuator positions when the
      // optical switches were hit.  Now we compute the actuator
      // positions when the joints should be at 0.

      const int LEFT_MOTOR = mechanism::WristTransmission::LEFT_MOTOR;
      const int RIGHT_MOTOR = mechanism::WristTransmission::RIGHT_MOTOR;
      const int FLEX_JOINT = mechanism::WristTransmission::FLEX_JOINT;
      const int ROLL_JOINT = mechanism::WristTransmission::ROLL_JOINT;

      // Sets up the data structures for passing joint and actuator
      // positions through the transmission.
      Actuator fake_as_mem[2];  // This way we don't need to delete the objects later
      mechanism::JointState fake_js_mem[2];
      std::vector<Actuator*> fake_as;
      std::vector<mechanism::JointState*> fake_js;
      fake_as.push_back(&fake_as_mem[0]);
      fake_as.push_back(&fake_as_mem[1]);
      fake_js.push_back(&fake_js_mem[0]);
      fake_js.push_back(&fake_js_mem[1]);

      // Finds the (uncalibrated) joint position where the flex optical switch triggers
      fake_as[LEFT_MOTOR]->state_.position_ = flex_switch_l_;
      fake_as[RIGHT_MOTOR]->state_.position_ = flex_switch_r_;
      transmission_->propagatePosition(fake_as, fake_js);
      double flex_joint_switch_ = fake_js[FLEX_JOINT]->position_;

      // Finds the (uncalibrated) joint position where the roll optical switch triggers
      fake_as[LEFT_MOTOR]->state_.position_ = roll_switch_l_;
      fake_as[RIGHT_MOTOR]->state_.position_ = roll_switch_r_;
      transmission_->propagatePosition(fake_as, fake_js);
      double roll_joint_switch_ = fake_js[ROLL_JOINT]->position_;

      // Finds the (uncalibrated) joint position at the desired zero
      fake_js[FLEX_JOINT]->position_ = flex_joint_switch_ - flex_joint_->joint_->reference_position_;
      fake_js[ROLL_JOINT]->position_ = roll_joint_switch_ - roll_joint_->joint_->reference_position_;

      // Determines the actuator zero position from the desired joint zero positions
      transmission_->propagatePositionBackwards(fake_js, fake_as);
      actuator_l_->state_.zero_offset_ = fake_as[LEFT_MOTOR]->state_.position_;
      actuator_r_->state_.zero_offset_ = fake_as[RIGHT_MOTOR]->state_.position_;

      flex_joint_->calibrated_ = true;
      roll_joint_->calibrated_ = true;
      state_ = CALIBRATED;

      vc_flex_.setCommand(0);
      vc_roll_.setCommand(0);
    }

    break;
  }
  case CALIBRATED:
    break;
  }

  if (state_ != CALIBRATED)
  {
    vc_flex_.update();
    vc_roll_.update();
  }

  prev_actuator_l_position_ = actuator_l_->state_.position_;
  prev_actuator_r_position_ = actuator_r_->state_.position_;
}


ROS_REGISTER_CONTROLLER(WristCalibrationControllerNode)

WristCalibrationControllerNode::WristCalibrationControllerNode()
: last_publish_time_(0), pub_calibrated_(NULL)
{
}

WristCalibrationControllerNode::~WristCalibrationControllerNode()
{
  if (pub_calibrated_)
    delete pub_calibrated_;
}

void WristCalibrationControllerNode::update()
{
  c_.update();

  if (c_.calibrated())
  {
    if (last_publish_time_ + 0.5 < robot_->hw_->current_time_)
    {
      assert(pub_calibrated_);
      if (pub_calibrated_->trylock())
      {
        last_publish_time_ = robot_->hw_->current_time_;
        pub_calibrated_->unlockAndPublish();
      }
    }
  }
}

bool WristCalibrationControllerNode::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  assert(robot);
  robot_ = robot;

  std::string name = config->Attribute("name") ? config->Attribute("name") : "";
  if (name == "")
  {
    fprintf(stderr, "No name given to WristCalibrationController\n");
    return false;
  }

  if (!c_.initXml(robot, config))
    return false;

  pub_calibrated_ = new misc_utils::RealtimePublisher<std_msgs::Empty>(name + "/calibrated", 1);

  return true;
}

}

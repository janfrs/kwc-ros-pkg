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
/*! \class controller::JointCalibratonController
    \brief Joint Controller that finds zerop point
    \author Timothy Hunter <tjhunter@willowgarage.com>


    This class moves the joint and reads the value of the clibration_reading_ field to find the zero position of the joint. Once these are determined, these values
 * are passed to the joint and enable the joint for the other controllers.

 Example XML:
 <controller type="JointCalibrationController">
   <calibrate joint="upperarm_roll_right_joint"
              actuator="upperarm_roll_right_act"
              transmission="upperarm_roll_right_trans"
              velocity="0.3" />
   <pid p="15" i="0" d="0" iClamp="0" />
 </controller>



*/
/***************************************************/


#include "joint_manual_calibration_controller.h"

// Services
#include <robot_mechanism_controllers/CalibrateJoint.h>


namespace controller
{

class JointCalibrationController : public controller::Controller
{
public:
  /*!
   * \brief Default Constructor.
   *
   */
  JointCalibrationController();

  /*!
   * \brief Destructor.
   */
  virtual ~JointCalibrationController();

  /*!
   * \brief Functional way to initialize limits and gains.
   *
   */
  virtual bool initXml(mechanism::RobotState *robot, TiXmlElement *config);


  /*!
   * \brief Issues commands to the joint. Should be called at regular intervals
   */
  virtual void update();

  bool calibrated() { return state_ == STOPPED; }
  void beginCalibration()
  {
    if (state_ == INITIALIZED)
      state_ = BEGINNING;
  }

protected:

  enum { INITIALIZED, BEGINNING, MOVING, STOPPED };
  int state_;

  double search_velocity_;
  bool original_switch_state_;

  Actuator *actuator_;
  mechanism::JointState *joint_;
  mechanism::Transmission *transmission_;

  controller::JointVelocityController vc_;
};


/** @class controller::JointControllerCalibrationNode
 *  @\brief ROS interface for a joint calibration controller
 *  This class is a wrapper around the calibrateCmd service call and it should be its only use
 */
class JointCalibrationControllerNode : public Controller
{
public:
  /*!
   * \brief Default Constructor
   *
   */
  JointCalibrationControllerNode();

  /*!
   * \brief Destructor
   */
  ~JointCalibrationControllerNode();

  void update();

  bool initXml(mechanism::RobotState *robot, TiXmlElement *config);

  /** \brief initializes the calibration procedure (blocking service)
   * This service starts the calibration sequence of the joint and waits to return until the calibration sequence is finished.
   *
   * @param req
   * @param resp
   * @return
   */
  bool calibrateCommand(robot_mechanism_controllers::CalibrateJoint::request &req,
                        robot_mechanism_controllers::CalibrateJoint::response &resp);

private:
  JointCalibrationController *c_;
  AdvertisedServiceGuard guard_calibrate_;
};

}



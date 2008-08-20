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

// Author: Advait Jain

/**

  @mainpage

  @htmlinclude manifest.html

  @b teleop_arm_keyboard teleoperates the arms of the PR2 by mapping
  key presses into joint angle set points.

  <hr>

  @section usage Usage
  @verbatim
  $ teleop_arm_keyboard [standard ROS args]
  @endverbatim

  Key mappings are printed to screen on startup.

  <hr>

  @section topic ROS topics

  Subscribes to (name/type):
  - None

  Publishes to (name / type):
  - @b "cmd_leftarmconfig"/PR2Arm : configuration of the left arm (all the joint angles); sent on every keypress.

  <hr>

  @section parameters ROS parameters

  - None

 **/

#include <termios.h>
#include <signal.h>
#include <math.h>

#include <ros/node.h>
#include <std_msgs/PR2Arm.h>

// For transform support
#include <rosTF/rosTF.h>

#define COMMAND_TIMEOUT_SEC 0.2

/// @todo Remove this giant enum, which was stoled from pr2Core/pr2Core.h.
/// It can be replaced by some simpler indexing scheme.
enum PR2_JOINT_ID
{
  CASTER_FL_STEER   , // 0
  CASTER_FL_DRIVE_L , // 1 
  CASTER_FL_DRIVE_R , // 2
  CASTER_FR_STEER   , // 3
  CASTER_FR_DRIVE_L , // 4
  CASTER_FR_DRIVE_R , // 5
  CASTER_RL_STEER   , // 6
  CASTER_RL_DRIVE_L , // 7 
  CASTER_RL_DRIVE_R , // 8
  CASTER_RR_STEER   , // 9 
  CASTER_RR_DRIVE_L , // 10
  CASTER_RR_DRIVE_R , // 11
  SPINE_ELEVATOR    ,
  ARM_L_PAN         , 
  ARM_L_SHOULDER_PITCH, 
  ARM_L_SHOULDER_ROLL,
  ARM_L_ELBOW_PITCH , 
  ARM_L_ELBOW_ROLL  ,
  ARM_L_WRIST_PITCH , 
  ARM_L_WRIST_ROLL  ,
  ARM_L_GRIPPER_GAP ,  // added 20080802 by john
  ARM_R_PAN         , 
  ARM_R_SHOULDER_PITCH, 
  ARM_R_SHOULDER_ROLL,
  ARM_R_ELBOW_PITCH , 
  ARM_R_ELBOW_ROLL  ,
  ARM_R_WRIST_PITCH , 
  ARM_R_WRIST_ROLL  ,
  ARM_R_GRIPPER_GAP ,  // added 20080802 by john
  HEAD_YAW          , 
  HEAD_PITCH        ,
  HEAD_LASER_PITCH  ,
  HEAD_PTZ_L_PAN    , 
  HEAD_PTZ_L_TILT   ,
  HEAD_PTZ_R_PAN    , 
  HEAD_PTZ_R_TILT   ,
  BASE_6DOF,
  PR2_WORLD,
  MAX_JOINTS   
};

class TArmK_Node : public ros::node
{
  private:
    std_msgs::PR2Arm cmd_leftarmconfig;
    std_msgs::PR2Arm cmd_rightarmconfig;

  public:
    TArmK_Node() : ros::node("tarmk"), tf(*this, true)
  {
    // cmd_armconfig should probably be initialised
    // with the current joint angles of the arm rather
    // than zeros.
    this->cmd_leftarmconfig.turretAngle = 0;
    this->cmd_leftarmconfig.shoulderLiftAngle = 0;
    this->cmd_leftarmconfig.upperarmRollAngle = 0;
    this->cmd_leftarmconfig.elbowAngle        = 0;
    this->cmd_leftarmconfig.forearmRollAngle  = 0;
    this->cmd_leftarmconfig.wristPitchAngle   = 0;
    this->cmd_leftarmconfig.wristRollAngle    = 0;
    this->cmd_leftarmconfig.gripperForceCmd   = 1000;
    this->cmd_leftarmconfig.gripperGapCmd     = 0;
    this->cmd_rightarmconfig.turretAngle = 0;
    this->cmd_rightarmconfig.shoulderLiftAngle = 0;
    this->cmd_rightarmconfig.upperarmRollAngle = 0;
    this->cmd_rightarmconfig.elbowAngle        = 0;
    this->cmd_rightarmconfig.forearmRollAngle  = 0;
    this->cmd_rightarmconfig.wristPitchAngle   = 0;
    this->cmd_rightarmconfig.wristRollAngle    = 0;
    this->cmd_rightarmconfig.gripperForceCmd   = 1000;
    this->cmd_rightarmconfig.gripperGapCmd     = 0;
    advertise<std_msgs::PR2Arm>("cmd_leftarmconfig");
    advertise<std_msgs::PR2Arm>("cmd_rightarmconfig");

    _leftInit = false;
    _rightInit = false;

    //for getting positions
    subscribe("left_pr2arm_pos", leftArmPosMsg, &TArmK_Node::leftArmPosReceived);
    subscribe("right_pr2arm_pos", rightArmPosMsg, &TArmK_Node::rightArmPosReceived);
  }
    ~TArmK_Node() { }

    void printCurrentJointValues() {
      std::cout << "Left joint angles:" << std::endl;
      std::cout << "turretAngle " << leftArmPosMsg.turretAngle << std::endl;
      std::cout << "shoulderLiftAngle " << leftArmPosMsg.shoulderLiftAngle << std::endl;
      std::cout << "upperarmRollAngle " << leftArmPosMsg.upperarmRollAngle << std::endl;
      std::cout << "elbowAngle        " << leftArmPosMsg.elbowAngle << std::endl;
      std::cout << "forearmRollAngle  " << leftArmPosMsg.forearmRollAngle << std::endl;
      std::cout << "wristPitchAngle   " << leftArmPosMsg.wristPitchAngle << std::endl;
      std::cout << "wristRollAngle    " << leftArmPosMsg.wristRollAngle << std::endl;
      std::cout << "gripperForceCmd   " << leftArmPosMsg.gripperForceCmd << std::endl;
      std::cout << "gripperGapCmd     " << leftArmPosMsg.gripperGapCmd << std::endl;

      std::cout << "Right joint angles:" << std::endl;
      std::cout << "turretAngle " << rightArmPosMsg.turretAngle << std::endl;
      std::cout << "shoulderLiftAngle " << rightArmPosMsg.shoulderLiftAngle << std::endl;
      std::cout << "upperarmRollAngle " << rightArmPosMsg.upperarmRollAngle << std::endl;
      std::cout << "elbowAngle        " << rightArmPosMsg.elbowAngle << std::endl;
      std::cout << "forearmRollAngle  " << rightArmPosMsg.forearmRollAngle << std::endl;
      std::cout << "wristPitchAngle   " << rightArmPosMsg.wristPitchAngle << std::endl;
      std::cout << "wristRollAngle    " << rightArmPosMsg.wristRollAngle << std::endl;
      std::cout << "gripperForceCmd   " << rightArmPosMsg.gripperForceCmd << std::endl;
      std::cout << "gripperGapCmd     " << rightArmPosMsg.gripperGapCmd << std::endl;
    }

    void printCurrentEndEffectorWorldCoord() {
      libTF::TFPose aPose;
      aPose.x = 0.0;
      aPose.y = 0.0;
      aPose.z = 0.0;
      aPose.roll = 0;
      aPose.pitch = 0;
      aPose.yaw = 0;
      aPose.time = 0;
      aPose.frame = "gripper_roll_right";

      libTF::TFPose inOdomFrame = tf.transformPose("FRAMEID_ODOM", aPose);

      std::cout << "In odom frame x " << inOdomFrame.x << std::endl;
      std::cout << "In odom frame y " << inOdomFrame.y << std::endl;
      std::cout << "In odom frame z " << inOdomFrame.z << std::endl;
    }

    void printCurrentEndEffectorShoulderCoord() {
      libTF::TFPose aPose;
      aPose.x = .64;
      aPose.y = -.37;
      aPose.z = 1.76;
      aPose.roll = 0;
      aPose.pitch = 0;
      aPose.yaw = 0;
      aPose.time = 0;
      aPose.frame = "FRAMEID_ODOM";

      libTF::TFPose inOdomFrame = tf.transformPose("gripper_roll_right", aPose);

      std::cout << "In shoulder frame x " << inOdomFrame.x << std::endl;
      std::cout << "In shoulder frame y " << inOdomFrame.y << std::endl;
      std::cout << "In shoulder frame z " << inOdomFrame.z << std::endl;
    }

    void leftArmPosReceived() {
      if(_leftInit == false) {
        this->cmd_leftarmconfig.turretAngle = leftArmPosMsg.turretAngle;
        this->cmd_leftarmconfig.shoulderLiftAngle = leftArmPosMsg.shoulderLiftAngle;
        this->cmd_leftarmconfig.upperarmRollAngle = leftArmPosMsg.upperarmRollAngle;
        this->cmd_leftarmconfig.elbowAngle        = leftArmPosMsg.elbowAngle;
        this->cmd_leftarmconfig.forearmRollAngle  = leftArmPosMsg.forearmRollAngle;
        this->cmd_leftarmconfig.wristPitchAngle   = leftArmPosMsg.wristPitchAngle;
        this->cmd_leftarmconfig.wristRollAngle    = leftArmPosMsg.wristRollAngle;
        this->cmd_leftarmconfig.gripperForceCmd   = leftArmPosMsg.gripperForceCmd;
        this->cmd_leftarmconfig.gripperGapCmd     = leftArmPosMsg.gripperGapCmd;
        _leftInit = true;
      }
    }

    void rightArmPosReceived() {
      if(_rightInit == false) {
        this->cmd_rightarmconfig.turretAngle = rightArmPosMsg.turretAngle;
        this->cmd_rightarmconfig.shoulderLiftAngle = rightArmPosMsg.shoulderLiftAngle;
        this->cmd_rightarmconfig.upperarmRollAngle = rightArmPosMsg.upperarmRollAngle;
        this->cmd_rightarmconfig.elbowAngle        = rightArmPosMsg.elbowAngle;
        this->cmd_rightarmconfig.forearmRollAngle  = rightArmPosMsg.forearmRollAngle;
        this->cmd_rightarmconfig.wristPitchAngle   = rightArmPosMsg.wristPitchAngle;
        this->cmd_rightarmconfig.wristRollAngle    = rightArmPosMsg.wristRollAngle;
        this->cmd_rightarmconfig.gripperForceCmd   = rightArmPosMsg.gripperForceCmd;
        this->cmd_rightarmconfig.gripperGapCmd     = rightArmPosMsg.gripperGapCmd;
        _rightInit = true;
      }
    }

    void keyboardLoop();
    void changeJointAngle(PR2_JOINT_ID jointID, bool increment);
    void openGripper(PR2_JOINT_ID jointID);
    void closeGripper(PR2_JOINT_ID jointID);


    bool _leftInit;
    bool _rightInit;
    std_msgs::PR2Arm leftArmPosMsg, rightArmPosMsg;
    rosTFClient tf;
};

TArmK_Node* tarmk;
int kfd = 0;
struct termios cooked, raw;

  void
quit(int sig)
{
  //  tbk->stopRobot();
  ros::fini();
  tcsetattr(kfd, TCSANOW, &cooked);
  exit(0);
}

  int
main(int argc, char** argv)
{
  ros::init(argc,argv);

  tarmk = new TArmK_Node();

  signal(SIGINT,quit);

  tarmk->keyboardLoop();

  return(0);
}

void TArmK_Node::openGripper(PR2_JOINT_ID jointID) {
  if(jointID != ARM_R_GRIPPER_GAP && jointID != ARM_L_GRIPPER_GAP) return;
  if(_leftInit == false || _rightInit == false) {
    printf("No init, so not sending command.\n");
    return;
  }
  if(jointID == ARM_R_GRIPPER_GAP) {
    this->cmd_rightarmconfig.gripperForceCmd = 50;
    this->cmd_rightarmconfig.gripperGapCmd = .2;
    printf("Opening right gripper\n");
  } else { 
    this->cmd_leftarmconfig.gripperForceCmd = 50;
    this->cmd_leftarmconfig.gripperGapCmd = .2;
    printf("Opening left gripper\n");
  }
}

void TArmK_Node::closeGripper(PR2_JOINT_ID jointID) {
  if(jointID != ARM_R_GRIPPER_GAP && jointID != ARM_L_GRIPPER_GAP) return;
  if(_leftInit == false || _rightInit == false) {
    printf("No init, so not sending command.\n");
    return;
  }
  if(jointID == ARM_R_GRIPPER_GAP) {
    this->cmd_rightarmconfig.gripperForceCmd = 50;
    this->cmd_rightarmconfig.gripperGapCmd = 0;
  } else { 
    this->cmd_leftarmconfig.gripperForceCmd = 50;
    this->cmd_leftarmconfig.gripperGapCmd = 0;
  }
}


void TArmK_Node::changeJointAngle(PR2_JOINT_ID jointID, bool increment)
{
  if(_leftInit == false || _rightInit == false) {
    printf("No init, so not sending command.\n");
    return;
  }
  float jointCmdStep = 5*M_PI/180;
  float gripperStep = 0.002;
  if (increment == false)
  {
    jointCmdStep *= -1;
    gripperStep *= -1;
  }

  this->cmd_leftarmconfig.gripperForceCmd = 10; // FIXME: why is this getting reset to 0?
  this->cmd_rightarmconfig.gripperForceCmd = 10; // FIXME: why is this getting reset to 0?

  switch(jointID)
  {
    case ARM_L_PAN:
      this->cmd_leftarmconfig.turretAngle += jointCmdStep;
      break;
    case ARM_L_SHOULDER_PITCH:
      this->cmd_leftarmconfig.shoulderLiftAngle += jointCmdStep;
      break;
    case ARM_L_SHOULDER_ROLL:
      this->cmd_leftarmconfig.upperarmRollAngle += jointCmdStep;
      break;
    case ARM_L_ELBOW_PITCH:
      this->cmd_leftarmconfig.elbowAngle += jointCmdStep;
      break;
    case ARM_L_ELBOW_ROLL:
      this->cmd_leftarmconfig.forearmRollAngle += jointCmdStep;
      break;
    case ARM_L_WRIST_PITCH:
      this->cmd_leftarmconfig.wristPitchAngle += jointCmdStep;
      break;
    case ARM_L_WRIST_ROLL:
      this->cmd_leftarmconfig.wristRollAngle += jointCmdStep;
      break;
    case ARM_L_GRIPPER_GAP:
      this->cmd_leftarmconfig.gripperGapCmd += gripperStep;
      break;
    case ARM_R_PAN:
      this->cmd_rightarmconfig.turretAngle += jointCmdStep;
      break;
    case ARM_R_SHOULDER_PITCH:
      this->cmd_rightarmconfig.shoulderLiftAngle += jointCmdStep;
      break;
    case ARM_R_SHOULDER_ROLL:
      this->cmd_rightarmconfig.upperarmRollAngle += jointCmdStep;
      break;
    case ARM_R_ELBOW_PITCH:
      this->cmd_rightarmconfig.elbowAngle += jointCmdStep;
      break;
    case ARM_R_ELBOW_ROLL:
      this->cmd_rightarmconfig.forearmRollAngle += jointCmdStep;
      break;
    case ARM_R_WRIST_PITCH:
      this->cmd_rightarmconfig.wristPitchAngle += jointCmdStep;
      break;
    case ARM_R_WRIST_ROLL:
      this->cmd_rightarmconfig.wristRollAngle += jointCmdStep;
      break;
    case ARM_R_GRIPPER_GAP:
      this->cmd_rightarmconfig.gripperGapCmd += gripperStep;
      break;
    default:
      printf("This joint is not handled.\n");
      break;
  }
}


  void
TArmK_Node::keyboardLoop()
{
  char c;
  bool dirty=false;
  PR2_JOINT_ID curr_jointID = ARM_L_PAN; // joint which will be actuated.
  bool right_arm = false;

  // get the console in raw mode
  tcgetattr(kfd, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termios));
  raw.c_lflag &=~ (ICANON | ECHO);
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  tcsetattr(kfd, TCSANOW, &raw);

  puts("Reading from keyboard");
  puts("---------------------------");
  printf("Press l/r to operate left/right arm.\n");
  printf("Numbers 1 through 8 to select the joint to operate.\n");
  printf("9 initializes teleop_arm_keyboard's robot state with the state of the robot in the simulation.\n");
  printf("+ and - will move the joint in different directions by 5 degrees.\n");
  puts("");
  puts("---------------------------");

  for(;;)
  {
    // get the next event from the keyboard
    if(read(kfd, &c, 1) < 0)
    {
      perror("read():");
      exit(-1);
    }

    switch(c)
    {
      case 'l':
      case 'L':
        right_arm = false;
        printf("Actuating left arm.\n");
        break;
      case 'r':
      case 'R':
        right_arm = true;
        printf("Actuating right arm.\n");
        break;
      case '+':
      case '=':
        changeJointAngle(curr_jointID, true);
        dirty=true;
        break;
      case '_':
      case '-':
        changeJointAngle(curr_jointID, false);
        dirty=true;
        break;
      case '.':
        _rightInit = false;
        _leftInit = false;
        openGripper(curr_jointID);
        dirty = true;
        break;
      case '/':
        _rightInit = false;
        _leftInit = false;
        sleep(1);
        closeGripper(curr_jointID);
        dirty = true;
        break;
      case 'q':
        printCurrentJointValues();
        break;
      case 'k':
        printCurrentEndEffectorWorldCoord();
        break;
      case 'j':
        printCurrentEndEffectorShoulderCoord();
        break;
      default:
        break;
    }

    if (right_arm==false)
    {
      switch(c)
      {
        case '1':
          curr_jointID = ARM_L_PAN;
          printf("left turret\n");
          break;
        case '2':
          curr_jointID = ARM_L_SHOULDER_PITCH;
          printf("left shoulder pitch\n");
          break;
        case '3':
          curr_jointID = ARM_L_SHOULDER_ROLL;
          printf("left shoulder roll\n");
          break;
        case '4':
          curr_jointID = ARM_L_ELBOW_PITCH;
          printf("left elbow pitch\n");
          break;
        case '5':
          curr_jointID = ARM_L_ELBOW_ROLL;
          printf("left elbow roll\n");
          break;
        case '6':
          curr_jointID = ARM_L_WRIST_PITCH;
          printf("left wrist pitch\n");
          break;
        case '7':
          curr_jointID = ARM_L_WRIST_ROLL;
          printf("left wrist roll\n");
          break;
        case '8':
          curr_jointID = ARM_L_GRIPPER_GAP;
          printf("left gripper\n");
          break;
        case '9':
          _leftInit = false;
          printf("Resetting left commands to current position.\n");
        default:
          break;
      }
    }
    else
    {
      switch(c)
      {
        case '1':
          curr_jointID = ARM_R_PAN;
          printf("right turret\n");
          break;
        case '2':
          curr_jointID = ARM_R_SHOULDER_PITCH;
          printf("right shoulder pitch\n");
          break;
        case '3':
          curr_jointID = ARM_R_SHOULDER_ROLL;
          printf("right shoulder roll\n");
          break;
        case '4':
          curr_jointID = ARM_R_ELBOW_PITCH;
          printf("right elbow pitch\n");
          break;
        case '5':
          curr_jointID = ARM_R_ELBOW_ROLL;
          printf("right elbow roll\n");
          break;
        case '6':
          curr_jointID = ARM_R_WRIST_PITCH;
          printf("right wrist pitch\n");
          break;
        case '7':
          curr_jointID = ARM_R_WRIST_ROLL;
          printf("right wrist roll\n");
          break;
        case '8':
          curr_jointID = ARM_R_GRIPPER_GAP;
          printf("right gripper\n");
          break;
        case '9':
          _rightInit = false;
          printf("Resetting right commands to current position.\n");
        default:
          break;
      }
    }

    if (dirty == true) {
      dirty=false; // Sending the command only once for each key press.
      if(!right_arm) {
        publish("cmd_leftarmconfig",cmd_leftarmconfig);
      } else {
        publish("cmd_rightarmconfig",cmd_rightarmconfig);
      }
    }
  }
}



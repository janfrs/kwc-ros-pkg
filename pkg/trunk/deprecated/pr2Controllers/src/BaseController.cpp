#include <pr2Controllers/BaseController.h>
#include <pthread.h>
#include <urdf/URDF.h>

// roscpp
#include <ros/node.h>

#include <sys/time.h>

#include <mechanism_model/joint.h>

#include <math_utils/math_utils.h>

#define BASE_NUM_JOINTS 12

using namespace controller;
using namespace PR2;

static pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;

void BaseController::computePointVelocity(double vx, double vy, double vw, double x_offset, double y_offset, double &pvx, double &pvy)
{
  pvx = vx - y_offset*vw;
  pvy = vy + x_offset*vw;
};

BaseController::BaseController()
{
  this->robot = NULL;
  this->name = "baseController";
}

BaseController::BaseController(mechanism::Robot *robot, std::string name)
{
  this->robot = robot;
  this->name = name;
}

BaseController::BaseController(mechanism::Robot *robot)
{
  this->robot = robot;
  this->name = "baseController";
}


BaseController::BaseController(char *ns)
{
  this->robot = NULL;
  this->name  = "baseController";
}
  
BaseController::~BaseController( )
{
  delete (this->baseJointControllers);
}

controllerErrorCode BaseController::loadXML(std::string filename)
{
  robot_desc::URDF model;
  int exists = 0;

  if(!model.loadFile(filename.c_str()))
    return CONTROLLER_MODE_ERROR;

  const robot_desc::URDF::Map &data = model.getMap();

  std::vector<std::string> types;
  std::vector<std::string> names;
  std::vector<std::string>::iterator iter;

  data.getMapTagFlags(types);

  for(iter = types.begin(); iter != types.end(); iter++){
    if(*iter == "controller"){
      exists = 1;
      break;
    }
  }

  if(!exists)
    return CONTROLLER_MODE_ERROR;

  exists = 0;

  for(iter = names.begin(); iter != names.end(); iter++){
    if(*iter == this->name){
      exists = 1;
      break;
    }
  }
  paramMap = data.getMapTagValues("controller",this->name);   

  loadParam("pGain",pGain);

  printf("BC:: %f\n",pGain);
  loadParam("dGain",dGain);
  loadParam("iGain",iGain);

  loadParam("pGainPos",pGainPos);
  loadParam("dGainPos",dGainPos);
  loadParam("iGainPos",iGainPos);

  loadParam("casterMode",casterMode);
  loadParam("wheelMode",wheelMode);


  loadParam("maxXDot",maxXDot);
  loadParam("maxYDot",maxYDot);
  loadParam("maxYawDot",maxYawDot);

  return CONTROLLER_ALL_OK;
}

void BaseController::init()
{
  xDotCmd = 0;
  yDotCmd = 0;
  yawDotCmd = 0;

  xDotNew = 0;
  yDotNew = 0;
  yawDotNew = 0;

  initJointControllers();

  //Subscribe to joystick message
  (ros::g_node)->subscribe("base_command", baseCommandMessage, &controller::BaseController::receiveBaseCommandMessage, this);
}

void BaseController::receiveBaseCommandMessage(){
  maxXDot = maxYDot = maxYawDot = 1; //Until we start reading the xml file for parameters
  double vx = math_utils::clamp<double>(baseCommandMessage.axes[1], -maxXDot, maxXDot);
  double vy = math_utils::clamp<double>(baseCommandMessage.axes[0], -maxYDot, maxYDot);
  double vyaw = math_utils::clamp<double>(baseCommandMessage.axes[2], -maxYawDot, maxYawDot);
  setVelocity(vx, vy, vyaw);
}

void BaseController::initJointControllers() 
{
  this->baseJointControllers = new JointController[BASE_NUM_JOINTS];

  // these loops are not correct, the number is currently: caster drive drive caster drive drive ... NOT caster caster caster caster drvie drive ...
  // see pr2Core JOINT_ID for details
  for(int ii = 0; ii < NUM_CASTERS; ii++){
    baseJointControllers[ii].init(pGainPos, iGainPos, dGainPos, iMax, iMin, ETHERDRIVE_SPEED, getTime(), maxEffort, minEffort, &(robot->joint[ii]));
    baseJointControllers[ii].enableController();
  }
  for(int ii = NUM_CASTERS; ii < BASE_NUM_JOINTS; ii++){
    baseJointControllers[ii].init(pGain, iGain, dGain, iMax, iMin, ETHERDRIVE_SPEED, getTime(), maxEffort, minEffort, &(robot->joint[ii]));
    baseJointControllers[ii].enableController();
  }
}  

double BaseController::getTime()
{
  struct timeval t;
  gettimeofday( &t, 0);
  return (double) (t.tv_usec *1e-6 + t.tv_sec);
}

double ModNPiBy2(double angle)
{
  if (angle < -M_PI/2) 
    angle += M_PI;
  if(angle > M_PI/2)
    angle -= M_PI;
  return angle;
}

void BaseController::update( )
{
  point drivePointVelocityL, drivePointVelocityR;
  double wheelSpeed[NUM_WHEELS];
  point steerPointVelocity[NUM_CASTERS];
  double steerAngle[NUM_CASTERS];
  point newDriveCenterL, newDriveCenterR;
  double errorSteer[NUM_CASTERS];
  double kp_local = 10;
  double cmdVel[NUM_CASTERS];

  if (pthread_mutex_trylock(&dataMutex) == 0){
    xDotCmd = xDotNew;
    yDotCmd = yDotNew;
    yawDotCmd = yawDotNew;
    pthread_mutex_unlock(&dataMutex);
  }
 
  for(int ii=0; ii < NUM_CASTERS; ii++){
    computePointVelocity(xDotCmd,yDotCmd,yawDotCmd,BASE_CASTER_OFFSET[ii].x,BASE_CASTER_OFFSET[ii].y,steerPointVelocity[ii].x,steerPointVelocity[ii].y);
    steerAngle[ii] = atan2(steerPointVelocity[ii].y,steerPointVelocity[ii].x);

    steerAngle[ii] = ModNPiBy2(steerAngle[ii]);//Clean steer Angle
    errorSteer[ii] = robot->joint[3*ii].position - steerAngle[ii];
    cmdVel[ii] = kp_local * errorSteer[ii];
    baseJointControllers[3*ii].setVelCmd(cmdVel[ii]);     
  }

  for(int ii = 0; ii < NUM_CASTERS; ii++){
#ifdef DEBUG
    printf("offset %d: %f, %f, %f, %f\n",ii,CASTER_DRIVE_OFFSET[ii*2].x,CASTER_DRIVE_OFFSET[ii*2].y,CASTER_DRIVE_OFFSET[ii*2+1].x,CASTER_DRIVE_OFFSET[ii*2+1].y);
#endif
    newDriveCenterL = Rot2D(CASTER_DRIVE_OFFSET[ii*2].x,CASTER_DRIVE_OFFSET[ii*2].y,steerAngle[ii]);
    newDriveCenterR = Rot2D(CASTER_DRIVE_OFFSET[ii*2+1].x,CASTER_DRIVE_OFFSET[ii*2+1].y,steerAngle[ii]);

#ifdef DEBUG
    printf("offset:: %f, %f, %f, %f\n",newDriveCenterL.x,newDriveCenterL.y,newDriveCenterR.x,newDriveCenterR.y);
#endif
    newDriveCenterL.x += BASE_CASTER_OFFSET[ii].x;
    newDriveCenterL.y += BASE_CASTER_OFFSET[ii].y;
    newDriveCenterR.x += BASE_CASTER_OFFSET[ii].x;
    newDriveCenterR.y += BASE_CASTER_OFFSET[ii].y;

    computePointVelocity(xDotCmd,yDotCmd,yawDotCmd,newDriveCenterL.x,newDriveCenterL.y,drivePointVelocityL.x,drivePointVelocityL.y);
    computePointVelocity(xDotCmd,yDotCmd,yawDotCmd,newDriveCenterR.x,newDriveCenterR.y,drivePointVelocityR.x,drivePointVelocityR.y);

#ifdef DEBUG
    printf("CPV1:: %f, %f, %f, %f, %f, %f, %f\n",xDotCmd,yDotCmd,yawDotCmd,newDriveCenterL.x,newDriveCenterL.y,drivePointVelocityL.x,drivePointVelocityL.y);
    printf("CPV2:: %f, %f, %f, %f, %f, %f, %f\n",xDotCmd,yDotCmd,yawDotCmd,newDriveCenterR.x,newDriveCenterR.y,drivePointVelocityR.x,drivePointVelocityR.y);
#endif

    double steerXComponent = cos(robot->joint[ii*3].position);
    double steerYComponent = sin(robot->joint[ii*3].position);
    double dotProdL = steerXComponent*drivePointVelocityL.x + steerYComponent*drivePointVelocityL.y;
    double dotProdR = steerXComponent*drivePointVelocityR.x + steerYComponent*drivePointVelocityR.y;
#ifdef DEBUG
    printf("Actual CASTER:: %d, %f, %f, %f\n",ii,robot->joint[ii*3].position,drivePointVelocityL.x,drivePointVelocityR.x);
#endif
    wheelSpeed[ii*2  ] = (-dotProdL - cmdVel[ii] * CASTER_DRIVE_OFFSET[ii*2].y)/WHEEL_RADIUS;
    wheelSpeed[ii*2+1] = (dotProdR  +cmdVel[ii] * CASTER_DRIVE_OFFSET[ii*2+1].y)/WHEEL_RADIUS;

    baseJointControllers[ii*3+1].setVelCmd(wheelSpeed[ii*2]);
    baseJointControllers[ii*3+2].setVelCmd(wheelSpeed[ii*2+1]);

#ifdef DEBUG       // send command
    printf("DRIVE::L:: %d, cmdVel:: %f\n",ii,wheelSpeed[ii*2]);
    printf("DRIVE::R:: %d, cmdVel:: %f\n",ii,wheelSpeed[ii*2+1]);
#endif
  }
  for(int ii = 0; ii < BASE_NUM_JOINTS; ii++) 
    baseJointControllers[ii].update();
}

PR2::PR2_ERROR_CODE BaseController::setCourse(double v , double yaw)
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setVelocity(double xDotNew, double yDotNew, double yawDotNew)
{

  pthread_mutex_lock(&dataMutex);
  this->xDotNew = xDotNew;
  this->yDotNew = yDotNew;
  this->yawDotNew = yawDotNew;
  pthread_mutex_unlock(&dataMutex);
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setTarget(double x,double y, double yaw, double xDot, double yDot, double yawDot)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setTraj(int num_pts, double x[],double y[], double yaw[], double xDot[], double yDot[], double yawDot[])
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setHeading(double yaw)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setForce(double fx, double fy)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setWrench(double yaw)
{

  return PR2::PR2_ALL_OK;
}

void BaseController::loadParam(std::string label, double &param)
{
  if(paramMap.find(label) != paramMap.end()) // if parameter value has been initialized in the xml file, set internal parameter value
    param = atof(paramMap[label].c_str());
}

void BaseController::loadParam(std::string label, int &param)
{
  if(paramMap.find(label) != paramMap.end())
    param = atoi(paramMap[label].c_str());
}

PR2::PR2_ERROR_CODE BaseController::setParam(std::string label,double value)
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE BaseController::setParam(std::string label,std::string value)
{   
  return PR2::PR2_ALL_OK;
}

// const double BaseController::PGain = 0.1; /**< Proportional gain for speed control */
// const double BaseController::IGain = 0.1; /**< Integral gain for speed control */
// const double BaseController::DGain = 0; /**< Derivative gain for speed control */
// const double BaseController::IMax  = 10; /**< Max integral error term */
// const double BaseController::IMin  = -10; /**< Min integral error term */
// const double BaseController::maxPositiveTorque = 0.75; /**< (in Nm) max current = 0.75 A. Torque constant = 70.4 mNm/A.Max Torque = 70.4*0.75 = 52.8 mNm */
// const double BaseController::maxNegativeTorque = -0.75; /**< max negative torque */
// const double BaseController::maxEffort = 0.75; /**< maximum effort */
// const double BaseController::PGain_Pos = 0.1; /**< Proportional gain for position control */
// const double BaseController::IGain_Pos = 0.1; /**< Integral gain for position control */
// const double BaseController::DGain_Pos = 0; /**< Derivative gain for position control */

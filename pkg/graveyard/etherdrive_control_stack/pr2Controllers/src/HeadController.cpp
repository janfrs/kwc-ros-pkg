#include <pr2Controllers/HeadController.h>

using namespace controller;

HeadController::HeadController()
{
}
    
HeadController::~HeadController( )
{
}

void
HeadController::Update( )
{

}

PR2::PR2_ERROR_CODE
HeadController::setPosition(double yaw , double pitch, double yawDot, double pitchDot)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
HeadController::setPositionRate(double yawDot, double pitchDot)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
HeadController::setGazePoint(double x,double y, double z, double xDot, double yDot, double zDot)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
HeadController::setGazePoint(double x,double y, double z)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
HeadController::setSaccadeSpeed(double xDot, double yDot, double zDot)
{

  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
HeadController::setParam(std::string label,double value)
{

  return PR2::PR2_ALL_OK;
}




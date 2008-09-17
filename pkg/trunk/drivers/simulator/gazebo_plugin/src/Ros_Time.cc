/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: Actuator array controller for a Pr2 robot.
 * Author: Sachin Chitta and John Hsu
 * Date: 1 June 2008
 * SVN info: $Id$
 */


#include <algorithm>
#include <assert.h>

#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>
#include <gazebo_plugin/Ros_Time.hh>

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("ros_time", Ros_Time);

////////////////////////////////////////////////////////////////////////////////
// Constructor
Ros_Time::Ros_Time(Entity *parent)
    : Controller(parent)
{

    rosnode_ = ros::g_node; // comes from where?
    int argc = 0;
    char** argv = NULL;
    if (rosnode_ == NULL)
    {
      // this only works for a single camera.
      ros::init(argc,argv);
      rosnode_ = new ros::node("ros_gazebo",ros::node::DONT_HANDLE_SIGINT);
      printf("-------------------- starting node in Ros_Time \n");
    }

    // for rostime
    rosnode_->advertise<rostools::Time>("time");

}

////////////////////////////////////////////////////////////////////////////////
// Destructor
Ros_Time::~Ros_Time()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void Ros_Time::LoadChild(XMLConfigNode *node)
{

}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void Ros_Time::InitChild()
{
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void Ros_Time::UpdateChild()
{
    // pass time to robot
    double currentTime = Simulator::Instance()->GetSimTime();
    //std::cout << "sim time: " << currentTime << std::endl;

    /***************************************************************/
    /*                                                             */
    /*  publish time to ros                                        */
    /*                                                             */
    /***************************************************************/
    this->lock.lock();
    timeMsg.rostime.sec  = (unsigned long)floor(currentTime);
    timeMsg.rostime.nsec = (unsigned long)floor(  1e9 * (  currentTime - timeMsg.rostime.sec) );
    rosnode_->publish("time",timeMsg);
    this->lock.unlock();
}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void Ros_Time::FiniChild()
{
    // TODO: will be replaced by global ros node eventually
    if (rosnode_ != NULL)
    {
      std::cout << "shutdown rosnode in Ros_Time" << std::endl;
      rosnode_->shutdown();
    }
}




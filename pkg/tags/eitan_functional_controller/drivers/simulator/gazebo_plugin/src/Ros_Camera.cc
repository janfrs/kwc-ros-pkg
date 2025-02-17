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
 @mainpage
   Desc: Ros_Camera plugin for simulating cameras in Gazebo
   Author: John Hsu
   Date: 24 Sept 2008
   SVN info: $Id$
 @htmlinclude manifest.html
 @b Ros_Camera plugin broadcasts ROS Image messages
 */

#include <algorithm>
#include <assert.h>

#include <gazebo_plugin/Ros_Camera.hh>
#include <gazebo/Sensor.hh>
#include <gazebo/Model.hh>
#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>
#include "MonoCameraSensor.hh"

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("ros_camera", Ros_Camera);

////////////////////////////////////////////////////////////////////////////////
// Constructor
Ros_Camera::Ros_Camera(Entity *parent)
    : Controller(parent)
{
  this->myParent = dynamic_cast<MonoCameraSensor*>(this->parent);

  if (!this->myParent)
    gzthrow("Ros_Camera controller requires a Camera Sensor as its parent");


  // set parent sensor to active automatically
  this->myParent->SetActive(true);


  rosnode = ros::g_node; // comes from where?
  int argc = 0;
  char** argv = NULL;
  if (rosnode == NULL)
  {
    // this only works for a single camera.
    ros::init(argc,argv);
    rosnode = new ros::node("ros_gazebo",ros::node::DONT_HANDLE_SIGINT);
    printf("-------------------- starting node in camera \n");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
Ros_Camera::~Ros_Camera()
{


}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void Ros_Camera::LoadChild(XMLConfigNode *node)
{
  this->topicName = node->GetString("topicName","default_ros_camera",0); //read from xml file
  this->frameName = node->GetString("frameName","default_ros_camera",0); //read from xml file

  std::cout << "================= " << this->topicName << std::endl;
  rosnode->advertise<std_msgs::Image>(this->topicName,10);
}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void Ros_Camera::InitChild()
{
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void Ros_Camera::UpdateChild()
{

  // do this first so there's chance for sensor to run 1 frame after activate
  if (this->myParent->IsActive())
    this->PutCameraData();

}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void Ros_Camera::FiniChild()
{
  rosnode->unadvertise(this->topicName);
}

////////////////////////////////////////////////////////////////////////////////
// Put laser data to the interface
void Ros_Camera::PutCameraData()
{
  const unsigned char *src;

  // Get a pointer to image data
  src = this->myParent->GetImageData(0);

  //std::cout << " updating camera " << this->topicName << " " << data->image_size << std::endl;
  if (src)
  {
    this->lock.lock();
    // copy data into image
    this->imageMsg.header.frame_id = this->frameName;
    this->imageMsg.header.stamp.sec = (unsigned long)floor(Simulator::Instance()->GetSimTime());
    this->imageMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  Simulator::Instance()->GetSimTime() - this->imageMsg.header.stamp.sec) );

    int    width            = this->myParent->GetImageWidth();
    int    height           = this->myParent->GetImageHeight();
    int    depth            = 3;

    this->imageMsg.width       = width;
    this->imageMsg.height      = height;
    this->imageMsg.compression = "raw";
    this->imageMsg.colorspace  = "rgb24";

    // set buffer size
    uint32_t       buf_size = (width) * (height) * (depth);

    this->imageMsg.set_data_size(buf_size);
    memcpy(&(this->imageMsg.data[0]), src, buf_size);

    // publish to ros
    rosnode->publish(this->topicName,this->imageMsg);
    this->lock.unlock();
  }

}


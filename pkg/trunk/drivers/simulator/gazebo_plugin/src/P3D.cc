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
 * Desc: 3D position interface for ground truth.
 * Author: Sachin Chitta and John Hsu
 * Date: 1 June 2008
 * SVN info: $Id$
 */

#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/Model.hh>
#include <gazebo/HingeJoint.hh>
#include <gazebo/Body.hh>
#include <gazebo/SliderJoint.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>
#include <gazebo_plugin/P3D.hh>

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("P3D", P3D);


////////////////////////////////////////////////////////////////////////////////
// Constructor
P3D::P3D(Entity *parent )
   : Controller(parent)
{
   this->myParent = dynamic_cast<Model*>(this->parent);

   if (!this->myParent)
      gzthrow("P3D controller requires a Model as its parent");

  rosnode = ros::g_node; // comes from where?
  int argc = 0;
  char** argv = NULL;
  if (rosnode == NULL)
  {
    // this only works for a single camera.
    ros::init(argc,argv);
    rosnode = new ros::node("ros_gazebo",ros::node::DONT_HANDLE_SIGINT);
    printf("-------------------- starting node in P3D \n");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
P3D::~P3D()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void P3D::LoadChild(XMLConfigNode *node)
{
  std::string bodyName = node->GetString("bodyName", "", 1);
  this->myBody = dynamic_cast<Body*>(this->myParent->GetBody(bodyName));
//  this->myBody = dynamic_cast<Body*>(this->myParent->GetBody(bodyName));

  this->topicName     = node->GetString("topicName", "ground_truth", 1);
  this->frameName     = node->GetString("frameName", "", 1);
  this->xyzOffsets    = node->GetVector3("xyzOffsets", Vector3(0,0,0));
  this->rpyOffsets    = node->GetVector3("rpyOffsets", Vector3(0,0,0));
  this->gaussianNoise = node->GetDouble("gaussianNoise",0.0,0); //read from xml file

  std::cout << "==== topic name for P3D ======== " << this->topicName << std::endl;
  if (this->topicName != "")
    rosnode->advertise<std_msgs::PoseWithRatesStamped>(this->topicName,10);
}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void P3D::InitChild()
{
  this->last_time = Simulator::Instance()->GetSimTime();
  this->last_vpos = this->myBody->GetPositionRate(); // get velocity in gazebo frame
  this->last_veul = this->myBody->GetEulerRate(); // get velocity in gazebo frame
  this->apos = 0;
  this->aeul = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void P3D::UpdateChild()
{
  Pose3d pose;
  Quatern rot;
  Vector3 pos;

  // Get Pose/Orientation ///@todo: verify correctness
  pose = this->myBody->GetPose(); // - this->myBody->GetCoMPose();

  // apply xyz offsets and get position and rotation components
  pos = pose.pos + this->xyzOffsets;
  rot = pose.rot;
  // std::cout << " --------- P3D rot " << rot.x << ", " << rot.y << ", " << rot.z << ", " << rot.u << std::endl;

  // apply rpy offsets
  Quatern qOffsets;
  qOffsets.SetFromEuler(this->rpyOffsets);
  rot = qOffsets*rot;
  rot.Normalize();

  double cur_time = Simulator::Instance()->GetSimTime();
  
  // get Rates
  Vector3 vpos = this->myBody->GetPositionRate(); // get velocity in gazebo frame
  Quatern vrot = this->myBody->GetRotationRate(); // get velocity in gazebo frame
  Vector3 veul = this->myBody->GetEulerRate(); // get velocity in gazebo frame

  // differentiate to get accelerations
  double tmp_dt = this->last_time - cur_time;
  if (tmp_dt != 0)
  {
    this->apos = (this->last_vpos - vpos) / tmp_dt;
    this->aeul = (this->last_veul - veul) / tmp_dt;
    this->last_vpos = vpos;
    this->last_veul = veul;
  }

  this->lock.lock();


  if (this->topicName != "")
  {
    // copy data into pose message
    this->poseMsg.header.frame_id = "map";  // @todo: should this be changeable?
    this->poseMsg.header.stamp.sec = (unsigned long)floor(cur_time);
    this->poseMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  cur_time - this->poseMsg.header.stamp.sec) );

    // pose is given in inertial frame for Gazebo, transform to the designated frame name

    this->poseMsg.pos.position.x    = pos.x;
    this->poseMsg.pos.position.y    = pos.y;
    this->poseMsg.pos.position.z    = pos.z;

    this->poseMsg.pos.orientation.x = rot.x;
    this->poseMsg.pos.orientation.y = rot.y;
    this->poseMsg.pos.orientation.z = rot.z;
    this->poseMsg.pos.orientation.w = rot.u;

    this->poseMsg.vel.vel.vx        = vpos.x + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.vel.vel.vy        = vpos.y + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.vel.vel.vz        = vpos.z + this->GaussianKernel(0,this->gaussianNoise) ;
    // pass euler anglular rates
    this->poseMsg.vel.ang_vel.vx    = veul.x + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.vel.ang_vel.vy    = veul.y + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.vel.ang_vel.vz    = veul.z + this->GaussianKernel(0,this->gaussianNoise) ;

    this->poseMsg.acc.acc.ax        = this->apos.x + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.acc.acc.ay        = this->apos.y + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.acc.acc.az        = this->apos.z + this->GaussianKernel(0,this->gaussianNoise) ;
    // pass euler anglular rates
    this->poseMsg.acc.ang_acc.ax    = this->aeul.x + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.acc.ang_acc.ay    = this->aeul.y + this->GaussianKernel(0,this->gaussianNoise) ;
    this->poseMsg.acc.ang_acc.az    = this->aeul.z + this->GaussianKernel(0,this->gaussianNoise) ;

    // publish to ros
    rosnode->publish(this->topicName,this->poseMsg);
  }

  this->lock.unlock();

  // save last time stamp
  this->last_time = cur_time;
}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void P3D::FiniChild()
{
  if (this->topicName != "")
    rosnode->unadvertise(this->topicName);
}

//////////////////////////////////////////////////////////////////////////////
// Utility for adding noise
double P3D::GaussianKernel(double mu,double sigma)
{
  // using Box-Muller transform to generate two independent standard normally disbributed normal variables
  // see wikipedia
  double U = (double)rand()/(double)RAND_MAX; // normalized uniform random variable
  double V = (double)rand()/(double)RAND_MAX; // normalized uniform random variable
  double X = sqrt(-2.0 * ::log(U)) * cos( 2.0*M_PI * V);
  //double Y = sqrt(-2.0 * ::log(U)) * sin( 2.0*M_PI * V); // the other indep. normal variable
  // we'll just use X
  // scale to our mu and sigma
  X = sigma * X + mu;
  return X;
}



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
 * Desc: Ros Block Laser controller.
 * Author: Nathan Koenig
 * Date: 01 Feb 2007
 * SVN info: $Id: Ros_Block_Laser.cc 6683 2008-06-25 19:12:30Z natepak $
 */

#include <algorithm>
#include <assert.h>

#include <gazebo/Sensor.hh>
#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/HingeJoint.hh>
#include <gazebo/World.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>
#include <RaySensor.hh>
#include <gazebo_plugin/Ros_Block_Laser.hh>

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("ros_block_laser", Ros_Block_Laser);

////////////////////////////////////////////////////////////////////////////////
// Constructor
Ros_Block_Laser::Ros_Block_Laser(Entity *parent)
    : Controller(parent)
{
  this->myParent = dynamic_cast<RaySensor*>(this->parent);

  if (!this->myParent)
    gzthrow("Ros_Block_Laser controller requires a Ray Sensor as its parent");

  // set parent sensor to active automatically
  this->myParent->SetActive(true);

  rosnode = ros::g_node; // comes from where?  common.h exports as global variable
  int argc = 0;
  char** argv = NULL;
  if (rosnode == NULL)
  {
    // start a ros node if none exist
    ros::init(argc,argv);
    rosnode = new ros::node("ros_gazebo",ros::node::DONT_HANDLE_SIGINT);
    printf("-------------------- starting node in laser \n");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
Ros_Block_Laser::~Ros_Block_Laser()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void Ros_Block_Laser::LoadChild(XMLConfigNode *node)
{
  this->topicName = node->GetString("topicName","default_ros_laser",0); //read from xml file
  std::cout << "================= " << this->topicName <<  std::endl;
  rosnode->advertise<std_msgs::PointCloudFloat32>(this->topicName,10);
  this->frameName = node->GetString("frameName","default_ros_laser",0); //read from xml file
  this->gaussianNoise = node->GetDouble("gaussianNoise",0.0,0); //read from xml file


  // set size of cloud message, starts at 0!! FIXME: not necessary
  Angle maxAngle = this->myParent->GetMaxAngle();
  Angle minAngle = this->myParent->GetMinAngle();
  int rangeCount = this->myParent->GetRangeCount();
  int verticalRangeCount = this->myParent->GetVerticalRangeCount();
  Angle verticalMaxAngle = this->myParent->GetVerticalMaxAngle();
  Angle verticalMinAngle = this->myParent->GetVerticalMinAngle();

  int r_size = rangeCount * verticalRangeCount;
  this->cloudMsg.set_pts_size(r_size);
  this->cloudMsg.set_chan_size(r_size);
  for (int i=0; i< r_size; i++)
    this->cloudMsg.chan[i].set_vals_size(1);

}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void Ros_Block_Laser::InitChild()
{

}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void Ros_Block_Laser::UpdateChild()
{
    this->PutLaserData();
}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void Ros_Block_Laser::FiniChild()
{
  rosnode->unadvertise(this->topicName);
}

////////////////////////////////////////////////////////////////////////////////
// Put laser data to the interface
void Ros_Block_Laser::PutLaserData()
{
  int i, hja, hjb;
  int j, vja, vjb;
  double vb, hb;
  int    j1, j2, j3, j4; // four corners indices
  double r1, r2, r3, r4, r; // four corner values + interpolated range
  int v;

  Angle maxAngle = this->myParent->GetMaxAngle();
  Angle minAngle = this->myParent->GetMinAngle();

  double maxRange = this->myParent->GetMaxRange();
  double minRange = this->myParent->GetMinRange();
  int rayCount = this->myParent->GetRayCount();
  int rangeCount = this->myParent->GetRangeCount();

  int verticalRayCount = this->myParent->GetVerticalRayCount();
  int verticalRangeCount = this->myParent->GetVerticalRangeCount();
  Angle verticalMaxAngle = this->myParent->GetVerticalMaxAngle();
  Angle verticalMinAngle = this->myParent->GetVerticalMinAngle();

  double yDiff = maxAngle.GetAsRadian() - minAngle.GetAsRadian();
  double pDiff = verticalMaxAngle.GetAsRadian() - verticalMinAngle.GetAsRadian();


  // set size of cloud message everytime!
  int r_size = rangeCount * verticalRangeCount;
  this->cloudMsg.set_pts_size(r_size);
  this->cloudMsg.set_chan_size(r_size);
  for (int i=0; i< r_size; i++)
    this->cloudMsg.chan[i].set_vals_size(1);

  /***************************************************************/
  /*                                                             */
  /*  point scan from laser                                      */
  /*                                                             */
  /***************************************************************/
  this->lock.lock();
  // Add Frame Name
  this->cloudMsg.header.frame_id = this->frameName;
  this->cloudMsg.header.stamp.sec = (unsigned long)floor(Simulator::Instance()->GetSimTime());
  this->cloudMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  Simulator::Instance()->GetSimTime() - this->cloudMsg.header.stamp.sec) );

  for (j = 0; j<verticalRangeCount; j++)
  {
    // interpolating in vertical direction
    vb = (verticalRangeCount == 1) ? 0 : (double) j * (verticalRayCount - 1) / (verticalRangeCount - 1);
    vja = (int) floor(vb);
    vjb = std::min(vja + 1, verticalRayCount - 1);
    vb = vb - floor(vb); // fraction from min

    assert(vja >= 0 && vja < verticalRayCount);
    assert(vjb >= 0 && vjb < verticalRayCount);

    for (i = 0; i<rangeCount; i++)
    {
      // Interpolate the range readings from the rays in horizontal direction
      hb = (rangeCount == 1)? 0 : (double) i * (rayCount - 1) / (rangeCount - 1);
      hja = (int) floor(hb);
      hjb = std::min(hja + 1, rayCount - 1);
      hb = hb - floor(hb); // fraction from min

      assert(hja >= 0 && hja < rayCount);
      assert(hjb >= 0 && hjb < rayCount);

      // indices of 4 corners
      j1 = hja + vja * rayCount;
      j2 = hjb + vja * rayCount;
      j3 = hja + vjb * rayCount;
      j4 = hjb + vjb * rayCount;
      // range readings of 4 corners
      r1 = std::min(this->myParent->GetRange(j1) , maxRange);
      r2 = std::min(this->myParent->GetRange(j2) , maxRange);
      r3 = std::min(this->myParent->GetRange(j3) , maxRange);
      r4 = std::min(this->myParent->GetRange(j4) , maxRange);

      // Range is linear interpolation if values are close,
      // and min if they are very different
      r = (1-vb)*((1 - hb) * r1 + hb * r2)
         +   vb *((1 - hb) * r3 + hb * r4);

      // Intensity is either-or
      v = (int) this->myParent->GetRetro(j1) || (int) this->myParent->GetRetro(j2) ||
          (int) this->myParent->GetRetro(j3) || (int) this->myParent->GetRetro(j4);

      // std::cout << " block debug "
      //           << "  ij("<<i<<","<<j<<")"
      //           << "  j1234("<<j1<<","<<j2<<","<<j3<<","<<j4<<")"
      //           << "  r1234("<<r1<<","<<r2<<","<<r3<<","<<r4<<")"
      //           << std::endl;

      // get angles of ray to get xyz for point
      double yAngle = 0.5*(hja+hjb) * yDiff / (rayCount -1) + minAngle.GetAsRadian();
      double pAngle = 0.5*(vja+vjb) * pDiff / (verticalRayCount -1) + verticalMinAngle.GetAsRadian();

      int n = i + j*rayCount;
      /***************************************************************/
      /*                                                             */
      /*  point scan from laser                                      */
      /*                                                             */
      /***************************************************************/
      if (r == maxRange)
      {
        // no noise if at max range
        this->cloudMsg.pts[n].x      = (r+minRange) * cos(pAngle)*cos(yAngle);
        this->cloudMsg.pts[n].y      = (r+minRange) *             sin(yAngle);
        this->cloudMsg.pts[n].z      = (r+minRange) * sin(pAngle)*cos(yAngle);
      }
      else
      {
        this->cloudMsg.pts[n].x      = (r+minRange) * cos(pAngle)*cos(yAngle) + this->GaussianKernel(0,this->gaussianNoise) ;
        this->cloudMsg.pts[n].y      = (r+minRange) *             sin(yAngle) + this->GaussianKernel(0,this->gaussianNoise) ;
        this->cloudMsg.pts[n].z      = (r+minRange) * sin(pAngle)*cos(yAngle) + this->GaussianKernel(0,this->gaussianNoise) ;
      }
      this->cloudMsg.chan[n].vals[0] = v + this->GaussianKernel(0,this->gaussianNoise) ;
      this->cloudMsg.chan[n].vals[0] = 4000; //v + this->GaussianKernel(0,this->gaussianNoise) ;
      this->cloudMsg.chan[0].vals[0] = 0; //v + this->GaussianKernel(0,this->gaussianNoise) ;
    }
  }

  // send data out via ros message
  rosnode->publish(this->topicName,this->cloudMsg);
  this->lock.unlock();



}


//////////////////////////////////////////////////////////////////////////////
// Utility for adding noise
double Ros_Block_Laser::GaussianKernel(double mu,double sigma)
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




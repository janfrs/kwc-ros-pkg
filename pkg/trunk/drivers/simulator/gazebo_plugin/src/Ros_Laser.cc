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
 * Desc: Ros Laser controller.
 * Author: Nathan Koenig
 * Date: 01 Feb 2007
 * SVN info: $Id: Ros_Laser.cc 6683 2008-06-25 19:12:30Z natepak $
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
#include "RaySensor.hh"
#include <gazebo_plugin/Ros_Laser.hh>

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("ros_laser", Ros_Laser);

////////////////////////////////////////////////////////////////////////////////
// Constructor
Ros_Laser::Ros_Laser(Entity *parent)
    : Controller(parent)
{
  this->myParent = dynamic_cast<RaySensor*>(this->parent);

  if (!this->myParent)
    gzthrow("Ros_Laser controller requires a Ray Sensor as its parent");

  // set parent sensor to active automatically
  this->myParent->SetActive(true);

  this->laserIface = NULL;
  this->fiducialIface = NULL;

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
  tfc = new rosTFClient(*rosnode); //, true, 1 * 1000000000ULL, 0ULL);
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
Ros_Laser::~Ros_Laser()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void Ros_Laser::LoadChild(XMLConfigNode *node)
{
  std::vector<Iface*>::iterator iter;

  for (iter = this->ifaces.begin(); iter != this->ifaces.end(); iter++)
  {
    if ((*iter)->GetType() == "laser")
      this->laserIface = dynamic_cast<LaserIface*>(*iter);
    else if ((*iter)->GetType() == "fiducial")
      this->fiducialIface = dynamic_cast<FiducialIface*>(*iter);
  }

  if (!this->laserIface) gzthrow("Ros_Laser controller requires a LaserIface");

  this->topicName = node->GetString("topicName","default_ros_laser",0); //read from xml file
  std::cout << "================= " << this->topicName <<  std::endl;
  rosnode->advertise<std_msgs::LaserScan>(this->topicName);
  this->frameName = node->GetString("frameName","default_ros_laser",0); //read from xml file
}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void Ros_Laser::InitChild()
{
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void Ros_Laser::UpdateChild()
{

  if (this->laserIface)
    this->PutLaserData();

  if (this->fiducialIface)
    this->PutFiducialData();

}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void Ros_Laser::FiniChild()
{
  // TODO: will be replaced by global ros node eventually
  if (rosnode != NULL)
  {
    std::cout << "shutdown rosnode in Ros_Laser" << std::endl;
    //ros::fini();
    rosnode->shutdown();
    //delete rosnode;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Put laser data to the interface
void Ros_Laser::PutLaserData()
{
  int i, ja, jb;
  double ra, rb, r, b;
  int v;

  double maxAngle = this->myParent->GetMaxAngle();
  double minAngle = this->myParent->GetMinAngle();

  double maxRange = this->myParent->GetMaxRange();
  double minRange = this->myParent->GetMinRange();
  int rayCount = this->myParent->GetRayCount();
  int rangeCount = this->myParent->GetRangeCount();

  if (this->laserIface->Lock(1))
  {
    // Add Frame Name


    // Data timestamp
    this->laserIface->data->head.time = Simulator::Instance()->GetSimTime();

    // Read out the laser range data
    this->laserIface->data->min_angle = minAngle;
    this->laserIface->data->max_angle = maxAngle;
    this->laserIface->data->res_angle = (maxAngle - minAngle) / (rangeCount - 1);
    this->laserIface->data->res_range = 0.1;
    this->laserIface->data->max_range = maxRange;
    this->laserIface->data->range_count = rangeCount;

    assert(this->laserIface->data->range_count < GZ_LASER_MAX_RANGES );

    /***************************************************************/
    /*                                                             */
    /*  point scan from laser                                      */
    /*                                                             */
    /***************************************************************/
    this->lock.lock();
    this->laserMsg.header.frame_id = this->frameName;
    this->laserMsg.header.stamp.sec = (unsigned long)floor(this->laserIface->data->head.time);
    this->laserMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  this->laserIface->data->head.time - this->laserMsg.header.stamp.sec) );


    double tmp_res_angle = (maxAngle - minAngle)/((double)(rangeCount -1)); // for computing yaw
    int    num_channels = 1;
    this->laserMsg.angle_min = minAngle;
    this->laserMsg.angle_max = maxAngle;
    this->laserMsg.angle_increment = tmp_res_angle;
    this->laserMsg.time_increment  = 0; // instantaneous simulator scan
    this->laserMsg.scan_time       = 0; // FIXME: what's this?
    this->laserMsg.range_min = minRange;
    this->laserMsg.range_max = maxRange;
    this->laserMsg.set_ranges_size(rangeCount);
    this->laserMsg.set_intensities_size(rangeCount);

    // Interpolate the range readings from the rays
    for (i = 0; i<rangeCount; i++)
    {
      b = (double) i * (rayCount - 1) / (rangeCount - 1);
      ja = (int) floor(b);
      jb = std::min(ja + 1, rayCount - 1);
      b = b - floor(b);

      assert(ja >= 0 && ja < rayCount);
      assert(jb >= 0 && jb < rayCount);

      ra = std::min(this->myParent->GetRange(ja) , maxRange);
      rb = std::min(this->myParent->GetRange(jb) , maxRange);

      // Range is linear interpolation if values are close,
      // and min if they are very different
      if (fabs(ra - rb) < 0.10)
        r = (1 - b) * ra + b * rb;
      else r = std::min(ra, rb);

      // Intensity is either-or
      v = (int) this->myParent->GetRetro(ja) || (int) this->myParent->GetRetro(jb);

      this->laserIface->data->ranges[i] =  r + minRange;
      this->laserIface->data->intensity[i] = v;

      /***************************************************************/
      /*                                                             */
      /*  point scan from laser                                      */
      /*                                                             */
      /***************************************************************/
      double sigma = 0.002;  // 2 milimeter noise
      if (r == maxRange)
        this->laserMsg.ranges[i]        = r; // no noise if at max range
      else
        this->laserMsg.ranges[i]        = r + this->GaussianKernel(0,sigma) ;
      this->laserMsg.intensities[i]   = v + this->GaussianKernel(0,sigma) ;
    }

    // iface writing can be skipped if iface is not used.
    // send data out via ros message
    rosnode->publish(this->topicName,this->laserMsg);
    this->lock.unlock();

    this->laserIface->Unlock();

    // New data is available
    this->laserIface->Post();
  }


}

//////////////////////////////////////////////////////////////////////////////
// Update the data in the interface
void Ros_Laser::PutFiducialData()
{
  int i, j, count;
  FiducialFid *fid;
  double r, b;
  double ax, ay, bx, by, cx, cy;

  double maxAngle = this->myParent->GetMaxAngle();
  double minAngle = this->myParent->GetMinAngle();

  double maxRange = this->myParent->GetMaxRange();
  double minRange = this->myParent->GetMinRange();
  int rayCount = this->myParent->GetRayCount();
  int rangeCount = this->myParent->GetRangeCount();

  if (this->fiducialIface->Lock(1))
  {
    // Data timestamp
    this->fiducialIface->data->head.time = Simulator::Instance()->GetSimTime();
    this->fiducialIface->data->count = 0;

    // TODO: clean this up
    count = 0;
    for (i = 0; i < rayCount; i++)
    {
      if (this->myParent->GetFiducial(i) < 0)
        continue;

      // Find the end of the fiducial
      for (j = i + 1; j < rayCount; j++)
      {
        if (this->myParent->GetFiducial(j) != this->myParent->GetFiducial(i))
          break;
      }
      j--;

      // Need at least three points to get orientation
      if (j - i + 1 >= 3)
      {
        r = minRange + this->myParent->GetRange(i);
        b = minAngle + i * ((maxAngle-minAngle) / (rayCount - 1));
        ax = r * cos(b);
        ay = r * sin(b);

        r = minRange + this->myParent->GetRange(j);
        b = minAngle + j * ((maxAngle-minAngle) / (rayCount - 1));
        bx = r * cos(b);
        by = r * sin(b);

        cx = (ax + bx) / 2;
        cy = (ay + by) / 2;

        assert(count < GZ_FIDUCIAL_MAX_FIDS);
        fid = this->fiducialIface->data->fids + count++;

        fid->id = this->myParent->GetFiducial(j);
        fid->pose.pos.x = cx;
        fid->pose.pos.y = cy;
        fid->pose.yaw = atan2(by - ay, bx - ax) + M_PI / 2;
      }

      // Fewer points get no orientation
      else
      {
        r = minRange + this->myParent->GetRange(i);
        b = minAngle + i * ((maxAngle-minAngle) / (rayCount - 1));
        ax = r * cos(b);
        ay = r * sin(b);

        r = minRange + this->myParent->GetRange(j);
        b = minAngle + j * ((maxAngle-minAngle) / (rayCount - 1));
        bx = r * cos(b);
        by = r * sin(b);

        cx = (ax + bx) / 2;
        cy = (ay + by) / 2;

        assert(count < GZ_FIDUCIAL_MAX_FIDS);
        fid = this->fiducialIface->data->fids + count++;

        fid->id = this->myParent->GetFiducial(j);
        fid->pose.pos.x = cx;
        fid->pose.pos.y = cy;
        fid->pose.yaw = atan2(cy, cx) + M_PI;
      }

      /*printf("fiducial %d i[%d] j[%d] %.2f %.2f %.2f\n",
        fid->id, i,j,fid->pos[0], fid->pos[1], fid->rot[2]);
        */
      i = j;
    }

    this->fiducialIface->data->count = count;

    this->fiducialIface->Unlock();
    this->fiducialIface->Post();
  }
}

//////////////////////////////////////////////////////////////////////////////
// Utility for adding noise
double Ros_Laser::GaussianKernel(double mu,double sigma)
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




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

#include "ros/node.h"



// Services
#include "point_cloud_assembler/BuildCloud.h"

// Messages
#include "std_msgs/PointCloudFloat32.h"


using namespace std_msgs ;

/***
 * This a simple test app that requests a point cloud from the point_cloud_assembler, and then publishes the resulting data
 */

namespace point_cloud_assembler
{

class GrabCloudData : public ros::node
{
  
public:

  GrabCloudData() : ros::node("grab_cloud_data")
  {
    advertise<PointCloudFloat32> ("full_cloud", 1) ;
  }
  
  ~GrabCloudData()
  {
    unadvertise("full_cloud") ;
  }
  
  bool spin()
  {
    ros::Duration period(4,0) ;         // Repeat Every 4 seconds
        
    ros::Time next_time = ros::Time::now() ;

    while ( ok() )
    {
      usleep(100) ;
      if (ros::Time::now() >= next_time)
      {
        next_time.from_double(next_time.to_double() + period.to_double()) ;
        
        BuildCloud::request req ;
        BuildCloud::response resp ;
        
        req.begin.from_double(next_time.to_double() - period.to_double() ) ;
        req.end   = next_time ;
        req.target_frame_id = "base" ;
        
        printf("Making Service Call...\n") ;
        ros::service::call("build_cloud", req, resp) ;
        printf("Done with service call\n") ;
        
        publish("full_cloud", resp.cloud) ;
        printf("Published Cloud size=%u\n", resp.cloud.get_pts_size()) ;
      }
    }
    return true ;
  }
} ;

}

using namespace point_cloud_assembler ;

int main(int argc, char **argv)
{
  ros::init(argc, argv);
  GrabCloudData grabber ;
  grabber.spin();
  ros::fini();
  return 0;
}

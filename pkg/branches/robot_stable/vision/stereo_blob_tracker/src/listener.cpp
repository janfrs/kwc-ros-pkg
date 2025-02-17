///////////////////////////////////////////////////////////////////////////////
// The roscpp_demo package has a few demos of the roscpp c++ client library 
//
// Copyright (C) 2008, Morgan Quigley
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//   
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////

#include "ros/node.h"
#include "std_msgs/String.h"
#include "std_msgs/PointStamped.h"
#include "stereo_blob_tracker/Rect2DStamped.h"
using namespace stereo_blob_tracker;

class Listener : public ros::node
{
public:
  std_msgs::PointStamped msg;
  Rect2DStamped sbox_msg;
  Rect2DStamped tbox_msg;
  Listener() : ros::node("listener")
  { 
    subscribe("points",       msg,      &Listener::point_cb,        1000); 
    subscribe("selectionbox", sbox_msg, &Listener::selectionbox_cb, 1000);
    subscribe("trackedbox",   tbox_msg, &Listener::trackedbox_cb,   1000);
  }
  void point_cb()
  {
    printf("I heard: [%f, %f, %f]\n", msg.point.x, msg.point.y, msg.point.z);
  }
  void selectionbox_cb() {
    printf("I heard selection box [%f, %f, %f, %f]\n", 
	   sbox_msg.rect.x, sbox_msg.rect.y,
	   sbox_msg.rect.w, sbox_msg.rect.h);
  }
  void trackedbox_cb() {
    printf("I heard tracked box [%f, %f, %f, %f]\n", 
	   tbox_msg.rect.x, tbox_msg.rect.y,
	   tbox_msg.rect.w, tbox_msg.rect.h);
  }
};

int main(int argc, char **argv)
{
  ros::init(argc, argv);
  Listener l;
  l.spin();
  ros::fini();
  return 0;
}

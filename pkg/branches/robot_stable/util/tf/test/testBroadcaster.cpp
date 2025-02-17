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

#include "tf/transform_broadcaster.h"

class testBroadcaster : public ros::node
{
public:
  //constructor
  testBroadcaster() : ros::node("broadcaster"),broadcaster(*this),count(2){};
  //Clean up ros connections
  ~testBroadcaster() { }

  //A pointer to the rosTFServer class
  tf::TransformBroadcaster broadcaster;


  // A function to call to send data periodically
  void test () {
    NEWMAT::Matrix mat(4,4);
    mat << 1 << 0 << 0 << 1
        << 0 << 1 << 0 << 2
        << 0 << 0 << 1 << 3
        << 0 << 0 << 0 << 1;

    broadcaster.sendTransform(btTransform(btQuaternion(0,0,0), btVector3(1,2,3)), ros::Time().fromSec(1), "frame1", "frame2");
    /*    pTFServer->sendEuler("count","count++",1,1,1,1,1,1,ros::Time(100000,100000));
    pTFServer->sendInverseEuler("count","count++",1,1,1,1,1,1,ros::Time(100000,100000));
    pTFServer->sendDH("count","count++",1,1,1,1,ros::Time(100000,100000));
    pTFServer->sendQuaternion("count","count++",1,1,1,1,1,1,1,ros::Time(100000,100000));
    pTFServer->sendMatrix("count","count++",mat, ros::Time::now());
*/
    if (count > 9000)
      count = 0;
    std::cerr<<count<<std::endl;
  };

private:
  int count;

};

int main(int argc, char ** argv)
{
  //Initialize ROS
  ros::init(argc, argv);

  //Construct/initialize the server
  testBroadcaster myTestBroadcaster;

  while(myTestBroadcaster.ok())
  {
      //Send some data
      myTestBroadcaster.test();
      usleep(1000);
  }
  ros::fini();

  return 0;
};


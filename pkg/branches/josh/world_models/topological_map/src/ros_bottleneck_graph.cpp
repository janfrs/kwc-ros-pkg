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


#include <iostream>
#include <ros/node.h>
#include <std_srvs/StaticMap.h>
#include "topological_map/bottleneck_graph.h"

using std::cout;
using std::endl;


class BottleneckGraphRos: public ros::node
{
public:
  BottleneckGraphRos(int size, int skip);
};

BottleneckGraphRos::BottleneckGraphRos(int size, int skip) : ros::node("bottleneckgraph_ros")
{

  std_srvs::StaticMap::request req;
  std_srvs::StaticMap::response resp;
  cout << "Requesting map... " << endl;
  while (!ros::service::call("static_map", req, resp))
  {
    sleep(2);
    cout << "Request failed: trying again..." << endl;
    usleep(1000000);
  }
  sleep(2);
  cout << "Received a " << resp.map.width << " by " << resp.map.height << " map at "
       << resp.map.resolution << " m/pix " << endl;
  int sx = resp.map.width;
  int sy = resp.map.height;
  
  topological_map::GridArray grid(boost::extents[sy][sx]);
  int i = 0;
  for (int r=0; r<sy; r++) {
    for (int c=0; c<sx; c++) {
      int val = resp.map.data[i++];
      grid[r][c] = (val == 100);
      if ((val != 0) && (val != 100) && (val != 255)) {
        cout << "Treating val " << val << " as occupied" << endl;
      }
    }
  }
  
  
  topological_map::BottleneckGraph g = topological_map::makeBottleneckGraph (grid, size, skip);
  topological_map::printBottlenecks (g, grid);
}  


  

int main(int argc, char** argv)
{
  ros::init(argc, argv);
  BottleneckGraphRos node (atoi(argv[1]), atoi(argv[2]));
  node.shutdown();
}




  
  
  
  
  

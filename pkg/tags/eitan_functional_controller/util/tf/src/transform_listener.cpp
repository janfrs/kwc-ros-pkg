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

/** \author Tully Foote */

#include "tf/transform_listener.h"

using namespace tf;

void TransformListener::transformPointCloud(const std::string & target_frame, const std_msgs::PointCloud & cloudIn, std_msgs::PointCloud & cloudOut)
{
  TransformLists t_list = lookupLists(lookupFrameNumber( target_frame), cloudIn.header.stamp.to_ull(), lookupFrameNumber( cloudIn.header.frame_id), cloudIn.header.stamp.to_ull(), 0);
  
  Transform bttransform = computeTransformFromList(t_list);
  
  NEWMAT::Matrix transform = transformAsMatrix(bttransform);

  unsigned int length = cloudIn.get_pts_size();

  NEWMAT::Matrix matIn(4, length);
  
  double * matrixPtr = matIn.Store();
  
  for (unsigned int i = 0; i < length ; i++) 
    { 
      matrixPtr[i] = cloudIn.pts[i].x;
      matrixPtr[length +i] = cloudIn.pts[i].y;
      matrixPtr[2 * length + i] = cloudIn.pts[i].z;
      matrixPtr[3 * length + i] = 1;
    };
  
  NEWMAT::Matrix matOut = transform * matIn;
  
  // Copy relevant data from cloudIn, if needed
  if (&cloudIn != &cloudOut)
  {
      cloudOut.header = cloudIn.header;
      cloudOut.set_pts_size(length);  
      cloudOut.set_chan_size(cloudIn.get_chan_size());
      for (unsigned int i = 0 ; i < cloudIn.get_chan_size() ; ++i)
	  cloudOut.chan[i] = cloudIn.chan[i];
  }
  
  matrixPtr = matOut.Store();
  
  //Override the positions
  cloudOut.header.frame_id = target_frame;
  for (unsigned int i = 0; i < length ; i++) 
    { 
      cloudOut.pts[i].x = matrixPtr[i];
      cloudOut.pts[i].y = matrixPtr[1*length + i];
      cloudOut.pts[i].z = matrixPtr[2*length + i];
    };
}

void TransformListener::subscription_callback()
{
  try 
  {
    Transform temp;
    TransformMsgToTF(msg_in_.transform, temp);
    setTransform(Stamped<Transform>(temp, msg_in_.header.stamp.to_ull(), msg_in_.header.frame_id), msg_in_.parent);
  }
  catch (TransformException& ex)
  {
    ///\todo Use error reporting
    std::string temp = ex.what();
    printf("Failure to set recieved transform %s to %s with error: %s\n", msg_in_.header.frame_id.c_str(), msg_in_.parent.c_str(), temp.c_str());
  }



};

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

/*
 * testVisOdom.cpp
 *
 *  Created on: Dec 2, 2008
 *      Author: jdchen
 */

#include "CvTest3DPoseEstimate.h"

int main(int argc, char **argv){
  CvTest3DPoseEstimate test3DPoseEstimate;

  test3DPoseEstimate.input_data_path_ = string("Data/");
  test3DPoseEstimate.output_data_path_ = string("Output/");

  if (argc >= 2) {
    char *option = argv[1];
    if (strcasecmp(option, "cartesian")==0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::Cartesian;
    } else if (strcasecmp(option, "disparity")==0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::Disparity;
    } else if (strcasecmp(option, "cartanddisp") == 0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::CartAndDisp;
    } else if (strcasecmp(option, "video") == 0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::Video;
    } else if (strcasecmp(option, "video2") == 0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::Video2;
    } else if (strcasecmp(option, "video3") == 0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::Video3;
    } else if (strcasecmp(option, "bundle") == 0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::VideoBundleAdj;
    } else if (strcasecmp(option, "bundle1") == 0) {
      test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::BundleAdj;
    } else {
      cerr << "Unknown option: "<<option<<endl;
      exit(1);
    }
  } else {
    test3DPoseEstimate.mTestType = CvTest3DPoseEstimate::Cartesian;
  }

  cout << "Testing wg3DPoseEstimate ..."<<endl;


    test3DPoseEstimate.test();

  cout << "Done testing wg3DPoseEstimate ..."<<endl;
  return 0;
}

/*********************************************************************
* A GUI for clicking on points which are then published as track initialization points.
*
**********************************************************************
*
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Caroline Pantofaru
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

#include <stdio.h>
#include <iostream>
#include <vector>

#include "ros/node.h"
#include "std_msgs/ImageArray.h"
#include "std_msgs/String.h"
#include "image_utils/cv_bridge.h"
#include "CvStereoCamModel.h"
#include <robot_msgs/PositionMeasurement.h>

#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;

struct PublishedPoint {
  CvPoint xy;
  bool published;
};

vector<PublishedPoint> gxys;
ros::thread::mutex g_selection_mutex;
ros::Time g_last_image_time;
bool g_do_cb;

using namespace std;


void on_mouse(int event, int x, int y, int flags, void *params){
  
  switch(event){
  case CV_EVENT_LBUTTONUP:
    // Add a clicked-on point to the list of points, to be published by the image callback on the next image.
    g_selection_mutex.lock();
    PublishedPoint p;
    p.xy = cvPoint(x,y);
    p.published = false;
    gxys.push_back(p);
    g_do_cb = true;
    g_selection_mutex.unlock();

    break;
    
  default:
    break;    
  }
}


class TrackStarterGUI: public ros::node
{
public:
  std_msgs::ImageArray image_msg_;
  CvBridge<std_msgs::Image> *cv_bridge_left_;
  CvBridge<std_msgs::Image> *cv_bridge_disp_;
  bool built_bridge_;
  bool quit_;
  CvStereoCamModel *cam_model_;
  std_msgs::String cal_params_;
  CvMat *uvd_, *xyz_;
  IplImage *cv_image_;
  IplImage *cv_disp_image_;
  IplImage *cv_image_cpy_;
  IplImage *cv_disp_image_cpy_;
  ros::thread::mutex cv_mutex_;
  robot_msgs::PositionMeasurement pos;
  

  TrackStarterGUI():
    node("track_starter_gui",ros::node::ANONYMOUS_NAME),
    cv_bridge_left_(NULL),
    cv_bridge_disp_(NULL),
    built_bridge_(false),
    quit_(false),
    cam_model_(NULL),
    uvd_(NULL),
    xyz_(NULL),
    cv_image_(NULL),
    cv_disp_image_(NULL),
    cv_image_cpy_(NULL),
    cv_disp_image_cpy_(NULL)
  {
    
    cvNamedWindow("Track Starter: Left Image",0);
    cvNamedWindow("Track Starter: Disparity",0);
    cvSetMouseCallback("Track Starter: Left Image", on_mouse, 0);

    uvd_ = cvCreateMat(1,3,CV_32FC1);
    xyz_ = cvCreateMat(1,3,CV_32FC1);

    advertise<robot_msgs::PositionMeasurement>("/track_starter_gui/position_measurement",1);
    subscribe("videre/images",image_msg_,&TrackStarterGUI::image_cb,1);
    subscribe("videre/cal_params",cal_params_,&TrackStarterGUI::cal_params_cb,1);
    subscribe("track_starter_gui/position_measurement",pos,&TrackStarterGUI::point_cb,1);
    
  }

  ~TrackStarterGUI() 
  {
    cvDestroyWindow("Track Starter: Left Image");
    cvDestroyWindow("Track Starter: Disparity");
    
    delete cam_model_;
    cvReleaseImage(&cv_image_);
    cvReleaseImage(&cv_disp_image_);
    cvReleaseImage(&cv_image_cpy_);
    cvReleaseImage(&cv_disp_image_cpy_);
    cvReleaseMat(&uvd_);
    cvReleaseMat(&xyz_);

    if (built_bridge_) {
      delete cv_bridge_left_;
      delete cv_bridge_disp_;
    }
  }


  // JD's small parser to pick up the projection matrix from the
  /// calibration message. This should really be somewhere else, this code is copied in multiple files.
  void parseCaliParams(const string& cal_param_str){

    if (cam_model_==NULL) {
      const string labelRightCamera("[right camera]");
      const string labelRightCamProj("proj");
      const string labelRightCamRect("rect");
      // move the current position to the section of "[right camera]"
      size_t rightCamSection = cal_param_str.find(labelRightCamera);
      // move the current position to part of proj in the section of "[right camera]"
      size_t rightCamProj = cal_param_str.find(labelRightCamProj, rightCamSection);
      // get the position of the word "rect", which is also the end of the projection matrix
      size_t rightCamRect = cal_param_str.find(labelRightCamRect, rightCamProj);
      // the string after the word "proj" is the starting of the matrix
      size_t matrix_start = rightCamProj + labelRightCamProj.length();
      // get the sub string that contains the matrix
      string mat_str = cal_param_str.substr(matrix_start, rightCamRect-matrix_start);
      // convert the string to a double array of 12
      stringstream sstr(mat_str);
      double matdata[12];
      for (int i=0; i<12; i++) {
	sstr >> matdata[i];
      }

      //if (cam_model_ == NULL) {
      double Fx  = matdata[0]; // 0,0
      double Fy  = matdata[5]; // 1,1
      double Crx = matdata[2]; // 0,2
      double Cy  = matdata[6]; // 1,2
      double Clx = Crx; // the same
      double Tx  = - matdata[3]/Fx;
      std::cout << "base length "<< Tx << std::endl;
      cam_model_ = new CvStereoCamModel(Fx, Fy, Tx, Clx, Crx, Cy, 0.25);
    }
  }

  // Calibration parameters callback
  void cal_params_cb() {
    parseCaliParams(cal_params_.data);
  }

  // Sanity check, print the published point.
  void point_cb() {
    printf("pos %f %f %f\n",pos.pos.x,pos.pos.y,pos.pos.z);

  }

  // Image callback. Draws selected points on images, publishes the point messages, and copies the images to be displayed.
  void image_cb(){

    cv_mutex_.lock();
    if (!g_do_cb) {
      cv_mutex_.unlock();
      return;
    }

    cv_mutex_.unlock();

    // Set up the cv bridges, should only run once.
    if (!built_bridge_) {
      cv_bridge_left_ = new CvBridge<std_msgs::Image>(&image_msg_.images[1],  CvBridge<std_msgs::Image>::CORRECT_BGR | CvBridge<std_msgs::Image>::MAXDEPTH_8U);
      cv_bridge_disp_ = new CvBridge<std_msgs::Image>(&image_msg_.images[0],  CvBridge<std_msgs::Image>::CORRECT_BGR | CvBridge<std_msgs::Image>::MAXDEPTH_8U);
      built_bridge_ = true;
    }

    // Convert the images to opencv format.
    if (cv_image_) {
      cvReleaseImage(&cv_image_);
      cvReleaseImage(&cv_disp_image_);
    } 
    cv_bridge_left_->to_cv(&cv_image_);
    cv_bridge_disp_->to_cv(&cv_disp_image_);

    CvSize im_size = cvGetSize(cv_image_);

    for (uint i = 0; i<gxys.size(); i++) {

      if (cam_model_ && !gxys[i].published) {
	bool search = true;
  
	int d = cvGetReal2D(cv_disp_image_,gxys[i].xy.y,gxys[i].xy.x);
	if (d==0.0) {
	  search = true;
	  int x1 = gxys[i].xy.x;	
	  int y1 = gxys[i].xy.y;
	  int x2 = gxys[i].xy.x;	
	  int y2 = gxys[i].xy.y;
	  while (d==0.0 && search) {
	    x1--; y1--;
	    x2++; y2++;
	    search = false;
	    if (x1 > 0) {
	      d = cvGetReal2D(cv_disp_image_,gxys[i].xy.y,x1);
	      search = true;
	    }
	    if (d==0.0 && x2 < cv_disp_image_->width) {     
	      d = cvGetReal2D(cv_disp_image_,gxys[i].xy.y,x2);
	      search = true;
	    }
	    if (d==0.0 && y1 > 0) {
	      d = cvGetReal2D(cv_disp_image_,y1,gxys[i].xy.x);
	      search = true;
	    }
	    if (d==0.0 && y2 < cv_disp_image_->height) {
	      d = cvGetReal2D(cv_disp_image_,y2,gxys[i].xy.x);
	      search = true;
	    }
	      
	  }
	}
	if (search) {  
	  robot_msgs::PositionMeasurement pm;
	  pm.header.stamp = g_last_image_time;
	  pm.name = "track_starter_gui";
	  pm.object_id = gxys.size();   
	  cvmSet(uvd_,0,0,gxys[i].xy.x);
	  cvmSet(uvd_,0,1,gxys[i].xy.y);
	  cvmSet(uvd_,0,2,d);
	  cam_model_->dispToCart(uvd_, xyz_);
	  pm.pos.x = cvmGet(xyz_,0,2);
	  pm.pos.y = -1.0*cvmGet(xyz_,0,0);
	  pm.pos.z = -1.0*cvmGet(xyz_,0,1);
	  pm.header.frame_id = "stereo_link";
	  pm.reliability = 1;
	  pm.initialization = 1;
	  publish("track_starter_gui/position_measurement",pm);
	  gxys[i].published = true;
	}	
      }

      cvCircle(cv_image_, gxys[i].xy, 2 , cvScalar(255,0,0) ,4);
    }

    cv_mutex_.lock();

    if (cv_image_cpy_ == NULL) {
      cv_image_cpy_ = cvCreateImage(im_size,IPL_DEPTH_8U,3);
    }
    cvCopy(cv_image_, cv_image_cpy_);

    if (cv_disp_image_cpy_==NULL) {
      cv_disp_image_cpy_ = cvCreateImage(im_size,IPL_DEPTH_8U,1);
    }
    cvCopy(cv_disp_image_, cv_disp_image_cpy_);

    g_last_image_time = image_msg_.header.stamp;

    cv_mutex_.unlock();
    
  }

  // Wait for thread to exit.
  bool spin() {
    while (ok() && !quit_) {
      // Display the image
      cv_mutex_.lock();
      if (cv_image_cpy_) {
	cvShowImage("Track Starter: Left Image", cv_image_cpy_);
      }
      if (cv_disp_image_cpy_) {
	cvShowImage("Track Starter: Disparity", cv_disp_image_cpy_);
      }
      cv_mutex_.unlock();

      int c = cvWaitKey(2);
      c &= 0xFF;
      // Quit on ESC, "q" or "Q"
      if((c == 27)||(c == 'q')||(c == 'Q')){
	quit_ = true;
      }
      else if ((c=='p') || (c=='P')){
	cv_mutex_.lock();
	g_do_cb = 1-g_do_cb;
	cv_mutex_.unlock();
      }
	
    }
    return true;
  }
};

// Main 
int main(int argc, char**argv) 
{
  ros::init(argc,argv);
 
  g_do_cb = true;
  TrackStarterGUI tsgui;
  tsgui.spin();
  ros::fini();
  return 0;

}

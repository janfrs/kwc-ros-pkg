/*********************************************************************
* A ros node to run face detection with images from the videre cameras.
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

#include "CvStereoCamModel.h"
#include <robot_msgs/PositionMeasurement.h>
#include "image_msgs/StereoInfo.h"
#include "image_msgs/CamInfo.h"
#include "image_msgs/Image.h"
#include "image_msgs/CvBridge.h"
#include "topic_synchronizer.h"

#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"

#include "people.h"


using namespace std;

// FaceDetector - A wrapper around OpenCV's face detection, plus some usage of depth to restrict the search.

class FaceDetector: public ros::node {
public:
  // Images and conversion
  image_msgs::Image limage_;
  image_msgs::Image dimage_;
  image_msgs::StereoInfo stinfo_;
  image_msgs::CamInfo rcinfo_;
  image_msgs::CvBridge lbridge_;
  image_msgs::CvBridge dbridge_;
  TopicSynchronizer<FaceDetector> sync_;

  // The left and disparity images.
  IplImage *cv_image_left_;
  IplImage *cv_image_disp_;

  bool use_depth_;
  CvStereoCamModel *cam_model_;

  People *people_;
  const char *haar_filename_;

  bool quit_;
  int detect_;

  ros::thread::mutex cv_mutex_;

  FaceDetector(const char *haar_filename, bool use_depth) : 
    ros::node("face_detector"),
    sync_(this, &FaceDetector::image_cb_all, ros::Duration(0.05), &FaceDetector::image_cb_timeout),
    cv_image_left_(NULL),
    cv_image_disp_(NULL),
    use_depth_(use_depth),
    cam_model_(NULL),
    people_(NULL),
    haar_filename_(haar_filename),
    quit_(false),
    detect_(0)
  { 

    // OpenCV: pop up an OpenCV highgui window
    cvNamedWindow("Face detector: Disparity",CV_WINDOW_AUTOSIZE);
    cvNamedWindow("Face detector: Face Detection", CV_WINDOW_AUTOSIZE);

    people_ = new People();

    // Subscribe to the images and parameters
    sync_.subscribe("dcam/left/image_rect_color",limage_,1);
    sync_.subscribe("dcam/disparity",dimage_,1);
    sync_.subscribe("dcam/stereo_info",stinfo_,1);
    sync_.subscribe("dcam/right/cam_info",rcinfo_,1);

    // Advertise a position measure message.
    advertise<robot_msgs::PositionMeasurement>("face_detection/position_measurement",1);
    //subscribe<robot_msgs::PositionMeasurement>("face_detection",pos,&FaceDetector::pos_cb,1);

  }

  ~FaceDetector()
  {

    cvReleaseImage(&cv_image_left_);
    cvReleaseImage(&cv_image_disp_);

    cvDestroyWindow("Face detector: Face Detection");
    cvDestroyWindow("Face detector: Disparity");

    delete cam_model_;

    delete people_;

  }

  /// The image callback when not all topics are sync'ed. Don't do anything, just wait for sync.
  void image_cb_timeout(ros::Time t) {
    if (limage_.header.stamp != t)
      printf("Timed out waiting for left image\n");
    if (dimage_.header.stamp != t)
      printf("Timed out waiting for disparity image\n");
  }

  /// The image callback. For each new image, copy it, perform face detection, and draw the rectangles on the image.
  void image_cb_all(ros::Time t)
  {

    detect_++;
    if (detect_ % 2) {
      return;
    }

    cv_mutex_.lock();
 
    CvSize im_size;

    // Convert the images to OpenCV format
    if (lbridge_.fromImage(limage_,"bgr")) {
      cv_image_left_ = lbridge_.toIpl();
    }
    if (dbridge_.fromImage(dimage_)) {
      cv_image_disp_ = dbridge_.toIpl();
    }

    // Convert the stereo calibration into a camera model.
    if (cam_model_) {
      delete cam_model_;
    }
    double Fx = rcinfo_.P[0];
    double Fy = rcinfo_.P[5];
    double Clx = rcinfo_.P[2];
    double Crx = Clx;
    double Cy = rcinfo_.P[6];
    double Tx = -rcinfo_.P[3]/Fx;
    cam_model_ = new CvStereoCamModel(Fx,Fy,Tx,Clx,Crx,Cy,1.0/stinfo_.dpp);
 
    if ( cv_image_left_ )  {
      im_size = cvGetSize(cv_image_left_);

      vector<CvRect> faces_vector = people_->detectAllFaces(cv_image_left_, haar_filename_, 1.0, cv_image_disp_, cam_model_, true);
      

      // Get the average disparity in the middle half of the bounding box, and compute the face center in 3d. Publish the face center as a track point.
      if (cv_image_disp_ && cam_model_) {
	int r, c, good_pix;
	ushort* ptr;
	double avg_disp;
	CvRect *one_face;
	robot_msgs::PositionMeasurement pos;
	CvMat *uvd = cvCreateMat(1,3,CV_32FC1);
	CvMat *xyz = cvCreateMat(1,3,CV_32FC1);
	for (uint iface = 0; iface < faces_vector.size(); iface++) {
	  one_face = &faces_vector[iface];
	  good_pix = 0;
	  avg_disp = 0;
	  for (r = floor(one_face->y+0.25*one_face->height); r < floor(one_face->y+0.75*one_face->height); r++) {
	    ptr = (ushort*)(cv_image_disp_->imageData + r*cv_image_disp_->widthStep);
	    for (c = floor(one_face->x+0.25*one_face->width); c < floor(one_face->x+0.75*one_face->width); c++) {
	      if (ptr[c] > 0) {
		avg_disp += ptr[c];
		good_pix++;
	      }
	    }
	  }
	  avg_disp /= (double)good_pix; // Take the average.
	  cvmSet(uvd,0,0,one_face->x+one_face->width/2.0);
	  cvmSet(uvd,0,1,one_face->y+one_face->height/2.0);
	  cvmSet(uvd,0,2,avg_disp);
	  cam_model_->dispToCart(uvd,xyz);
	  pos.header.stamp = limage_.header.stamp;
	  pos.name = "face_detection";
	  pos.object_id = -1;
	  pos.pos.x = cvmGet(xyz,0,2);
	  pos.pos.y = -1.0*cvmGet(xyz,0,0);
	  pos.pos.z = -1.0*cvmGet(xyz,0,1);
	  pos.header.frame_id = "stereo_link";
	  pos.reliability = 0.8;
	  pos.initialization = 0;
	  //pos.covariance = ;
	  publish("face_detection/position_measurement",pos);
	}
	cvReleaseMat(&uvd);
	cvReleaseMat(&xyz);
      }


      cvShowImage("Face detector: Face Detection",cv_image_left_);
      cvShowImage("Face detector: Disparity",cv_image_disp_);
 
      cv_mutex_.unlock();
    }
  }


  // Wait for completion, wait for user input, display images.
  bool spin() {
    while (ok() && !quit_) {

	// Display all of the images.
	cv_mutex_.lock();

	// Get user input and allow OpenCV to refresh windows.
	int c = cvWaitKey(2);
	c &= 0xFF;
	// Quit on ESC, "q" or "Q"
	if((c == 27)||(c == 'q')||(c == 'Q'))
	  quit_ = true;

	cv_mutex_.unlock();
	usleep(10000);

    }
    return true;
  } 

};


// Main
int main(int argc, char **argv)
{
  ros::init(argc, argv);
  bool use_depth = true;

  if (argc < 2) {
    cerr << "Path to cascade file required.\n" << endl;
    return 0;
  }
  char *haar_filename = argv[1]; 
  FaceDetector fd(haar_filename, use_depth);
 
  fd.spin();


  ros::fini();
  return 0;
}



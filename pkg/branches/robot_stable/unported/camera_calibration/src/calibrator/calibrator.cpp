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

#include <iostream>
#include <sstream>
#include <fstream>

#include <sys/stat.h>
#include <time.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "SDL/SDL.h"

#include "ros/ros_slave.h"
#include "common_flows/FlowImage.h"
#include "common_flows/ImageCodec.h"
#include "unstable_flows/FlowPTZActuatorNoSub.h"
#include "simple_sdl_gui/FlowSDLKeyEvent.h"

void matToScreen(CvMat *mat, const char* matname) {
  cout << matname << " = [\r\n";
  for(int i=0; i<mat->rows; i++) {
    for(int j=0; j<mat->cols; j++)
      cout << " " << cvmGet(mat, i, j);
    cout << ";\r\n";
  }
  cout << "]\r\n";   
}


class Calibrator : public ROS_Slave
{
public:
  FlowImage *image_in;
  ImageCodec<FlowImage> *codec_in;

  FlowImage *image_out;
  ImageCodec<FlowImage> *codec_out;

  FlowPTZActuatorNoSub *control;
  FlowPTZActuatorNoSub *observe;
  FlowSDLKeyEvent *key;

  CvMat *cvimage_in;
  CvMat *cvimage_out;
  CvMat *cvimage_bgr;
  CvMat *cvimage_undistort;

  CvMat* intrinsic_matrix;
  CvMat* distortion_coeffs;

  bool calibrated;
  bool undistort;

  bool centering;
  bool take_pic;
  int img_cnt;

  char dir_name[256];

  CvPoint2D32f* last_corners;

  Calibrator() : ROS_Slave()
  {
    register_sink(image_in = new FlowImage("image_in"), ROS_CALLBACK(Calibrator, image_received));
    codec_in = new ImageCodec<FlowImage>(image_in);

    register_source(image_out = new FlowImage("image_out"));
    codec_out = new ImageCodec<FlowImage>(image_out);

    register_sink(observe = new FlowPTZActuatorNoSub("observe"), ROS_CALLBACK(Calibrator, ptz_received));
    register_source(control = new FlowPTZActuatorNoSub("control"));
    register_sink(key = new FlowSDLKeyEvent("key"), ROS_CALLBACK(Calibrator, key_received));

    register_with_master();

    cvimage_in = cvCreateMatHeader(480, 704, CV_8UC3);
    cvimage_out = cvCreateMatHeader(480, 704, CV_8UC3);

    cvimage_bgr = cvCreateMat(480, 704, CV_8UC3);
    cvimage_undistort = cvCreateMat(480, 704, CV_8UC3);

    if ((intrinsic_matrix = (CvMat*)cvLoad("intrinsic.dat")) == 0) {
      intrinsic_matrix  = cvCreateMat( 3, 3, CV_32FC1 );
    }

    if ((distortion_coeffs = (CvMat*)cvLoad("distortion.dat")) == 0) {
      distortion_coeffs = cvCreateMat( 4, 1, CV_32FC1 );
    }

    matToScreen(intrinsic_matrix, "intrinsic");
    matToScreen(distortion_coeffs, "distortion");    

    calibrated = false;
    undistort = false;
    centering = false;
    take_pic = false;
    img_cnt = 0;

    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    sprintf(dir_name, "images/%.2d%.2d%.2d_%.2d%.2d%.2d", timeinfo->tm_mon + 1, timeinfo->tm_mday,timeinfo->tm_year - 100,timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    if (mkdir(dir_name, 0755)) {
      std::cout << "Failed to make directory: " << dir_name;
    }

    last_corners = new CvPoint2D32f[12*12];
    
  }
  virtual ~Calibrator() { }

  void key_received() {
    control->pan_valid = false;      
    control->tilt_valid = false;
    control->lens_zoom_valid = false;
    control->lens_focus_valid = false;
    control->lens_iris_valid = false;

    //    std::cout << "Got keypress: " << key->data << std::endl;

    float tmp_focus;
    if (key->state == SDL_PRESSED) {
      switch (key->sym) {
      case SDLK_UP:
	control->tilt_val = 1;
	control->tilt_rel = true;
	control->tilt_valid = true;
	break;
      case SDLK_DOWN:
	control->tilt_val = -1;
	control->tilt_rel = true;
	control->tilt_valid = true;
	break;
      case SDLK_LEFT:
	control->pan_val = -1;
	control->pan_rel = true;
	control->pan_valid = true;
	break;
      case SDLK_RIGHT:
	control->pan_val = 1;
	control->pan_rel = true;
	control->pan_valid = true;
	break;
      case 61:
	control->lens_zoom_val = 100;
	control->lens_zoom_rel = true;
	control->lens_zoom_valid = true;
	break;
      case 45:
	control->lens_zoom_val = -100;
	control->lens_zoom_rel = true;
	control->lens_zoom_valid = true;
	break;
      case SDLK_RIGHTBRACKET:
	control->lens_focus_val = 100;
	control->lens_focus_rel = true;
	control->lens_focus_valid = true;
	break;
      case SDLK_LEFTBRACKET:
	control->lens_focus_val = -100;
	control->lens_focus_rel = true;
	control->lens_focus_valid = true;
	break;
      case SDLK_SPACE:
	control->pan_val = 0;
	control->pan_rel = false;
	control->pan_valid = true;

	control->tilt_val = 0;
	control->tilt_rel = false;
	control->tilt_valid = true;

	control->lens_zoom_val = 5000;
	control->lens_zoom_rel = false;
	control->lens_zoom_valid = true;
	break;
      case SDLK_c:
	centering = !centering;
	break;
      case SDLK_d:
	undistort = !undistort;
	break;
      case SDLK_f:
	observe->lock_atom();
	tmp_focus = observe->lens_focus_val;
	observe->unlock_atom();

	if (tmp_focus > 0) {
	  control->lens_focus_val = -1;
	  control->lens_focus_rel = false;
	  control->lens_focus_valid = true;
	} else {
	  control->lens_focus_val = 0;
	  control->lens_focus_rel = true;
	  control->lens_focus_valid = true;
	}
	break;
      case SDLK_RETURN:
	take_pic = true;
	break;
      case SDLK_a:
	do_calibration();
      }

      control->publish();
      return;
    }
  }

  void ptz_received() {
    /*
    std::cout << observe->pan << ", " 
	      << observe->tilt << ", " 
	      << observe->zoom << ", " 
	      << observe->focus <<  std::endl;
    */
    return;
  }

  void image_received() {
    return;
  }

  void process_image()
  {

    //    std::cout << "Checking publish count: " << image_in->publish_count << std::endl;

    //    image_in->lock_atom();

    if (image_in->publish_count > 0) {

      cvSetData(cvimage_in, codec_in->get_raster(), 3*704);
      cvConvertImage(cvimage_in, cvimage_bgr, CV_CVTIMG_SWAP_RB);

      //      image_in->unlock_atom();

      CvSize board_sz = cvSize(12, 12);
      CvPoint2D32f* corners = new CvPoint2D32f[12*12];
      int corner_count = 0;
    
      //This function has a memory leak in the current version of opencv!
      int found = cvFindChessboardCorners(cvimage_bgr, board_sz, corners, &corner_count, 
      					  CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);



      IplImage* gray = cvCreateImage(cvSize(cvimage_bgr->width, cvimage_bgr->height), IPL_DEPTH_8U, 1);
      cvCvtColor(cvimage_bgr, gray, CV_BGR2GRAY);
      cvFindCornerSubPix(gray, corners, corner_count, 
      			 cvSize(5, 5), cvSize(-1, -1),
      			 cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10, 0.01f ));
      cvReleaseImage(&gray);


      if (take_pic && corner_count == 144) {
	std::stringstream ss;
	img_cnt++;
	ss << dir_name << "/Image" << img_cnt << ".jpg";
	//	std::ofstream imgfile(ss.str().c_str());
	//	imgfile.write((char*)image_in->jpeg_buffer, image_in->compressed_size);
	//	imgfile.close();

	cvSaveImage(ss.str().c_str(), cvimage_bgr);
	
	ss.str("");
	ss << dir_name << "/Position" << img_cnt << ".txt";

	std::ofstream posfile(ss.str().c_str());
	observe->lock_atom();
	posfile << "P: " << observe->pan_val << std::endl
		<< "T: " << observe->tilt_val << std::endl
		<< "Z: " << observe->lens_zoom_val << std::endl
		<< "F: " << observe->lens_focus_val;
	observe->unlock_atom();

	posfile.close();

	take_pic = false;
      }

      float maxdiff = 0;

      for(int c=0; c<12*12; c++) {
	float diff = sqrt( pow(corners[c].x - last_corners[c].x, 2.0) + 
		     pow(corners[c].y - last_corners[c].y, 2.0));
	last_corners[c].x = corners[c].x;
	last_corners[c].y = corners[c].y;

	if (diff > maxdiff) {
	  maxdiff = diff;
	}
      }

      printf("Max diff: %g\n", maxdiff);


      cvDrawChessboardCorners(cvimage_bgr, board_sz, corners, corner_count, found);

      if (undistort) {
	cvUndistort2(cvimage_bgr, cvimage_undistort, intrinsic_matrix, distortion_coeffs);
      } else {
	cvCopy(cvimage_bgr, cvimage_undistort);
      }

      CvFont font;
      cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.8, 0.8, 0, 2);    
      std::stringstream ss;

      observe->lock_atom();
      ss << "P: " << observe->pan_val;
      ss << " T: " << observe->tilt_val;
      ss << " Z: " << observe->lens_zoom_val;
      ss << " F: " << observe->lens_focus_val;
      observe->unlock_atom();
      cvPutText(cvimage_undistort, ss.str().c_str(), cvPoint(15,30), &font, CV_RGB(255,0,0));

      ss.str("");

      ss << "Found " << corner_count << " corners";
      if (centering) {
	ss << " -- Autocentering";
      }
      cvPutText(cvimage_undistort, ss.str().c_str(), cvPoint(15,60), &font, CV_RGB(255,0,0));

      image_out->width = 704;
      image_out->height = 480;
      image_out->compression = "raw";
      image_out->colorspace = "rgb24";

      //      codec_out->realloc_raster_if_needed();
      cvSetData(cvimage_out, codec_out->get_raster(), 3*image_out->width);      
      cvConvertImage(cvimage_undistort, cvimage_out, CV_CVTIMG_SWAP_RB);

      codec_out->set_flow_data();

      image_out->publish();


      CvPoint2D32f COM = cvPoint2D32f(0,0);
    
      if (centering && corner_count > 20) {
	//average corners:
	for (int i = 0; i < corner_count; i++) {
	  COM.x += corners[i].x / corner_count;
	  COM.y += corners[i].y / corner_count;
	}
      
	if ( (fabs(COM.x - 354.0) > 10) || (fabs(COM.y - 240.0) > 10) ) {
	  float rel_pan,rel_tilt;

	  rel_pan = (COM.x - 354.0) * .001;
	  rel_tilt = -(COM.y - 240.0) * .001;

	  control->pan_val = rel_pan;      
	  control->pan_rel = true;
	  control->pan_valid = true;

	  control->tilt_val = rel_tilt;
	  control->tilt_rel = true;
	  control->tilt_valid = true;

	  control->publish();
	}

      }

      delete[] corners;
      
    } else {
      //      image_in->unlock_atom();
    }
  }


  //borrowed from Patrick...
  int do_calibration() {
    IplImage* image = NULL; 
    int xc = 12;
    int yc = 12;
    int nc = xc*yc;
    float side_length = 15;

    CvSize board_sz = cvSize(xc, yc);

    int ntrials = img_cnt;

    char filename[128];

    CvMat* object_points     = cvCreateMat( ntrials * nc, 3, CV_32FC1 );
    CvMat* image_points      = cvCreateMat( ntrials * nc, 2, CV_32FC1 );
    CvMat* point_counts      = cvCreateMat( ntrials, 1, CV_32SC1 );
    
    CvPoint2D32f* corners = new CvPoint2D32f[nc];

    for(int t=1; t<=ntrials; t++) {
      // load an image
      sprintf(filename, "%s/Image%d.jpg", dir_name, t);
      image = cvLoadImage(filename);
      if(!image){
	printf("Could not load image file: %s\n",filename);
	exit(0);
      }
      
      int corner_count;
      int found = cvFindChessboardCorners(image, board_sz, corners, &corner_count, 
					  CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
      
      IplImage* gray = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 1);
      cvCvtColor(image, gray, CV_BGR2GRAY);
      
      cvFindCornerSubPix(gray, corners, corner_count, 
			 cvSize(5, 5), cvSize(-1, -1),
			 cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10, 0.01f ));
      
      printf("Image: %d, %d / %d\n", t, corner_count, nc);
      
      // get data points
      if(corner_count == nc && found != 0) {
	for(int c=0; c<nc; c++) {
	  CV_MAT_ELEM( *image_points,  float, (t-1)*nc+c, 0 ) = corners[c].x;
	  CV_MAT_ELEM( *image_points,  float, (t-1)*nc+c, 1 ) = corners[c].y;
	  CV_MAT_ELEM( *object_points, float, (t-1)*nc+c, 0 ) = (float)(c/xc)*side_length;
	  CV_MAT_ELEM( *object_points, float, (t-1)*nc+c, 1 ) = (float)(c%xc)*side_length;
	  CV_MAT_ELEM( *object_points, float, (t-1)*nc+c, 2 ) = 0.0f;
	}
	CV_MAT_ELEM( *point_counts,  int, t-1, 0 ) = nc;
      }
      else printf("Bad board! How did this happen?\n");
    }
    // calibrate
    // Initialize the intrinsic matrix such that the two focal
    // lengths have a ratio of 1.0
    CV_MAT_ELEM( *intrinsic_matrix, float, 0, 0 ) = 1.0f;
    CV_MAT_ELEM( *intrinsic_matrix, float, 1, 1 ) = 1.0f;
    
    cvCalibrateCamera2(
		       object_points,
		       image_points,
		       point_counts,
		       cvGetSize(image),
		       intrinsic_matrix,
		       distortion_coeffs,
		       NULL,
		       NULL,
		       0 //			 CV_CALIB_FIX_ASPECT_RATIO
		       );

    calibrated = true;

    cvSave("intrinsic.dat", intrinsic_matrix);
    cvSave("distortion.dat", distortion_coeffs);

    matToScreen(intrinsic_matrix, "intrinsic");
    matToScreen(distortion_coeffs, "distortion");

    /*    
    // save data
    sprintf(filename, "%s/cal.m", dir_name);
    ofstream out(filename, ios_base::app);
    if(out.is_open()) {
      char matName[64];
      matToFile(intrinsic_matrix, out, "instrinsics");
      matToFile(distortion_coeffs, out, "distortion");
      out.close();
    } else printf("  error saving to file %s!\n", filename);
    */

    delete[] corners;

  }

};

int main(int argc, char **argv)
{
  Calibrator c;

  while (c.happy())
  {
    c.process_image();
  }
  return 0;
}


/*********************************************************************
* People-specific computer vision algorithms.
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

#ifndef PEOPLE_H
#define PEOPLE_H

#include <stdio.h>
#include <iostream>
#include <vector>

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/cvaux.h>

#include "CvStereoCamModel.h"

// Thresholds for the face detection algorithm
#define FACE_SIZE_MIN_MM 100
#define FACE_SIZE_MAX_MM 400
#define MAX_Z_MM 10000

using namespace std;

struct Person {
  CvHistogram *face_color_hist_;
  CvHistogram *shirt_color_hist_;
  double body_height_;
  double body_width_;
  CvRect body_bbox_;
  IplImage *body_mask_;
  double face_size_;
  CvRect face_bbox_;
  IplImage *face_mask_;
};

class People
{
 public:

  // Create an empty list of people.
  People();

  // Destroy a list of people.
  ~People();

  // Add a person to the list of people.
  void addPerson(){}

  // Remove a person from the list of people.
  void removePerson(){}

  // Use the list of people to recognize a face image.
  void recognizePerson(){}

  /********
   * Detect all faces in an image.
   * Input:
   * image - The image in which to detect faces.
   * haar_classifier_filename - Path to the xml file containing the trained haar classifier cascade.
   * threshold - Detection threshold. Currently unused.
   * disparity_image - Image of disparities (from stereo). To avoid using depth information, set this to NULL.
   * cam_model - The camera model created by CvStereoCamModel.
   * do_draw - If true, draw a box on `image' around each face.
   * Output:
   * A vector of CvRects containing the bounding boxes around found faces.
   *********/ 
  vector<CvRect> detectAllFaces(IplImage *image, const char* haar_classifier_filename, double threshold, IplImage *disparity_image, CvStereoCamModel *cam_model, bool do_draw);

  // Detect only known faces in an image.
  void detectKnownFaces(){}

  // Track a face.
  void track(){}

 ////////////////////
 private:


  // The list of people.
  vector<Person> list_;
  // Classifier cascade for face detection.
  CvHaarClassifierCascade *cascade_;
  // Storage for OpenCV functions.
  CvMemStorage *storage_;
  // Grayscale image (to avoid reallocating an image each time an OpenCV function is run.)
  IplImage *cv_image_gray_;


};

#endif

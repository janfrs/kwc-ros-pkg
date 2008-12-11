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

#include "calc_leg_features.h"

#include "opencv/cxcore.h"
#include "opencv/cv.h"

using namespace laser_processor;
using namespace std;

vector<float> calcLegFeatures(SampleSet* cluster, std_msgs::LaserScan& scan)
{

  vector<float> features;

  // Number of points
  int num_points = cluster->size();
  features.push_back(num_points);

  // Compute mean and median points for future use
  float x_mean = 0.0;
  float y_mean = 0.0;
  vector<float> x_median_set;
  vector<float> y_median_set;
  for (SampleSet::iterator i = cluster->begin();
       i != cluster->end();
       i++)
      
  {
    x_mean += ((*i)->x)/num_points;
    y_mean += ((*i)->y)/num_points;
    x_median_set.push_back((*i)->x);
    y_median_set.push_back((*i)->y);
  }

  std::sort(x_median_set.begin(), x_median_set.end());
  std::sort(y_median_set.begin(), y_median_set.end());
    
  float x_median = 0.5 * ( *(x_median_set.begin() + (num_points-1)/2) + *(x_median_set.begin() + num_points/2) );
  float y_median = 0.5 * ( *(y_median_set.begin() + (num_points-1)/2) + *(y_median_set.begin() + num_points/2) );

  //Compute std and avg diff from median

  double sum_std_diff = 0.0;
  double sum_med_diff = 0.0;


  for (SampleSet::iterator i = cluster->begin();
       i != cluster->end();
       i++)
      
  {
    sum_std_diff += pow( (*i)->x - x_mean, 2.0) + pow((*i)->y - y_mean, 2.0);
    sum_med_diff += sqrt(pow( (*i)->x - x_median, 2.0) + pow((*i)->y - y_median, 2.0));
  }

  float std = sqrt( 1.0/(num_points - 1.0) * sum_std_diff);
  float avg_median_dev = sum_med_diff / num_points;

  features.push_back(std);
  features.push_back(avg_median_dev);


  // Take first at last
  SampleSet::iterator first = cluster->begin();
  SampleSet::iterator last = cluster->end();
  last--;

  // Compute Jump distance
  int prev_ind = (*first)->index - 1;
  int next_ind = (*last)->index + 1;

  float prev_jump = 0;
  float next_jump = 0;

  if (prev_ind >= 0)
  {
    Sample* prev = Sample::Extract(prev_ind, scan);
    if (prev)
    {
      prev_jump = sqrt( pow( (*first)->x - prev->x, 2.0) + pow((*first)->y - prev->y, 2.0));
      delete prev;
    }
    
  }

  if (next_ind < scan.ranges.size())
  {
    Sample* next = Sample::Extract(next_ind, scan);
    if (next)
    {
      next_jump = sqrt( pow( (*last)->x - next->x, 2.0) + pow((*last)->y - next->y, 2.0));
      delete next;
    }
  }

  features.push_back(prev_jump);
  features.push_back(next_jump);

  // Compute Width
  float width = sqrt( pow( (*first)->x - (*last)->x, 2.0) + pow((*first)->y - (*last)->y, 2.0));
  features.push_back(width);

  // Compute Linearity

  CvMat* points = cvCreateMat( num_points, 2, CV_64FC1);
  {
    int j = 0;
    for (SampleSet::iterator i = cluster->begin();
         i != cluster->end();
         i++)
    {
      cvmSet(points, j, 0, (*i)->x - x_mean);
      cvmSet(points, j, 1, (*i)->y - y_mean);
      j++;
    }
  }

  CvMat* W = cvCreateMat( 2, 2, CV_64FC1);        
  CvMat* U = cvCreateMat( num_points, 2, CV_64FC1);
  CvMat* V = cvCreateMat( 2, 2, CV_64FC1);
  cvSVD(points, W, U, V);

  CvMat* rot_points = cvCreateMat(num_points, 2, CV_64FC1);
  cvMatMul(U,W,rot_points);

  float linearity = 0.0;
  for (int i = 0; i < num_points; i++)
  {
    linearity += pow(cvmGet(rot_points, i, 1), 2.0);
  }

  cvReleaseMat(&points);
  cvReleaseMat(&W);
  cvReleaseMat(&U);
  cvReleaseMat(&V);
  cvReleaseMat(&rot_points);

  features.push_back(linearity);

  // Compute Circularity
  CvMat* A = cvCreateMat( num_points, 3, CV_64FC1);
  CvMat* B = cvCreateMat( num_points, 1, CV_64FC1);
  {
    int j = 0;
    for (SampleSet::iterator i = cluster->begin();
         i != cluster->end();
         i++)
    {
      float x = (*i)->x;
      float y = (*i)->y;
        
      cvmSet(A, j, 0, -2.0*x);
      cvmSet(A, j, 1, -2.0*y);
      cvmSet(A, j, 2, 1);
        
      cvmSet(B, j, 0, -pow(x,2.0)-pow(y,2.0));
      j++;
    }
  }
  CvMat* sol = cvCreateMat( 3, 1, CV_64FC1);
  cvSolve(A, B, sol, CV_SVD);

  float xc = cvmGet(sol, 0, 0);
  float yc = cvmGet(sol, 1, 0);
  float rc = sqrt(pow(xc,2.0) + pow(yc,2.0) - cvmGet(sol, 2, 0));

  float circularity = 0.0;
  for (SampleSet::iterator i = cluster->begin();
       i != cluster->end();
       i++)
  {
    circularity += pow( rc - sqrt( pow(xc - (*i)->x, 2.0) + pow( yc - (*i)->y, 2.0) ), 2.0);
  }

  features.push_back(circularity);

  // Radius
  float radius = rc;

  features.push_back(radius);

  //Curvature:
  float mean_curvature = 0.0;

  //Boundary length:
  float boundary_length = 0.0;
  float last_boundary_seg = 0.0;

  float boundary_regularity = 0.0;
  double sum_boundary_reg_sq = 0.0;

  // Mean angular difference
  SampleSet::iterator left = cluster->begin();
  left++;
  left++;
  SampleSet::iterator mid = cluster->begin();
  mid++;
  SampleSet::iterator right = cluster->begin();
    
  float ang_diff = 0.0;

  while (left != cluster->end())
  {
    float mlx = (*left)->x - (*mid)->x;
    float mly = (*left)->y - (*mid)->y;
    float L_ml = sqrt(mlx*mlx + mly*mly);

    float mrx = (*right)->x - (*mid)->x;
    float mry = (*right)->y - (*mid)->y;
    float L_mr = sqrt(mrx*mrx + mry*mry);

    float lrx = (*left)->x - (*right)->x;
    float lry = (*left)->y - (*right)->y;
    float L_lr = sqrt(lrx*lrx + lry*lry);
      
    boundary_length += L_mr;
    sum_boundary_reg_sq += L_mr*L_mr;
    last_boundary_seg = L_ml;

    float A = (mlx*mrx + mly*mry) / pow(L_mr, 2.0);
    float B = (mlx*mry - mly*mrx) / pow(L_mr, 2.0);

    float th = atan2(B,A);

    if (th < 0)
      th += 2*M_PI;

    th -= M_PI;

    ang_diff += th / num_points;

    float s = 0.5*(L_ml+L_mr+L_lr);
    float area = sqrt( s*(s-L_ml)*(s-L_mr)*(s-L_lr) );

    if (th > 0)
      mean_curvature += 4*(area)/(L_ml*L_mr*L_lr*num_points);
    else
      mean_curvature -= 4*(area)/(L_ml*L_mr*L_lr*num_points);

    left++;
    mid++;
    right++;
  }

  boundary_length += last_boundary_seg;
  sum_boundary_reg_sq += last_boundary_seg*last_boundary_seg;

  boundary_regularity = sqrt( (sum_boundary_reg_sq - pow(boundary_length,2.0)/num_points)/(num_points - 1) );

  features.push_back(boundary_length);
  features.push_back(ang_diff);
  features.push_back(mean_curvature);

  features.push_back(boundary_regularity);

  return features;
}

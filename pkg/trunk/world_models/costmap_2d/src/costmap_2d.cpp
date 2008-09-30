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
 *
 * Authors: E. Gil Jones, Conor McGann
 */

/*

  Map format is

(x,y)

       width (x)
  |--------------|
 -
 | (0,0) (1,0) ... (width-1,0)
h| (0,1)
e|   .
i|   .
g|   .
h|   .
t| (0, height-1) ... (width-1, height-1)
 -
(y)

*/

#include "costmap_2d/costmap_2d.h"
#include <algorithm>
#include <set>
#include <assert.h>

const unsigned char CostMap2D::NO_INFORMATION(255);
const unsigned char CostMap2D::INFLATED_OBSTACLE(254);

CostMap2D::CostMap2D(unsigned int width, unsigned int height, const unsigned char* data, 
		     double resolution, double window_length,  unsigned char threshold, 
		     double maxZ, double inflationRadius)
: width_(width), 
   height_(height),  
   resolution_(resolution), 
   tickLength_(window_length/WATCHDOG_LIMIT),
   threshold_(threshold),
   maxZ_(maxZ),
   inflationRadius_(inflationRadius),
   staticData_(NULL), fullData_(NULL), obsWatchDog_(NULL), lastTimeStamp_(0.0)
{
  staticData_ = new unsigned char[width_*height_];

  // For the first pass, just clean up the data and get the set of original obstacles.
  std::set<unsigned int> obstacles;
  for (unsigned int i=0;i<width_;i++){
    for (unsigned int j=0;j<height_;j++){
      unsigned int ind = MC_IND(i, j);
      staticData_[ind] = data[ind];

      // If the source value is greater than the threshold but less than the NO_INFORMATION LIMIT
      // then set it to the threshold
      if(staticData_[ind] > threshold_ && staticData_[ind] < NO_INFORMATION)
	staticData_[ind] = threshold_;

      if(staticData_[ind] == threshold_){
	obstacles.insert(ind);
	staticObstacles_.push_back(ind);
      }
    }
  }

  // Now for every original obstacle, go back and fill in the inflated obstacles
  for(std::set<unsigned int>::const_iterator it = obstacles.begin(); it != obstacles.end(); ++it){
    unsigned int ind = *it;
    std::vector<unsigned int> inflation;
    computeInflation(ind, inflation);

    // All cells in the inflation which have not been marked yet, should be marked, and inserted as
    // permanent obstacles
    for(unsigned int i = 0; i<inflation.size(); i++){
      unsigned int cellId = inflation[i];

      // Must enforce set semantics and ensure that we correctly assign the inflated obstacle value
      if(staticData_[cellId] != threshold_)
	staticData_[cellId] = (cellId == ind ? threshold_ : INFLATED_OBSTACLE);
    }
  }

  fullData_ = new unsigned char[width_*height_];
  obsWatchDog_ = new TICK[width_*height_];
  memcpy(fullData_, staticData_, width_*height_);
  memset(obsWatchDog_, 0, width_*height_);

  std::cout << "CostMap 2D created with " << staticObstacles_.size() << " static obstacles" << std::endl;
}

CostMap2D::~CostMap2D() {
  if(staticData_ != NULL) delete[] staticData_;
  if(fullData_ != NULL) delete[] fullData_;
  if(obsWatchDog_ != NULL) delete[] obsWatchDog_;
}

void CostMap2D::updateDynamicObstacles(double ts,
				       const std_msgs::PointCloudFloat32& cloud,
				       std::vector<unsigned int>& newObstacles, 
				       std::vector<unsigned int>& deletedObstacles)
{
  //std::cout << "Updating for time: " << ts << std::endl;

  newObstacles.clear();

  for(size_t i = 0; i < cloud.get_pts_size(); i++) {
    //std::cout << "Evaluating point:" << i << std::endl;
    if(cloud.pts[i].z > maxZ_)
      continue;

    unsigned int ind = WC_IND(cloud.pts[i].x, cloud.pts[i].y);

    updateCell(ind, threshold_, newObstacles);


    // Compute inflation set
    std::vector<unsigned int> inflation;
    computeInflation(ind, inflation);
    for(unsigned int i = 0; i<inflation.size(); i++)
      updateCell(inflation[i], INFLATED_OBSTACLE, newObstacles);
  }

  // We always process deletions too
  removeStaleObstacles(ts, deletedObstacles);

  if(!newObstacles.empty() || !deletedObstacles.empty()){
    std::cout << newObstacles.size() << " insertions, " << 
      deletedObstacles.size() << " deletions: " << 
      dynamicObstacles_.size() << " dynamic obstacles\n";
  }
}

void CostMap2D::updateCell(unsigned int cell, unsigned int cellState, std::vector<unsigned int>& newObstacles){
  // If it is new, append
  if(obsWatchDog_[cell] == 0 && staticData_[cell] != threshold_){
    newObstacles.push_back(cell);
    dynamicObstacles_.push_back(cell);
  }

  // If the new value is a raw obstacle, set it as such, no matter how current
  if(cellState == threshold_)
    fullData_[cell] = threshold_;

  // Set to inflation if not a raw obstacle by other means: cannot over-write a statically inflated obstacle 
  // and it cannot over-write a raw obstacle that is up to date
  if(obsWatchDog_[cell] < WATCHDOG_LIMIT && staticData_[cell] != threshold_)
      fullData_[cell] = cellState;
  
  // Always pet the watchdog
  obsWatchDog_[cell] = WATCHDOG_LIMIT;
}

void CostMap2D::updateDynamicObstacles(double ts, const std_msgs::PointCloudFloat32& cloud){
  std::vector<unsigned int> insertions, deletions;
  updateDynamicObstacles(ts, cloud, insertions, deletions);
}

/**
 * This algorithm uses the concept of a watchdog timer to decide when an obstacle can be removed.
 */
void CostMap2D::removeStaleObstacles(double ts, std::vector<unsigned int>& deletedObstacles){
  deletedObstacles.clear();

  // Calculate elapsed time in terms of ticks
  TICK elapsedTime = getElapsedTime(ts);

  // Iterate over the set of dyanmic obstacles
  std::list<unsigned int>::iterator it = dynamicObstacles_.begin();

  while(it != dynamicObstacles_.end()){
    unsigned int ind = *it;
    TICK timeLeft = obsWatchDog_[ind];

    // Case 1: The watchdog has just been reset. Here we decrement the watchdog by 1 and move on
    if(timeLeft == WATCHDOG_LIMIT){
      obsWatchDog_[ind]--;
      ++it;
      continue;
    }

    // Case 2: The watchdog has timed out. Here we remove the obstacle (if not static), zero the watchdog timer, and move on.
    if(timeLeft <= elapsedTime){
      it = dynamicObstacles_.erase(it);
      obsWatchDog_[ind] = 0;
      fullData_[ind] = staticData_[ind];
      deletedObstacles.push_back(ind);
      continue;
    }

    // Otherwise: Just have to decrement the watchdog and move on
    obsWatchDog_[ind] = timeLeft - elapsedTime;
    ++it;
  }
}

const unsigned char* CostMap2D::getMap() const {
  return fullData_;
}

void CostMap2D::getOccupiedCellDataIndexList(std::vector<unsigned int>& results) const {
  results.clear();
  unsigned int maxCellCount = getWidth() * getHeight();
  for(unsigned int i = 0; i < maxCellCount; i++){
    if(staticData_[i] == threshold_ || staticData_[i] == INFLATED_OBSTACLE || obsWatchDog_[i] > 0)
      results.push_back(i);
  }
}


void CostMap2D::IND_MC(unsigned int ind,
		       unsigned int& mx,
		       unsigned int& my) const {
  //depending on implicit casting to get floors
  my = ind/width_;
  mx = ind-(my*width_);

  //std::cout << "Going from " << ind << " to " << x << " " << y << std::endl;
}

void CostMap2D::IND_WC(unsigned int ind, double& wx, double& wy) const{
  unsigned int mx, my;
  IND_MC(ind, mx, my);
  MC_WC(mx, my, wx, wy);
}

unsigned int CostMap2D::MC_IND(unsigned int x, unsigned int y) const 
{
  return(x+y*width_);
}

void CostMap2D::MC_WC(unsigned int mx, unsigned int my,
		      double& wx, double& wy) const {
  wx = (mx*1.0)*resolution_;
  wy = (my*1.0)*resolution_;
}

unsigned int CostMap2D::WC_IND(double wx, double wy) const {
  unsigned int mx, my;
  WC_MC(wx, wy, mx, my);
  return MC_IND(mx, my);
}

void CostMap2D::WC_MC(double wx, double wy,
		      unsigned int& mx, unsigned int& my) const {
  
  if(wx < 0 || wy < 0) {
    //std::cerr << "CostMap2D::WC_MC problem with " << wx << " or " << wy << std::endl;
    mx = 0;
    my = 0;
    return;
  }

  //not much for now
  mx = (int) (wx/resolution_);
  my = (int) (wy/resolution_);

  if(mx > width_) {
    std::cout << "CostMap2D::WC_MC converted x " << wx << " greater than width " << width_ << std::endl;
    mx = 0;
    return;
  } 
  if(my > height_) {
    std::cout << "CostMap2D::WC_MC converted y " << wy << " greater than height " << height_ << std::endl;
    my = 0;
    return;
  }
}

unsigned int CostMap2D::getWidth() const {
  return width_;
}

unsigned int CostMap2D::getHeight() const {
  return height_;
}

double CostMap2D::getResolution() const {
  return resolution_;
}

TICK CostMap2D::getElapsedTime(double ts) {
  //ROS_ASSERT(ts >= lastTimeStamp_);

  double count = (ts - lastTimeStamp_) / tickLength_;

  TICK result = (count > WATCHDOG_LIMIT ? WATCHDOG_LIMIT : (TICK) count);

  lastTimeStamp_ = ts;

  return result;
}

bool CostMap2D::isObstacle(unsigned int ind) const {
  return(fullData_[ind] == threshold_);
}

bool CostMap2D::isInflatedObstacle(unsigned int ind) const {
  return(fullData_[ind] == INFLATED_OBSTACLE);
}

unsigned char CostMap2D::operator [](unsigned int ind) const{
  // ROS_ASSERT on index
  return fullData_[ind];
}

/**
 * @brief Returns the set of cell id's for cells that should be considered obstacles given
 * the inflation radius and the map resolution.
 *
 * @param ind The index of the new obstacle
 * @param inflation The resulting inflation set
 * @todo Consider using euclidean distance check for inflation, by testng if the origin of a cell
 * is within the inflation radius distance of the origitn of the given cell.
 */
void CostMap2D::computeInflation(unsigned int ind, std::vector<unsigned int>& inflation) const {
  unsigned int mx, my;
  IND_MC(ind, mx, my);

  unsigned int cellRadius = static_cast<unsigned int>(inflationRadius_/resolution_);
  unsigned int x_min = static_cast<unsigned int>(std::max<int>(0, mx - cellRadius));
  unsigned int x_max = static_cast<unsigned int>(std::min<int>(getWidth() - 1, mx + cellRadius));
  unsigned int y_min = static_cast<unsigned int>(std::max<int>(0, my - cellRadius));
  unsigned int y_max = static_cast<unsigned int>(std::min<int>(getHeight() - 1, my + cellRadius));

  inflation.clear();
  for(unsigned int i = x_min; i <= x_max; i++)
    for(unsigned int j = y_min; j <= y_max; j++)
      inflation.push_back(MC_IND(i, j));
}

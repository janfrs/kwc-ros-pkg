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
 * Author: E. Gil Jones
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

CostMap2D::CostMap2D(double window_length) :
window_length_(window_length)
{  
  static_data_ = NULL;
  full_data_ = NULL;
  obs_count_ = NULL;
  width_ = 0;
  height_ = 0;
  resolution_ = 0;
  total_obs_points_ = 0;
}

CostMap2D::~CostMap2D() {
  std::list<ObstaclePts*>::iterator oit = obstacle_pts_.begin();
  while(oit != obstacle_pts_.end()) {
    delete (*oit);
    oit++;
  }
  obstacle_pts_.clear();
  
  if(static_data_ != NULL) delete[] static_data_;
  if(full_data_ != NULL) delete[] full_data_;
  if(obs_count_ != NULL) delete[] obs_count_;
}

void CostMap2D::setStaticMap(size_t width, size_t height,
			     double resolution, const unsigned char* data)
{
  width_ = width;
  height_ = height;
  resolution_ = resolution;

  //TODO handle resize?
  static_data_ = new unsigned char[width_*height_];
  full_data_ = new unsigned char[width_*height_];
  obs_count_ = new unsigned int[width_*height_];
  //if(size(data) < (width_*height_)) {
  //  std::cerr << "CostMap2D::setStaticMap Error - data has " << sizeof(data) << " bytes instead of the required " << width_*height_ << std::endl;
  //  return;
  //}
  memcpy(static_data_, data, width_*height_);
  memcpy(full_data_, static_data_, width_*height_);
  memset(obs_count_, 0, width_*height_);
}

void CostMap2D::addObstacles(const std_msgs::PointCloudFloat32* cloud,
                             std::list<unsigned int>& occ_ids,
                             std::list<unsigned int>& unocc_ids) 
{
  //assume that these points are in the map frame
  
  if(full_data_ == NULL) {
    std::cerr << "CostMap2D::addObstacles warning - addObstacles called before static map initialization.\n";
    return;
  }

  //no points to add
  if(cloud->get_pts_size() == 0) return;

  //update cur time from scan as long as it advances us
  if(cloud->header.stamp > cur_time_) {
    cur_time_ = cloud->header.stamp;
  } else {
    //sanity check that scan isn't already old compared to cur_time_
    if(!isTimeWithinWindow(cloud->header.stamp)) return;
  }

  ObstaclePts* obs = new ObstaclePts();
  obs->setSize(cloud->get_pts_size());
  obs->ts_ = cloud->header.stamp;

  for(size_t i = 0; i < cloud->get_pts_size(); i++) {
    obs->setPoint(i, cloud->pts[i].x, cloud->pts[i].y);
  }
  
  addObstaclePointsToFullMap(obs, occ_ids);
  obstacle_pts_.push_back(obs);  

  refreshFullMap(unocc_ids);
}

void CostMap2D::update(ros::Time ts, std::list<unsigned int>& unocc_ids) {
  if(ts > cur_time_ || ts == cur_time_) {
    cur_time_ = ts;
  } else {
    std::cerr << "CostMap2D::update warning - this map has data more recent than the update time " << ts << std::endl;
  }
  refreshFullMap(unocc_ids);
}

const unsigned char* CostMap2D::getMap() const {
  return full_data_;
}

std::list<unsigned int> CostMap2D::getOccupiedCellDataIndexList() const {
  std::list<unsigned int> ret_ind;

  for(std::map<unsigned int, bool>::const_iterator it = obstacle_index_map_.begin();
      it != obstacle_index_map_.end();
      it++) {
    ret_ind.push_back(it->first);
  }
  return ret_ind;
}

void CostMap2D::refreshFullMap(std::list<unsigned int>& unocc_ids) {
  //this encodes a system whereby if a cell is an obstacle in
  //any scan we treat is as an obstacle, only reverting to the static map if all
  //scans think it's free
  //std::cout << "Cur time " << cur_time_ << std::endl;

  std::list<ObstaclePts*>::iterator oit = obstacle_pts_.begin();
  while(oit != obstacle_pts_.end()) {
    if(!isTimeWithinWindow((*oit)->ts_)) 
    {
      //std::cout << "Ditching obstacle time " << (*oit)->ts_ << std::endl;
      for(size_t i = 0; i < (*oit)->pts_num_; i++) 
      {
        size_t mx, my;
        convertFromWorldCoordToIndexes((*oit)->pts_[i*2],(*oit)->pts_[i*2+1], mx, my);
        size_t ind = getMapIndex(mx, my);
        //full map reverts to static map
        if(obs_count_[ind] == 0) {
          std::cout << "CostMap2D::refreshFullMap problem - obs_count for cell ind " << ind << " is zero and we're trying to decrement.\n";
        } else {
          obs_count_[ind]--;
        }
        //this means that no scans think this is an obstacle, and we can get rid of it.
        if(obs_count_[ind] == 0) {
          full_data_[ind] = static_data_[ind];
          //erasing entry in obstacle map
          obstacle_index_map_.erase(ind); 
          unocc_ids.push_back(ind);
          if(total_obs_points_ == 0) {
            std::cout << "CostMap2D::refreshFullMap problem - total_obs_points_ is zero and we're trying to decrement.\n";
          } else {
            total_obs_points_--;
          }
        }
      }
      //can get rid of this
      delete (*oit);
      oit = obstacle_pts_.erase(oit);
    } else {
      //std::cout << "Obstacle time " << (*oit)->ts_ << std::endl;
      oit++;
    }
  }
}

void CostMap2D::addObstaclePointsToFullMap(const ObstaclePts* pts, std::list<unsigned int>& occ_ids) {
  //sanity check
  if(!isTimeWithinWindow(pts->ts_)) {
    std::cerr << " CostMap2D::addObstaclePointsToFullMap won't add old data to map.\n";
    return;
  }
  for(size_t i = 0; i < pts->pts_num_; i++) {
    size_t mx, my;
    convertFromWorldCoordToIndexes(pts->pts_[i*2],pts->pts_[i*2+1], mx, my);
    size_t ind = getMapIndex(mx, my);
    if(full_data_[ind] != 75) {
      //this is a new obstacle cell being set
      total_obs_points_++;
      occ_ids.push_back(ind);
    }
    full_data_[ind] = 75;
    obs_count_[ind]++;
    obstacle_index_map_[ind] = true;
  }
}

bool CostMap2D::isTimeWithinWindow(ros::Time ts) const {
  if(ts+window_length_ > cur_time_ || ts+window_length_ == cur_time_) {
    return true;
  } 
  return false;
}

size_t CostMap2D::getMapIndex(unsigned int x, unsigned int y) const 
{
  return(x+y*width_);
}

void CostMap2D::convertFromWorldCoordToIndexes(double wx, double wy,
					     size_t& mx, size_t& my) const {
  
  if(wx < 0 || wy < 0) {
    //std::cerr << "CostMap2D::convertFromWorldCoordToIndex problem with " << wx << " or " << wy << std::endl;
    mx = 0;
    my = 0;
    return;
  }

  //not much for now
  mx = (int) (wx/resolution_);
  my = (int) (wy/resolution_);

  if(mx > width_) {
    //std::cerr << "CostMap2D::convertFromWorldCoordToIndex converted x " << wx << " greater than width " << width_ << std::endl;
    mx = 0;
    return;
  } 
  if(my > height_) {
    //std::cerr << "CostMap2D::convertFromWorldCoordToIndex converted y " << wy << " greater than height " << height_ << std::endl;
    my = 0;
    return;
  }
}

void CostMap2D::convertFromIndexesToWorldCoord(size_t mx, size_t my,
                                               double& wx, double& wy) const {
  wx = (mx*1.0)*resolution_;
  wy = (my*1.0)*resolution_;
}

void CostMap2D::convertFromMapIndexToXY(unsigned int ind,
                                        unsigned int& x,
                                        unsigned int& y) const {
  //depending on implicit casting to get floors
  y = ind/width_;
  x = ind-(y*width_);

  //std::cout << "Going from " << ind << " to " << x << " " << y << std::endl;
}

size_t CostMap2D::getWidth() const {
  return width_;
}

size_t CostMap2D::getHeight() const {
  return height_;
}

unsigned int CostMap2D::getTotalObsPoints() const {
  return total_obs_points_;
}

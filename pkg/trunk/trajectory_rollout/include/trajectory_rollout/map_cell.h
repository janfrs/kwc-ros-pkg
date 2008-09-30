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
#ifndef MAP_CELL_H_
#define MAP_CELL_H_

#include <trajectory_rollout/trajectory_inc.h>

//Information contained in each map cell
class MapCell{
  public:
    //default constructor
    MapCell();

    MapCell(const MapCell& mc);

    //cell index in the grid map
    unsigned int cx, cy;

    //distance to planner's path
    double path_dist;

    //distance to goal
    double goal_dist;

    //grown obstacles
    double occ_dist;

    //occupancy state (-1 = free, 0 = unknown, 1 = occupied)
    int occ_state;

    bool path_mark, goal_mark;

    //compares two cells based on path_dist so we can use stl priority queues
    const bool operator< (const MapCell& mc) const;
};

struct ComparePathDist {
  bool operator()(const MapCell* cell1, const MapCell* cell2) const {
    return cell1->path_dist > cell2->path_dist;
  }
};

struct CompareGoalDist {
  bool operator()(const MapCell* cell1, const MapCell* cell2) const {
    return cell1->goal_dist > cell2->goal_dist;
  }
};
#endif

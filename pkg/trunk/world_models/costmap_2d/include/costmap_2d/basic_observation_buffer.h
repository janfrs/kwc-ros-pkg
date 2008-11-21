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
 * Authors: Conor McGann
 */

#ifndef COSTMAP_2D_BASIC_OBSERVATION_BUFFER_H
#define COSTMAP_2D_BASIC_OBSERVATION_BUFFER_H

#include <costmap_2d/observation_buffer.h>

namespace costmap_2d {

  /**
   * @brief Extend base class to handle buffering until a transform is available, and to support locking for mult-threaded
   * access
   */
  class BasicObservationBuffer: public ObservationBuffer {
  public:
    BasicObservationBuffer(const std::string& frame_id, rosTFClient& tf, ros::Duration keepAlive, double robotRadius, double minZ, double maxZ);

    virtual void buffer_cloud(const std_msgs::PointCloud& local_cloud);

    virtual void get_observations(std::vector<Observation>& observations);

  private:

    /**
     * @brief Test if point in the square footprint of the robot
     */
    bool inFootprint(double x, double y) const;

    /**
     * @brief Provide a filtered set of points based on the extraction of the robot footprint and the 
     * filter based on the min and max z values
     */
    std_msgs::PointCloud * extractFootprintAndGround(const std_msgs::PointCloud& baseFrameCloud) const;

    const std::string frame_id_;
    rosTFClient& tf_;
    std::deque<std_msgs::PointCloud> point_clouds_; /**< Buffer point clouds until a transform is available */
    ros::thread::mutex buffer_mutex_;
    const double robotRadius_, minZ_, maxZ_; /**< Constraints for filtering points */
  };
}
#endif

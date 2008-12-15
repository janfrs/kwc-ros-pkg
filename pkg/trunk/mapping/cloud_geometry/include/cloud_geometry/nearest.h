/*
 * Copyright (c) 2008 Radu Bogdan Rusu <rusu -=- cs.tum.edu>
 *
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
 * $Id$
 *
 */

/** \author Radu Bogdan Rusu */

#ifndef _CLOUD_GEOMETRY_NEAREST_H_
#define _CLOUD_GEOMETRY_NEAREST_H_

#include "std_msgs/PointCloud.h"
#include "std_msgs/Point32.h"

#include "Eigen/Core"
#include "cloud_geometry/lapack.h"

namespace cloud_geometry
{

  namespace nearest
  {

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute the centroid of a set of points and return it as a Point32 message.
      * \param points the input point cloud
      * \param centroid the output centroid
      */
    inline void
      computeCentroid (std_msgs::PointCloud points, std_msgs::Point32 &centroid)
    {
      // For each point in the cloud
      for (unsigned int i = 0; i < points.get_pts_size (); i++)
      {
        centroid.x += points.pts.at (i).x;
        centroid.y += points.pts.at (i).y;
        centroid.z += points.pts.at (i).z;
      }

      centroid.x /= points.get_pts_size ();
      centroid.y /= points.get_pts_size ();
      centroid.z /= points.get_pts_size ();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute the centroid of a set of points using their indices and return it as a Point32 message.
      * \param points the input point cloud
      * \param indices the point cloud indices that need to be used
      * \param centroid the output centroid
      */
    inline void
      computeCentroid (std_msgs::PointCloud points, std::vector<int> indices, std_msgs::Point32 &centroid)
    {
      // For each point in the cloud
      for (unsigned int i = 0; i < indices.size (); i++)
      {
        centroid.x += points.pts.at (indices.at (i)).x;
        centroid.y += points.pts.at (indices.at (i)).y;
        centroid.z += points.pts.at (indices.at (i)).z;
      }

      centroid.x /= indices.size ();
      centroid.y /= indices.size ();
      centroid.z /= indices.size ();
    }

    void computeCentroid (std_msgs::PointCloud points, std_msgs::PointCloud &centroid);
    void computeCentroid (std_msgs::PointCloud points, std::vector<int> indices, std_msgs::PointCloud &centroid);

    void computeCovarianceMatrix (std_msgs::PointCloud points, Eigen::Matrix3d &covariance_matrix);
    void computeCovarianceMatrix (std_msgs::PointCloud points, Eigen::Matrix3d &covariance_matrix, std_msgs::Point32 &centroid);
    void computeCovarianceMatrix (std_msgs::PointCloud points, std::vector<int> indices, Eigen::Matrix3d &covariance_matrix);
    void computeCovarianceMatrix (std_msgs::PointCloud points, std::vector<int> indices, Eigen::Matrix3d &covariance_matrix, std_msgs::Point32 &centroid);

    void computeSurfaceNormalCurvature (std_msgs::PointCloud points, Eigen::Vector4d &plane_parameters, double &curvature);
    void computeSurfaceNormalCurvature (std_msgs::PointCloud points, std::vector<int> indices, Eigen::Vector4d &plane_parameters, double &curvature);

  }
}

#endif

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
 */

#ifndef OGRE_VISUALIZER_POINT_CLOUD_VISUALIZER_H
#define OGRE_VISUALIZER_POINT_CLOUD_VISUALIZER_H

#include "../visualizer_base.h"
#include "ogre_tools/point_cloud.h"

#include "std_msgs/PointCloudFloat32.h"

namespace ros
{
  class node;
}

class rosTFClient;

namespace ogre_vis
{

/**
 * \class PointCloudVisualizer
 * \brief Displays a point cloud of type std_msgs::PointCloudFloat32
 *
 */
class PointCloudVisualizer : public VisualizerBase
{
public:
  /**
   * \enum Style
   * \brief The different styles of pointcloud drawing
   */
  enum Style
  {
    Points,    ///< Points -- points are drawn as a fixed size in 2d space, ie. always 1 pixel on screen
    Billboards,///< Billboards -- points are drawn as camera-facing quads in 3d space
  };

  PointCloudVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name, bool enabled );
  ~PointCloudVisualizer();

  /**
   * Set the incoming PointCloudFloat32 topic
   * @param topic The topic we should listen to
   */
  void setTopic( const std::string& topic );
  /**
   * Set the primary color of this point cloud.  This color is used verbatim for the highest intensity points, and interpolates
   * down to black for the lowest intensity points
   * @param r Red component, in the range [0,1]
   * @param g Green component, in the range [0,1]
   * @param b Blue component, in the range [0,1]
   */
  void setColor( float r, float g, float b );
  /**
   * \brief Set the rendering style
   * @param style The rendering style
   */
  void setStyle( Style style );
  /**
   * \brief Sets the size each point will be when drawn in 3D as a billboard
   * @note Only applicable if the style is set to Billboards (default)
   * @param size The size
   */
  void setBillboardSize( float size );

  // Overrides from VisualizerBase
  virtual void fillPropertyGrid( wxPropertyGrid* property_grid );
  virtual void propertyChanged( wxPropertyGridEvent& event );

protected:
  virtual void onEnable();
  virtual void onDisable();

  /**
   * \brief Subscribes to the topic set by setTopic()
   */
  void subscribe();
  /**
   * \brief Unsubscribes from the current topic
   */
  void unsubscribe();

  /**
   * \brief ROS callback for an incoming point cloud message
   */
  void incomingCloudCallback();



  ogre_tools::PointCloud* cloud_;             ///< Handles actually drawing the point cloud

  std::string topic_;                         ///< The PointCloudFloat32 topic set by setTopic()
  std_msgs::PointCloudFloat32 message_;       ///< Our point cloud message

  float r_;                                   ///< Red component of our color.  Range [0,1]
  float g_;                                   ///< Green component of our color.  Range [0,1]
  float b_;                                   ///< Blue component of our color.  Range [0,1]

  Style style_;                               ///< Our rendering style
  float billboard_size_;                      ///< Size to draw our billboards
};

} // namespace ogre_vis

#endif

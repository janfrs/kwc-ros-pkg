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

#ifndef OGRE_VISUALIZER_LASER_SCAN_DISPLAY_H
#define OGRE_VISUALIZER_LASER_SCAN_DISPLAY_H

#include "display.h"
#include "ogre_tools/point_cloud.h"
#include "helpers/color.h"

#include "std_msgs/LaserScan.h"
#include "std_msgs/PointCloud.h"
#include "std_msgs/Empty.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <deque>

namespace ros
{
  class node;
}

namespace tf
{
template<class Message> class MessageNotifier;
}

namespace ogre_vis
{

class IntProperty;
class FloatProperty;
class StringProperty;
class ROSTopicStringProperty;
class ColorProperty;
class EnumProperty;

/**
 * \class LaserScanDisplay
 * \brief Visualizes a laser scan, received either as a std_msgs::LaserScan or std_msgs::PointCloud
 *
 * \todo find out some way to share most of this code with PointCloudDisplay
 */
class LaserScanDisplay : public Display
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

    StyleCount,
  };

  LaserScanDisplay( const std::string& name, VisualizationManager* manager );
  ~LaserScanDisplay();

  /**
   * \brief Set the PointCloud topic we should listen on
   * @param topic The topic
   */
  void setCloudTopic( const std::string& topic );
  /**
   * \brief Set the LaserScan topic we should listen on
   * @param topic The topic
   */
  void setScanTopic( const std::string& topic );

  /**
   * Set the primary color of this point cloud.  This color is used verbatim for the highest intensity points, and interpolates
   * down to black for the lowest intensity points
   */
  void setColor( const Color& color );
  /**
   * \brief Set the amount of time each scan should stick around for
   * @param time Decay time, in seconds
   */
  void setDecayTime( float time );
  /**
   * \brief Set the rendering style
   * @param style The rendering style
   */
  void setStyle( int style );
  /**
   * \brief Sets the size each point will be when drawn in 3D as a billboard
   * @note Only applicable if the style is set to Billboards (default)
   * @param size The size
   */
  void setBillboardSize( float size );

  const std::string& getScanTopic() { return scan_topic_; }
  const std::string& getCloudTopic() { return cloud_topic_; }
  float getBillboardSize() { return billboard_size_; }
  float getDecayTime() { return point_decay_time_; }
  const Color& getColor() { return color_; }
  int getStyle() { return style_; }

  virtual void update( float dt );

  // Overrides from Display
  virtual void targetFrameChanged();
  virtual void fixedFrameChanged();
  virtual void createProperties();
  virtual void reset();

  static const char* getTypeStatic() { return "Laser Scan"; }
  virtual const char* getType() { return getTypeStatic(); }
  static const char* getDescription();

protected:
  virtual void onEnable();
  virtual void onDisable();

  void clear();

  /**
   * \brief Subscribes to the cloud and scan topics if they have been set
   */
  void subscribe();
  /**
   * \brief Unsubscribes from the cloud and scan topics if they have been set
   */
  void unsubscribe();

  /**
   * \brief Transforms a point cloud into the correct frame, adds it to our point list
   */
  void transformCloud( const std_msgs::PointCloud& message );
  /**
   * \brief Culls points that have been around for longer than the decay time
   */
  void cullPoints();

  /**
   * \brief Callback for incoming PointCloud messages
   */
  void incomingCloudCallback(const boost::shared_ptr<std_msgs::PointCloud>& cloud);
  /**
   * \brief Callback for incoming LaserScan messages
   */
  void incomingScanCallback(const boost::shared_ptr<std_msgs::LaserScan>& scan);

  void updateCloud();

  ogre_tools::PointCloud* cloud_;                 ///< Handles actually rendering the point cloud

  std::string cloud_topic_;                       ///< The PointCloud topic we're listening on
  std::string scan_topic_;                        ///< The LaserScan topic we're listening on

  Color color_;

  float intensity_min_;                           ///< Running min of intensity values, used for normalization
  float intensity_max_;                           ///< Running max of intensity values, used for normalization

  typedef std::vector< ogre_tools::PointCloud::Point > V_Point;
  typedef std::deque< V_Point > DV_Point;
  DV_Point points_;                               ///< The points we're displaying, split per message

  typedef std::deque< float > D_float;
  D_float point_times_;                           ///< A running time of how long each scan's points have been around
  float point_decay_time_;                        ///< How long scans should stick around for before they are culled

  boost::mutex points_mutex_;

  int style_;                                     ///< Our rendering style
  float billboard_size_;                          ///< Size to draw our billboards

  ROSTopicStringProperty* scan_topic_property_;
  ROSTopicStringProperty* cloud_topic_property_;
  FloatProperty* billboard_size_property_;
  FloatProperty* decay_time_property_;
  ColorProperty* color_property_;
  EnumProperty* style_property_;

  tf::MessageNotifier<std_msgs::PointCloud>* cloud_notifier_;
  tf::MessageNotifier<std_msgs::LaserScan>* scan_notifier_;
};

} // namespace ogre_vis

#endif

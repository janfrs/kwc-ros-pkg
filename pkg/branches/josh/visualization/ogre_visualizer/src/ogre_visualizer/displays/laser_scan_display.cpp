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

#include "laser_scan_display.h"
#include "properties/property.h"
#include "properties/property_manager.h"
#include "common.h"
#include "ros_topic_property.h"

#include "ros/node.h"
#include "ogre_tools/point_cloud.h"

#include <tf/transform_listener.h>
#include <std_msgs/PointCloud.h>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

namespace ogre_vis
{

LaserScanDisplay::LaserScanDisplay( const std::string& name, VisualizationManager* manager )
: Display( name, manager )
, color_( 1.0f, 0.0f, 0.0f )
, intensity_min_( 999999.0f )
, intensity_max_( -999999.0f )
, point_decay_time_( 20.0f )
, style_( Billboards )
, billboard_size_( 0.01 )
, scan_topic_property_( NULL )
, cloud_topic_property_( NULL )
, billboard_size_property_( NULL )
, decay_time_property_( NULL )
, color_property_( NULL )
, style_property_( NULL )
{
  cloud_ = new ogre_tools::PointCloud( scene_manager_ );

  setStyle( style_ );
  setBillboardSize( billboard_size_ );
}

LaserScanDisplay::~LaserScanDisplay()
{
  unsubscribe();

  delete cloud_;
}

void LaserScanDisplay::setCloudTopic( const std::string& topic )
{
  unsubscribe();

  cloud_topic_ = topic;

  subscribe();

  if ( cloud_topic_property_ )
  {
    cloud_topic_property_->changed();
  }

  causeRender();
}

void LaserScanDisplay::setScanTopic( const std::string& topic )
{
  unsubscribe();

  scan_topic_ = topic;

  subscribe();

  if ( scan_topic_property_ )
  {
    scan_topic_property_->changed();
  }

  causeRender();
}

void LaserScanDisplay::setColor( const Color& color )
{
  color_ = color;

  if ( color_property_ )
  {
    color_property_->changed();
  }

  causeRender();
}

void LaserScanDisplay::setStyle( int style )
{
  {
    RenderAutoLock render_lock( this );

    ROS_ASSERT(style < StyleCount);

    style_ = style;
    cloud_->setUsePoints( style == Points );
  }

  causeRender();

  if ( style_property_ )
  {
    style_property_->changed();
  }

  causeRender();
}

void LaserScanDisplay::setBillboardSize( float size )
{
  {
    RenderAutoLock render_lock( this );

    billboard_size_ = size;
    cloud_->setBillboardDimensions( size, size );
  }

  causeRender();

  if ( billboard_size_property_ )
  {
    billboard_size_property_->changed();
  }

  causeRender();
}

void LaserScanDisplay::setDecayTime( float time )
{
  point_decay_time_ = time;

  if ( decay_time_property_ )
  {
    decay_time_property_->changed();
  }

  causeRender();
}

void LaserScanDisplay::clear()
{
  RenderAutoLock renderLock( this );

  cloud_->clear();
  points_.clear();
  point_times_.clear();
  cloud_messages_.clear();

  intensity_min_ = 9999999.0f;
  intensity_max_ = -9999999.0f;
}

void LaserScanDisplay::onEnable()
{
  cloud_->setCloudVisible( true );
  subscribe();
}

void LaserScanDisplay::onDisable()
{
  unsubscribe();

  clear();

  cloud_->setCloudVisible( false );
}

void LaserScanDisplay::subscribe()
{
  if ( !isEnabled() )
  {
    return;
  }

  if ( !cloud_topic_.empty() )
  {
    ros_node_->subscribe( cloud_topic_, cloud_message_, &LaserScanDisplay::incomingCloudCallback, this, 1 );
  }

  if ( !scan_topic_.empty() )
  {
    ros_node_->subscribe( scan_topic_, scan_message_, &LaserScanDisplay::incomingScanCallback, this, 1 );
  }
}

void LaserScanDisplay::unsubscribe()
{
  if ( !cloud_topic_.empty() )
  {
    ros_node_->unsubscribe( cloud_topic_, &LaserScanDisplay::incomingCloudCallback, this );
  }

  if ( !scan_topic_.empty() )
  {
    ros_node_->unsubscribe( scan_topic_, &LaserScanDisplay::incomingScanCallback, this );
  }
}

void LaserScanDisplay::update( float dt )
{
  cloud_message_.lock();

  D_float::iterator it = point_times_.begin();
  D_float::iterator end = point_times_.end();
  for ( ; it != end; ++it )
  {
    *it += dt;
  }

  cullPoints();

  cloud_message_.unlock();
}

void LaserScanDisplay::cullPoints()
{
  if ( point_decay_time_ == 0.0f )
  {
    return;
  }

  bool removed = false;
  while ( !point_times_.empty() && point_times_.front() > point_decay_time_ )
  {
    point_times_.pop_front();
    points_.pop_front();
    cloud_messages_.pop_front();

    removed = true;
  }

  if ( removed )
  {
    updateCloud();
  }
}

void LaserScanDisplay::transformCloud( std_msgs::PointCloud& message )
{
  if ( point_decay_time_ == 0.0f )
  {
    points_.clear();
    point_times_.clear();
    cloud_messages_.clear();
  }

  // Push back before transforming.  This will perform a full copy.
  cloud_messages_.push_back( message );

  if ( message.header.frame_id.empty() )
  {
    message.header.frame_id = fixed_frame_;
  }

  try
  {
    std_msgs::PointCloud* casted_message = reinterpret_cast<std_msgs::PointCloud*>(&message);
    tf_->transformPointCloud(fixed_frame_, *casted_message, *casted_message);
  }
  catch(tf::TransformException& e)
  {
    ROS_ERROR( "Error transforming laser scan '%s', frame '%s' to frame '%s'\n", name_.c_str(), message.header.frame_id.c_str(), fixed_frame_.c_str() );
  }

  uint32_t point_count_ = message.get_pts_size();
  for(uint32_t i = 0; i < point_count_; i++)
  {
    float& intensity = message.chan[0].vals[i];
    // arbitrarily cap to 4096 for now
    intensity = std::min( intensity, 4096.0f );
    intensity_min_ = std::min( intensity_min_, intensity );
    intensity_max_ = std::max( intensity_max_, intensity );
  }

  float diff_intensity = intensity_max_ - intensity_min_;

  points_.push_back( V_Point() );
  V_Point& points = points_.back();
  points.resize( point_count_ );

  point_times_.push_back( 0.0f );
  for(uint32_t i = 0; i < point_count_; i++)
  {
    Ogre::Vector3 point( message.pts[i].x, message.pts[i].y, message.pts[i].z );
    robotToOgre( point );

    float intensity = message.chan[0].vals[i];

    float normalized_intensity = (diff_intensity > 0.0f) ? ( intensity - intensity_min_ ) / diff_intensity : 1.0f;

    Ogre::Vector3 color( color_.r_, color_.g_, color_.b_ );
    color *= normalized_intensity;

    ogre_tools::PointCloud::Point& current_point = points[ i ];
    current_point.x_ = point.x;
    current_point.y_ = point.y;
    current_point.z_ = point.z;
    current_point.r_ = color.x;
    current_point.g_ = color.y;
    current_point.b_ = color.z;
  }

  updateCloud();
}

void LaserScanDisplay::updateCloud()
{
  {
    RenderAutoLock render_lock( this );

    cloud_->clear();

    if ( !points_.empty() )
    {
      DV_Point::iterator it = points_.begin();
      DV_Point::iterator end = points_.end();
      for ( ; it != end; ++it )
      {
        V_Point& points = *it;

        if ( !points.empty() )
        {
          cloud_->addPoints( &points.front(), points.size() );
        }
      }
    }
  }

  causeRender();
}

void LaserScanDisplay::incomingCloudCallback()
{
  transformCloud( cloud_message_ );
}

void LaserScanDisplay::incomingScanCallback()
{
  cloud_message_.lock();

  if ( scan_message_.header.frame_id.empty() )
  {
    scan_message_.header.frame_id = fixed_frame_;
  }

  std_msgs::PointCloud* casted_message = reinterpret_cast<std_msgs::PointCloud*>(&cloud_message_);
  tf_->transformLaserScanToPointCloud( scan_message_.header.frame_id, *casted_message, scan_message_ );
  transformCloud( cloud_message_ );

  cloud_message_.unlock();
}

#if 0
void LaserScanDisplay::targetFrameChanged()
{
  cloud_message_.lock();

  D_CloudMessage messages;
  messages.swap( cloud_messages_ );
  points_.clear();
  point_times_.clear();
  cloud_messages_.clear();

  D_CloudMessage::iterator it = messages.begin();
  D_CloudMessage::iterator end = messages.end();
  for ( ; it != end; ++it )
  {
    transformCloud( *it );
  }

  cloud_message_.unlock();
}
#endif

void LaserScanDisplay::fixedFrameChanged()
{
  clear();
}

void LaserScanDisplay::createProperties()
{
  style_property_ = property_manager_->createProperty<EnumProperty>( "Style", property_prefix_, boost::bind( &LaserScanDisplay::getStyle, this ),
                                                                     boost::bind( &LaserScanDisplay::setStyle, this, _1 ), parent_category_, this );
  style_property_->addOption( "Billboards", Billboards );
  style_property_->addOption( "Points", Points );

  color_property_ = property_manager_->createProperty<ColorProperty>( "Color", property_prefix_, boost::bind( &LaserScanDisplay::getColor, this ),
                                                                        boost::bind( &LaserScanDisplay::setColor, this, _1 ), parent_category_, this );

  billboard_size_property_ = property_manager_->createProperty<FloatProperty>( "Billboard Size", property_prefix_, boost::bind( &LaserScanDisplay::getBillboardSize, this ),
                                                                                boost::bind( &LaserScanDisplay::setBillboardSize, this, _1 ), parent_category_, this );
  billboard_size_property_->setMin( 0.0001 );
  decay_time_property_ = property_manager_->createProperty<FloatProperty>( "Decay Time", property_prefix_, boost::bind( &LaserScanDisplay::getDecayTime, this ),
                                                                                  boost::bind( &LaserScanDisplay::setDecayTime, this, _1 ), parent_category_, this );
  billboard_size_property_->setMin( 0.0 );

  scan_topic_property_ = property_manager_->createProperty<ROSTopicStringProperty>( "Scan Topic", property_prefix_, boost::bind( &LaserScanDisplay::getScanTopic, this ),
                                                                            boost::bind( &LaserScanDisplay::setScanTopic, this, _1 ), parent_category_, this );
  cloud_topic_property_ = property_manager_->createProperty<ROSTopicStringProperty>( "Cloud Topic", property_prefix_, boost::bind( &LaserScanDisplay::getCloudTopic, this ),
                                                                              boost::bind( &LaserScanDisplay::setCloudTopic, this, _1 ), parent_category_, this );
}

void LaserScanDisplay::reset()
{
  clear();
}

const char* LaserScanDisplay::getDescription()
{
  return "Displays the data from either a std_msgs::PointCloud or std_msgs::LaserScan message, accumulated over a period of time.";
}

} // namespace ogre_vis

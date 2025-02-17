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

#include "laser_scan_visualizer.h"
#include "common.h"
#include "../ros_topic_property.h"

#include "ros/node.h"
#include "ogre_tools/point_cloud.h"

#include <rosTF/rosTF.h>

#include <Ogre.h>
#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/confbase.h>

#define SCAN_TOPIC_PROPERTY wxT("Scan Topic")
#define CLOUD_TOPIC_PROPERTY wxT("Cloud Topic")
#define COLOR_PROPERTY wxT("Color")
#define DECAY_TIME_PROPERTY wxT("Decay Time")
#define STYLE_PROPERTY wxT("Style")
#define BILLBOARD_SIZE_PROPERTY wxT("Billboard Size")

namespace ogre_vis
{

LaserScanVisualizer::LaserScanVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name )
: VisualizerBase( scene_manager, node, tf_client, name )
, r_( 1.0 )
, g_( 0.0 )
, b_( 0.0 )
, intensity_min_( 999999.0f )
, intensity_max_( -999999.0f )
, point_decay_time_( 20.0f )
, style_( Billboards )
, billboard_size_( 0.003 )
{
  cloud_ = new ogre_tools::PointCloud( scene_manager_ );

  setStyle( style_ );
  setBillboardSize( billboard_size_ );
}

LaserScanVisualizer::~LaserScanVisualizer()
{
  unsubscribe();

  delete cloud_;
}

void LaserScanVisualizer::setCloudTopic( const std::string& topic )
{
  unsubscribe();

  cloud_topic_ = topic;

  subscribe();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + CLOUD_TOPIC_PROPERTY ), wxString::FromAscii( cloud_topic_.c_str() ) );
  }
}

void LaserScanVisualizer::setScanTopic( const std::string& topic )
{
  unsubscribe();

  scan_topic_ = topic;

  subscribe();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + SCAN_TOPIC_PROPERTY ), wxString::FromAscii( scan_topic_.c_str() ) );
  }
}

void LaserScanVisualizer::setColor( float r, float g, float b )
{
  r_ = r;
  g_ = g;
  b_ = b;

  if ( property_grid_ )
  {
    wxVariant color;
    color << wxColour( r_ * 255, g_ * 255, b_ * 255 );
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + COLOR_PROPERTY ), color );
  }
}

void LaserScanVisualizer::setStyle( Style style )
{
  {
    RenderAutoLock render_lock( this );

    style_ = style;
    cloud_->setUsePoints( style == Points );
  }

  causeRender();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + STYLE_PROPERTY ), (long)style_ );
  }
}

void LaserScanVisualizer::setBillboardSize( float size )
{
  {
    RenderAutoLock render_lock( this );

    billboard_size_ = size;
    cloud_->setBillboardDimensions( size, size );
  }

  causeRender();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + BILLBOARD_SIZE_PROPERTY ), billboard_size_ );
  }
}

void LaserScanVisualizer::setDecayTime( float time )
{
  point_decay_time_ = time;

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + DECAY_TIME_PROPERTY ), point_decay_time_ );
  }
}

void LaserScanVisualizer::onEnable()
{
  cloud_->setVisible( true );
  subscribe();
}

void LaserScanVisualizer::onDisable()
{
  unsubscribe();

  cloud_->setVisible( false );
  cloud_->clear();
  points_.clear();
  point_times_.clear();
  cloud_messages_.clear();
}

void LaserScanVisualizer::subscribe()
{
  if ( !isEnabled() )
  {
    return;
  }

  if ( !cloud_topic_.empty() )
  {
    ros_node_->subscribe( cloud_topic_, cloud_message_, &LaserScanVisualizer::incomingCloudCallback, this, 1 );
  }

  if ( !scan_topic_.empty() )
  {
    ros_node_->subscribe( scan_topic_, scan_message_, &LaserScanVisualizer::incomingScanCallback, this, 1 );
  }
}

void LaserScanVisualizer::unsubscribe()
{
  if ( !cloud_topic_.empty() )
  {
    ros_node_->unsubscribe( cloud_topic_, &LaserScanVisualizer::incomingCloudCallback, this );
  }

  if ( !scan_topic_.empty() )
  {
    ros_node_->unsubscribe( scan_topic_, &LaserScanVisualizer::incomingScanCallback, this );
  }
}

void LaserScanVisualizer::update( float dt )
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

void LaserScanVisualizer::cullPoints()
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

void LaserScanVisualizer::transformCloud( std_msgs::PointCloudFloat32& message )
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
    message.header.frame_id = target_frame_;
  }

  try
  {
    tf_client_->transformPointCloud(target_frame_, message, message);
  }
  catch(libTF::Exception& e)
  {
    printf( "Error transforming laser scan '%s', frame '%s' to frame '%s'\n", name_.c_str(), message.header.frame_id.c_str(), target_frame_.c_str() );
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

    Ogre::Vector3 color( r_, g_, b_ );
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

void LaserScanVisualizer::updateCloud()
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

void LaserScanVisualizer::incomingCloudCallback()
{
  transformCloud( cloud_message_ );
}

void LaserScanVisualizer::incomingScanCallback()
{
  cloud_message_.lock();

  if ( scan_message_.header.frame_id.empty() )
  {
    scan_message_.header.frame_id = target_frame_;
  }

  laser_projection_.projectLaser( scan_message_, cloud_message_ );
  transformCloud( cloud_message_ );

  cloud_message_.unlock();
}

void LaserScanVisualizer::targetFrameChanged()
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

void LaserScanVisualizer::fillPropertyGrid()
{
  wxArrayString style_names;
  style_names.Add( wxT("Billboards") );
  style_names.Add( wxT("Points") );
  wxArrayInt style_ids;
  style_ids.Add( Billboards );
  style_ids.Add( Points );

  property_grid_->Append( new wxEnumProperty( STYLE_PROPERTY, property_prefix_ + STYLE_PROPERTY, style_names, style_ids, style_ ) );

  property_grid_->Append( new ROSTopicProperty( ros_node_, SCAN_TOPIC_PROPERTY, property_prefix_ + SCAN_TOPIC_PROPERTY, wxString::FromAscii( scan_topic_.c_str() ) ) );
  property_grid_->Append( new ROSTopicProperty( ros_node_, CLOUD_TOPIC_PROPERTY, property_prefix_ + CLOUD_TOPIC_PROPERTY, wxString::FromAscii( cloud_topic_.c_str() ) ) );
  property_grid_->Append( new wxColourProperty( COLOR_PROPERTY, property_prefix_ + COLOR_PROPERTY, wxColour( r_ * 255, g_ * 255, b_ * 255 ) ) );
  wxPGId prop = property_grid_->Append( new wxFloatProperty( DECAY_TIME_PROPERTY, property_prefix_ + DECAY_TIME_PROPERTY, point_decay_time_ ) );

  property_grid_->SetPropertyAttribute( prop, wxT("Min"), 0.0 );

  prop = property_grid_->Append( new wxFloatProperty( BILLBOARD_SIZE_PROPERTY, property_prefix_ + BILLBOARD_SIZE_PROPERTY, billboard_size_ ) );
  property_grid_->SetPropertyAttribute( prop, wxT("Min"), 0.0 );
}

void LaserScanVisualizer::propertyChanged( wxPropertyGridEvent& event )
{
  wxPGProperty* property = event.GetProperty();

  const wxString& name = property->GetName();
  wxVariant value = property->GetValue();

  if ( name == property_prefix_ + SCAN_TOPIC_PROPERTY )
  {
    wxString topic = value.GetString();
    setScanTopic( std::string(topic.fn_str()) );
  }
  else if ( name == property_prefix_ + CLOUD_TOPIC_PROPERTY )
  {
    wxString topic = value.GetString();
    setCloudTopic( std::string(topic.fn_str()) );
  }
  else if ( name == property_prefix_ + COLOR_PROPERTY )
  {
    wxColour color;
    color << value;

    setColor( color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f );
  }
  else if ( name == property_prefix_ + DECAY_TIME_PROPERTY )
  {
    float val = value.GetDouble();
    setDecayTime( val );
  }
  else if ( name == property_prefix_ + STYLE_PROPERTY )
  {
    int val = value.GetLong();
    setStyle( (Style)val );
  }
  else if ( name == property_prefix_ + BILLBOARD_SIZE_PROPERTY )
  {
    float val = value.GetDouble();
    setBillboardSize( val );
  }
}

void LaserScanVisualizer::loadProperties( wxConfigBase* config )
{
  wxString scan_topic, cloud_topic;
  double r, g, b;
  double decay_time;
  long style;
  double billboard_size;

  {
    config->Read( SCAN_TOPIC_PROPERTY, &scan_topic, wxString::FromAscii( scan_topic_.c_str() ) );
    config->Read( CLOUD_TOPIC_PROPERTY, &cloud_topic, wxString::FromAscii( cloud_topic_.c_str() ) );
  }

  {
    config->Read( wxString(COLOR_PROPERTY) + wxT("R"), &r, r_ );
    config->Read( wxString(COLOR_PROPERTY) + wxT("G"), &g, g_ );
    config->Read( wxString(COLOR_PROPERTY) + wxT("B"), &b, b_ );
  }

  {
    config->Read( DECAY_TIME_PROPERTY, &decay_time, point_decay_time_ );
  }

  {
    config->Read( STYLE_PROPERTY, &style, style_ );
  }

  {
    config->Read( BILLBOARD_SIZE_PROPERTY, &billboard_size, billboard_size_ );
  }

  setScanTopic( (const char*)scan_topic.fn_str() );
  setCloudTopic( (const char*)cloud_topic.fn_str() );
  setColor( r, g, b );
  setStyle( (Style)style );
  setBillboardSize( billboard_size );
}

void LaserScanVisualizer::saveProperties( wxConfigBase* config )
{
  config->Write( SCAN_TOPIC_PROPERTY, wxString::FromAscii( scan_topic_.c_str() ) );
  config->Write( CLOUD_TOPIC_PROPERTY, wxString::FromAscii( cloud_topic_.c_str() ) );

  config->Write( wxString(COLOR_PROPERTY) + wxT("R"), r_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("G"), g_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("B"), b_ );

  config->Write( DECAY_TIME_PROPERTY, point_decay_time_ );
  config->Write( STYLE_PROPERTY, style_ );
  config->Write( BILLBOARD_SIZE_PROPERTY, billboard_size_ );
}

} // namespace ogre_vis

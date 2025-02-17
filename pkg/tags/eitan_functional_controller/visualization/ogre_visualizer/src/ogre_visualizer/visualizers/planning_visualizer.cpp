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

#include "planning_visualizer.h"
#include "common.h"
#include "helpers/robot.h"

#include <ogre_tools/axes.h>

#include <urdf/URDF.h>
#include <rosTF/rosTF.h>
#include <planning_models/kinematic.h>

#include <Ogre.h>
#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/confbase.h>

#define VISUAL_ENABLED_PROPERTY wxT("Show Visual")
#define COLLISION_ENABLED_PROPERTY wxT("Show Collision")
#define STATE_DISPLAY_TIME_PROPERTY wxT("State Display Time")
#define ROBOT_DESCRIPTION_PROPERTY wxT("Robot Description")
#define TOPIC_PROPERTY wxT("Topic")

namespace ogre_vis
{

PlanningVisualizer::PlanningVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name )
: VisualizerBase( scene_manager, node, tf_client, name )
, kinematic_model_( NULL )
, new_kinematic_path_( false )
, animating_path_( false )
, state_display_time_( 0.05f )
{
  robot_ = new Robot( scene_manager );

  setVisualVisible( false );
  setCollisionVisible( true );
  robot_->setUserData( Ogre::Any( (void*)this ) );
}

PlanningVisualizer::~PlanningVisualizer()
{
  unsubscribe();

  delete robot_;
}

void PlanningVisualizer::initialize( const std::string& description_param, const std::string& kinematic_path_topic )
{
  setRobotDescription( description_param );
  setTopic( kinematic_path_topic );
}

void PlanningVisualizer::setRobotDescription( const std::string& description_param )
{
  description_param_ = description_param;

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + ROBOT_DESCRIPTION_PROPERTY ), wxString::FromAscii( description_param_.c_str() ) );
  }

  if ( isEnabled() )
  {
    load();
    causeRender();
  }
}

void PlanningVisualizer::setTopic( const std::string& topic )
{
  unsubscribe();
  kinematic_path_topic_ = topic;
  subscribe();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + TOPIC_PROPERTY ), wxString::FromAscii( kinematic_path_topic_.c_str() ) );
  }
}

void PlanningVisualizer::setStateDisplayTime( float time )
{
  state_display_time_ = time;

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + STATE_DISPLAY_TIME_PROPERTY ), state_display_time_ );
  }
}

void PlanningVisualizer::setVisualVisible( bool visible )
{
  robot_->setVisualVisible( visible );

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + VISUAL_ENABLED_PROPERTY ), visible );
  }
}

void PlanningVisualizer::setCollisionVisible( bool visible )
{
  robot_->setCollisionVisible( visible );

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + COLLISION_ENABLED_PROPERTY ), visible );
  }
}

void PlanningVisualizer::load()
{
  std::string content;
  ros_node_->get_param(description_param_, content);
  robot_desc::URDF file;
  file.loadString(content.c_str());

  robot_->load( &file );

  delete kinematic_model_;
  kinematic_model_ = new planning_models::KinematicModel();
  kinematic_model_->setVerbose( false );
  kinematic_model_->build( file );
  kinematic_model_->defaultState();

  robot_->update( kinematic_model_, target_frame_ );
}

void PlanningVisualizer::onEnable()
{
  subscribe();

  load();
  robot_->setVisible( true );
}

void PlanningVisualizer::onDisable()
{
  unsubscribe();
  robot_->setVisible( false );
}

void PlanningVisualizer::subscribe()
{
  if ( !isEnabled() )
  {
    return;
  }

  if ( !kinematic_path_topic_.empty() )
  {
    ros_node_->subscribe( kinematic_path_topic_, incoming_kinematic_path_message_, &PlanningVisualizer::incomingKinematicPath, this, 0 );
  }

}

void PlanningVisualizer::unsubscribe()
{
  if ( !kinematic_path_topic_.empty() )
  {
    ros_node_->unsubscribe( kinematic_path_topic_, &PlanningVisualizer::incomingKinematicPath, this );
  }
}

void PlanningVisualizer::update( float dt )
{
  incoming_kinematic_path_message_.lock();

  if ( !animating_path_ && new_kinematic_path_ )
  {
    displaying_kinematic_path_message_ = incoming_kinematic_path_message_;

    animating_path_ = true;
    new_kinematic_path_ = false;
    current_state_ = -1;
    current_state_time_ = state_display_time_ + 1.0f;

    kinematic_model_->computeTransforms(&displaying_kinematic_path_message_.start_state.vals[0]);
    robot_->update( kinematic_model_, target_frame_ );
  }

  incoming_kinematic_path_message_.unlock();

  if ( animating_path_ )
  {
    if ( current_state_time_ > state_display_time_ )
    {
      ++current_state_;

      calculateRobotPosition();

      if ( (size_t)current_state_ < displaying_kinematic_path_message_.path.get_states_size() )
      {
        int group_id = kinematic_model_->getGroupID( displaying_kinematic_path_message_.model_name );
        kinematic_model_->computeTransforms(&displaying_kinematic_path_message_.path.states[ current_state_ ].vals[0], group_id);
        robot_->update( kinematic_model_, target_frame_ );

        causeRender();
      }
      else
      {
        animating_path_ = false;
      }

      current_state_time_ = 0.0f;
    }

    current_state_time_ += dt;
  }
}

void PlanningVisualizer::calculateRobotPosition()
{
  libTF::TFPose pose = { 0, 0, 0, 0, 0, 0, 0, displaying_kinematic_path_message_.frame_id };

  try
  {
    pose = tf_client_->transformPose( target_frame_, pose );
  }
  catch(libTF::Exception& e)
  {
    printf( "Error transforming from frame '%s' to frame '%s'\n", pose.frame.c_str(), target_frame_.c_str() );
  }

  Ogre::Vector3 position( pose.x, pose.y, pose.z );
  robotToOgre( position );

  Ogre::Matrix3 orientation;
  orientation.FromEulerAnglesYXZ( Ogre::Radian( pose.yaw ), Ogre::Radian( pose.pitch ), Ogre::Radian( pose.roll ) );

  robot_->setPosition( position );
  robot_->setOrientation( orientation );
}

void PlanningVisualizer::incomingKinematicPath()
{
  new_kinematic_path_ = true;
}

void PlanningVisualizer::targetFrameChanged()
{
  calculateRobotPosition();
}

void PlanningVisualizer::fillPropertyGrid()
{
  property_grid_->Append( new wxStringProperty( ROBOT_DESCRIPTION_PROPERTY, property_prefix_ + ROBOT_DESCRIPTION_PROPERTY, wxString::FromAscii( description_param_.c_str() ) ) );
  property_grid_->Append( new wxStringProperty( TOPIC_PROPERTY, property_prefix_ + TOPIC_PROPERTY, wxString::FromAscii( kinematic_path_topic_.c_str() ) ) );
  wxPGProperty* prop = property_grid_->Append( new wxBoolProperty( VISUAL_ENABLED_PROPERTY, property_prefix_ + VISUAL_ENABLED_PROPERTY, robot_->isVisualVisible() ) );
  prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
  prop = property_grid_->Append( new wxBoolProperty( COLLISION_ENABLED_PROPERTY, property_prefix_ + COLLISION_ENABLED_PROPERTY, robot_->isCollisionVisible() ) );
  prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );

  property_grid_->Append( new wxFloatProperty( STATE_DISPLAY_TIME_PROPERTY, property_prefix_ + STATE_DISPLAY_TIME_PROPERTY, state_display_time_ ) );
}

void PlanningVisualizer::propertyChanged( wxPropertyGridEvent& event )
{
  wxPGProperty* property = event.GetProperty();

  const wxString& name = property->GetName();
  wxVariant value = property->GetValue();

  if ( name == property_prefix_ + VISUAL_ENABLED_PROPERTY )
  {
    bool visible = value.GetBool();
    setVisualVisible( visible );
  }
  else if ( name == property_prefix_ + COLLISION_ENABLED_PROPERTY )
  {
    bool visible = value.GetBool();
    setCollisionVisible( visible );
  }
  else if ( name == property_prefix_ + STATE_DISPLAY_TIME_PROPERTY )
  {
    setStateDisplayTime( value.GetDouble() );
  }
  else if ( name == property_prefix_ + ROBOT_DESCRIPTION_PROPERTY )
  {
    wxString prop = value.GetString();
    setRobotDescription( (const char*)prop.fn_str() );
  }
  else if ( name == property_prefix_ + TOPIC_PROPERTY )
  {
    wxString prop = value.GetString();
    setTopic( (const char*)prop.fn_str() );
  }

  causeRender();
}

void PlanningVisualizer::loadProperties( wxConfigBase* config )
{
  bool visual_enabled, collision_enabled;
  double state_display_time;
  wxString robot_description, topic;

  {
    config->Read( VISUAL_ENABLED_PROPERTY, &visual_enabled, robot_->isVisualVisible() );
    config->Read( COLLISION_ENABLED_PROPERTY, &collision_enabled, robot_->isCollisionVisible() );
  }

  {
    config->Read( STATE_DISPLAY_TIME_PROPERTY, &state_display_time, state_display_time_ );
  }

  {
    config->Read( ROBOT_DESCRIPTION_PROPERTY, &robot_description, wxString::FromAscii( description_param_.c_str() ) );
  }

  {
    config->Read( TOPIC_PROPERTY, &topic, wxString::FromAscii( kinematic_path_topic_.c_str() ) );
  }

  setVisualVisible( visual_enabled );
  setCollisionVisible( collision_enabled );
  setStateDisplayTime( state_display_time );
}

void PlanningVisualizer::saveProperties( wxConfigBase* config )
{
  config->Write( VISUAL_ENABLED_PROPERTY, robot_->isVisualVisible() );
  config->Write( COLLISION_ENABLED_PROPERTY, robot_->isCollisionVisible() );
  config->Write( STATE_DISPLAY_TIME_PROPERTY, state_display_time_ );
  config->Write( ROBOT_DESCRIPTION_PROPERTY, wxString::FromAscii( description_param_.c_str() ) );
  config->Write( TOPIC_PROPERTY, wxString::FromAscii( kinematic_path_topic_.c_str() ) );
}

} // namespace ogre_vis



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

#include "visualization_manager.h"
#include "visualization_panel.h"

#include "display.h"
#include "properties/property_manager.h"
#include "properties/property.h"
#include "common.h"
#include "factory.h"
#include "new_display_dialog.h"

#include "tools/tool.h"
#include "tools/move_tool.h"
#include "tools/pose_tool.h"
#include "tools/selection_tool.h"

#include <ogre_tools/wx_ogre_render_window.h>
#include <ogre_tools/camera_base.h>

#include <ros/common.h>
#include <ros/node.h>
#include <tf/transform_listener.h>

#include <wx/timer.h>
#include <wx/propgrid/propgrid.h>
#include <wx/confbase.h>

#include <boost/bind.hpp>

#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreLight.h>

namespace ogre_vis
{

VisualizationManager::VisualizationManager( VisualizationPanel* panel )
: ogre_root_( Ogre::Root::getSingletonPtr() )
, current_tool_( NULL )
, vis_panel_( panel )
, needs_reset_( false )
, new_ros_time_( false )
, wall_clock_begin_( ros::Time() )
, ros_time_begin_( ros::Time() )
{
  initializeCommon();
  registerFactories( this );

  ros_node_ = ros::node::instance();

  /// @todo This should go away once creation of the ros::node is more well-defined
  if (!ros_node_)
  {
    int argc = 0;
    ros::init( argc, 0 );
    ros_node_ = new ros::node( "VisualizationManager", ros::node::DONT_HANDLE_SIGINT | ros::node::ANONYMOUS_NAME );
  }
  ROS_ASSERT( ros_node_ );

  tf_ = new tf::TransformListener( *ros_node_, true, (uint64_t)10000000000ULL);

  scene_manager_ = ogre_root_->createSceneManager( Ogre::ST_GENERIC );

  target_relative_node_ = scene_manager_->getRootSceneNode()->createChildSceneNode();
  updateRelativeNode();

  Ogre::Light* directional_light = scene_manager_->createLight( "MainDirectional" );
  directional_light->setType( Ogre::Light::LT_DIRECTIONAL );
  directional_light->setDirection( Ogre::Vector3( 0, -1, 1 ) );
  directional_light->setDiffuseColour( Ogre::ColourValue( 1.0f, 1.0f, 1.0f ) );

  update_timer_ = new wxTimer( this );
  update_timer_->Start( 10 );
  update_stopwatch_.Start();
  Connect( update_timer_->GetId(), wxEVT_TIMER, wxTimerEventHandler( VisualizationManager::onUpdate ), NULL, this );

  property_manager_ = new PropertyManager( vis_panel_->getPropertyGrid() );

  CategoryProperty* time_category = property_manager_->createCategory( ".Time", "", NULL );
  wall_clock_property_ = property_manager_->createProperty<DoubleProperty>( "Wall Clock Time", "", boost::bind( &VisualizationManager::getWallClock, this ),
                                                                                    DoubleProperty::Setter(), time_category );
  ros_time_property_ = property_manager_->createProperty<DoubleProperty>( "ROSTime Time", "", boost::bind( &VisualizationManager::getROSTime, this ),
                                                                                   DoubleProperty::Setter(), time_category );
  wall_clock_elapsed_property_ = property_manager_->createProperty<DoubleProperty>( "Wall Clock Elapsed Time", "", boost::bind( &VisualizationManager::getWallClockElapsed, this ),
                                                                                    DoubleProperty::Setter(), time_category );
  ros_time_elapsed_property_ = property_manager_->createProperty<DoubleProperty>( "ROSTime Elapsed Time", "", boost::bind( &VisualizationManager::getROSTimeElapsed, this ),
                                                                                   DoubleProperty::Setter(), time_category );
  time_category->collapse();

  CategoryProperty* options_category = property_manager_->createCategory( ".Global Options", "", NULL );
  target_frame_property_ = property_manager_->createProperty<StringProperty>( "Target Frame", "", boost::bind( &VisualizationManager::getTargetFrame, this ),
                                                                              boost::bind( &VisualizationManager::setTargetFrame, this, _1 ), options_category );
  fixed_frame_property_ = property_manager_->createProperty<StringProperty>( "Fixed Frame", "", boost::bind( &VisualizationManager::getFixedFrame, this ),
                                                                                boost::bind( &VisualizationManager::setFixedFrame, this, _1 ), options_category );

  options_category->collapse();

  setTargetFrame( "base" );
  setFixedFrame( "map" );

  ros_node_->subscribe( "/time", time_message_, &VisualizationManager::incomingROSTime, this, 1 );
}

VisualizationManager::~VisualizationManager()
{
  ros_node_->unsubscribe( "/time", &VisualizationManager::incomingROSTime, this );

  Disconnect( wxEVT_TIMER, update_timer_->GetId(), wxTimerEventHandler( VisualizationManager::onUpdate ), NULL, this );
  delete update_timer_;

  vis_panel_->getPropertyGrid()->Freeze();

  V_DisplayInfo::iterator vis_it = displays_.begin();
  V_DisplayInfo::iterator vis_end = displays_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    Display* display = (*vis_it)->display_;
    delete display;

    delete *vis_it;
  }
  displays_.clear();

  M_FactoryInfo::iterator factory_it = factories_.begin();
  M_FactoryInfo::iterator factory_end = factories_.end();
  for ( ; factory_it != factory_end; ++factory_it )
  {
    delete factory_it->second.factory_;
  }
  factories_.clear();

  V_Tool::iterator tool_it = tools_.begin();
  V_Tool::iterator tool_end = tools_.end();
  for ( ; tool_it != tool_end; ++tool_it )
  {
    delete *tool_it;
  }
  tools_.clear();

  delete tf_;

  delete property_manager_;
  vis_panel_->getPropertyGrid()->Thaw();

  ogre_root_->destroySceneManager( scene_manager_ );
}

void VisualizationManager::initialize()
{
  MoveTool* move_tool = createTool< MoveTool >( "Move Camera", 'm' );
  setCurrentTool( move_tool );
  setDefaultTool( move_tool );

  createTool< SelectionTool >( "Focus", 'f' );

  PoseTool* goal_tool = createTool< PoseTool >( "Set Goal", 'g' );
  goal_tool->setIsGoal( true );

  createTool< PoseTool >( "Set Pose", 'p' );
}

DisplayInfo* VisualizationManager::getDisplayInfo( const Display* display )
{
  V_DisplayInfo::iterator vis_it = displays_.begin();
  V_DisplayInfo::iterator vis_end = displays_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    Display* it_display = (*vis_it)->display_;

    if ( display == it_display )
    {
     return *vis_it;
    }
  }

  return NULL;
}

void VisualizationManager::onUpdate( wxTimerEvent& event )
{
  long millis = update_stopwatch_.Time();
  float dt = millis / 1000.0f;

  update_stopwatch_.Start();

  V_DisplayInfo::iterator vis_it = displays_.begin();
  V_DisplayInfo::iterator vis_end = displays_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    Display* display = (*vis_it)->display_;

    if ( display->isEnabled() )
    {
      display->update( dt );
    }
  }

  updateRelativeNode();

  vis_panel_->getCurrentCamera()->update();

  if ( needs_reset_ )
  {
    needs_reset_ = false;
    resetDisplays();
    tf_->clear();

    ros_time_begin_ = ros::Time();
    wall_clock_begin_ = ros::Time();
  }

  static float time_update_timer = 0.0f;
  time_update_timer += dt;

  if ( time_update_timer > 0.1f )
  {
    time_update_timer = 0.0f;

    if ( new_ros_time_ )
    {
      time_message_.lock();

      if ( ros_time_begin_.is_zero() )
      {
        ros_time_begin_ = time_message_.rostime;
        wall_clock_begin_ = ros::Time::now();
      }

      ros_time_elapsed_ = time_message_.rostime - ros_time_begin_;

      time_message_.unlock();


      ros_time_property_->changed();
      ros_time_elapsed_property_->changed();
    }

    if ( wall_clock_begin_.is_zero() )
    {
      wall_clock_begin_ = ros::Time::now();
    }

    wall_clock_elapsed_ = ros::Time::now() - wall_clock_begin_;
    wall_clock_property_->changed();
    wall_clock_elapsed_property_->changed();
  }
}

std::string getCategoryLabel( DisplayInfo* info )
{
  char buf[1024];
  snprintf( buf, 1024, "%02d. %s (%s)", info->index_ + 1, info->display_->getName().c_str(), info->display_->getType() );
  return buf;
}

void VisualizationManager::addDisplay( Display* display, bool enabled )
{
  DisplayInfo* info = new DisplayInfo;
  info->display_ = display;
  info->index_ = displays_.size();
  displays_.push_back( info );

  display->setRenderCallback( boost::bind( &VisualizationPanel::queueRender, vis_panel_ ) );
  display->setLockRenderCallback( boost::bind( &VisualizationPanel::lockRender, vis_panel_ ) );
  display->setUnlockRenderCallback( boost::bind( &VisualizationPanel::unlockRender, vis_panel_ ) );

  display->setTargetFrame( target_frame_ );
  display->setFixedFrame( fixed_frame_ );

  vis_panel_->getPropertyGrid()->Freeze();

  std::string category_label = getCategoryLabel( info );
  info->category_ = property_manager_->createCategory( display->getName(), "", NULL );
  info->category_->setLabel( category_label );
  info->category_->setUserData( display );

  setDisplayEnabled( display, enabled );
  display->setPropertyManager( property_manager_, info->category_ );

  vis_panel_->getPropertyGrid()->Sort( vis_panel_->getPropertyGrid()->GetRoot() );

  vis_panel_->getPropertyGrid()->Thaw();
  vis_panel_->getPropertyGrid()->Refresh();
}

void VisualizationManager::resetDisplayIndices()
{
  V_DisplayInfo::iterator it = displays_.begin();
  V_DisplayInfo::iterator end = displays_.end();
  for ( uint32_t i = 0; it != end; ++it, ++i )
  {
    DisplayInfo* info = *it;

    info->index_ = i;
    info->category_->setLabel( getCategoryLabel( info ) );
  }

  vis_panel_->getPropertyGrid()->Freeze();
  vis_panel_->getPropertyGrid()->Sort( vis_panel_->getPropertyGrid()->GetRoot() );
  vis_panel_->getPropertyGrid()->Thaw();
}

void VisualizationManager::removeDisplay( Display* display )
{
  V_DisplayInfo::iterator it = displays_.begin();
  V_DisplayInfo::iterator end = displays_.end();
  for ( ; it != end; ++it )
  {
    if ( (*it)->display_ == display )
    {
      break;
    }
  }
  ROS_ASSERT( it != displays_.end() );

  DisplayInfo* info = *it;
  displays_.erase( it );

  delete info;

  vis_panel_->getPropertyGrid()->Freeze();

  delete display;

  resetDisplayIndices();

  vis_panel_->getPropertyGrid()->Thaw();

  vis_panel_->queueRender();
}

void VisualizationManager::removeAllDisplays()
{
  vis_panel_->getPropertyGrid()->Freeze();

  while (!displays_.empty())
  {
    removeDisplay(displays_.back()->display_);
  }

  vis_panel_->getPropertyGrid()->Thaw();
}

void VisualizationManager::removeDisplay( const std::string& name )
{
  Display* display = getDisplay( name );

  if ( !display )
  {
    return;
  }

  removeDisplay( display );
}

void VisualizationManager::moveDisplayUp( Display* display )
{
  DisplayInfo* info = getDisplayInfo( display );
  ROS_ASSERT( info );

  if ( info->index_ == 0 )
  {
    return;
  }

  DisplayInfo* other_info = displays_[ info->index_ - 1 ];
  displays_[ info->index_ - 1 ] = info;
  displays_[ info->index_ ] = other_info;

  --info->index_;
  ++other_info->index_;

  info->category_->setLabel( getCategoryLabel( info ) );
  other_info->category_->setLabel( getCategoryLabel( other_info ) );

  vis_panel_->getPropertyGrid()->Freeze();
  vis_panel_->getPropertyGrid()->Sort( vis_panel_->getPropertyGrid()->GetRoot() );
  vis_panel_->getPropertyGrid()->Thaw();
}

void VisualizationManager::moveDisplayDown( Display* display )
{
  DisplayInfo* info = getDisplayInfo( display );
  ROS_ASSERT( info );

  if ( info->index_ == displays_.size() - 1 )
  {
    return;
  }

  DisplayInfo* other_info = displays_[ info->index_ + 1 ];
  displays_[ info->index_ + 1 ] = info;
  displays_[ info->index_ ] = other_info;

  ++info->index_;
  --other_info->index_;

  info->category_->setLabel( getCategoryLabel( info ) );
  other_info->category_->setLabel( getCategoryLabel( other_info ) );

  vis_panel_->getPropertyGrid()->Freeze();
  vis_panel_->getPropertyGrid()->Sort( vis_panel_->getPropertyGrid()->GetRoot() );
  vis_panel_->getPropertyGrid()->Refresh(false);
  vis_panel_->getPropertyGrid()->Thaw();
}

void VisualizationManager::resetDisplays()
{
  V_DisplayInfo::iterator vis_it = displays_.begin();
  V_DisplayInfo::iterator vis_end = displays_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    Display* display = (*vis_it)->display_;

    display->reset();
  }
}

void VisualizationManager::addTool( Tool* tool )
{
  tools_.push_back( tool );

  vis_panel_->addTool( tool );
}

void VisualizationManager::setCurrentTool( Tool* tool )
{
  if ( current_tool_ )
  {
    current_tool_->deactivate();
  }

  current_tool_ = tool;
  current_tool_->activate();

  vis_panel_->setTool( tool );
}

void VisualizationManager::setDefaultTool( Tool* tool )
{
  default_tool_ = tool;
}

Tool* VisualizationManager::getTool( int index )
{
  ROS_ASSERT( index >= 0 );
  ROS_ASSERT( index < (int)tools_.size() );

  return tools_[ index ];
}

Display* VisualizationManager::getDisplay( const std::string& name )
{
  V_DisplayInfo::iterator vis_it = displays_.begin();
  V_DisplayInfo::iterator vis_end = displays_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    Display* display = (*vis_it)->display_;

    if ( display->getName() == name )
    {
      return display;
    }
  }

  return NULL;
}

void VisualizationManager::setDisplayEnabled( Display* display, bool enabled )
{
  if ( enabled )
  {
    display->enable();
  }
  else
  {
    display->disable();
  }

  display_state_( display );
}

#define PROPERTY_GRID_CONFIG wxT("Property Grid State")

void VisualizationManager::loadConfig( wxConfigBase* config )
{
  int i = 0;
  while (1)
  {
    wxString type, name;
    type.Printf( wxT("Display%d/Type"), i );
    name.Printf( wxT("Display%d/Name"), i );

    wxString vis_type, vis_name;
    if ( !config->Read( type, &vis_type ) )
    {
      break;
    }

    if ( !config->Read( name, &vis_name ) )
    {
      break;
    }

    createDisplay( (const char*)vis_type.mb_str(), (const char*)vis_name.mb_str(), false );

    ++i;
  }

  property_manager_->load( config );

  wxString grid_state;
  if ( config->Read( PROPERTY_GRID_CONFIG, &grid_state ) )
  {
    vis_panel_->getPropertyGrid()->RestoreEditableState( grid_state );
  }
}

void VisualizationManager::saveConfig( wxConfigBase* config )
{
  int i = 0;
  V_DisplayInfo::iterator vis_it = displays_.begin();
  V_DisplayInfo::iterator vis_end = displays_.end();
  for ( ; vis_it != vis_end; ++vis_it, ++i )
  {
    Display* display = (*vis_it)->display_;

    wxString type, name;
    type.Printf( wxT("Display%d/Type"), i );
    name.Printf( wxT("Display%d/Name"), i );
    config->Write( type, wxString::FromAscii( display->getType() ) );
    config->Write( name, wxString::FromAscii( display->getName().c_str() ) );
  }

  property_manager_->save( config );

  config->Write( PROPERTY_GRID_CONFIG, vis_panel_->getPropertyGrid()->SaveEditableState() );
}

bool VisualizationManager::registerFactory( const std::string& type, const std::string& description, DisplayFactory* factory )
{
  M_FactoryInfo::iterator it = factories_.find( type );
  if ( it != factories_.end() )
  {
    return false;
  }

  factories_.insert( std::make_pair( type, FactoryInfo( type, description, factory ) ) );

  return true;
}

Display* VisualizationManager::createDisplay( const std::string& type, const std::string& name, bool enabled )
{
  M_FactoryInfo::iterator it = factories_.find( type );
  if ( it == factories_.end() )
  {
    return NULL;
  }

  Display* current_vis = getDisplay( name );
  if ( current_vis )
  {
    return NULL;
  }

  DisplayFactory* factory = it->second.factory_;
  Display* display = factory->create( name, this );

  addDisplay( display, enabled );

  return display;
}

void VisualizationManager::setTargetFrame( const std::string& frame )
{
  target_frame_ = frame;

  V_DisplayInfo::iterator it = displays_.begin();
  V_DisplayInfo::iterator end = displays_.end();
  for ( ; it != end; ++it )
  {
    Display* display = (*it)->display_;

    display->setTargetFrame(frame);
  }

  target_frame_property_->changed();

  updateRelativeNode();
  if ( vis_panel_->getCurrentCamera() )
  {
    vis_panel_->getCurrentCamera()->lookAt( target_relative_node_->getPosition() );
  }
}

void VisualizationManager::setFixedFrame( const std::string& frame )
{
  fixed_frame_ = frame;

  V_DisplayInfo::iterator it = displays_.begin();
  V_DisplayInfo::iterator end = displays_.end();
  for ( ; it != end; ++it )
  {
    Display* display = (*it)->display_;

    display->setFixedFrame(frame);
  }

  fixed_frame_property_->changed();
}

bool VisualizationManager::isValidDisplay( Display* display )
{
  DisplayInfo* info = getDisplayInfo( display );
  return info != NULL;
}

void VisualizationManager::getRegisteredTypes( std::vector<std::string>& types, std::vector<std::string>& descriptions )
{
  M_FactoryInfo::iterator it = factories_.begin();
  M_FactoryInfo::iterator end = factories_.end();
  for ( ; it != end; ++it )
  {
    types.push_back( it->first );
    descriptions.push_back( it->second.description_ );
  }
}

void VisualizationManager::updateRelativeNode()
{
  tf::Stamped<tf::Pose> pose( btTransform( btQuaternion( 0.0f, 0.0f, 0.0f ), btVector3( 0.0f, 0.0f, 0.0f ) ),
                              ros::Time(), target_frame_ );

  if (tf_->canTransform(fixed_frame_, target_frame_, ros::Time()))
  {
    try
    {
      tf_->transformPose( fixed_frame_, pose, pose );
    }
    catch(tf::TransformException& e)
    {
      ROS_ERROR( "Error transforming from frame '%s' to frame '%s'\n", target_frame_.c_str(), fixed_frame_.c_str() );
    }

    Ogre::Vector3 position = Ogre::Vector3( pose.getOrigin().x(), pose.getOrigin().y(), pose.getOrigin().z() );
    robotToOgre( position );

    btQuaternion quat;
    pose.getBasis().getRotation( quat );
    Ogre::Quaternion orientation( Ogre::Quaternion::IDENTITY );
    ogreToRobot( orientation );
    orientation = Ogre::Quaternion( quat.w(), quat.x(), quat.y(), quat.z() ) * orientation;
    robotToOgre( orientation );

    target_relative_node_->setPosition( position );
    target_relative_node_->setOrientation( orientation );
  }
}

double VisualizationManager::getWallClock()
{
  return ros::Time::now().toSec();
}

double VisualizationManager::getROSTime()
{
  return (ros_time_begin_ + ros_time_elapsed_).toSec();
}

double VisualizationManager::getWallClockElapsed()
{
  return wall_clock_elapsed_.toSec();
}

double VisualizationManager::getROSTimeElapsed()
{
  return ros_time_elapsed_.toSec();
}

void VisualizationManager::incomingROSTime()
{
  static ros::Time last_time = ros::Time();

  if ( time_message_.rostime < last_time )
  {
    needs_reset_ = true;
  }

  last_time = time_message_.rostime;

  new_ros_time_ = true;
}

void VisualizationManager::handleChar( wxKeyEvent& event )
{
  if ( event.GetKeyCode() == WXK_ESCAPE )
  {
    setCurrentTool( getDefaultTool() );

    return;
  }

  char key = event.GetKeyCode();
  V_Tool::iterator it = tools_.begin();
  V_Tool::iterator end = tools_.end();
  for ( ; it != end; ++it )
  {
    Tool* tool = *it;
    if ( tool->getShortcutKey() == key )
    {
      setCurrentTool( tool );
      return;
    }
  }
}

} // namespace ogre_vis

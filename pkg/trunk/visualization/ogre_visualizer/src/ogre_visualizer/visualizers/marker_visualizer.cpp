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

#include "marker_visualizer.h"
#include "common.h"
#include "helpers/robot.h"

#include <ogre_tools/arrow.h>
#include <ogre_tools/super_ellipsoid.h>

#include <ros/node.h>
#include <tf/transform_listener.h>

#include <urdf/URDF.h>
#include <planning_models/kinematic.h>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>


namespace ogre_vis
{

MarkerVisualizer::MarkerVisualizer( const std::string& name, VisualizationManager* manager )
: VisualizerBase( name, manager )
{
  scene_node_ = scene_manager_->getRootSceneNode()->createChildSceneNode();

  urdf_ = new robot_desc::URDF();

  kinematic_model_ = new planning_models::KinematicModel();
  kinematic_model_->setVerbose( false );
}

MarkerVisualizer::~MarkerVisualizer()
{
  delete urdf_;
  delete kinematic_model_;

  unsubscribe();

  clearMarkers();
}

void MarkerVisualizer::clearMarkers()
{
  M_IDToMarker::iterator marker_it = markers_.begin();
  M_IDToMarker::iterator marker_end = markers_.end();
  for ( ; marker_it != marker_end; ++marker_it )
  {
    MarkerInfo& info = marker_it->second;
    delete info.object_;
  }
  markers_.clear();
}

void MarkerVisualizer::onEnable()
{
  subscribe();

  scene_node_->setVisible( true );

  std::string content;
  /// @todo pass this in
  ros_node_->get_param("robotdesc/pr2", content);
  urdf_->loadString(content.c_str());

  kinematic_model_->build( *urdf_ );
  kinematic_model_->defaultState();
}

void MarkerVisualizer::onDisable()
{
  unsubscribe();

  clearMarkers();

  scene_node_->setVisible( false );
}

void MarkerVisualizer::subscribe()
{
  if ( !isEnabled() )
  {
    return;
  }

  ros_node_->subscribe("visualizationMarker", current_message_, &MarkerVisualizer::incomingMarker, this, 0);
}

void MarkerVisualizer::unsubscribe()
{
  ros_node_->unsubscribe( "visualizationMarker", &MarkerVisualizer::incomingMarker, this );
}

void MarkerVisualizer::incomingMarker()
{
  message_queue_.push_back( current_message_ );
}

void MarkerVisualizer::processMessage( const std_msgs::VisualizationMarker& message )
{
  switch ( message.action )
  {
  case MarkerActions::Add:
    processAdd( message );
    break;

  case MarkerActions::Modify:
    processModify( message );
    break;

  case MarkerActions::Delete:
    processDelete( message );
    break;

  default:
    ROS_ERROR( "Unknown marker action: %d\n", message.action );
  }
}

void MarkerVisualizer::processAdd( const std_msgs::VisualizationMarker& message )
{
  ogre_tools::Object* object = NULL;
  bool create = true;

  M_IDToMarker::iterator it = markers_.find( message.id );
  if ( it != markers_.end() )
  {
    MarkerInfo& info = it->second;
    if ( message.type == info.message_.type )
    {
      object = info.object_;

      info.message_ = message;
      create = false;
    }
    else
    {
      delete it->second.object_;
      markers_.erase( it );
    }
  }

  if ( create )
  {
    switch ( message.type )
    {
    case MarkerTypes::Cube:
      {
        ogre_tools::SuperEllipsoid* cube = new ogre_tools::SuperEllipsoid( scene_manager_, scene_node_ );
        cube->create( ogre_tools::SuperEllipsoid::Cube, 10, Ogre::Vector3( 1.0f, 1.0f, 1.0f ) );

        object = cube;
      }
      break;

    case MarkerTypes::Sphere:
      {
        ogre_tools::SuperEllipsoid* sphere = new ogre_tools::SuperEllipsoid( scene_manager_, scene_node_ );
        sphere->create( ogre_tools::SuperEllipsoid::Sphere, 20, Ogre::Vector3( 1.0f, 1.0f, 1.0f ) );

        object = sphere;
      }
      break;

    case MarkerTypes::Arrow:
      {
        object = new ogre_tools::Arrow( scene_manager_, scene_node_, 0.8, 0.5, 0.2, 1.0 );
      }
      break;

    case MarkerTypes::Robot:
      {
        Robot* robot = new Robot( scene_manager_ );
        robot->load( urdf_, false, true );
        robot->update( kinematic_model_, fixed_frame_ );

        object = robot;
      }
      break;

    default:
      ROS_ERROR( "Unknown marker type: %d\n", message.type );
    }

    if ( object )
    {
      markers_.insert( std::make_pair( message.id, MarkerInfo(object, message) ) );
    }
  }

  if ( object )
  {
    setCommonValues( message, object );

    causeRender();
  }
}

void MarkerVisualizer::processModify( const std_msgs::VisualizationMarker& message )
{
  M_IDToMarker::iterator it = markers_.find( message.id );
  if ( it == markers_.end() )
  {
    ROS_ERROR( "Tried to modify marker with id %d that does not exist\n", message.id );
    return;
  }

  MarkerInfo& info = it->second;
  info.message_ = message;
  setCommonValues( message, info.object_ );

  causeRender();
}

void MarkerVisualizer::processDelete( const std_msgs::VisualizationMarker& message )
{
  M_IDToMarker::iterator it = markers_.find( message.id );
  if ( it != markers_.end() )
  {
    delete it->second.object_;
    markers_.erase( it );
  }

  causeRender();
}

void MarkerVisualizer::setCommonValues( const std_msgs::VisualizationMarker& message, ogre_tools::Object* object )
{
  std::string frame_id = message.header.frame_id;
  if ( frame_id.empty() )
  {
    frame_id = fixed_frame_;
  }

  tf::Stamped<tf::Pose> pose( btTransform( btQuaternion( message.yaw, message.pitch, message.roll ), btVector3( message.x, message.y, message.z ) ),
                              ros::Time(message.header.stamp), frame_id );
  try
  {
    tf_->transformPose( fixed_frame_, pose, pose );
  }
  catch(tf::TransformException& e)
  {
    ROS_ERROR( "Error transforming marker '%d' from frame '%s' to frame '%s'\n", message.id, frame_id.c_str(), fixed_frame_.c_str() );
  }

  Ogre::Vector3 position( pose.getOrigin().x(), pose.getOrigin().y(), pose.getOrigin().z() );
  robotToOgre( position );

  btScalar yaw, pitch, roll;
  pose.getBasis().getEulerZYX( yaw, pitch, roll );

  Ogre::Matrix3 orientation;
  orientation.FromEulerAnglesZXY( Ogre::Radian( roll ), Ogre::Radian( pitch ), Ogre::Radian( -yaw) );
  Ogre::Vector3 scale( message.xScale, message.yScale, message.zScale );
  scaleRobotToOgre( scale );

  object->setPosition( position );
  object->setOrientation( orientation );
  object->setScale( scale );
  object->setColor( message.r / 255.0f, message.g / 255.0f, message.b / 255.0f, message.alpha / 255.0f );
  object->setUserData( Ogre::Any( (void*)this ) );
}

void MarkerVisualizer::update( float dt )
{
  current_message_.lock();

  V_MarkerMessage local_queue;
  local_queue.swap( message_queue_ );

  current_message_.unlock();

  if ( !local_queue.empty() )
  {
    V_MarkerMessage::iterator message_it = local_queue.begin();
    V_MarkerMessage::iterator message_end = local_queue.end();
    for ( ; message_it != message_end; ++message_it )
    {
      std_msgs::VisualizationMarker& marker = *message_it;

      processMessage( marker );
    }
  }
}

#if 0
void MarkerVisualizer::targetFrameChanged();
{
  M_IDToMarker::iterator marker_it = markers_.begin();
  M_IDToMarker::iterator marker_end = markers_.end();
  for ( ; marker_it != marker_end; )
  {
    MarkerInfo& info = marker_it->second;

    ++marker_it;

    processMessage( info.message_ );
  }
}
#endif

void MarkerVisualizer::fixedFrameChanged()
{
  clearMarkers();
}

void MarkerVisualizer::reset()
{
  clearMarkers();
}

} // namespace ogre_vis

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

#include "pose_tool.h"
#include "common.h"
#include "visualization_manager.h"
#include "visualization_panel.h"
#include "ogre_tools/camera_base.h"
#include "ogre_tools/arrow.h"
#include "ogre_tools/wx_ogre_render_window.h"

#include <std_msgs/Planner2DGoal.h>
#include <std_msgs/Pose2DFloat32.h>

#include <OgreRay.h>
#include <OgrePlane.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>

#include <ros/node.h>
#include <tf/transform_listener.h>

#include <wx/wx.h>

namespace ogre_vis
{

PoseTool::PoseTool( const std::string& name, VisualizationManager* manager )
: Tool( name, manager )
, is_goal_( false )
{
  arrow_ = new ogre_tools::Arrow( scene_manager_, NULL, 2.0f, 0.2f, 0.5f, 0.35f );
  arrow_->setColor( 0.0f, 1.0f, 0.0f, 1.0f );
  arrow_->getSceneNode()->setVisible( false );

  ros_node_->advertise<std_msgs::Planner2DGoal>("goal", 1);
  ros_node_->advertise<std_msgs::Pose2DFloat32>("initialpose", 1);
}

PoseTool::~PoseTool()
{
  ros_node_->unadvertise("goal");
  ros_node_->unadvertise("initialpose");

  delete arrow_;
}

void PoseTool::activate()
{
  state_ = Position;

}

void PoseTool::deactivate()
{
  arrow_->getSceneNode()->setVisible( false );
}

void PoseTool::setIsGoal( bool is_goal )
{
  is_goal_ = is_goal;

  arrow_->setColor( 0.0f, is_goal_ ? 1.0f : 0.0f, is_goal_ ? 0.0f : 1.0f, 1.0f );
}

Ogre::Vector3 PoseTool::getPositionFromMouseXY( int mouse_x, int mouse_y )
{
  int width, height;
  manager_->getVisualizationPanel()->getRenderPanel()->GetSize( &width, &height );
  Ogre::Ray mouse_ray = manager_->getVisualizationPanel()->getCurrentCamera()->getOgreCamera()->getCameraToViewportRay( (float)mouse_x / (float)width, (float)mouse_y / (float)height );
  Ogre::Plane ground_plane( Ogre::Vector3::UNIT_Y, 0.0f );
  std::pair<bool, Ogre::Real> intersection = mouse_ray.intersects( ground_plane );
  if ( !intersection.first )
  {
    return Ogre::Vector3::ZERO;
  }

  return mouse_ray.getPoint( intersection.second );
}

int PoseTool::processMouseEvent( wxMouseEvent& event, int last_x, int last_y )
{
  int flags = 0;

  if ( event.LeftDown() )
  {
    ROS_ASSERT( state_ == Position );

    pos_ = getPositionFromMouseXY( event.GetX(), event.GetY() );
    arrow_->setPosition( pos_ );

    state_ = Orientation;
    flags |= Render;
  }
  else if ( event.Dragging() )
  {
    if ( state_ == Orientation )
    {
      Ogre::Vector3 cur_pos = getPositionFromMouseXY( event.GetX(), event.GetY() );
      double angle = atan2(pos_.z - cur_pos.z, cur_pos.x - pos_.x);

      arrow_->getSceneNode()->setVisible( true );

      Ogre::Quaternion base_orient = Ogre::Quaternion( Ogre::Radian(Ogre::Math::HALF_PI), Ogre::Vector3::NEGATIVE_UNIT_Z );
      arrow_->setOrientation( Ogre::Quaternion( Ogre::Radian(angle - Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_Y ) * base_orient );

      flags |= Render;
    }
  }
  else if ( event.LeftUp() )
  {
    if ( state_ == Orientation )
    {
      Ogre::Vector3 cur_pos = getPositionFromMouseXY( event.GetX(), event.GetY() );
      ogreToRobot( cur_pos );

      Ogre::Vector3 robot_pos = pos_;
      ogreToRobot( robot_pos );

      const std::string& fixed_frame = manager_->getFixedFrame();
      tf::Stamped<tf::Point> cur_pos_transformed( tf::Point(cur_pos.x, cur_pos.y, cur_pos.z), ros::Time(0ULL), fixed_frame );
      tf::Stamped<tf::Point> robot_pos_transformed( tf::Point(robot_pos.x, robot_pos.y, robot_pos.z), ros::Time(0ULL), fixed_frame );

      tf::TransformListener* tf = manager_->getTFClient();
      try
      {
        tf->transformPoint( "map", cur_pos_transformed, cur_pos_transformed );
        tf->transformPoint( "map", robot_pos_transformed, robot_pos_transformed );
      }
      catch(tf::TransformException& e)
      {
        ROS_ERROR( "Error transforming pose from frame '%s' to frame 'map': %s\n", fixed_frame.c_str(), e.what() );
      }

      double angle = atan2(cur_pos_transformed.y() - robot_pos_transformed.y(), cur_pos_transformed.x() - robot_pos_transformed.x());

      if ( is_goal_ )
      {
        std_msgs::Planner2DGoal goal;
        goal.goal.x = robot_pos_transformed.x();
        goal.goal.y = robot_pos_transformed.y();
        goal.goal.th = angle;
        goal.enable = 1;
        goal.header.frame_id = "map";
        ROS_INFO("setting goal: %.3f %.3f %.3f\n", goal.goal.x, goal.goal.y, goal.goal.th);
        ros_node_->publish( "goal", goal );
      }
      else
      {
        std_msgs::Pose2DFloat32 pose;
        pose.x = robot_pos_transformed.x();
        pose.y = robot_pos_transformed.y();
        pose.th = angle;
        ROS_INFO( "setting pose: %.3f %.3f %.3f\n", pose.x, pose.y, pose.th );
        ros_node_->publish( "initialpose", pose );
      }

      flags |= (Finished|Render);
    }
  }

  return flags;
}

}


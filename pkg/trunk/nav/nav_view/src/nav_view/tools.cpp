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

#include "tools.h"
#include <ogre_tools/arrow.h>
#include <ogre_tools/wx_ogre_render_window.h>

#include <ros/node.h>
#include <Ogre.h>
#include <wx/wx.h>

#include "nav_view_panel.h"

namespace nav_view
{

Tool::Tool( NavViewPanel* panel )
: scene_manager_( panel->getSceneManager() )
, ros_node_( panel->getROSNode() )
, panel_( panel )
{

}

MoveTool::MoveTool( NavViewPanel* panel )
: Tool( panel )
{

}

int MoveTool::processMouseEvent( wxMouseEvent& event, int last_x, int last_y, float& scale )
{
  int flags = 0;

  if ( event.Dragging() )
  {
    int32_t diff_x = event.GetX() - last_x;
    int32_t diff_y = event.GetY() - last_y;

    if ( event.LeftIsDown() )
    {
      panel_->getRootNode()->translate(Ogre::Vector3(diff_x / scale, -diff_y / scale, 0.0f));

      flags |= Render;
    }
    else if ( event.RightIsDown() )
    {
      scale *= 1.0 - diff_y * 0.01;

      flags |= Render;
    }
  }

  return flags;
}

PoseTool::PoseTool( NavViewPanel* panel, bool goal )
: Tool( panel )
, arrow_( NULL )
, state_( Position )
, is_goal_( goal )
{

}

PoseTool::~PoseTool()
{
  delete arrow_;
}

Ogre::Vector3 PoseTool::getPositionFromMouseXY( int mouse_x, int mouse_y )
{
  int width, height;
  panel_->getRenderPanel()->GetSize( &width, &height );

  Ogre::Vector3 pos;
  pos.x = mouse_x;
  pos.y = (height - mouse_y);
  pos.z = 0.0f;

  Ogre::SceneNode* root_node = panel_->getRootNode();
  Ogre::Vector3 root_pos = root_node->getPosition();

  float scale = panel_->getScale();

  pos.x = (pos.x - (width / 2.0f)) / scale - root_pos.x;
  pos.y = (pos.y - (height / 2.0f)) / scale - root_pos.y;

  return pos;
}

int PoseTool::processMouseEvent( wxMouseEvent& event, int last_x, int last_y, float& scale )
{
  int flags = 0;

  if ( event.LeftDown() )
  {
    ROS_ASSERT( state_ == Position );

    pos_ = getPositionFromMouseXY( event.GetX(), event.GetY() );

    state_ = Orientation;
    flags |= Render;
  }
  else if ( event.Dragging() )
  {
    if ( state_ == Orientation )
    {
      Ogre::Vector3 cur_pos = getPositionFromMouseXY( event.GetX(), event.GetY() );
      double angle = atan2(cur_pos.y - pos_.y, cur_pos.x - pos_.x);

      if ( !arrow_ )
      {
        Ogre::SceneNode* root_node = panel_->getRootNode();
        arrow_ = new ogre_tools::Arrow( scene_manager_, root_node, 2.0f, 0.2f, 0.5f, 0.35f );
        arrow_->setPosition( pos_ );
        arrow_->setColor( 0.0f, is_goal_ ? 1.0f : 0.0f, is_goal_ ? 0.0f : 1.0f, 1.0f );
      }

      Ogre::Quaternion base_orient = Ogre::Quaternion( Ogre::Radian(Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_X );
      arrow_->setOrientation( Ogre::Quaternion( Ogre::Radian(angle - Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_Z ) * base_orient );

      flags |= Render;
    }
  }
  else if ( event.LeftUp() )
  {
    if ( state_ == Orientation )
    {
      Ogre::Vector3 cur_pos = getPositionFromMouseXY( event.GetX(), event.GetY() );
      double angle = atan2(cur_pos.y - pos_.y, cur_pos.x - pos_.x);

      if ( is_goal_ )
      {
        std_msgs::Planner2DGoal goal;
        goal.goal.x = pos_.x;
        goal.goal.y = pos_.y;
        goal.goal.th = angle;
        goal.enable = 1;
        printf("setting goal: %.3f %.3f %.3f\n", goal.goal.x, goal.goal.y, goal.goal.th);
        ros_node_->publish( "goal", goal );
      }
      else
      {
        std_msgs::Pose2DFloat32 pose;
        pose.x = pos_.x;
        pose.y = pos_.y;
        pose.th = angle;
        printf( "setting pose: %.3f %.3f %.3f\n", pose.x, pose.y, pose.th );
        ros_node_->publish( "initialpose", pose );
      }

      flags |= Finished;
    }
  }

  return flags;
}

} // namespace nav_view

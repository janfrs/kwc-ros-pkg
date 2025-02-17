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

#ifndef OGRE_VISUALIZER_POSE_TOOL_H
#define OGRE_VISUALIZER_POSE_TOOL_H

#include "tool.h"

#include <OgreVector3.h>

namespace ogre_tools
{
class Arrow;
}

namespace ogre_vis
{

class VisualizationManager;

class PoseTool : public Tool
{
public:
  PoseTool( const std::string& name, char shortcut_key, VisualizationManager* manager );
  virtual ~PoseTool();

  void setIsGoal( bool is_goal );

  virtual void activate();
  virtual void deactivate();

  virtual int processMouseEvent( wxMouseEvent& event, int last_x, int last_y );

protected:
  Ogre::Vector3 getPositionFromMouseXY( int mouse_x, int mouse_y );

  ogre_tools::Arrow* arrow_;

  enum State
  {
    Position,
    Orientation
  };
  State state_;

  Ogre::Vector3 pos_;

  bool is_goal_;
};

}

#endif



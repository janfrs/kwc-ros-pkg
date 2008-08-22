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

#ifndef OGRE_VISUALIZER_MARKER_VISUALIZER_H
#define OGRE_VISUALIZER_MARKER_VISUALIZER_H

#include "../visualizer_base.h"

#include <map>

#include <std_msgs/VisualizationMarker.h>

namespace Ogre
{
class SceneManager;
class SceneNode;
}

namespace ros
{
class node;
}

namespace ogre_tools
{
class Object;
}

class rosTFClient;

namespace ogre_vis
{

namespace MarkerTypes
{
enum MarkerType
{
  Arrow,
  Cube,
  Sphere,
};
}
typedef MarkerTypes::MarkerType MarkerType;

namespace MarkerActions
{
enum MarkerAction
{
  Add,
  Modify,
  Delete,
};
}
typedef MarkerActions::MarkerAction MarkerAction;

class MarkerVisualizer : public VisualizerBase
{
public:
  MarkerVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name, bool enabled );
  virtual ~MarkerVisualizer();

  virtual void update( float dt );

protected:
  virtual void onEnable();
  virtual void onDisable();

  void subscribe();
  void unsubscribe();

  void clearMarkers();

  void processMessage( const std_msgs::VisualizationMarker& message );
  void processAdd( const std_msgs::VisualizationMarker& message );
  void processModify( const std_msgs::VisualizationMarker& message );
  void processDelete( const std_msgs::VisualizationMarker& message );
  void setCommonValues( const std_msgs::VisualizationMarker& message, ogre_tools::Object* object );

  void incomingMarker();

  typedef std::map<int, ogre_tools::Object*> M_IDToObject;
  M_IDToObject markers_;

  std_msgs::VisualizationMarker current_message_;

  typedef std::vector< std_msgs::VisualizationMarker > V_MarkerMessage;
  V_MarkerMessage message_queue_;

  Ogre::SceneNode* scene_node_;
};

} // namespace ogre_vis

#endif /* OGRE_VISUALIZER_MARKER_VISUALIZER_H */

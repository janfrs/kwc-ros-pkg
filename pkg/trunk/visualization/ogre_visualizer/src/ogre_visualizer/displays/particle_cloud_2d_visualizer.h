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


#ifndef OGRE_VISUALIZER_PARTICLE_CLOUD_2D_VISUALIZER_H_
#define OGRE_VISUALIZER_PARTICLE_CLOUD_2D_VISUALIZER_H_

#include "visualizer_base.h"
#include "helpers/color.h"

#include <std_msgs/ParticleCloud2D.h>

namespace ogre_tools
{
class Arrow;
}

namespace Ogre
{
class SceneNode;
class ManualObject;
}

namespace ogre_vis
{

class ROSTopicStringProperty;
class ColorProperty;
class FloatProperty;

/**
 * \class ParticleCloud2DVisualizer
 * \brief Displays a std_msgs::ParticleCloud2D message
 */
class ParticleCloud2DVisualizer : public VisualizerBase
{
public:
  ParticleCloud2DVisualizer( const std::string& name, VisualizationManager* manager );
  virtual ~ParticleCloud2DVisualizer();

  void setTopic( const std::string& topic );
  const std::string& getTopic() { return topic_; }

  void setColor( const Color& color );
  const Color& getColor() { return color_; }

  // Overrides from VisualizerBase
  virtual void targetFrameChanged() {}
  virtual void fixedFrameChanged();
  virtual void createProperties();
  virtual void update( float dt );
  virtual bool isObjectPickable( const Ogre::MovableObject* object ) const { return true; }
  virtual void reset();

  static const char* getTypeStatic() { return "ParticleCloud2D"; }
  virtual const char* getType() { return getTypeStatic(); }
  static const char* getDescription();

protected:
  void subscribe();
  void unsubscribe();
  void clear();
  void incomingMessage();
  void processMessage();

  // overrides from VisualizerBase
  virtual void onEnable();
  virtual void onDisable();

  std::string topic_;
  Color color_;

#if 0
  typedef std::vector<ogre_tools::Arrow*> V_Arrow;
  V_Arrow arrows_;
  int arrow_count_;
#endif

  Ogre::SceneNode* scene_node_;
  Ogre::ManualObject* manual_object_;

  bool new_message_;
  std_msgs::ParticleCloud2D message_;

  ColorProperty* color_property_;
  ROSTopicStringProperty* topic_property_;
};

} // namespace ogre_vis

#endif /* OGRE_VISUALIZER_PARTICLE_CLOUD_2D_VISUALIZER_H_ */


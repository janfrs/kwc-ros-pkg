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

#ifndef OGRE_VISUALIZER_AXES_VISUALIZER_H
#define OGRE_VISUALIZER_AXES_VISUALIZER_H

#include "visualizer_base.h"

namespace ogre_tools
{
class Axes;
}

namespace ogre_vis
{

/**
 * \class AxesVisualizer
 * \brief Displays a set of XYZ axes at the origin
 */
class AxesVisualizer : public VisualizerBase
{
public:
  AxesVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name );
  virtual ~AxesVisualizer();

  /**
   * \brief Set the parameters for the axes
   * @param length Length of each axis
   * @param radius Radius of each axis
   */
  void set( float length, float radius );

  // Overrides from VisualizerBase
  virtual void fillPropertyGrid();
  virtual void propertyChanged( wxPropertyGridEvent& event );
  virtual void loadProperties( wxConfigBase* config );
  virtual void saveProperties( wxConfigBase* config );
  virtual void targetFrameChanged() {}

  static const char* getTypeStatic() { return "Axes"; }
  virtual const char* getType() { return getTypeStatic(); }

protected:
  /**
   * \brief Create the axes with the current parameters
   */
  void create();

  // overrides from VisualizerBase
  virtual void onEnable();
  virtual void onDisable();

  float length_;                ///< Length of each axis
  float radius_;                ///< Radius of each axis
  ogre_tools::Axes* axes_;      ///< Handles actually drawing the axes
};

} // namespace ogre_vis

 #endif

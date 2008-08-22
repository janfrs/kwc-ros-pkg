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

#ifndef OGRE_VISUALIZER_VISUALIZER_BASE
#define OGRE_VISUALIZER_VISUALIZER_BASE

#include <string>

namespace Ogre
{
class SceneManager;
}

namespace ros
{
class node;
}

class wxPanel;
class wxWindow;
class abstractFunctor;
class rosTFClient;
class wxPropertyGrid;
class wxPropertyGridEvent;

namespace ogre_vis
{

/** Abstract base class for all visualizers.  This provides a common interface for the visualization panel to interact with,
 * so that new visualizers can be added without the visualization panel knowing anything about them.
 * */
class VisualizerBase
{
public:
  VisualizerBase( Ogre::SceneManager* sceneManager, ros::node* node, rosTFClient* tfClient, const std::string& name, bool enabled = false );
  virtual ~VisualizerBase();

  /// Enable this visualizer
  void enable();
  /// Disable this visualizer
  void disable();

  bool isEnabled() { return enabled_; }
  const std::string& getName() { return name_; }

  /// Called periodically by the visualization panel
  virtual void update( float dt ) {}

  /// Called by the visualization panel to tell set our functor used for causing a render to happen
  void setRenderCallback( abstractFunctor* func );

  void setLockRenderCallback( abstractFunctor* func );
  void setUnlockRenderCallback( abstractFunctor* func );

  /// Override this to fill out the property grid when this visualizer is selected
  virtual void fillPropertyGrid( wxPropertyGrid* propertyGrid ) {} // default to no options

  /// Override this to handle a changing property value.  This provides the opportunity to veto a change if there is an invalid value
  /// event.Veto() will prevent the change.
  virtual void propertyChanging( wxPropertyGridEvent& event ) {}
  /// Override this to handle a changed property value
  virtual void propertyChanged( wxPropertyGridEvent& event ) {}


  void setTargetFrame( const std::string& frame ) { target_frame_ = frame; }

protected:
  /// Derived classes override this to do the actual work of enabling themselves
  virtual void onEnable() = 0;
  /// Derived classes override this to do the actual work of disabling themselves
  virtual void onDisable() = 0;

  /// Called by derived classes to cause the scene we're in to be rendered.
  void causeRender();

  void lockRender();
  void unlockRender();

  Ogre::SceneManager* scene_manager_;
  std::string name_;
  bool enabled_;

  std::string target_frame_;

  abstractFunctor* render_callback_;
  abstractFunctor* render_lock_;
  abstractFunctor* render_unlock_;

  ros::node* ros_node_;
  rosTFClient* tf_client_;

  friend class RenderAutoLock;
};

class RenderAutoLock
{
public:
  RenderAutoLock( VisualizerBase* visualizer )
  : visualizer_( visualizer )
  {
    visualizer_->lockRender();
  }

  ~RenderAutoLock()
  {
    visualizer_->unlockRender();
  }

private:
  VisualizerBase* visualizer_;
};

} // namespace ogre_vis

#endif

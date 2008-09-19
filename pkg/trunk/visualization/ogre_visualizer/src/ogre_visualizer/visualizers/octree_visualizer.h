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

/*
 * octree_visualizer.h
 *
 *  Created on: Aug 20, 2008
 *      Author: Matthew Piccoli and Matei Ciocarlie
 */

#ifndef OCTREE_VISUALIZER_H_
#define OCTREE_VISUALIZER_H_

#include "visualizer_base.h"
#include <scan_utils/OctreeMsg.h>

#include <Ogre.h>

namespace ogre_vis
{

/**
 * \class OctreeVisualizer
 * \brief Visualizes a scan_utils::Octree, using its triangulation
 */
class OctreeVisualizer : public VisualizerBase
{
public:
  OctreeVisualizer( Ogre::SceneManager* sceneManager, ros::node* node, rosTFClient* tfClient, const std::string& name );
  virtual ~OctreeVisualizer();

  /**
   * \brief Set the ROS topic to listen on for Octree messages
   * @param topic The ROS topic
   */
  void setOctreeTopic( const std::string& topic );
  /**
   * \brief Set the color to display as
   * @param r Red component, range [0,1]
   * @param g Green component, range [0,1]
   * @param b Blue component, range [0,1]
   */
  void setColor( float r, float g, float b );

  // Overrides from VisualizerBase
  virtual void fillPropertyGrid( wxPropertyGrid* property_grid );
  virtual void propertyChanged( wxPropertyGridEvent& event );
  virtual void loadProperties( wxConfigBase* config );
  virtual void saveProperties( wxConfigBase* config );

  virtual void update( float dt );

protected:
  // overrides from VisualizerBase
  virtual void onEnable();
  virtual void onDisable();

  /**
   * \brief Subscribes to the topic set by setOctreeTopic()
   */
  void subscribe();
  /**
   * \brief Unsubscribes from the topic set by setOctreeTopic()
   */
  void unsubscribe();

  /**
   * \brief ROS callback for an incoming octree message
   */
  void incomingOctreeCallback();

  float r_;                             ///< Color, red component, range [0,1]
  float g_;                             ///< Color, green component, range [0,1]
  float b_;                             ///< Color, blue component, range [0,1]

  Ogre::SceneNode* scene_node_;         ///< Scene node we're attached to
  Ogre::ManualObject* manual_object_;   ///< Manual object used to display the octree
  Ogre::MaterialPtr material_;          ///< Material created for this object
  std::string material_name_;           ///< Name of the material created for this object
  std::string octree_topic_;            ///< Topic to listen on

  scan_utils::OctreeMsg octree_message_;  ///< Octree message
  typedef std::vector<Ogre::Vector3> V_Vector3;

  ros::thread::mutex triangles_mutex_;  ///< Locks #vertices_ and #normals_
  V_Vector3 vertices_;                  ///< List of vertices spit out by the Octree's triangulator.  Every 3 vertices form a triangle.
                                        ///< Must lock #octree_message_ before accessing in any way.

  V_Vector3 normals_;                   ///< List of normals for each triangle -- this list is always 1/3 the size of #vertices_.
                                        ///< Must lock #octree_message_ before accessing in any way.

  bool new_message_;                    ///< Informs our update function that there is new data in #vertices_ and #normals_, so it can rebuild the ManualObject
};

}
#endif /* OCTREE_VISUALIZER_H_ */

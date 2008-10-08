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
 * octree_visualizer.cpp
 *
 *  Created on: Aug 20, 2008
 *      Author: Matthew Piccoli and Matei Ciocarlie
 */

#include "octree_visualizer.h"
#include "../ros_topic_property.h"

#include <dataTypes.h>
#include <ros/node.h>

#include <rosTF/rosTF.h>
#include <octree.h>

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/confbase.h>

#include "common.h"

#define COLOR_PROPERTY wxT("Color")
#define TOPIC_PROPERTY wxT("Topic")

namespace ogre_vis
{

OctreeVisualizer::OctreeVisualizer(Ogre::SceneManager* sceneManager,ros::node* node, rosTFClient* tfClient, const std::string& name)
: VisualizerBase(sceneManager, node, tfClient, name)
, r_( 1.0f )
, g_( 1.0f )
, b_( 1.0f )
, new_message_(false)
{
  static uint32_t count = 0;
  std::stringstream ss;
  ss << "Octree" << count++;

  manual_object_ = scene_manager_->createManualObject(ss.str());
  manual_object_->setDynamic( true );

  scene_node_ = scene_manager_->getRootSceneNode()->createChildSceneNode();
  scene_node_->attachObject(manual_object_);

  ss << "Material";
  material_name_ = ss.str();
  material_ = Ogre::MaterialManager::getSingleton().create( material_name_, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
  material_->setReceiveShadows(false);
  material_->getTechnique(0)->setLightingEnabled(true);
  setColor( r_, g_, b_ );
}

OctreeVisualizer::~OctreeVisualizer()
{
  scene_node_->detachAllObjects();
  scene_manager_->destroySceneNode(scene_node_->getName());
  scene_manager_->destroyManualObject(manual_object_);
}

void OctreeVisualizer::onEnable()
{
  manual_object_->setVisible(true);
  subscribe();
}

void OctreeVisualizer::onDisable()
{
  manual_object_->setVisible(false);
  unsubscribe();
}

void OctreeVisualizer::subscribe()
{
  if (!isEnabled())
  {
    return;
  }

  if (!octree_topic_.empty())
  {
    ros_node_->subscribe(octree_topic_, octree_message_, &OctreeVisualizer::incomingOctreeCallback, this, 10);
  }
}

void OctreeVisualizer::unsubscribe()
{
  if (!octree_topic_.empty())
  {
    ros_node_->unsubscribe(octree_topic_, &OctreeVisualizer::incomingOctreeCallback, this);
  }
}

void OctreeVisualizer::update(float dt)
{
  triangles_mutex_.lock();

  if (new_message_)
  {
    const size_t numVertices = vertices_.size();
    printf( "Received an octree with %d vertices\n", numVertices );

    manual_object_->clear();
    manual_object_->estimateVertexCount( numVertices );
    manual_object_->begin(material_name_, Ogre::RenderOperation::OT_TRIANGLE_LIST);

    size_t vertexIndex = 0;
    size_t normalIndex = 0;
    for ( ; vertexIndex < numVertices; )
    {
      Ogre::Vector3& v1 = vertices_[vertexIndex++];
      Ogre::Vector3& v2 = vertices_[vertexIndex++];
      Ogre::Vector3& v3 = vertices_[vertexIndex++];
      Ogre::Vector3& n = normals_[normalIndex++];

      manual_object_->position( v1 );
      manual_object_->normal( n );
      manual_object_->position( v2 );
      manual_object_->normal( n );
      manual_object_->position( v3 );
      manual_object_->normal( n );
    }

    manual_object_->end();

    new_message_ = false;

    causeRender();
  }

  triangles_mutex_.unlock();
}

void OctreeVisualizer::incomingOctreeCallback()
{
  scan_utils::Octree<char> octree(0, 0, 0, 0, 0, 0, 1, 0);
  octree.setFromMsg(octree_message_);

  std::list<scan_utils::Triangle> triangles;
  octree.getAllTriangles(triangles);

  V_Vector3 vertices;
  V_Vector3 normals;

  vertices.resize( triangles.size() * 3 );
  normals.resize( triangles.size() );
  size_t vertexIndex = 0;
  size_t normalIndex = 0;
  std::list<scan_utils::Triangle>::iterator it = triangles.begin();
  std::list<scan_utils::Triangle>::iterator end = triangles.end();
  for ( ; it != end; it++ )
  {
    Ogre::Vector3& v1 = vertices[vertexIndex++];
    Ogre::Vector3& v2 = vertices[vertexIndex++];
    Ogre::Vector3& v3 = vertices[vertexIndex++];
    Ogre::Vector3& n = normals[normalIndex++];

    v1 = Ogre::Vector3( it->p1.x, it->p1.y, it->p1.z );
    v2 = Ogre::Vector3( it->p2.x, it->p2.y, it->p2.z );
    v3 = Ogre::Vector3( it->p3.x, it->p3.y, it->p3.z );
    robotToOgre(v1);
    robotToOgre(v2);
    robotToOgre(v3);

    n = ( v2 - v1 ).crossProduct( v3 - v1 );
    n.normalise();
  }

  triangles_mutex_.lock();

  vertices_.clear();
  normals_.clear();

  vertices.swap( vertices_ );
  normals.swap( normals_ );

  new_message_ = true;
  triangles_mutex_.unlock();
}

void OctreeVisualizer::setOctreeTopic(const std::string& topic)
{
  unsubscribe();

  octree_topic_ = topic;

  subscribe();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + TOPIC_PROPERTY ), wxString::FromAscii( octree_topic_.c_str() ) );
  }
}

void OctreeVisualizer::setColor(float r, float g, float b)
{
  material_->setAmbient( r * 0.5, g * 0.5, b * 0.5 );
  material_->setDiffuse( r, g, b, 1.0f );

  r_ = r;
  g_ = g;
  b_ = b;

  if ( property_grid_ )
  {
    wxVariant color;
    color << wxColour( r_ * 255, g_ * 255, b_ * 255 );
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + COLOR_PROPERTY ), color );
  }
}

void OctreeVisualizer::fillPropertyGrid()
{
  property_grid_->Append( new ROSTopicProperty( ros_node_, TOPIC_PROPERTY, property_prefix_ + TOPIC_PROPERTY, wxString::FromAscii( octree_topic_.c_str() ) ) );
  property_grid_->Append( new wxColourProperty( COLOR_PROPERTY, property_prefix_ + COLOR_PROPERTY, wxColour( r_ * 255, g_ * 255, b_ * 255 ) ) );
}

void OctreeVisualizer::propertyChanged( wxPropertyGridEvent& event )
{
  wxPGProperty* property = event.GetProperty();

  const wxString& name = property->GetName();
  wxVariant value = property->GetValue();

  if ( name == property_prefix_ + TOPIC_PROPERTY )
  {
    wxString topic = value.GetString();
    setOctreeTopic( std::string(topic.fn_str()) );
  }
  else if ( name == property_prefix_ + COLOR_PROPERTY )
  {
    wxColour color;
    color << value;

    setColor( color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f );
  }
}

void OctreeVisualizer::loadProperties( wxConfigBase* config )
{
  wxString topic;
  double r, g, b;

  {
    config->Read( TOPIC_PROPERTY, &topic, wxString::FromAscii( octree_topic_.c_str() ) );
  }

  {
    config->Read( wxString(COLOR_PROPERTY) + wxT("R"), &r, r_ );
    config->Read( wxString(COLOR_PROPERTY) + wxT("G"), &g, g_ );
    config->Read( wxString(COLOR_PROPERTY) + wxT("B"), &b, b_ );
  }

  setOctreeTopic( (const char*)topic.fn_str() );
  setColor( r, g, b );
}

void OctreeVisualizer::saveProperties( wxConfigBase* config )
{
  config->Write( TOPIC_PROPERTY, wxString::FromAscii( octree_topic_.c_str() ) );

  config->Write( wxString(COLOR_PROPERTY) + wxT("R"), r_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("G"), g_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("B"), b_ );
}

void OctreeVisualizer::targetFrameChanged()
{
  printf( "Warning: octree is always in the global frame\n" );
}

}

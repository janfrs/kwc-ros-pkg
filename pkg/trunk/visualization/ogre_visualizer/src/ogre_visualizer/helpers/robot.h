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

#ifndef OGRE_VISUALIZER_ROBOT_H_
#define OGRE_VISUALIZER_ROBOT_H_

#include <ogre_tools/object.h>

#include <string>
#include <map>

#include <urdf/URDF.h>

#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreAny.h>

namespace Ogre
{
class SceneManager;
class Entity;
class SceneNode;
class Vector3;
class Quaternion;
class Any;
class RibbonTrail;
}

namespace ogre_tools
{
class Object;
}

namespace robot_desc
{
class URDF;
}

namespace planning_models
{
class KinematicModel;
}

class rosTFClient;

namespace ogre_vis
{

class PropertyManager;
class CategoryProperty;
class Vector3Property;
class QuaternionProperty;
class BoolProperty;

/**
 * \class Robot
 *
 * A helper class to draw a representation of a robot, as specified by a URDF.  Can display either the visual models of the robot,
 * or the collision models.
 */
class Robot : public ogre_tools::Object
{
public:
  Robot( Ogre::SceneManager* scene_manager, const std::string& name = "" );
  ~Robot();

  void setPropertyManager( PropertyManager* property_manager, CategoryProperty* parent );

  /**
   * \brief Loads meshes/primitives from a robot description.  Calls clear() before loading.
   *
   * @param urdf The robot description to read from
   * @param visual Whether or not to load the visual representation
   * @param collision Whether or not to load the collision representation
   */
  void load( robot_desc::URDF* urdf, bool visual = true, bool collision = true );
  /**
   * \brief Clears all data loaded from a URDF
   */
  void clear();

  /**
   * \brief Updates positions/orientations from a rosTF client
   *
   * @param tf_client The rosTF client to load from
   * @param target_frame The frame to transform into
   */
  void update( rosTFClient* tf_client, const std::string& target_frame );

  /**
   * \brief Updates positions/orientations from a kinematic planning model
   *
   * @param kinematic_model The model to load from
   * @param target_frame The frame to transform into
   */
  void update( planning_models::KinematicModel* kinematic_model, const std::string& target_frame );

  /**
   * \brief Set the robot as a whole to be visible or not
   * @param visible Should we be visible?
   */
  void setVisible( bool visible );

  /**
   * \brief Set whether the visual meshes of the robot should be visible
   * @param visible Whether the visual meshes of the robot should be visible
   */
  void setVisualVisible( bool visible );

  /**
   * \brief Set whether the collision meshes/primitives of the robot should be visible
   * @param visible Whether the collision meshes/primitives should be visible
   */
  void setCollisionVisible( bool visible );

  /**
   * \brief Returns whether or not the visual representation is set to be visible
   */
  bool isVisualVisible();
  /**
   * \brief Returns whether or not the collision representation is set to be visible
   */
  bool isCollisionVisible();

  struct LinkInfo;
  bool isShowingTrail( const LinkInfo* info );
  void setShowTrail( LinkInfo* info, bool show );

  LinkInfo* getLinkInfo( const std::string& name );

  // Overrides from ogre_tools::Object
  virtual void setPosition( const Ogre::Vector3& position );
  virtual void setOrientation( const Ogre::Quaternion& orientation );
  virtual void setScale( const Ogre::Vector3& scale );
  virtual void setColor( float r, float g, float b, float a );
  virtual void setUserData( const Ogre::Any& user_data );
  virtual const Ogre::Vector3& getPosition();
  virtual const Ogre::Quaternion& getOrientation();

  /**
   * \struct LinkInfo
   * \brief Contains any data we need from a link in the robot.
   */
  struct LinkInfo
  {
    LinkInfo()
    : visual_mesh_( NULL )
    , collision_mesh_( NULL )
    , collision_object_( NULL )
    , visual_node_( NULL )
    , collision_node_( NULL )
    , position_property_( NULL )
    , orientation_property_( NULL )
    , trail_( NULL )
    , trail_property_( NULL )
    {}

    std::string name_;                          ///< Name of this link
    std::string material_name_;                 ///< Name of the ogre material used by the meshes in this link

    Ogre::Entity* visual_mesh_;                 ///< The entity representing the visual mesh of this link (if it exists)

    Ogre::Entity* collision_mesh_;              ///< The entity representing the collision mesh of this link (if it exists)
    ogre_tools::Object* collision_object_;      ///< The object representing the collision primitive of this link (if it exists)

    Ogre::SceneNode* visual_node_;              ///< The scene node the visual mesh is attached to
    Ogre::SceneNode* collision_node_;           ///< The scene node the collision mesh/primitive is attached to

    Ogre::Vector3 collision_offset_position_;   ///< Collision origin offset position
    Ogre::Quaternion collision_offset_orientation_; ///< Collision origin offset orientation

    Ogre::Vector3 position_;
    Ogre::Quaternion orientation_;

    Vector3Property* position_property_;
    QuaternionProperty* orientation_property_;

    Ogre::RibbonTrail* trail_;
    BoolProperty* trail_property_;
  };

protected:

  void createVisualForLink( LinkInfo* info, robot_desc::URDF::Link* link );
  void createCollisionForLink( LinkInfo* info, robot_desc::URDF::Link* link );
  void createPropertiesForLink( LinkInfo* info );

  Ogre::Vector3 getPositionForLinkInRobotFrame( const LinkInfo* info );
  Ogre::Quaternion getOrientationForLinkInRobotFrame( const LinkInfo* info );


  typedef std::map< std::string, LinkInfo* > M_NameToLinkInfo;
  M_NameToLinkInfo links_;                      ///< Map of name to link info, stores all loaded links.

  Ogre::SceneNode* root_visual_node_;           ///< Node all our visual nodes are children of
  Ogre::SceneNode* root_collision_node_;        ///< Node all our collision nodes are children of
  Ogre::SceneNode* root_other_node_;

  bool visual_visible_;                         ///< Should we show the visual representation?
  bool collision_visible_;                      ///< Should we show the collision representation?

  PropertyManager* property_manager_;
  CategoryProperty* parent_property_;
  CategoryProperty* links_category_;

  Ogre::Any user_data_;

  std::string name_;
};

} // namespace ogre_vis

#endif /* OGRE_VISUALIZER_ROBOT_H_ */

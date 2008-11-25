/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: Bumper Controller
 * Author: Nate Koenig mod by John Hsu
 * Date: 24 Sept 2008
 */
#ifndef ROS_BUMPER_CONTROLLER_HH
#define ROS_BUMPER_CONTROLLER_HH

#include <sys/time.h>

#include "Controller.hh"
#include "Entity.hh"

namespace gazebo
{
  class ContactSensor;

  /// \addtogroup gazebo_dynamic_plugins Gazebo ROS Dynamic Plugins
  /// \{
  /** \defgroup Ros_Bumper Ros Bumper Plugin
  
    \brief A controller that returns bump contacts

  \verbatim
    <model:physical name="camera_model">
      <body:empty name="camera_body_name">
          <sensor:contact name="finger_tip_l_left_contact_sensor">
          <updateRate>15.0</updateRate>
          <geom>pr2_finger_tip_l_collision_geom</geom>
          <controller:ros_bumper name="finger_tip_l_contact_controller" plugin="libRos_Bumper.so">
            <alwaysOn>true</alwaysOn>
            <updateRate>15.0</updateRate>
            <topicName>finger_tip_l_contact</topicName>
            <frameName>finger_tip_l_contact</frameName>
            <interface:bumper name="dummy_bumper_iface" />
          </controller:ros_bumper>
        </sensor:contact>
      </body:empty>
    </model:phyiscal>
  \endverbatim

    \{
  */
  
  /// \brief A Bumper controller
  class Ros_Bumper : public Controller
  {
    /// Constructor
      public: Ros_Bumper(Entity *parent );
  
    /// Destructor
      public: virtual ~Ros_Bumper();
  
    /// Load the controller
    /// \param node XML config node
    protected: virtual void LoadChild(XMLConfigNode *node);
  
    /// Init the controller
    protected: virtual void InitChild();
  
    /// Update the controller
    protected: virtual void UpdateChild();
  
    /// Finalize the controller
    protected: virtual void FiniChild();
  
    /// The parent Model
    private: ContactSensor *myParent;
  
  };
  
  /** \} */
  /// \}

}

#endif


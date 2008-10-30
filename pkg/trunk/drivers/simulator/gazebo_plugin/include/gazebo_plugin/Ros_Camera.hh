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
 * Desc: A dynamic controller plugin that publishes ROS image topic for generic camera sensor.
 * Author: John Hsu
 * Date: 24 Sept 2008
 * SVN: $Id$
 */
#ifndef ROS_CAMERA_HH
#define ROS_CAMERA_HH

#include <ros/node.h>
#include <std_msgs/Image.h>
#include <gazebo/Controller.hh>

namespace gazebo
{
  class MonoCameraSensor;

/// @addtogroup gazebo_dynamic_plugins Gazebo ROS Dynamic Plugins
/// @{
/** \defgroup Ros_Camera Ros Camera Plugin XML Reference and Example

  \brief Ros Camera Plugin Controller.
  
  This is a controller that collects data from a Camera Sensor and populates a libgazebo camera interface as well as publish a ROS std_msgs::Image (under the field \b \<topicName\>). This controller should only be used as a child of a camera sensor (see example below.

  Example Usage:
  \verbatim
  <model:physical name="camera_model">
    <body:empty name="camera_body_name">
      <sensor:camera name="camera_sensor">
        <controller:ros_camera name="controller-name" plugin="libRos_Camera.so">
            <alwaysOn>true</alwaysOn>
            <updateRate>15.0</updateRate>
            <topicName>camera_name/image</topicName>
            <frameName>camera_body_name</frameName>
        </controller:ros_camera>
      </sensor:camera>
    </body:empty>
  </model:phyiscal>
  \endverbatim
 
\{
*/

/**


    \brief Ros_Camera Controller.
           \li Starts a ROS node if none exists. \n
           \li Simulates a generic camera and broadcast std_msgs::Image topic over ROS.
           \li Example Usage:
  \verbatim
  <model:physical name="camera_model">
    <body:empty name="camera_body_name">
      <sensor:camera name="camera_sensor">
        <controller:ros_camera name="controller-name" plugin="libRos_Camera.so">
            <alwaysOn>true</alwaysOn>
            <updateRate>15.0</updateRate>
            <topicName>camera_name/image</topicName>
            <frameName>camera_body_name</frameName>
        </controller:ros_camera>
      </sensor:camera>
    </body:empty>
  </model:phyiscal>
  \endverbatim
           .
 
*/

class Ros_Camera : public Controller
{
  /// \brief Constructor
  /// \param parent The parent entity, must be a Model or a Sensor
  public: Ros_Camera(Entity *parent);

  /// \brief Destructor
  public: virtual ~Ros_Camera();

  /// \brief Load the controller
  /// \param node XML config node
  protected: virtual void LoadChild(XMLConfigNode *node);

  /// \brief Init the controller
  protected: virtual void InitChild();

  /// \brief Update the controller
  protected: virtual void UpdateChild();

  /// \brief Finalize the controller, unadvertise topics
  protected: virtual void FiniChild();

  /// \brief Put camera data to the ROS topic
  private: void PutCameraData();

  /// \brief A pointer to the parent camera sensor
  private: MonoCameraSensor *myParent;

  /// \brief A pointer to the ROS node.  A node will be instantiated if it does not exist.
  private: ros::node *rosnode;

  /// \brief ROS image message
  private: std_msgs::Image imageMsg;

  /// \brief ROS image topic name
  private: std::string topicName;

  /// \brief ROS frame transform name to use in the image message header.
  ///        This should typically match the link name the sensor is attached.
  private: std::string frameName;

  /// \brief A mutex to lock access to fields that are used in ROS message callbacks
  private: ros::thread::mutex lock;

  /// \brief size of image buffer
  private: uint32_t buf_size;
};

/** \} */
/// @}

}
#endif


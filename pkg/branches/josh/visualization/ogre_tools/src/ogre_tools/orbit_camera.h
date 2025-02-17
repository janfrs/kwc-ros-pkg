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

#ifndef OGRE_TOOLS_ORBIT_CAMERA_H_
#define OGRE_TOOLS_ORBIT_CAMERA_H_

#include "camera_base.h"
#include <OgreVector3.h>

namespace Ogre
{
  class Camera;
  class SceneNode;
  class SceneManager;
}

namespace ogre_tools
{

class SuperEllipsoid;

/**
 * \brief An orbital camera, controlled by yaw, pitch, distance, and focal point
 *
 * This camera is based on the equation of a sphere in spherical coordinates:
 @verbatim
 x = d*cos(theta)sin(phi)
 y = d*cos(phi)
 z = d*sin(theta)sin(phi)
 @endverbatim
 * Where:<br>
 * d = #distance_<br>
 * theta = #yaw_<br>
 * phi = #pitch_
 */
class OrbitCamera : public CameraBase
{
public:
  OrbitCamera( Ogre::SceneManager* scene_manager );
  virtual ~OrbitCamera();

  /**
   * \brief Move in/out from the focal point, ie. adjust #distance_ by amount
   * @param amount The distance to move.  Positive amount moves towards the focal point, negative moves away
   */
  void zoom( float amount );
  /**
   * \brief Set the focal point of the camera.  Keeps the pitch/yaw/distance the same
   * @param focal_point The new focal point
   */
  void setFocalPoint( const Ogre::Vector3& focal_point );

  virtual void setFrom( CameraBase* camera );
  virtual void yaw( float angle );
  virtual void pitch( float angle );
  virtual void roll( float angle );
  virtual void setOrientation( float x, float y, float z, float w );
  virtual void setPosition( float x, float y, float z );
  virtual void move( float x, float y, float z );

  virtual Ogre::Vector3 getPosition();
  virtual Ogre::Quaternion getOrientation();

  virtual void lookAt( const Ogre::Vector3& point );

  virtual void mouseLeftDrag( int diff_x, int diff_y );
  virtual void mouseMiddleDrag( int diff_x, int diff_y );
  virtual void mouseRightDrag( int diff_x, int diff_y );
  virtual void scrollWheel( int diff );

  /**
   * \brief Calculates the camera's position and orientation from the yaw, pitch, distance and focal point
   */
  virtual void update();

  virtual void mouseLeftDown( int x, int y );
  virtual void mouseMiddleDown( int x, int y );
  virtual void mouseRightDown( int x, int y );
  virtual void mouseLeftUp( int x, int y );
  virtual void mouseMiddleUp( int x, int y );
  virtual void mouseRightUp( int x, int y );

private:

  /**
   * \brief Calculates pitch and yaw values given a new position and the current focal point
   * @param position Position to calculate the pitch/yaw for
   */
  void calculatePitchYawFromPosition( const Ogre::Vector3& position );
  /**
   * \brief Normalizes the camera's pitch, preventing it from reaching vertical (or turning upside down)
   */
  void normalizePitch();
  /**
   * \brief Normalizes the camera's yaw in the range [0, 2*pi)
   */
  void normalizeYaw();

  Ogre::Vector3 focal_point_;         ///< The camera's focal point
  float yaw_;                         ///< The camera's yaw (rotation around the y-axis), in radians
  float pitch_;                       ///< The camera's pitch (rotation around the x-axis), in radians
  float distance_;                    ///< The camera's distance from the focal point

  SuperEllipsoid* focal_point_object_;
};

} // namespace ogre_tools

#endif /*OGRE_TOOLS_ORBIT_CAMERA_H_*/

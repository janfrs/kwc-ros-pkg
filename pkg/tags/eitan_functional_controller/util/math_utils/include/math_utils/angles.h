/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#ifndef MATH_UTILS_ANGLES
#define MATH_UTILS_ANGLES

#include <cmath>


namespace math_utils
{
    
  /** Convert degrees to radians */
    
  static inline double from_degrees(double degrees)
  {
    return degrees * M_PI / 180.0;   
  }
    
  /** Convert radians to degrees */
  static inline double to_degrees(double radians)
  {
    return radians * 180.0 / M_PI;
  }
    

  /*
   * normalize_angle_positive
   *
   * Normalizes the angle to be 0 to 2*M_PI
   * It takes and returns native units.
   */
  static inline double normalize_angle_positive(double angle)
  {
    return fmod(fmod(angle, 2.0*M_PI) + 2.0*M_PI, 2.0*M_PI);
  }


  /*
   * normalize
   *
   * Normalizes the angle to be -M_PI circle to +M_PI circle
   * It takes and returns native units.
   *
   */    
  static inline double normalize_angle(double angle)
  {
    double a = normalize_angle_positive(angle);
    if (a > M_PI)
      a -= 2.0 *M_PI;
    return a;
  }

    
  /*
   * shortest_angular_distance
   *
   * Given 2 angles, this returns the shortest angular
   * difference.  The inputs and ouputs are of course native
   * units.
   *
   * As an example, if native units are degrees, the result
   * would always be -180 <= result <= 180.  Adding the result
   * to "from" will always get you an equivelent angle to "to".
   */
    
  static inline double shortest_angular_distance(double from, double to)
  {
    double result = normalize_angle_positive(normalize_angle_positive(to) - normalize_angle_positive(from));
	
    if (result > M_PI)
      // If the result > 180,
      // It's shorter the other way.
      result = -(2.0*M_PI - result);
	
    return normalize_angle(result);
  }

  /*!
   * \function
   *
   * \brief returns the angle in [0, 2*M_PI]  going the other way along the unit circle. 
   * E.g. add_mod_2Pi(-M_PI/4) returns 7_M_PI/4
   * add_mod_2Pi(M_PI/4) returns -7*M_PI/4
   *
   */
  static inline double add_mod_2Pi(double angle)
  {
    if(angle < 0)
      return (2*M_PI+angle);
    else
      return (-2*M_PI+angle);
  }

  /*! 
   * \function
   *
   * \brief Returns the min and max amount (in radians) that can be moved from "from" angle to "left_limit" and "right_limit".
   * \return returns false if "from" angle does not lie in the interval [left_limit,right_limit]
   * \param from - "from" angle
   * \param left_limit - left limit of valid interval for angular position
   * \param right_limit - right limit of valid interval for angular position
   * \param min_delta - minimum (delta) angle (in radians) that can be moved from "from" position before hitting the joint stop
   * \param max_delta - maximum (delta) angle (in radians) that can be movedd from "from" position before hitting the joint stop
   */
  static bool find_min_max_delta(double from, double min_angle, double max_angle, double &result_min_delta, double &result_max_delta)
  {
    double delta[4];

    delta[0] = shortest_angular_distance(from,min_angle);
    delta[1] = shortest_angular_distance(from,max_angle);

    delta[2] = add_mod_2Pi(delta[0]);
    delta[3] = add_mod_2Pi(delta[1]);

    double delta_min = delta[0];
    double delta_min_2pi = delta[2];
    if(delta[2] < delta_min)
    {
      delta_min = delta[2];
      delta_min_2pi = delta[0];
    }

    double delta_max = delta[1];
    double delta_max_2pi = delta[3];
    if(delta[3] > delta_max)
    {
      delta_max = delta[3];
      delta_max_2pi = delta[1];
    }


    if((delta_min <= delta_max_2pi) || (delta_max >= delta_min_2pi))
    {
      result_min_delta = delta_max_2pi;
      result_max_delta = delta_min_2pi;
      return false;
    }
//      printf("%f %f %f %f\n",delta_min,delta_min_2pi,delta_max,delta_max_2pi);
    result_min_delta = delta_min;
    result_max_delta = delta_max;
    return true;
  }


  /*!
   * \function 
   *
   * \brief Returns the delta from "from_angle" to "to_angle" making sure it does not violate limits specified by left_limit and right_limit.
   * The valid interval of angular positions is [left_limit,right_limit]. E.g., [-0.25,0.25] is a 0.5 radians wide interval that contains 0. 
   * But [0.25,-0.25] is a 2*M_PI-0.5 wide interval that contains M_PI (but not 0). 
   * The value of shortest_angle is the angular difference between "from" and "to" that lies within the defined valid interval. 
   * E.g. shortest_angular_distance_with_limits(-0.5,0.5,0.25,-0.25,ss) evaluates ss to 2*M_PI-1.0 and returns true while
   * shortest_angular_distance_with_limits(-0.5,0.5,-0.25,0.25,ss) returns false since -0.5 and 0.5 do not lie in the interval [-0.25,0.25]
   *
   * \return true if "from" and "to" positions are within the limit interval, false otherwise
   * \param from - "from" angle
   * \param to - "to" angle
   * \param left_limit - left limit of valid interval for angular position
   * \param right_limit - right limit of valid interval for angular position
   * \param shortest_angle - result of the shortest angle calculation
   */
  static inline bool shortest_angular_distance_with_limits(double from, double to, double left_limit, double right_limit, double &shortest_angle)
  {

    double min_delta = -2*M_PI;
    double max_delta = 2*M_PI;
    double min_delta_to = -2*M_PI;
    double max_delta_to = 2*M_PI;
    bool flag    = find_min_max_delta(from,left_limit,right_limit,min_delta,max_delta);
    double delta = shortest_angular_distance(from,to);
    double delta_mod_2pi  = add_mod_2Pi(delta);


    if(flag)//from position is within the limits
    {
      if(delta >= min_delta && delta <= max_delta)
      {
        shortest_angle = delta;
        return true;
      }
      else if(delta_mod_2pi >= min_delta && delta_mod_2pi <= max_delta)
      {
        shortest_angle = delta_mod_2pi;
        return true;
      }
      else //to position is outside the limits
      {
        find_min_max_delta(to,left_limit,right_limit,min_delta_to,max_delta_to);
          if(fabs(min_delta_to) < fabs(max_delta_to))
            shortest_angle = std::max<double>(delta,delta_mod_2pi);
          else if(fabs(min_delta_to) > fabs(max_delta_to))
            shortest_angle =  std::min<double>(delta,delta_mod_2pi);
          else
          {
            if (fabs(delta) < fabs(delta_mod_2pi))
              shortest_angle = delta;
            else
              shortest_angle = delta_mod_2pi;
          }
          return false;
      }
    }
    else // from position is outside the limits
    {
        find_min_max_delta(to,left_limit,right_limit,min_delta_to,max_delta_to);

          if(fabs(min_delta) < fabs(max_delta))
            shortest_angle = std::min<double>(delta,delta_mod_2pi);
          else if (fabs(min_delta) > fabs(max_delta))
            shortest_angle =  std::max<double>(delta,delta_mod_2pi);
          else
          {
            if (fabs(delta) < fabs(delta_mod_2pi))
              shortest_angle = delta;
            else
              shortest_angle = delta_mod_2pi;
          }
      return false;
    }

    shortest_angle = delta;
    return false;
  }


    

  /*
   * modNPiBy2
   *
   * Returns the angle between -M_PI/2 to M_PI/2
   */
  static inline double modNPiBy2(double angle)
  {
    if (angle < -M_PI/2) 
      angle += M_PI;
    if(angle > M_PI/2)
      angle -= M_PI;
    return angle;
  }
}

#endif

//Software License Agreement (BSD License)

//Copyright (c) 2008, Willow Garage, Inc.
//All rights reserved.

//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:

// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
//   copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided
//   with the distribution.
// * Neither the name of the Willow Garage nor the names of its
//   contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.

//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.

#ifndef LIBTF_HH
#define LIBTF_HH
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <newmat10/newmat.h>
#include <newmat10/newmatio.h>
#include <cmath>
#include <vector>
#include <sstream>

#include "libTF/Pose3DCache.h"

namespace libTF
{

/** ** Point ****
 *  \brief A simple point class incorperating the time and frameID
 * 
 * This is a point class designed to interact with libTF.  It 
 * incorperates the timestamp and associated frame to make 
 * association easier for the programmer.    
 */
 struct TFPoint 
{
  double x,y,z;
  unsigned long long time;
  unsigned int frame;
 };

/** ** Point2D ****
 *  \brief A simple point class incorperating the time and frameID
 * 
 * This is a point class designed to interact with libTF.  It 
 * incorperates the timestamp and associated frame to make 
 * association easier for the programmer.    
 */
struct TFPoint2D
{
  double x,y;
  unsigned long long time;
  unsigned int frame;
};


/** TFVector
 *  \brief A representation of a vector
 */
struct TFVector
{
  double x,y,z;
  unsigned long long time;
  unsigned int frame;
};

/** TFVector2D
 *  \brief A representation of a 2D vector
 * 
 */
struct TFVector2D
{
  double x,y;
  unsigned long long time;
  unsigned int frame;
};

/** TFEulerYPR
 * \brief A representation of Euler angles
 * Using Yaw, Pitch, Roll
 * commonly known as xyz Euler angles */
struct TFEulerYPR
{
  double yaw, pitch, roll;
  unsigned long long time;
  unsigned int frame;
};

/** TFYaw
 * \brief Rotation about the Z axis.  
 */
struct TFYaw
{
  double yaw;
  unsigned long long time;
  unsigned int frame;
};

/** TFPose
 *  \brief A representation of position in free space
 */
struct TFPose
{
  double x,y,z,yaw,pitch,roll;
  unsigned long long time;
  unsigned int frame;
};

/** TFPose2D
 *  \brief A representation of 2D position
 */
struct TFPose2D
{
  double x,y,yaw;
  unsigned long long time;
  unsigned int frame;
};


/** \brief A Class which provides coordinate transforms between any two frames in a system. 
 * 
 * This class provides a simple interface to allow recording and lookup of 
 * relationships between arbitrary frames of the system.
 * 
 * libTF assumes that there is a tree of coordinate frame transforms which define the relationship between all coordinate frames.  
 * For example your typical robot would have a transform from global to real world.  And then from base to hand, and from base to head.  
 * But Base to Hand really is composed of base to shoulder to elbow to wrist to hand.  
 * libTF is designed to take care of all the intermediate steps for you.  
 * 
 * Internal Representation 
 * libTF will store frames with the parameters necessary for generating the transform into that frame from it's parent and a reference to the parent frame.
 * Frames are designated using an unsigned int
 * 0 is a frame without a parent (the top of a tree)
 * The positions of frames over time must be pushed in.  
 * 
 * All function calls which pass frame ids can potentially throw the exception TransformReference::LookupException
 */

class TransformReference
{
public:
  /// All time within libTF is this format.
  typedef unsigned long long ULLtime;
  
  /************* Constants ***********************/
  static const unsigned int NO_PARENT = 0;  //!< The value indicating no parent frame (The top of a tree)
  static const unsigned int MAX_NUM_FRAMES = 10000; //!< The max value of frameID (due to preallocation of pointers)
  static const unsigned int MAX_GRAPH_DEPTH = 100;   //!< The maximum number of time to recurse before assuming the tree has a loop.
  static const ULLtime DEFAULT_CACHE_TIME = 10 * 1000000000ULL;  //!< The default amount of time to cache data
  static const ULLtime DEFAULT_MAX_EXTRAPOLATION_DISTANCE = 10 * 1000000000ULL; //!< The default amount of time to extrapolate


  /** Constructor 
   * \param interpolating Whether to interpolate, if this is false the closest value will be returned
   * \param cache_time How long to keep a history of transforms in nanoseconds
   * \param max_extrapolation_distance How far to extrapolate before throwing an exception
   */
  TransformReference(bool interpolating = true, 
                     ULLtime cache_time = DEFAULT_CACHE_TIME,
                     unsigned long long max_extrapolation_distance = DEFAULT_MAX_EXTRAPOLATION_DISTANCE);
  virtual ~TransformReference(void);

  /********** Mutators **************/
  /** \brief Add a new frame and parent
   * The frame transformatiosn are left unspecified.
   * \param frameid The id of a frame to add
   * \param parentid The id of the parent frame to the one being added
   *
   *  Possible exceptions are: TransformReference::InvaildFrame
   */
  void addFrame(unsigned int frameid, unsigned int parentid);

  /** \brief Set a new frame or update an old one.
   * \param frameid The destination frame
   * \param parentid The frame id of the parent frame.  
   * \param x Translation forward
   * \param y Translation left
   * \param z Translation up
   * \param yaw Rotation about Z
   * \param pitch Rotation about Y
   * \param roll Rotation about X
   * \param time The tiem at which to set the transform
   *
   * These euler angles are applied in the order top to bottom to the parent coordinate frame to 
   * transform it to the coordinate frame, frameid.  
   * 
   *  Possible exceptions are: TransformReference::LookupException
   */
  void setWithEulers(unsigned int frameid, unsigned int parentid, double x, double y, double z, double yaw, double pitch, double roll, ULLtime time);

  /** \brief Set a transform using DH Parameters 
   * Conventions from http://en.wikipedia.org/wiki/Robotics_conventions 
   *  Possible exceptions are: TransformReference::LookupException
   */
  void setWithDH(unsigned int frameid, unsigned int parentid, double length, double alpha, double offset, double theta, ULLtime time);

  /** \brief Set the transform using a matrix 
   *  Possible exceptions are: TransformReference::LookupException
   */
  void setWithMatrix(unsigned int frameid, unsigned int parentid, const NEWMAT::Matrix & matrix_in, ULLtime time);
  /** \brief Set the transform using quaternions natively 
   *  Possible exceptions are: TransformReference::LookupException
   */
  void setWithQuaternion(unsigned int frameid, unsigned int parentid, double xt, double yt, double zt, double xr, double yr, double zr, double w, ULLtime time);
  

  /*********** Accessors *************/

  /** \brief Get the transform between two frames by frame ID.  
   * \param target_frame The frame to which data should be transformed
   * \param source_frame The frame where the data originated
   * \param time The time at which the value of the transform is desired. (0 will get the latest)
   * 
   * Possible exceptions TransformReference::LookupException, TransformReference::ConnectivityException, 
   * TransformReference::MaxDepthException
   */
  NEWMAT::Matrix getMatrix(unsigned int target_frame, unsigned int source_frame, ULLtime time);


  /** \brief Transform a point to a different frame */
  TFPoint transformPoint(unsigned int target_frame, const TFPoint & point_in);
  /** \brief Transform a 2D point to a different frame */
  TFPoint2D transformPoint2D(unsigned int target_frame, const TFPoint2D & point_in);
  /** \brief Transform a vector to a different frame */
  TFVector transformVector(unsigned int target_frame, const TFVector & vector_in);
  /** \brief Transform a 2D vector to a different frame */
  TFVector2D transformVector2D(unsigned int target_frame, const TFVector2D & vector_in);
  /** \brief Transform Euler angles between frames */
  TFEulerYPR transformEulerYPR(unsigned int target_frame, const TFEulerYPR & euler_in);
  /** \brief Transform Yaw between frames. Useful for 2D navigation */
  TFYaw transformYaw(unsigned int target_frame, const TFYaw & euler_in);
  /** \brief Transform a 6DOF pose.  (x, y, z, yaw, pitch, roll). */
  TFPose transformPose(unsigned int target_frame, const TFPose & pose_in);
  /** \brief Transform a planar pose, x,y,yaw */
  TFPose2D transformPose2D(unsigned int target_frame, const TFPose2D & pose_in);

  /** \brief Debugging function that will print the spanning chain of transforms.
   * Possible exceptions TransformReference::LookupException, TransformReference::ConnectivityException, 
   * TransformReference::MaxDepthException
   */
  std::string viewChain(unsigned int target_frame, unsigned int source_frame);

  /** \brief A way to see what frames have been cached 
   * Useful for debugging 
   */
  std::string viewFrames();

  /************ Possible Exceptions ****************************/

  /** \brief An exception class to notify of bad frame number 
   * 
   * This is an exception class to be thrown in the case that 
   * a frame not in the graph has been attempted to be accessed.
   * The most common reason for this is that the frame is not
   * being published, or a parent frame was not set correctly 
   * causing the tree to be broken.  
   */
  class LookupException : public std::exception
  {
  public:
    LookupException(const std::string errorDescription) { 
      errorDescription_ = new std::string(errorDescription);
    };
    std::string * errorDescription_;
    ~LookupException() throw() { delete errorDescription_; };
    virtual const char* what() const throw()    { return errorDescription_->c_str(); };
  };

  /** \brief An exception class to notify of no connection
   * 
   * This is an exception class to be thrown in the case 
   * that the Reference Frame tree is not connected between
   * the frames requested. */
  class ConnectivityException : public std::runtime_error
  {
  public:
    ConnectivityException(const std::string errorDescription) : std::runtime_error(errorDescription) { ; };
  };
  
  /** \brief An exception class to notify that the search for connectivity descended too deep. 
   * 
   * This is an exception class which will be thrown if the tree search 
   * recurses too many times.  This is to prevent the search from 
   * infinitely looping in the case that a tree was malformed and 
   * became cyclic.
   */
  class MaxDepthException : public std::runtime_error
  {
  public:
    MaxDepthException(const std::string errorDescription): std::runtime_error(errorDescription) { ; };
  };
  
  /** \brief An exception class to notify that the requested value would have required extrapolation, and extrapolation is not allowed.
   * 
   */
  class ExtrapolateException : public std::runtime_error
  { 
  public:
    ExtrapolateException(const std::string &errorDescription): std::runtime_error(errorDescription) { ; };
  };

private:

  /** \brief The internal storage class for ReferenceTransform.  
   * 
   * An instance of this class is created for each frame in the system.
   * This class natively handles the relationship between frames.  
   *
   * The derived class Pose3DCache provides a buffered history of positions
   * with interpolation.
   * 
   */
  
  class RefFrame: public Pose3DCache 
    {
    public:

      /** Constructor */
      RefFrame(bool interpolating = true,  
               unsigned long long  max_cache_time = DEFAULT_MAX_STORAGE_TIME,
               unsigned long long  max_extrapolation_time = DEFAULT_MAX_EXTRAPOLATION_TIME); 
      
      /** \brief Get the parent nodeID */
      inline unsigned int getParent(){return parent;};
      
      /** \brief Set the parent node 
       * return: false => change of parent, cleared history
       * return: true => no change of parent 
       * \param parentID The frameID of the parent
       */
      bool setParent(unsigned int parentID);

    private:
      
      /** Internal storage of the parent */
      unsigned int parent;

    };

  /******************** Internal Storage ****************/

  /** \brief The pointers to potential frames that the tree can be made of.
   * The frames will be dynamically allocated at run time when set the first time. */
  RefFrame** frames;

  /// How long to cache transform history
  ULLtime cache_time;

  /// whether or not to interpolate or extrapolate
  bool interpolating;
  
  /// whether or not to allow extrapolation
  unsigned long long max_extrapolation_distance;

 public:
  /** \brief An internal representation of transform chains
   * 
   * This struct is how the list of transforms are stored before being passed to computeTransformFromList. */
  typedef struct 
  {
    std::vector<unsigned int> inverseTransforms;
    std::vector<unsigned int> forwardTransforms;
  } TransformLists;

 private: 
  /************************* Internal Functions ****************************/
  
  /** \brief An accessor to get a frame, which will throw an exception if the frame is no there. 
   * \param frame_number The frameID of the desired Reference Frame
   * 
   * This is an internal function which will get the pointer to the frame associated with the frame id
   * Possible Exception: TransformReference::LookupException
   */
  inline RefFrame* getFrame(unsigned int frame_number) { if (frames[frame_number] == NULL) { std::stringstream ss; ss << "Frame " << frame_number << " does not exist."; throw LookupException(ss.str());} else return frames[frame_number];};

  /** Find the list of connected frames necessary to connect two different frames */
  TransformLists  lookUpList(unsigned int target_frame, unsigned int source_frame);
  
  /** Compute the transform based on the list of frames */
  NEWMAT::Matrix computeTransformFromList(const TransformLists & list, ULLtime time);

};
}
#endif //LIBTF_HH

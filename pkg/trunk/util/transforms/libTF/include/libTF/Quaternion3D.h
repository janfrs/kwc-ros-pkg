// Software License Agreement (BSD License)
//
// Copyright (c) 2008, Willow Garage, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//  * Neither the name of the Willow Garage nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#ifndef QUATERNION3D_HH
#define QUATERNION3D_HH

#include <iostream>
#include <newmat10/newmat.h>
#include <newmat10/newmatio.h>
#include <math.h>
#include <pthread.h>


namespace libTF{


struct Position
{
  double x,y,z;
};

struct Quaternion
{
  double x,y,z,w;
};


class Euler3D {
public:
  //Constructor
  Euler3D();
  Euler3D(double _x, double _y, double _z, double _yaw, double _pitch, double _roll);

  //Storage
  double  x,y,z,yaw,pitch,roll;
};


class Pose3D 
{
 public:
  /* Constructors */
  ///Empty Constructor initialize to zero
  Pose3D();
  ///Translation only constructor
  Pose3D(double xt, double yt, double zt); 
  /// Quaternion only constructor
  Pose3D(double xr, double yt, double zt, double w);
  /// Trans and Quat constructor
  Pose3D(double xt, double yt, double zt, 
	 double xr, double yt, double zt, double w);
  
  // Utility functions to normalize and get magnitude.
  void Normalize();
  double getMagnitude();
  Pose3D & operator=(const Pose3D & input);
  
    
    /* accessors */
    NEWMAT::Matrix asMatrix();
    NEWMAT::Matrix getInverseMatrix();
    Quaternion asQuaternion();
    Position asPosition();
    

    /** Mutators **/
    //Set the values from a matrix
    void setFromMatrix(const NEWMAT::Matrix& matIn);
    // Set the values using Euler angles
    void setFromEuler(double _x, double _y, double _z, double _yaw, double _pitch, double _roll);
    // Set the values using DH Parameters
    void setFromDH(double length, double alpha, double offset, double theta);

    /* Internal Data */    
    double xt, yt, zt, xr, yr, zr, w;







    /**************** Static Helper Functions ***********************/
    // Convert DH Parameters to a Homogeneous Transformation Matrix
    static NEWMAT::Matrix matrixFromDH(double length, double alpha, double offset, double theta);
    // Convert Euler Angles to a Homogeneous Transformation Matrix
    static NEWMAT::Matrix matrixFromEuler(double ax,
					  double ay, double az, double yaw,
					  double pitch, double roll);

    static Euler3D eulerFromMatrix(const NEWMAT::Matrix & matrix_in, unsigned int solution_number=1);
    
    
};



 
class Quaternion3D {

public:
  static const int MIN_INTERPOLATION_DISTANCE = 5; //Number of nano-seconds to not interpolate below.
  static const unsigned int MAX_LENGTH_LINKED_LIST = 1000000; // Maximum length of linked list, to make sure not to be able to use unlimited memory.
  static const unsigned long long DEFAULT_MAX_STORAGE_TIME = 10ULL * 1000000000ULL; // default value of 10 seconds storage
  static const unsigned long long DEFAULT_MAX_EXTRAPOLATION_TIME = 10000000000ULL; // default max extrapolation of 10 seconds
  // Storage class
  class Quaternion3DStorage : public Pose3D
  {
  public:
    Quaternion3DStorage & operator=(const Quaternion3DStorage & input);
    unsigned long long time; //nanoseconds since 1970
  };

  /** \brief An exception class to notify that the requested value would have required extrapolation, and extrapolation is not allowed.
   * 
   */
  class ExtrapolateException : public std::exception
  {
  public:
    virtual const char* what() const throw()    { return "Disallowed extrapolation required."; }
  };
  
  /** Constructors **/
  // Standard constructor max_cache_time is how long to cache transform data
  Quaternion3D(bool interpolating = true, 
               unsigned long long  max_cache_time = DEFAULT_MAX_STORAGE_TIME,
               unsigned long long  max_extrapolation_time = DEFAULT_MAX_EXTRAPOLATION_TIME); 
  ~Quaternion3D();  

  /** Mutators **/
  // Set the values manually
  void addFromQuaternion(double _xt, double _yt, double _zt, double _xr, double _yr, double _zr, double _w, unsigned long long time);
  //Set the values from a matrix
  void addFromMatrix(const NEWMAT::Matrix& matIn, unsigned long long time);
  // Set the values using Euler angles
  void addFromEuler(double _x, double _y, double _z, double _yaw, double _pitch, double _roll, unsigned long long time);
  // Set the values using DH Parameters
  void addFromDH(double length, double alpha, double offset, double theta, unsigned long long time);

  
  /** Interpolated Accessors **/
  // Return a Quaternion
  Quaternion3D::Quaternion3DStorage getQuaternion(unsigned long long time);
  // Return a Matrix
  NEWMAT::Matrix getMatrix(unsigned long long time);
  // Return the inverse matrix
  NEWMAT::Matrix getInverseMatrix(unsigned long long time);
  
  //Print as a matrix
  void printMatrix(unsigned long long time);  //Not a critical part either
  void printStorage(const Quaternion3DStorage &storage); //Do i need this now that i've got the ostream method??

  // Remove all nodes from the list
  void clearList();
  
  
private:
  /**** Linked List stuff ****/

  struct data_LL;

  struct data_LL{
    Quaternion3DStorage data;
    struct data_LL * next;
    struct data_LL * previous;
  };

  bool getValue(Quaternion3DStorage& buff, unsigned long long time, long long  &time_diff);
  void add_value(const Quaternion3DStorage&);//todo fixme finish implementing this


  ExtrapolateException MaxExtrapolation;

  // insert a node into the sorted linked list
  void insertNode(const Quaternion3DStorage & );
  // prune data older than max_storage_time from the list
  void pruneList();

  //Find the closest two points in the list  
  //Return the distance to the closest one
  int findClosest(Quaternion3DStorage& one, Quaternion3DStorage& two, const unsigned long long target_time, long long &time_diff);

  //Interpolate between two nodes and return the interpolated value
  // This must always take two valid points!!!!!!
  // Only Cpose version implemented
  void interpolate(const Quaternion3DStorage &one, const Quaternion3DStorage &two, const unsigned long long target_time, Quaternion3DStorage& output);

  //Used by interpolate to interpolate between double values
  double interpolateDouble(const double, const unsigned long long, const double, const unsigned long long, const unsigned long long);

  //How long to cache incoming values
  unsigned long long max_storage_time;
  //Max length of linked list
  unsigned long long max_length_linked_list;
  //Whether to allow extrapolation
  unsigned long long max_extrapolation_time;

  //A mutex to prevent linked list collisions
  pthread_mutex_t linked_list_mutex;

  //Pointers for the start and end of a sorted linked list.
  data_LL* first;
  data_LL* last;

  unsigned int list_length;

  //Whether or not to interpolate
  bool interpolating;

};


//A namespace ostream overload for displaying storage
std::ostream & operator<<(std::ostream& mystream,const Quaternion3D::Quaternion3DStorage & storage);    

};







#endif //QUATERNION3D_HH

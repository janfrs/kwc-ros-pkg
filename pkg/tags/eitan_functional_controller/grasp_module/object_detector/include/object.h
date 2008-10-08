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

/* author: Matei Ciocarlie */

#ifndef _object_h_
#define _object_h_

#include "assert.h"

#include <libTF/libTF.h>
#include "grasp_module/object_msg.h"

namespace grasp_module {

/*!  This class represents an abstraction of an object, as opposed to
  a set of raw sensor data. It is designed to be an intermediate step
  between the raw data and the grasp planner: the raw data is
  segmented into instances of this class, which are in turn passed to
  the grasp planner.

  For now, this can be considered a stub, holding a very simple
  implementation. It is designed to work with a simple segmentation of
  connected componetns from point clouds. It holds the centroid and
  the principal axes of the object as detected in the point cloud.

  We are working for now with the principle that we only add more data
  if we need it. In the future, we might therefore add more labels,
  data points, etc. as needed.
 */

class Object {
 private:
	//! Object centroid
	libTF::TFPoint mCentroid;
	//! Object principal axes, ordered from most dominant (\a mA1 ) to least dominant (\a mA3 )
	libTF::TFVector mA1, mA2, mA3;

	//! Copy another object
	void copyFrom(const Object &o) {
		mCentroid = o.mCentroid;
		mA1 = o.mA1; mA2 = o.mA2; mA3 = o.mA3; }

 public:
	//! Empty constructor does no initialization at all
	Object(){}
	//! Constructor using centroid and axes information
	Object(const libTF::TFPoint &c, const libTF::TFVector &a1,
	       const libTF::TFVector &a2, const libTF::TFVector &a3) {
		setCentroid(c);
		setAxes(a1,a2,a3);
	}
	//! Copy constructor
	Object(const Object &o){copyFrom(o);}
	//! Assignment operator
	Object& operator=(const Object &o){copyFrom(o); return *this;}

	//! Set the centroid of the object
	void setCentroid(const libTF::TFPoint &c){mCentroid = c;}
	//! Set the principal axes of the object, from most dominant (\a a1 ) to least dominant ( \a a3 )
	void setAxes(const libTF::TFVector &a1,
		     const libTF::TFVector &a2,
		     const libTF::TFVector &a3){mA1 = a1; mA2 = a2; mA3 = a3;}
	//! Set the centroid of this object
	libTF::TFPoint getCentroid() const {return mCentroid;}
	//! Get the principal axes of the object, from most dominant (\a a1 ) to least dominant ( \a a3 )
	void getAxes(libTF::TFVector &a1,
		     libTF::TFVector &a2,
		     libTF::TFVector &a3) const {a1 = mA1; a2 = mA2; a3 = mA3;}
	//! Get one principal axis. Legal values of \a i are between 1 (most dominant axis) and 3 (least dominant axis)
	libTF::TFVector getAxis(int i) const { 
		switch(i) {
		case 1: return mA1; break;
		case 2: return mA2; break;
		case 3: return mA3; break;
		default: assert(0);} 
	}

	//! Get this object in the corresponding ROS message type
	object_msg getMsg() const;

	//! Set this object from a corresponding ROS message type
	void setFromMsg(const object_msg &om);


};

} //namespace grasp_module

#endif

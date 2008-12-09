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


#include "gaussian_pos_vel.h"
#include <wrappers/rng/rng.h>
#include <cmath> 
#include <cassert>

using namespace tf;

namespace BFL
{
  GaussianPosVel::GaussianPosVel (const StatePosVel& mu, const StatePosVel& sigma)
    : Pdf<StatePosVel> ( 1 ),
      mu_(mu),
      sigma_changed_(true)
  {
    for (unsigned int i=0; i<3; i++){
      assert(sigma.pos_[i] > 0);
      assert(sigma.vel_[i] > 0);
    }

    for (unsigned int i=0; i<3; i++){
      sigma_sq_.pos_[i] = pow(sigma.pos_[i],2);
      sigma_sq_.vel_[i] = pow(sigma.vel_[i],2);
    }
  }


  GaussianPosVel::~GaussianPosVel(){}


  std::ostream& operator<< (std::ostream& os, const GaussianPosVel& g)
  {
    os << "\nMu pos :\n"    << g.ExpectedValueGet().pos_ << endl
       << "\nMu vel :\n"    << g.ExpectedValueGet().vel_ << endl 
       << "\nSigma:\n" << g.CovarianceGet() << endl;
    return os;
  }


  Probability GaussianPosVel::ProbabilityGet(const StatePosVel& input) const
  {
    if (sigma_changed_){
      sigma_changed_ = false;
      sqrt_ = 1/ sqrt(M_PI*2 * 
		      sigma_sq_.pos_[0] * sigma_sq_.pos_[1] * sigma_sq_.pos_[2] *
		      sigma_sq_.vel_[0] * sigma_sq_.vel_[1] * sigma_sq_.vel_[2]);
    }

    Vector3 diff_pos = input.pos_ - mu_.pos_;
    Vector3 diff_vel = input.vel_ - mu_.vel_;
    double temp = 0;
    for (unsigned int i=0; i<3; i++){
      temp += (diff_pos[i] * (1 / sigma_sq_.pos_[i]) * diff_pos[i]);
      temp += (diff_vel[i] * (1 / sigma_sq_.vel_[i]) * diff_vel[i]);
    }

    Probability result = exp(-0.5 * temp) * sqrt_;
    return result;
  }


  bool
  GaussianPosVel::SampleFrom (vector<Sample<StatePosVel> >& list_samples, const int num_samples, int method, void * args) const
  {
    list_samples.resize(num_samples);
    vector<Sample<StatePosVel> >::iterator sample_it = list_samples.begin();
    for (sample_it=list_samples.begin(); sample_it!=list_samples.end(); sample_it++)
      SampleFrom( *sample_it, method, args);

    return true;
  }


  bool
  GaussianPosVel::SampleFrom (Sample<StatePosVel>& one_sample, int method, void * args) const
  {
    one_sample.ValueSet( StatePosVel(Vector3(rnorm(mu_.pos_[0], sigma_sq_.pos_[0]), 
					     rnorm(mu_.pos_[1], sigma_sq_.pos_[1]),
					     rnorm(mu_.pos_[2], sigma_sq_.pos_[2])),
				     Vector3(rnorm(mu_.vel_[0], sigma_sq_.vel_[0]), 
					     rnorm(mu_.vel_[1], sigma_sq_.vel_[1]),
					     rnorm(mu_.vel_[2], sigma_sq_.vel_[2]))) );
    return true;
  }


  StatePosVel
  GaussianPosVel::ExpectedValueGet (  ) const 
  { 
    return mu_;
  }

  SymmetricMatrix
  GaussianPosVel::CovarianceGet () const
  {
    SymmetricMatrix sigma(6); sigma = 0;
    for (unsigned int i=0; i<3; i++){
      sigma(i+1,i+1) = sigma_sq_.pos_[i];
      sigma(i+4,i+4) = sigma_sq_.vel_[i];
    }
    return sigma;
  }

} // End namespace BFL

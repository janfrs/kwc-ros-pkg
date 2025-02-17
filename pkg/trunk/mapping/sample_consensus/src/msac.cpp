/*
 * Copyright (c) 2008 Radu Bogdan Rusu <rusu -=- cs.tum.edu>
 *
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
 *
 * $Id$
 *
 */

/** \author Radu Bogdan Rusu */

#include <cfloat>
#include <limits>
#include "sample_consensus/msac.h"

namespace sample_consensus
{
  ////////////////////////////////////////////////////////////////////////////////
  /** \brief MSAC (M-Estimator SAmple Consensus) main constructor
    * \param model a Sample Consensus model
    * \param threshold distance to model threshold
    */
  MSAC::MSAC (SACModel *model, double threshold) : SAC (model)
  {
    this->threshold_ = threshold;
    // Desired probability of choosing at least one sample free from outliers
    this->probability_    = 0.99;
    // Maximum number of trials before we give up.
    this->max_iterations_ = 10000;

    this->iterations_ = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /** \brief MSAC (M-Estimator SAmple Consensus) main constructor
    * \param model a Sample Consensus model
    */
  MSAC::MSAC (SACModel* model) : SAC (model) { }

  ////////////////////////////////////////////////////////////////////////////////
  /** \brief Compute the actual model and find the inliers
    * \param debug enable/disable on-screen debug information
    */
  bool
    MSAC::computeModel (int debug)
  {
    iterations_ = 0;
    double d_best_penalty = DBL_MAX;

    double k = 1.0;

    std::vector<int> best_model;
    std::vector<int> best_inliers, inliers;
    std::vector<int> selection;

    int n_inliers_count = 0;

    // Iterate
    while (iterations_ < k)
    {
      // Get X samples which satisfy the model criteria
      selection = sac_model_->getSamples (iterations_);

      if (selection.size () == 0) break;

      // Search for inliers in the point cloud for the current plane model M
      sac_model_->computeModelCoefficients (selection);

      double d_cur_penalty = 0;
      // d_cur_penalty = sum (min (dist, threshold))
      // Iterate through the 3d points and calculate the distances from them to the model
      std::vector<double> distances = sac_model_->getDistancesToModel (sac_model_->getModelCoefficients ());
      for (unsigned int i = 0; i < sac_model_->getIndices ()->size (); i++)
        d_cur_penalty += std::min ((double)distances[i], threshold_);

      // Better match ?
      if (d_cur_penalty < d_best_penalty)
      {
        d_best_penalty = d_cur_penalty;
        best_model = selection;

        // Save the inliers
        best_inliers.resize (sac_model_->getIndices ()->size ());
        int n_inliers_count = 0;
        for (unsigned int i = 0; i < sac_model_->getIndices ()->size (); i++)
        {
          if (distances[i] <= threshold_)
          {
            best_inliers[n_inliers_count] = sac_model_->getIndices ()->at (i);
            n_inliers_count++;
          }
        }
        best_inliers.resize (n_inliers_count);

        // Compute the k parameter (k=log(z)/log(1-w^n))
        double w = (double)((double)n_inliers_count / (double)sac_model_->getIndices ()->size ());
        double p_no_outliers = 1 - pow (w, (double)selection.size ());
        p_no_outliers = std::max (std::numeric_limits<double>::epsilon (), p_no_outliers);       // Avoid division by -Inf
        p_no_outliers = std::min (1 - std::numeric_limits<double>::epsilon (), p_no_outliers);   // Avoid division by 0
        k = log (1 - probability_) / log (p_no_outliers);
      }

      iterations_ += 1;
      if (debug > 1)
        std::cerr << "[MSAC::computeModel] Trial " << iterations_ << " out of " << ceil (k) << ": best number of inliers so far is " << n_inliers_count << "." << std::endl;
      if (iterations_ > max_iterations_)
      {
        if (debug > 0)
          std::cerr << "[MSAC::computeModel] MSAC reached the maximum number of trials." << std::endl;
        break;
      }
    }

    if (best_model.size () != 0)
    {
      if (debug > 0)
        std::cerr << "[MSAC::computeModel] Model found: " << best_inliers.size () << " inliers." << std::endl;
      sac_model_->setBestModel (best_model);
      sac_model_->setBestInliers (best_inliers);
      return (true);
    }
    else
      if (debug > 0)
        std::cerr << "[MSAC::computeModel] Unable to find a solution!" << std::endl;
    return (false);
  }
}

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

#ifndef _SAMPLE_CONSENSUS_SAC_H_
#define _SAMPLE_CONSENSUS_SAC_H_

#include <std_msgs/Point32.h>     // ROS float point type
#include <std_msgs/PointCloud.h>  // ROS point cloud type

#include <stdlib.h>
#include "sample_consensus/sac_model.h"

namespace sample_consensus
{
  class SAC
  {
    public:

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Constructor for base SAC. */
      SAC ()
      {
        srand ((unsigned)time (0)); // set a random seed
      };

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Constructor for base SAC.
        * \param model a SAmple Consensus model
        */
      SAC (SACModel *model) : sac_model_(model)
      {
        srand ((unsigned)time (0)); // set a random seed
      };

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Destructor for base SAC. */
      virtual ~SAC () { }

      ////////////////////////////////////////////////////////////////////////////////
      /** \brief Set the threshold to model.
        * \param threshold distance to model threshold
        */
      virtual inline void
        setThreshold (double threshold)
      {
        this->threshold_ = threshold;
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Set the maximum number of iterations.
        * \param max_iterations maximum number of iterations
        */
      virtual inline void
        setMaxIterations (int max_iterations)
      {
        this->max_iterations_ = max_iterations;
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Set the desired probability of choosing at least one sample free from outliers.
        * \param probability the desired probability of choosing at least one sample free from outliers
        */
      virtual inline void
        setProbability (double probability)
      {
        this->probability_ = probability;
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Compute the actual model. Pure virtual. */
      virtual bool computeModel (int debug = 0) = 0;

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Compute the coefficients of the model and return them. */
      virtual std::vector<double>
        computeCoefficients ()
      {
        sac_model_->computeModelCoefficients (sac_model_->getBestModel ());
        return (sac_model_->getModelCoefficients ());
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Use Least-Squares optimizations to refine the coefficients of the model, and return them. */
      virtual std::vector<double>
        refineCoefficients ()
      {
        return (sac_model_->refitModel (sac_model_->getBestInliers ()));
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Remove the model inliers from the list of data indices. Returns the number of indices left. */
      virtual int removeInliers () { return (sac_model_->removeInliers ()); }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Get a list of the model inliers, found after computeModel () */
      virtual std::vector<int>
        getInliers ()
      {
        return (sac_model_->getBestInliers ());
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Return the point cloud representing a set of given indices.
        * \param indices a set of indices that represent the data that we're interested in
        */
      std_msgs::PointCloud getPointCloud (std::vector<int> indices);

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /** \brief Project a set of given points (using their indices) onto the model and return their projections.
        * \param indices a set of indices that represent the data that we're interested in
        * \param model_coefficients the coefficients of the underlying model
        */
      virtual std_msgs::PointCloud
        projectPointsToModel (std::vector<int> indices, std::vector<double> model_coefficients)
      {
        return (sac_model_->projectPoints (indices, model_coefficients));
      }


      ////////////////////////////////////////////////////////////////////////////////
      /** \brief Get a set of randomly selected indices.
        * \note Since we return a set, we do not guarantee that we'll return precisely
        * the desired number of samples (ie. nr_samples).
        * \param points the point cloud data set to be used
        * \param nr_samples the desired number of point indices
        */
      std::set<int>
        getRandomSamples (std_msgs::PointCloud points, int nr_samples)
      {
        std::set<int> random_idx;
        for (int i = 0; i < nr_samples; i++)
          random_idx.insert ((int) (points.pts.size () * (rand () / (RAND_MAX + 1.0))));
        return (random_idx);
      }

      ////////////////////////////////////////////////////////////////////////////////
      /** \brief Get a vector of randomly selected indices.
        * \note Since we return a set, we do not guarantee that we'll return precisely
        * the desired number of samples (ie. nr_samples).
        * \param points the point cloud data set to be used (unused)
        * \param indices a set of indices that represent the data that we're interested in
        * \param nr_samples the desired number of point indices
        */
      std::set<int>
        getRandomSamples (std_msgs::PointCloud points, std::vector<int> indices, int nr_samples)
      {
        std::set<int> random_idx;
        for (int i = 0; i < nr_samples; i++)
          random_idx.insert ((int) (indices.size () * (rand () / (RAND_MAX + 1.0))));
        return (random_idx);
      }

    protected:
      /** \brief The underlying data model used (i.e. what is it that we attempt to search for). */
      SACModel *sac_model_;

      /** \brief Desired probability of choosing at least one sample free from outliers. */
      double probability_;

      /** \brief Total number of internal loop iterations that we've done so far. */
      int iterations_;

      /** \brief Maximum number of iterations before giving up. */
      int max_iterations_;

      /** \brief Distance to model threshold. */
      double threshold_;
  };
}

#endif

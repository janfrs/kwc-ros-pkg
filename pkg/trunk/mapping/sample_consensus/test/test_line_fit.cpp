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

#include <gtest/gtest.h>
#include "std_msgs/PointCloud.h"

#include "cloud_geometry/point.h"
#include "sample_consensus/sac.h"
#include "sample_consensus/lmeds.h"
#include "sample_consensus/ransac.h"
#include "sample_consensus/rransac.h"
#include "sample_consensus/msac.h"
#include "sample_consensus/rmsac.h"
#include "sample_consensus/mlesac.h"
#include "sample_consensus/sac_model.h"
#include "sample_consensus/sac_model_line.h"

using namespace sample_consensus;

TEST (LMedS, SACModelLine)
{
  std_msgs::PointCloud points;
  points.pts.resize (10);

  points.pts[0].x = 1;  points.pts[0].y = 2;    points.pts[0].z = 3;
  points.pts[1].x = 4;  points.pts[1].y = 5;    points.pts[1].z = 6;
  points.pts[2].x = 7;  points.pts[2].y = 8;    points.pts[2].z = 9;
  points.pts[3].x = 10; points.pts[3].y = 11;   points.pts[3].z = 12;
  points.pts[4].x = 13; points.pts[4].y = 14;   points.pts[4].z = 15;
  points.pts[5].x = 16; points.pts[5].y = 17;   points.pts[5].z = 18;
  points.pts[6].x = 19; points.pts[6].y = 20;   points.pts[6].z = 21;
  points.pts[7].x = 22; points.pts[7].y = 23;   points.pts[7].z = 24;
  points.pts[8].x = -5; points.pts[8].y = 1.57; points.pts[8].z = 0.75;
  points.pts[9].x = 4;  points.pts[9].y = 2;    points.pts[9].z = 3;

  SACModel *model = new SACModelLine ();
  SAC *sac        = new LMedS (model, 0.001);
  model->setDataSet (&points);
  EXPECT_EQ ((int)model->getCloud ()->pts.size (), 10);

  bool result = sac->computeModel ();
  EXPECT_EQ (result, true);

  std::vector<int> inliers = sac->getInliers ();
  EXPECT_EQ ((int)inliers.size (), 8);

  std::vector<double> coeff = sac->computeCoefficients ();
  EXPECT_EQ ((int)coeff.size (), 6);
  //printf ("Line coefficients: %f %f %f %f %f %f\n", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5]);
  std_msgs::Point32 dir;
  dir.x = fabs (coeff[3] - coeff[0]);
  dir.y = fabs (coeff[4] - coeff[1]);
  dir.z = fabs (coeff[5] - coeff[2]);
  cloud_geometry::normalizePoint (dir);
  //printf ("Line direction: %f %f %f\n", dir.x, dir.y, dir.z);
  EXPECT_NEAR (dir.x, 0.577, 1e-3);
  EXPECT_NEAR (dir.y, 0.577, 1e-3);
  EXPECT_NEAR (dir.z, 0.577, 1e-3);

  std::vector<double> coeff_ref = sac->refineCoefficients ();
  EXPECT_EQ ((int)coeff_ref.size (), 6);
  //printf ("Line coefficients (refined): %f %f %f %f %f %f\n", coeff_ref[0], coeff_ref[1], coeff_ref[2], coeff_ref[3], coeff_ref[4], coeff_ref[5]);
  std_msgs::Point32 dir_ref;
  dir_ref.x = fabs (coeff_ref[3] - coeff_ref[0]);
  dir_ref.y = fabs (coeff_ref[4] - coeff_ref[1]);
  dir_ref.z = fabs (coeff_ref[5] - coeff_ref[2]);
  cloud_geometry::normalizePoint (dir_ref);
  //printf ("Line direction: %f %f %f\n", dir_ref.x, dir_ref.y, dir_ref.z);
  EXPECT_NEAR (dir_ref.x, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.y, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.z, 0.577, 1e-3);

  int nr_points_left = sac->removeInliers ();
  EXPECT_EQ (nr_points_left, 2);
}

TEST (RANSAC, SACModelLine)
{
  std_msgs::PointCloud points;
  points.pts.resize (10);

  points.pts[0].x = 1;  points.pts[0].y = 2;    points.pts[0].z = 3;
  points.pts[1].x = 4;  points.pts[1].y = 5;    points.pts[1].z = 6;
  points.pts[2].x = 7;  points.pts[2].y = 8;    points.pts[2].z = 9;
  points.pts[3].x = 10; points.pts[3].y = 11;   points.pts[3].z = 12;
  points.pts[4].x = 13; points.pts[4].y = 14;   points.pts[4].z = 15;
  points.pts[5].x = 16; points.pts[5].y = 17;   points.pts[5].z = 18;
  points.pts[6].x = 19; points.pts[6].y = 20;   points.pts[6].z = 21;
  points.pts[7].x = 22; points.pts[7].y = 23;   points.pts[7].z = 24;
  points.pts[8].x = -5; points.pts[8].y = 1.57; points.pts[8].z = 0.75;
  points.pts[9].x = 4;  points.pts[9].y = 2;    points.pts[9].z = 3;

  SACModel *model = new SACModelLine ();
  SAC *sac        = new RANSAC (model, 0.001);
  model->setDataSet (&points);
  EXPECT_EQ ((int)model->getCloud ()->pts.size (), 10);

  bool result = sac->computeModel ();
  EXPECT_EQ (result, true);

  std::vector<int> inliers = sac->getInliers ();
  EXPECT_EQ ((int)inliers.size (), 8);

  std::vector<double> coeff = sac->computeCoefficients ();
  EXPECT_EQ ((int)coeff.size (), 6);
  //printf ("Line coefficients: %f %f %f %f %f %f\n", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5]);
  std_msgs::Point32 dir;
  dir.x = fabs (coeff[3] - coeff[0]);
  dir.y = fabs (coeff[4] - coeff[1]);
  dir.z = fabs (coeff[5] - coeff[2]);
  cloud_geometry::normalizePoint (dir);
  //printf ("Line direction: %f %f %f\n", dir.x, dir.y, dir.z);
  EXPECT_NEAR (dir.x, 0.577, 1e-3);
  EXPECT_NEAR (dir.y, 0.577, 1e-3);
  EXPECT_NEAR (dir.z, 0.577, 1e-3);

  std::vector<double> coeff_ref = sac->refineCoefficients ();
  EXPECT_EQ ((int)coeff_ref.size (), 6);
  //printf ("Line coefficients (refined): %f %f %f %f %f %f\n", coeff_ref[0], coeff_ref[1], coeff_ref[2], coeff_ref[3], coeff_ref[4], coeff_ref[5]);
  std_msgs::Point32 dir_ref;
  dir_ref.x = fabs (coeff_ref[3] - coeff_ref[0]);
  dir_ref.y = fabs (coeff_ref[4] - coeff_ref[1]);
  dir_ref.z = fabs (coeff_ref[5] - coeff_ref[2]);
  cloud_geometry::normalizePoint (dir_ref);
  //printf ("Line direction: %f %f %f\n", dir_ref.x, dir_ref.y, dir_ref.z);
  EXPECT_NEAR (dir_ref.x, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.y, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.z, 0.577, 1e-3);

  int nr_points_left = sac->removeInliers ();
  EXPECT_EQ (nr_points_left, 2);
}

TEST (MSAC, SACModelLine)
{
  std_msgs::PointCloud points;
  points.pts.resize (10);

  points.pts[0].x = 1;  points.pts[0].y = 2;    points.pts[0].z = 3;
  points.pts[1].x = 4;  points.pts[1].y = 5;    points.pts[1].z = 6;
  points.pts[2].x = 7;  points.pts[2].y = 8;    points.pts[2].z = 9;
  points.pts[3].x = 10; points.pts[3].y = 11;   points.pts[3].z = 12;
  points.pts[4].x = 13; points.pts[4].y = 14;   points.pts[4].z = 15;
  points.pts[5].x = 16; points.pts[5].y = 17;   points.pts[5].z = 18;
  points.pts[6].x = 19; points.pts[6].y = 20;   points.pts[6].z = 21;
  points.pts[7].x = 22; points.pts[7].y = 23;   points.pts[7].z = 24;
  points.pts[8].x = -5; points.pts[8].y = 1.57; points.pts[8].z = 0.75;
  points.pts[9].x = 4;  points.pts[9].y = 2;    points.pts[9].z = 3;

  SACModel *model = new SACModelLine ();
  SAC *sac        = new MSAC (model, 0.001);
  model->setDataSet (&points);
  EXPECT_EQ ((int)model->getCloud ()->pts.size (), 10);

  bool result = sac->computeModel ();
  EXPECT_EQ (result, true);

  std::vector<int> inliers = sac->getInliers ();
  EXPECT_EQ ((int)inliers.size (), 8);

  std::vector<double> coeff = sac->computeCoefficients ();
  EXPECT_EQ ((int)coeff.size (), 6);
  //printf ("Line coefficients: %f %f %f %f %f %f\n", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5]);
  std_msgs::Point32 dir;
  dir.x = fabs (coeff[3] - coeff[0]);
  dir.y = fabs (coeff[4] - coeff[1]);
  dir.z = fabs (coeff[5] - coeff[2]);
  cloud_geometry::normalizePoint (dir);
  //printf ("Line direction: %f %f %f\n", dir.x, dir.y, dir.z);
  EXPECT_NEAR (dir.x, 0.577, 1e-3);
  EXPECT_NEAR (dir.y, 0.577, 1e-3);
  EXPECT_NEAR (dir.z, 0.577, 1e-3);

  std::vector<double> coeff_ref = sac->refineCoefficients ();
  EXPECT_EQ ((int)coeff_ref.size (), 6);
  //printf ("Line coefficients (refined): %f %f %f %f %f %f\n", coeff_ref[0], coeff_ref[1], coeff_ref[2], coeff_ref[3], coeff_ref[4], coeff_ref[5]);
  std_msgs::Point32 dir_ref;
  dir_ref.x = fabs (coeff_ref[3] - coeff_ref[0]);
  dir_ref.y = fabs (coeff_ref[4] - coeff_ref[1]);
  dir_ref.z = fabs (coeff_ref[5] - coeff_ref[2]);
  cloud_geometry::normalizePoint (dir_ref);
  //printf ("Line direction: %f %f %f\n", dir_ref.x, dir_ref.y, dir_ref.z);
  EXPECT_NEAR (dir_ref.x, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.y, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.z, 0.577, 1e-3);

  int nr_points_left = sac->removeInliers ();
  EXPECT_EQ (nr_points_left, 2);
}

TEST (MLESAC, SACModelLine)
{
  std_msgs::PointCloud points;
  points.pts.resize (10);

  points.pts[0].x = 1;  points.pts[0].y = 2;    points.pts[0].z = 3;
  points.pts[1].x = 4;  points.pts[1].y = 5;    points.pts[1].z = 6;
  points.pts[2].x = 7;  points.pts[2].y = 8;    points.pts[2].z = 9;
  points.pts[3].x = 10; points.pts[3].y = 11;   points.pts[3].z = 12;
  points.pts[4].x = 13; points.pts[4].y = 14;   points.pts[4].z = 15;
  points.pts[5].x = 16; points.pts[5].y = 17;   points.pts[5].z = 18;
  points.pts[6].x = 19; points.pts[6].y = 20;   points.pts[6].z = 21;
  points.pts[7].x = 22; points.pts[7].y = 23;   points.pts[7].z = 24;
  points.pts[8].x = -5; points.pts[8].y = 1.57; points.pts[8].z = 0.75;
  points.pts[9].x = 4;  points.pts[9].y = 2;    points.pts[9].z = 3;

  SACModel *model = new SACModelLine ();
  SAC *sac        = new MLESAC (model, 0.001);
  model->setDataSet (&points);
  EXPECT_EQ ((int)model->getCloud ()->pts.size (), 10);

  bool result = sac->computeModel (0);
  EXPECT_EQ (result, true);

  std::vector<int> inliers = sac->getInliers ();
  EXPECT_EQ ((int)inliers.size (), 8);

  std::vector<double> coeff = sac->computeCoefficients ();
  EXPECT_EQ ((int)coeff.size (), 6);
  //printf ("Line coefficients: %f %f %f %f %f %f\n", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5]);
  std_msgs::Point32 dir;
  dir.x = fabs (coeff[3] - coeff[0]);
  dir.y = fabs (coeff[4] - coeff[1]);
  dir.z = fabs (coeff[5] - coeff[2]);
  cloud_geometry::normalizePoint (dir);
  //printf ("Line direction: %f %f %f\n", dir.x, dir.y, dir.z);
  EXPECT_NEAR (dir.x, 0.577, 1e-3);
  EXPECT_NEAR (dir.y, 0.577, 1e-3);
  EXPECT_NEAR (dir.z, 0.577, 1e-3);

  std::vector<double> coeff_ref = sac->refineCoefficients ();
  EXPECT_EQ ((int)coeff_ref.size (), 6);
  //printf ("Line coefficients (refined): %f %f %f %f %f %f\n", coeff_ref[0], coeff_ref[1], coeff_ref[2], coeff_ref[3], coeff_ref[4], coeff_ref[5]);
  std_msgs::Point32 dir_ref;
  dir_ref.x = fabs (coeff_ref[3] - coeff_ref[0]);
  dir_ref.y = fabs (coeff_ref[4] - coeff_ref[1]);
  dir_ref.z = fabs (coeff_ref[5] - coeff_ref[2]);
  cloud_geometry::normalizePoint (dir_ref);
  //printf ("Line direction: %f %f %f\n", dir_ref.x, dir_ref.y, dir_ref.z);
  EXPECT_NEAR (dir_ref.x, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.y, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.z, 0.577, 1e-3);

  int nr_points_left = sac->removeInliers ();
  EXPECT_EQ (nr_points_left, 2);
}

TEST (RRANSAC, SACModelLine)
{
  std_msgs::PointCloud points;
  points.pts.resize (10);

  points.pts[0].x = 1;  points.pts[0].y = 2;    points.pts[0].z = 3;
  points.pts[1].x = 4;  points.pts[1].y = 5;    points.pts[1].z = 6;
  points.pts[2].x = 7;  points.pts[2].y = 8;    points.pts[2].z = 9;
  points.pts[3].x = 10; points.pts[3].y = 11;   points.pts[3].z = 12;
  points.pts[4].x = 13; points.pts[4].y = 14;   points.pts[4].z = 15;
  points.pts[5].x = 16; points.pts[5].y = 17;   points.pts[5].z = 18;
  points.pts[6].x = 19; points.pts[6].y = 20;   points.pts[6].z = 21;
  points.pts[7].x = 22; points.pts[7].y = 23;   points.pts[7].z = 24;
  points.pts[8].x = -5; points.pts[8].y = 1.57; points.pts[8].z = 0.75;
  points.pts[9].x = 4;  points.pts[9].y = 2;    points.pts[9].z = 3;

  SACModel *model = new SACModelLine ();
  SAC *sac        = new RRANSAC (model, 0.001);
  reinterpret_cast<RRANSAC*>(sac)->setFractionNrPretest (10);
  model->setDataSet (&points);
  EXPECT_EQ ((int)model->getCloud ()->pts.size (), 10);

  bool result = sac->computeModel ();
  EXPECT_EQ (result, true);

  std::vector<int> inliers = sac->getInliers ();
  EXPECT_EQ ((int)inliers.size (), 8);

  std::vector<double> coeff = sac->computeCoefficients ();
  EXPECT_EQ ((int)coeff.size (), 6);
  //printf ("Line coefficients: %f %f %f %f %f %f\n", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5]);
  std_msgs::Point32 dir;
  dir.x = fabs (coeff[3] - coeff[0]);
  dir.y = fabs (coeff[4] - coeff[1]);
  dir.z = fabs (coeff[5] - coeff[2]);
  cloud_geometry::normalizePoint (dir);
  //printf ("Line direction: %f %f %f\n", dir.x, dir.y, dir.z);
  EXPECT_NEAR (dir.x, 0.577, 1e-3);
  EXPECT_NEAR (dir.y, 0.577, 1e-3);
  EXPECT_NEAR (dir.z, 0.577, 1e-3);

  std::vector<double> coeff_ref = sac->refineCoefficients ();
  EXPECT_EQ ((int)coeff_ref.size (), 6);
  //printf ("Line coefficients (refined): %f %f %f %f %f %f\n", coeff_ref[0], coeff_ref[1], coeff_ref[2], coeff_ref[3], coeff_ref[4], coeff_ref[5]);
  std_msgs::Point32 dir_ref;
  dir_ref.x = fabs (coeff_ref[3] - coeff_ref[0]);
  dir_ref.y = fabs (coeff_ref[4] - coeff_ref[1]);
  dir_ref.z = fabs (coeff_ref[5] - coeff_ref[2]);
  cloud_geometry::normalizePoint (dir_ref);
  //printf ("Line direction: %f %f %f\n", dir_ref.x, dir_ref.y, dir_ref.z);
  EXPECT_NEAR (dir_ref.x, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.y, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.z, 0.577, 1e-3);

  int nr_points_left = sac->removeInliers ();
  EXPECT_EQ (nr_points_left, 2);
}

TEST (RMSAC, SACModelLine)
{
  std_msgs::PointCloud points;
  points.pts.resize (10);

  points.pts[0].x = 1;  points.pts[0].y = 2;    points.pts[0].z = 3;
  points.pts[1].x = 4;  points.pts[1].y = 5;    points.pts[1].z = 6;
  points.pts[2].x = 7;  points.pts[2].y = 8;    points.pts[2].z = 9;
  points.pts[3].x = 10; points.pts[3].y = 11;   points.pts[3].z = 12;
  points.pts[4].x = 13; points.pts[4].y = 14;   points.pts[4].z = 15;
  points.pts[5].x = 16; points.pts[5].y = 17;   points.pts[5].z = 18;
  points.pts[6].x = 19; points.pts[6].y = 20;   points.pts[6].z = 21;
  points.pts[7].x = 22; points.pts[7].y = 23;   points.pts[7].z = 24;
  points.pts[8].x = -5; points.pts[8].y = 1.57; points.pts[8].z = 0.75;
  points.pts[9].x = 4;  points.pts[9].y = 2;    points.pts[9].z = 3;

  SACModel *model = new SACModelLine ();
  SAC *sac        = new RMSAC (model, 0.001);
  reinterpret_cast<RMSAC*>(sac)->setFractionNrPretest (10);
  model->setDataSet (&points);
  EXPECT_EQ ((int)model->getCloud ()->pts.size (), 10);

  bool result = sac->computeModel ();
  EXPECT_EQ (result, true);

  std::vector<int> inliers = sac->getInliers ();
  EXPECT_EQ ((int)inliers.size (), 8);

  std::vector<double> coeff = sac->computeCoefficients ();
  EXPECT_EQ ((int)coeff.size (), 6);
  //printf ("Line coefficients: %f %f %f %f %f %f\n", coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5]);
  std_msgs::Point32 dir;
  dir.x = fabs (coeff[3] - coeff[0]);
  dir.y = fabs (coeff[4] - coeff[1]);
  dir.z = fabs (coeff[5] - coeff[2]);
  cloud_geometry::normalizePoint (dir);
  //printf ("Line direction: %f %f %f\n", dir.x, dir.y, dir.z);
  EXPECT_NEAR (dir.x, 0.577, 1e-3);
  EXPECT_NEAR (dir.y, 0.577, 1e-3);
  EXPECT_NEAR (dir.z, 0.577, 1e-3);

  std::vector<double> coeff_ref = sac->refineCoefficients ();
  EXPECT_EQ ((int)coeff_ref.size (), 6);
  //printf ("Line coefficients (refined): %f %f %f %f %f %f\n", coeff_ref[0], coeff_ref[1], coeff_ref[2], coeff_ref[3], coeff_ref[4], coeff_ref[5]);
  std_msgs::Point32 dir_ref;
  dir_ref.x = fabs (coeff_ref[3] - coeff_ref[0]);
  dir_ref.y = fabs (coeff_ref[4] - coeff_ref[1]);
  dir_ref.z = fabs (coeff_ref[5] - coeff_ref[2]);
  cloud_geometry::normalizePoint (dir_ref);
  //printf ("Line direction: %f %f %f\n", dir_ref.x, dir_ref.y, dir_ref.z);
  EXPECT_NEAR (dir_ref.x, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.y, 0.577, 1e-3);
  EXPECT_NEAR (dir_ref.z, 0.577, 1e-3);

  int nr_points_left = sac->removeInliers ();
  EXPECT_EQ (nr_points_left, 2);
}

/* ---[ */
int
  main (int argc, char** argv)
{
  testing::InitGoogleTest (&argc, argv);
  return (RUN_ALL_TESTS ());
}
/* ]--- */

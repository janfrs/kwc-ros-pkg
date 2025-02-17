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

/**
 * @author Conor McGann
 * @file Test harness for CostMap2D
 */

#include <costmap_2d/costmap_2d.h>
#include <costmap_2d/observation_buffer.h>
#include <set>
#include <gtest/gtest.h>

using namespace costmap_2d;

const unsigned char MAP_10_BY_10_CHAR[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 200, 200, 200,
  0, 0, 0, 0, 100, 0, 0, 200, 200, 200,
  0, 0, 0, 0, 100, 0, 0, 200, 200, 200,
  70, 70, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 200, 200, 200, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 255, 255, 255,
  0, 0, 0, 0, 0, 0, 0, 255, 255, 255
};

std::vector<unsigned char> MAP_10_BY_10;
std::vector<unsigned char> EMPTY_10_BY_10;
std::vector<unsigned char> EMPTY_100_BY_100;

const unsigned int GRID_WIDTH(10);
const unsigned int GRID_HEIGHT(10);
const double RESOLUTION(1);
const double WINDOW_LENGTH(10);
const unsigned char THRESHOLD(100);
const double MAX_Z(1.0);
const double ROBOT_RADIUS(1.0);

bool find(const std::vector<unsigned int>& l, unsigned int n){
  for(std::vector<unsigned int>::const_iterator it = l.begin(); it != l.end(); ++it){
    if(*it == n)
      return true;
  }

  return false;
}

/**
 * Tests the reset method
 */
TEST(costmap, testResetForStaticMap){
  // Define a static map with a large object in the center
  std::vector<unsigned char> staticMap;
  for(unsigned int i=0; i<10; i++){
    for(unsigned int j=0; j<10; j++){
      staticMap.push_back(CostMap2D::LETHAL_OBSTACLE);
    }
  }

  // Allocate the cost map, with a inflation to 3 cells all around
  CostMap2D map(10, 10, staticMap, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z,	3, 3, 3);

  // Populate the cost map with a wall around the perimeter. Free space should clear out the room.
  std_msgs::PointCloud cloud;
  cloud.set_pts_size(40);

  // Left wall
  unsigned int ind = 0;
  for (unsigned int i = 0; i < 10; i++){
    // Left
    cloud.pts[ind].x = 0;
    cloud.pts[ind].y = i;
    cloud.pts[ind].z = MAX_Z;
    ind++;

    // Top
    cloud.pts[ind].x = i;
    cloud.pts[ind].y = 0;
    cloud.pts[ind].z = MAX_Z;
    ind++;

    // Right
    cloud.pts[ind].x = 9;
    cloud.pts[ind].y = i;
    cloud.pts[ind].z = MAX_Z;
    ind++;

    // Bottom
    cloud.pts[ind].x = i;
    cloud.pts[ind].y = 9;
    cloud.pts[ind].z = MAX_Z;
    ind++;
  }

  double wx = 5.0, wy = 5.0;
  std_msgs::Point p;
  p.x = wx;
  p.y = wy;
  p.z = MAX_Z;
  Observation obs(p, &cloud);
  std::vector<Observation> obsBuf;
  obsBuf.push_back(obs);

  // Update the cost map for this observation
  map.updateDynamicObstacles(wx, wy, obsBuf);

  // Verify that we now have only 8 * 4 + 4 cells with lethal cost, thus provong that we have correctly cleared
  // free space
  unsigned int hitCount = 0;
  for(unsigned int i=0; i <100; i++){
    if(map.getMap()[i] == CostMap2D::LETHAL_OBSTACLE)
      hitCount++;
  }
  ASSERT_EQ(hitCount, 36);

  // Veriy that we have 4 free cells
  hitCount = 0;
  for(unsigned int i=0; i < 100; i++){
    if(map.getMap()[i] == 0)
      hitCount++;
  }
  ASSERT_EQ(hitCount, 4);

  // Now if we reset the cost map, we shold retain the free space, and also retain values of INSCRIBED circle
  // in the region of the circumscribed radius (3 cells)
  map.revertToStaticMap(wx, wy);
  unsigned int mx, my;
  map.WC_MC(wx, wy, mx, my);
  for(unsigned int x = mx - 3; x <= mx+3; x++){
    for(unsigned int y = my -3; y <= my + 3; y++){
      ASSERT_EQ(map.getCost(x, y) < CostMap2D::LETHAL_OBSTACLE, true);
    }
  }
}

/**
 * Basic testing for observation buffer
 */
TEST(costmap, test15){
  // Rate calculations
  ASSERT_EQ(ObservationBuffer::computeRefreshInterval(-100), ros::Duration(0, 0));
  ASSERT_EQ(ObservationBuffer::computeRefreshInterval(100), ros::Duration(0, 10000000));
  ASSERT_EQ(ObservationBuffer::computeRefreshInterval(0), ros::Duration(0, 0));
  ASSERT_EQ(ObservationBuffer::computeRefreshInterval(5), ros::Duration(0, (int)(0.2 * pow(10.0, 9.0))));
  ASSERT_EQ(ObservationBuffer::computeRefreshInterval(0.3), ros::Duration(3, (int)((1.0/3.0) * pow(10.0, 9.0))));

  ros::Duration keep_alive(10, 0);
  ros::Duration refresh_interval(0, 200000000); // 200 ms
  ObservationBuffer buffer("Foo", keep_alive, refresh_interval);

  // Initially it should be false
  ASSERT_EQ(buffer.isCurrent(), false);

  std_msgs::Point origin; // Map origin
  origin.x = 0;
  origin.y = 0;
  origin.z = 0;

  ros::Time epoch; // Beginning of time

  // Buffer a point cloud with a time stamp that is very old. It should still not be current
  std_msgs::PointCloud* p0 = new std_msgs::PointCloud();
  p0->set_pts_size(1);
  p0->pts[0].x = 50;
  p0->pts[0].y = 50;
  p0->pts[0].z = MAX_Z;
  p0->header.stamp = epoch;
  Observation o0(origin, p0);
  buffer.buffer_observation(o0);
  // Up to date - ignores the time stamp.
  ASSERT_EQ(buffer.isCurrent(), true);

  // Now buffer another which has a current time stamp
  std_msgs::PointCloud* p1 = new std_msgs::PointCloud();
  p1->set_pts_size(1);
  p1->pts[0].x = 50;
  p1->pts[0].y = 50;
  p1->pts[0].z = MAX_Z;
  p1->header.stamp = ros::Time::now();
  Observation o1(origin, p1);
  buffer.buffer_observation(o1);
  ASSERT_EQ(buffer.isCurrent(), true);

  // Now go again after sleeping for a bit too long - 300 ms
  ros::Time oldValue = ros::Time::now();
  ros::Duration excessiveSleep(0, 300000000);
  excessiveSleep.sleep();
  ASSERT_EQ(buffer.isCurrent(), false);
}

/**
 * Test for the cost function correctness with a larger range and different values
 */
TEST(costmap, test14){
  CostMap2D map(100, 100, EMPTY_100_BY_100, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z,
		ROBOT_RADIUS * 10.5, ROBOT_RADIUS * 8.0, ROBOT_RADIUS * 5.0, 0.5, 100.0, 100.0);

  // Verify that the circumscribed cost lower bound is as expected: based on the cost function.
  unsigned char c = (unsigned char) ((CostMap2D::INSCRIBED_INFLATED_OBSTACLE -1) * 0.5/(1 + pow(3.0, 2)));
  ASSERT_EQ(map.getCircumscribedCostLowerBound(), c);

  // Add a point in the center
  std_msgs::PointCloud cloud;
  cloud.set_pts_size(1);
  cloud.pts[0].x = 50;
  cloud.pts[0].y = 50;
  cloud.pts[0].z = MAX_Z;

  map.updateDynamicObstacles(0, 0, CostMap2D::toVector(cloud));

  for(unsigned int i = 0; i <= (unsigned int)ceil(ROBOT_RADIUS * 5.0); i++){
    // To the right
    ASSERT_EQ(map.isDefinitelyBlocked(50 + i, 50), true);
    ASSERT_EQ(map.getCost(50 + i, 50) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    ASSERT_EQ(map.getCost(50 + i, 50) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    // To the left
    ASSERT_EQ(map.isDefinitelyBlocked(50 - i, 50), true);
    ASSERT_EQ(map.getCost(50 - i, 50) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    ASSERT_EQ(map.getCost(50 - i, 50) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    // Down
    ASSERT_EQ(map.isDefinitelyBlocked(50, 50 + i), true);
    ASSERT_EQ(map.getCost(50, 50 + i) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    ASSERT_EQ(map.getCost(50, 50 + i) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    // Up
    ASSERT_EQ(map.isDefinitelyBlocked(50, 50 - i), true);
    ASSERT_EQ(map.getCost(50, 50 - i) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
    ASSERT_EQ(map.getCost(50, 50 - i) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
  }

  // Verify the normalized cost attenuates as expected
  for(unsigned int i = (unsigned int)(ceil(ROBOT_RADIUS * 5.0) + 1); i <= (unsigned int)ceil(ROBOT_RADIUS * 10.5); i++){
    unsigned char expectedValue = ( unsigned char )((CostMap2D::INSCRIBED_INFLATED_OBSTACLE - 1)* 0.5 /(1 + pow(i-ceil(ROBOT_RADIUS * 5.0), 2)));
    ASSERT_EQ(map.getCost(50 + i, 50), expectedValue);
  }

  // Update with no hits. Should clear (revert to the static map
  map.revertToStaticMap();
  cloud.set_pts_size(0);
  map.updateDynamicObstacles(0, 0, CostMap2D::toVector(cloud));

  for(unsigned int i = 0; i < 100*100; i++)
    ASSERT_EQ(map[i], 0);

  // On the next update, with Z too high, we should still see the projection onto 2D to give the same cost value as before
}

// Test Priority queue handling
TEST(costmap, test13){
  QUEUE q;
  q.push(new QueueElement(1, 1, 1));
  q.push(new QueueElement(2.3, 1, 1));
  q.push(new QueueElement(2.1, 1, 1));
  q.push(new QueueElement(3.9, 1, 1));
  q.push(new QueueElement(1.07, 1, 1));

  double prior = 0.0;
  while (!q.empty()){
    QueueElement * c = q.top();
    q.pop();
    ASSERT_EQ(c->distance >= prior, true);
    prior = c->distance;
    delete c;
  }
}

/**
 * Test for wave interference
 */
TEST(costmap, test12){
  // Start with an empty map
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, EMPTY_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z * 2, MAX_Z, MAX_Z,
		ROBOT_RADIUS*3, ROBOT_RADIUS * 2, ROBOT_RADIUS);


  // Lay out 3 obstacles in a line - along the diagonal, separated by a cell.
  std_msgs::PointCloud cloud;
  cloud.set_pts_size(3);
  cloud.pts[0].x = 3;
  cloud.pts[0].y = 3;
  cloud.pts[0].z = MAX_Z;
  cloud.pts[1].x = 5;
  cloud.pts[1].y = 5;
  cloud.pts[1].z = MAX_Z;
  cloud.pts[2].x = 7;
  cloud.pts[2].y = 7;
  cloud.pts[2].z = MAX_Z;

  std::vector<unsigned int> updates;
  map.updateDynamicObstacles(cloud, updates);

  // Expect to see a union of obstacles
  ASSERT_EQ(updates.size(), 79);
}

/**
 * Test for ray tracing free space
 */
TEST(costmap, test0){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z, 
		ROBOT_RADIUS, ROBOT_RADIUS, ROBOT_RADIUS);
  // Add a point cloud and verify its insertion. There should be only one new one
  std_msgs::PointCloud cloud;
  cloud.set_pts_size(1);
  cloud.pts[0].x = 0;
  cloud.pts[0].y = 0;
  cloud.pts[0].z = MAX_Z;

  std::vector<unsigned int> updates;
  map.updateDynamicObstacles(cloud, updates);

  ASSERT_EQ(updates.size(), 3);
}

TEST(costmap, test1){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD);
  ASSERT_EQ(map.getWidth(), 10);
  ASSERT_EQ(map.getHeight(), 10);

  // Verify that obstacles correctly identified from the static map.
  std::vector<unsigned int> occupiedCells;
  map.getOccupiedCellDataIndexList(occupiedCells);
  ASSERT_EQ(occupiedCells.size(), 20);

  // Iterate over all id's and verify that they are present according to their
  const unsigned char* costData = map.getMap();
  for(std::vector<unsigned int>::const_iterator it = occupiedCells.begin(); it != occupiedCells.end(); ++it){
    unsigned int ind = *it;
    unsigned int x, y;
    map.IND_MC(ind, x, y);
    ASSERT_EQ(find(occupiedCells, map.MC_IND(x, y)), true);
    ASSERT_EQ(MAP_10_BY_10[ind] >= 100, true);
    ASSERT_EQ(costData[ind] >= 100, true);
  }

  // Block of 200
  ASSERT_EQ(find(occupiedCells, map.MC_IND(7, 2)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(8, 2)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(9, 2)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(7, 3)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(8, 3)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(9, 3)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(7, 4)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(8, 4)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(9, 4)), true);

  // Block of 100
  ASSERT_EQ(find(occupiedCells, map.MC_IND(4, 3)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(4, 4)), true);

  // Block of 200
  ASSERT_EQ(find(occupiedCells, map.MC_IND(3, 7)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(4, 7)), true);
  ASSERT_EQ(find(occupiedCells, map.MC_IND(5, 7)), true);


  // Verify Coordinate Transformations, ROW MAJOR ORDER
  ASSERT_EQ(map.WC_IND(0.0, 0.0), 0);
  ASSERT_EQ(map.WC_IND(0.0, 0.99), 0);
  ASSERT_EQ(map.WC_IND(0.0, 1.0), 10);
  ASSERT_EQ(map.WC_IND(1.0, 0.99), 1);
  ASSERT_EQ(map.WC_IND(9.99, 9.99), 99);
  ASSERT_EQ(map.WC_IND(8.2, 3.4), 38);

  // Ensure we hit the middle of the cell for world co-ordinates
  double wx, wy;
  map.IND_WC(99, wx, wy);
  ASSERT_EQ(wx, 9.5);
  ASSERT_EQ(wy, 9.5);
}


/**
 * Verify that dynamic obstacles are added
 */

TEST(costmap, test3){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD);

  // Add a point cloud and verify its insertion. There should be only one new one
  std_msgs::PointCloud cloud;
  cloud.set_pts_size(3);
  cloud.pts[0].x = 0;
  cloud.pts[0].y = 0;
  cloud.pts[1].x = 0;
  cloud.pts[1].y = 0;
  cloud.pts[2].x = 0;
  cloud.pts[2].y = 0;

  std::vector<unsigned int> updates;
  std::vector<unsigned int> ids;
  map.updateDynamicObstacles(cloud, updates);

  // Should now have 1 insertion and no deletions
  ASSERT_EQ(updates.size(), 1);
  map.getOccupiedCellDataIndexList(ids);
  ASSERT_EQ(ids.size(), 21);

  // Repeating the call - we should see no insertions or deletions
  map.updateDynamicObstacles(cloud, updates);
  ASSERT_EQ(updates.empty(), true);
}

/**
 * Verify that if we add a point that is already a static obstacle we do not end up with a new ostacle
 */
TEST(costmap, test4){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD);

  // A point cloud with one point that falls within an existing obstacle
  std_msgs::PointCloud cloud;
  cloud.set_pts_size(1);
  cloud.pts[0].x = 7;
  cloud.pts[0].y = 2;

  std::vector<unsigned int> updates;
  std::vector<unsigned int> ids;
  map.updateDynamicObstacles(cloud, updates);
  ASSERT_EQ(updates.empty(), true);
}

/**
 * Make sure we ignore points outside of our z threshold
 */
TEST(costmap, test6){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z);

  // A point cloud with 2 points falling in a cell with a non-lethal cost
  std_msgs::PointCloud c0;
  c0.set_pts_size(2);
  c0.pts[0].x = 0;
  c0.pts[0].y = 5;
  c0.pts[0].z = 0.4;
  c0.pts[1].x = 1;
  c0.pts[1].y = 5;
  c0.pts[1].z = 1.2;

  std::vector<unsigned int> updates;
  map.updateDynamicObstacles(c0, updates);
  ASSERT_EQ(updates.size(), 1);
}

/**
 * Test inflation for both static and dynamic obstacles
 */

TEST(costmap, test7){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z,
		ROBOT_RADIUS, ROBOT_RADIUS, ROBOT_RADIUS);


  // Verify that obstacles correctly identified
  std::vector<unsigned int> occupiedCells;
  map.getOccupiedCellDataIndexList(occupiedCells);

  // There should be no duplicates
  std::set<unsigned int> setOfCells;
  for(unsigned int i=0;i<occupiedCells.size(); i++)
    setOfCells.insert(i);

  ASSERT_EQ(setOfCells.size(), occupiedCells.size());
  ASSERT_EQ(setOfCells.size(), 48);

  const unsigned char* costData = map.getMap();

  // Iterate over all id's and verify they are obstacles
  for(std::vector<unsigned int>::const_iterator it = occupiedCells.begin(); it != occupiedCells.end(); ++it){
    unsigned int ind = *it;
    unsigned int x, y;
    map.IND_MC(ind, x, y);
    ASSERT_EQ(find(occupiedCells, map.MC_IND(x, y)), true);
    ASSERT_EQ(costData[ind] == CostMap2D::LETHAL_OBSTACLE || costData[ind] == CostMap2D::INSCRIBED_INFLATED_OBSTACLE, true);
  }

  // Set an obstacle at the origin and observe insertions for it and its neighbors
  std::vector<unsigned int> updates;
  std_msgs::PointCloud c0;
  c0.set_pts_size(1);
  c0.pts[0].x = 0;
  c0.pts[0].y = 0;
  c0.pts[0].z = 0.4;
  map.updateDynamicObstacles(c0, updates);

  // It and its 2 neighbors makes 3 obstacles
  ASSERT_EQ(updates.size(), 3);

  // @todo Rewrite 
  // Add an obstacle at <2,0> which will inflate and refresh to of the other inflated cells
  std_msgs::PointCloud c1;
  c1.set_pts_size(1);
  c1.pts[0].x = 2;
  c1.pts[0].y = 0;
  c1.pts[0].z = 0.0;
  map.updateDynamicObstacles(c1, updates);

  // Now we expect insertions for it, and 2 more neighbors, but not all 5. Free space will propagate from
  // the origin to the target, clearing the point at <0, 0>, but not over-writing the inflation of the obstacle
  // at <0, 1>
  ASSERT_EQ(updates.size(), 3);


  // Add an obstacle at <1, 9>. This will inflate obstacles around it
  vector<std_msgs::PointCloud*> cv2;
  std_msgs::PointCloud c2;
  cv2.push_back(&c2);
  c2.set_pts_size(1);
  c2.pts[0].x = 1;
  c2.pts[0].y = 9;
  c2.pts[0].z = 0.0;
  map.updateDynamicObstacles(0.0, 0.0, cv2); //, updates); //WINDOW_LENGTH + 2
  ASSERT_EQ(map.getCost(1, 9), CostMap2D::LETHAL_OBSTACLE);
  ASSERT_EQ(map.getCost(0, 9), CostMap2D::INSCRIBED_INFLATED_OBSTACLE);
  ASSERT_EQ(map.getCost(2, 9), CostMap2D::INSCRIBED_INFLATED_OBSTACLE);

  // Add an obstacle and verify that it over-writes its inflated status
  vector<std_msgs::PointCloud*> cv3;
  std_msgs::PointCloud c3;
  cv3.push_back(&c3);
  c3.set_pts_size(1);
  c3.pts[0].x = 0;
  c3.pts[0].y = 9;
  c3.pts[0].z = 0.0;
  map.updateDynamicObstacles(0.0, 0.0, cv3); //, updates); //WINDOW_LENGTH + 3
  ASSERT_EQ(map.getCost(0, 9), CostMap2D::LETHAL_OBSTACLE);
}

/**
 * Test specific inflation scenario to ensure we do not set inflated obstacles to be raw obstacles.
 */
TEST(costmap, test8){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z, 
		ROBOT_RADIUS, ROBOT_RADIUS, ROBOT_RADIUS);

  std::vector<unsigned int> updates;

  // Creat a small L-Shape all at once
  std_msgs::PointCloud c0;
  c0.set_pts_size(3);
  c0.pts[0].x = 1;
  c0.pts[0].y = 1;
  c0.pts[0].z = MAX_Z;
  c0.pts[1].x = 1;
  c0.pts[1].y = 2;
  c0.pts[1].z = MAX_Z;
  c0.pts[2].x = 2;
  c0.pts[2].y = 2;
  c0.pts[2].z = MAX_Z;

  map.updateDynamicObstacles(c0, updates);
  ASSERT_EQ(map.getCost(3, 2), CostMap2D::INSCRIBED_INFLATED_OBSTACLE);  
  ASSERT_EQ(map.getCost(3, 3), CostMap2D::INSCRIBED_INFLATED_OBSTACLE);
}

/**
 * Test inflation behavior, starting with an empty map
 */
TEST(costmap, test9){
  std::vector<unsigned char> mapData;
  for(unsigned int i=0; i< GRID_WIDTH; i++){
    for(unsigned int j = 0; j < GRID_HEIGHT; j++){
      mapData.push_back(0);
    }
  }

  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, mapData, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, MAX_Z, 
		ROBOT_RADIUS * 3, ROBOT_RADIUS * 2, ROBOT_RADIUS);

  // There should be no occupied cells
  std::vector<unsigned int> ids;
  map.getOccupiedCellDataIndexList(ids);
  ASSERT_EQ(ids.size(), 0);

  // Add an obstacle at 5,5
  std_msgs::PointCloud c0;
  c0.set_pts_size(1);
  c0.pts[0].x = 5;
  c0.pts[0].y = 5;
  c0.pts[0].z = MAX_Z;

  std::vector<unsigned int> updates;
  map.updateDynamicObstacles(c0, updates);

  ASSERT_EQ(updates.size(), 45);

  map.getOccupiedCellDataIndexList(ids);
  ASSERT_EQ(ids.size(), 5);

  // Update again - should see no change
  map.updateDynamicObstacles(c0, updates);
  ASSERT_EQ(updates.size(), 0);
}

/**
 * Test for the cost map accessor
 */
TEST(costmap, test10){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z, MAX_Z, ROBOT_RADIUS);

  // A window around a robot in the top left
  CostMapAccessor ma(map, 5, 0, 0);
  double wx, wy;

  // Origin
  ma.MC_WC(0, 0, wx, wy);
  ASSERT_EQ(wx, 0.5);
  ASSERT_EQ(wy, 0.5);


  // Max in x and y
  ma.updateForRobotPosition(9.5, 9.5);
  ma.MC_WC(0, 0, wx, wy);
  ASSERT_EQ(wx, 4.5);
  ASSERT_EQ(wy, 4.5);

  // Off the map in x - assume it ignores the change
  ma.updateForRobotPosition(10.5, 9.5);
  ma.MC_WC(0, 0, wx, wy);
  ASSERT_EQ(wx, 4.5);
  ASSERT_EQ(wy, 4.5);

  // Off the map in y - assume it ignores the change
  ma.updateForRobotPosition(9.5, 10.5);
  ma.MC_WC(0, 0, wx, wy);
  ASSERT_EQ(wx, 4.5);
  ASSERT_EQ(wy, 4.5);

  ASSERT_EQ(map.getCircumscribedCostLowerBound(), ma.getCircumscribedCostLowerBound());
}

/**
 * Test for ray tracing free space
 */

TEST(costmap, test11){
  CostMap2D map(GRID_WIDTH, GRID_HEIGHT, MAP_10_BY_10, RESOLUTION, THRESHOLD, MAX_Z * 2, MAX_Z, MAX_Z, ROBOT_RADIUS, 0, 0, 1, 100.0, 100.0);

  // The initial position will be <0,0> by default. So if we add an obstacle at 9,9, we would expect cells
  // <0, 0> thru <8, 8> to be free
  std_msgs::PointCloud c0;
  c0.set_pts_size(1);
  c0.pts[0].x = 9.5;
  c0.pts[0].y = 9.5;
  c0.pts[0].z = MAX_Z;
  std::vector<unsigned int> updates;
  map.updateDynamicObstacles(c0, updates);

  // Actual hit point and 3 cells along the diagonal. Note that neigbors are unchanged because they have higher cost in the static map (NO_INFORMATION).
  // I considered allowing the cost function to over-ride this case but we quickly find that the planner will plan through walls once it gets out of sensor range.
  // Note that this will not be the case when we persist the changes to the static map more aggressively since we will retain high cost obstacle data that 
  // has not been ray tarced thru. If that is the case, this update count would change to 6
  ASSERT_EQ(updates.size(), 4);

  // all cells will have been switched to free space along the diagonal except for this inflated in the update
  for(unsigned int i=0; i < 8; i++)
    ASSERT_EQ(map.getCost(i, i), 0);
}

int main(int argc, char** argv){
  for(unsigned int i = 0; i< GRID_WIDTH * GRID_HEIGHT; i++){
    EMPTY_10_BY_10.push_back(0);
    MAP_10_BY_10.push_back(MAP_10_BY_10_CHAR[i]);
  }

  for(unsigned int i = 0; i< 100 * 100; i++)
    EMPTY_100_BY_100.push_back(0);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

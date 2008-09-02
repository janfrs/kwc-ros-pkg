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

#ifndef OGRE_VISUALIZER_GRID_VISUALIZER_H
#define OGRE_VISUALIZER_GRID_VISUALIZER_H

#include "visualizer_base.h"

namespace ogre_tools
{
class Grid;
}

class wxFocusEvent;
class wxCommandEvent;

class GridOptionsPanel;

namespace ogre_vis
{

/**
 * \class GridVisualizer
 * \brief Displays a grid along the XZ plane (XY in robot space)
 *
 * For more information see ogre_tools::Grid
 */
class GridVisualizer : public VisualizerBase
{
public:
  GridVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name );
  virtual ~GridVisualizer();

  /**
   * @return The cell count for this grid
   */
  uint32_t getCellCount() { return cell_count_; }
  /**
   * @return The cell size for this grid
   */
  float getCellSize() { return cell_size_; }
  /**
   * @param r (return) The red color component, range [0,1]
   * @param g (return) The green color component, range [0,1]
   * @param b (return) The blue color component, range [0,1]
   */
  void getColor( float& r, float& g, float& b );

  /**
   * \brief Set all the parameters of the grid
   * @param cell_count The number of cells
   * @param cell_size The size of each cell
   * @param r Red color component, range [0,1]
   * @param g Green color component, range [0,1]
   * @param b Blue color component, range [0,1]
   */
  void set( uint32_t cell_count, float cell_size, float r, float g, float b );
  /**
   * \brief Set the number of cells
   * @param cell_count The number of cells
   */
  void setCellCount( uint32_t cell_count );
  /**
   * \brief Set the cell size
   * @param cell_size The cell size
   */
  void setCellSize( float cell_size );
  /**
   * \brief Set the color
   * @param r Red color component, range [0,1]
   * @param g Green color component, range [0,1]
   * @param b Blue color component, range [0,1]
   */
  void setColor( float r, float g, float b );

  // Overrides from VisualizerBase
  virtual void fillPropertyGrid( wxPropertyGrid* property_grid );
  virtual void propertyChanged( wxPropertyGridEvent& event );

protected:
  /**
   * \brief Creates the grid with the currently-set parameters
   */
  void create();

  // overrides from VisualizerBase
  virtual void onEnable();
  virtual void onDisable();

  float cell_size_;                   ///< The size of each cell drawn.  Cells are square.
  uint32_t cell_count_;               ///< The number of rows/columns to draw.
  float r_;                           ///< Red color component, range [0,1]
  float g_;                           ///< Green color component, range [0,1]
  float b_;                           ///< Blue color component, range [0,1]
  ogre_tools::Grid* grid_;            ///< Handles actually drawing the grid
};

} // namespace ogre_vis

 #endif

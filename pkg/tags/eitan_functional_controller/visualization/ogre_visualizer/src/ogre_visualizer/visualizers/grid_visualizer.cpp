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

#include "grid_visualizer.h"

#include "ogre_tools/grid.h"

#include <Ogre.h>
#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/confbase.h>

#define COLOR_PROPERTY wxT("Color")
#define CELLSIZE_PROPERTY wxT("CellSize")
#define CELLCOUNT_PROPERTY wxT("CellCount")

namespace ogre_vis
{

GridVisualizer::GridVisualizer( Ogre::SceneManager* scene_manager, ros::node* node, rosTFClient* tf_client, const std::string& name )
: VisualizerBase( scene_manager, node, tf_client, name )
, cell_size_( 1.0f )
, cell_count_( 10 )
, r_( 0.5 )
, g_( 0.5 )
, b_( 0.5 )
{
  grid_ = new ogre_tools::Grid( scene_manager_, cell_count_, cell_size_, r_, g_, b_ );
}

GridVisualizer::~GridVisualizer()
{
  delete grid_;
}

void GridVisualizer::onEnable()
{
  grid_->getSceneNode()->setVisible( true );
}

void GridVisualizer::onDisable()
{
  grid_->getSceneNode()->setVisible( false );
}

void GridVisualizer::create()
{
  grid_->set( cell_count_, cell_size_, r_, g_, b_ );

  causeRender();
}

void GridVisualizer::set( uint32_t cell_count, float cell_size, float r, float g, float b )
{
  cell_count_ = cell_count;
  cell_size_ = cell_size;
  r_ = r;
  g_ = g;
  b_ = b;

  create();

  if ( property_grid_ )
  {
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + CELLCOUNT_PROPERTY ), (long)cell_count_ );
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + CELLSIZE_PROPERTY ), cell_size_ );
    wxVariant color;
    color << wxColour( r_ * 255, g_ * 255, b_ * 255 );
    property_grid_->SetPropertyValue( property_grid_->GetProperty( property_prefix_ + COLOR_PROPERTY ), color );
  }
}

void GridVisualizer::setCellSize( float size )
{
  set( cell_count_, size, r_, g_, b_ );
}

void GridVisualizer::setCellCount( uint32_t count )
{
  set( count, cell_size_, r_, g_, b_ );
}

void GridVisualizer::setColor( float r, float g, float b )
{
  set( cell_count_, cell_size_, r, g, b );
}

void GridVisualizer::fillPropertyGrid()
{
  wxPGId prop = property_grid_->Append( new wxIntProperty( CELLCOUNT_PROPERTY, property_prefix_ + CELLCOUNT_PROPERTY, cell_count_ ) );
  property_grid_->SetPropertyAttribute( prop, wxT("Min"), 1 );
  property_grid_->SetPropertyAttribute( prop, wxT("Step"), 1 );
  property_grid_->SetPropertyEditor( prop, wxPG_EDITOR(SpinCtrl) );

  prop = property_grid_->Append( new wxFloatProperty( CELLSIZE_PROPERTY, property_prefix_ + CELLSIZE_PROPERTY, cell_size_ ) );
  property_grid_->SetPropertyAttribute( prop, wxT("Min"), 0.0001 );

  property_grid_->Append( new wxColourProperty( COLOR_PROPERTY, property_prefix_ + COLOR_PROPERTY, wxColour( r_ * 255, g_ * 255, b_ * 255 ) ) );
}

void GridVisualizer::propertyChanged( wxPropertyGridEvent& event )
{
  wxPGProperty* property = event.GetProperty();

  const wxString& name = property->GetName();
  wxVariant value = property->GetValue();

  if ( name == property_prefix_ + CELLCOUNT_PROPERTY )
  {
    int cell_count = value.GetLong();
    setCellCount( cell_count );
  }
  else if ( name == property_prefix_ + CELLSIZE_PROPERTY )
  {
    float cell_size = value.GetDouble();
    setCellSize( cell_size );
  }
  else if ( name == property_prefix_ + COLOR_PROPERTY )
  {
    wxColour color;
    color << value;

    setColor( color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f );
  }
}

void GridVisualizer::getColor( float& r, float& g, float& b )
{
  r = r_;
  g = g_;
  b = b_;
}

void GridVisualizer::loadProperties( wxConfigBase* config )
{
  long cell_count = 0;
  double cell_size = 0.0f;
  double r, g, b;

  {
    config->Read( CELLCOUNT_PROPERTY, &cell_count, cell_count_ );
  }

  {
    config->Read( CELLSIZE_PROPERTY, &cell_size, cell_size_ );
  }

  {
    config->Read( wxString(COLOR_PROPERTY) + wxT("R"), &r, r_ );
    config->Read( wxString(COLOR_PROPERTY) + wxT("G"), &g, g_ );
    config->Read( wxString(COLOR_PROPERTY) + wxT("B"), &b, b_ );
  }

  set( cell_count, cell_size, r, g, b );
}

void GridVisualizer::saveProperties( wxConfigBase* config )
{
  config->Write( CELLCOUNT_PROPERTY, (int)cell_count_ );
  config->Write( CELLSIZE_PROPERTY, (double)cell_size_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("R"), r_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("G"), g_ );
  config->Write( wxString(COLOR_PROPERTY) + wxT("B"), b_ );
}

} // namespace ogre_vis

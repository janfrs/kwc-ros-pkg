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

#include "property_manager.h"

#include <rosconsole/rosconsole.h>

#include <wx/wx.h>
#include <wx/confbase.h>

namespace ogre_vis
{

PropertyManager::PropertyManager( wxPropertyGrid* grid )
: grid_( grid )
{
  log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( ROSCONSOLE_DEFAULT_NAME ".properties" );
  if ( logger->getLevel() == 0  )
  {
    logger->setLevel( log4cxx::Level::getInfo() );
  }
}

PropertyManager::~PropertyManager()
{
  PROPERTY_DEBUG("PropertyManager Destructor");
  M_Property::iterator it = properties_.begin();
  M_Property::iterator end = properties_.end();
  for ( ; it != end; ++it )
  {
    PROPERTY_DEBUG("Deleting property: %s%s", it->first.first.c_str(), it->first.second.c_str());
    delete it->second;
  }
  properties_.clear();
}

CategoryProperty* PropertyManager::createCategory(const std::string& name, const std::string& prefix, CategoryProperty* parent, void* user_data)
{
  CategoryProperty* category = createProperty<CategoryProperty>(name, prefix, CategoryProperty::Getter(), CategoryProperty::Setter(), parent, user_data);
  category->setSave( false );

  return category;
}

void PropertyManager::deleteProperty( PropertyBase* property )
{
  if ( !property )
  {
    return;
  }

  PROPERTY_DEBUG("PropertyManager::deleteProperty( PropertyBase* property ): 0x%08x, %s%s", (int)property, property->getPrefix().c_str(), property->getName().c_str());

  deleteProperty( property->getName(), property->getPrefix() );
}

void PropertyManager::deleteProperty( const std::string& name, const std::string& prefix )
{
  PROPERTY_DEBUG("PropertyManager::deleteProperty( const std::string& name, const std::string& prefix ): %s%s", prefix.c_str(), name.c_str());

  M_Property::iterator found_it = properties_.find( std::make_pair( prefix, name ) );
  ROS_ASSERT( found_it != properties_.end() );

  std::set<PropertyBase*> to_delete;
  // search for any children of this property, and delete them as well
  M_Property::iterator prop_it = properties_.begin();
  M_Property::iterator prop_end = properties_.end();
  for ( ; prop_it != prop_end; ++prop_it )
  {
    PropertyBase* property = prop_it->second;

    if ( property->getParent() == found_it->second )
    {
      to_delete.insert( property );
    }
  }

  std::set<PropertyBase*>::iterator del_it = to_delete.begin();
  std::set<PropertyBase*>::iterator del_end = to_delete.end();
  for ( ; del_it != del_end; ++del_it )
  {
    deleteProperty( *del_it );
  }

  delete found_it->second;

  properties_.erase( found_it );
}

void PropertyManager::deleteByUserData( void* user_data )
{
  PROPERTY_DEBUG("PropertyManager::deleteByUserData: user_data=0x%08x", (int)user_data);

  std::set<PropertyBase*> to_delete;

  M_Property::iterator it = properties_.begin();
  M_Property::iterator end = properties_.end();
  for ( ; it != end; ++it )
  {
    PropertyBase* property = it->second;

    if ( property->getUserData() == user_data )
    {
      if ( !property->getParent() || property->getParent()->getUserData() != user_data )
      {
        to_delete.insert( property );
      }
    }
  }

  std::set<PropertyBase*>::iterator prop_it = to_delete.begin();
  std::set<PropertyBase*>::iterator prop_end = to_delete.end();
  for ( ; prop_it != prop_end; ++prop_it )
  {
    deleteProperty( *prop_it );
  }
}

void PropertyManager::propertyChanging( wxPropertyGridEvent& event )
{

}

void PropertyManager::propertyChanged( wxPropertyGridEvent& event )
{
  wxPGProperty* property = event.GetProperty();

  void* client_data = property->GetClientData();
  if ( client_data )
  {
    PropertyBase* property = reinterpret_cast<PropertyBase*>(client_data);

    property->readFromGrid();
  }
}

void PropertyManager::propertySet( PropertyBase* property )
{
  property->writeToGrid();
}

void PropertyManager::save( wxConfigBase* config )
{
  M_Property::iterator it = properties_.begin();
  M_Property::iterator end = properties_.end();
  for ( ; it != end; ++it )
  {
    PropertyBase* property = it->second;

    if ( property->getSave() )
    {
      property->saveToConfig( config );
    }
  }
}

void PropertyManager::load( wxConfigBase* config )
{
  M_Property::iterator it = properties_.begin();
  M_Property::iterator end = properties_.end();
  for ( ; it != end; ++it )
  {
    PropertyBase* property = it->second;

    if ( property->getSave() )
    {
      property->loadFromConfig( config );
    }
  }
}

}

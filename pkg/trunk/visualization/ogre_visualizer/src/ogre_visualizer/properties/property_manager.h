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

#ifndef OGRE_VISUALIZER_PROPERTY_MANAGER_H
#define OGRE_VISUALIZER_PROPERTY_MANAGER_H

#include "property.h"
#include "rosconsole/rosassert.h"

#include <boost/bind.hpp>

#include <map>

class wxPropertyGrid;
class wxPropertyGridEvent;
class wxConfigBase;

namespace ogre_vis
{

/**
 * \brief Manages a set of properties
 *
 * The PropertyManager manages a set of properties, allowing you to create, delete, and load/save them from disk
 */
class PropertyManager
{
public:
  /**
   * \brief Constructor
   * @param grid The property grid to be associated with these properties
   */
  PropertyManager( wxPropertyGrid* grid );
  /**
   * \brief Destructor
   */
  ~PropertyManager();

  /**
   * \brief Create a property.
   * @param name Name of the property (eg, "Color")
   * @param prefix Prefix for the property (eg, "Head Laser Scan")
   * @param getter Getter object (see boost::function and boost::bind).  Must be compatible with Property<T>::Getter
   * @param setter Setter object (see boost::function and boost::bind).  Must be compatible with Property<T>::Setter
   * @param parent Parent category
   * @param user_data User data to associate with the created property
   * @return The new property
   */
  template<typename T, typename G, typename S>
  T* createProperty(const std::string& name, const std::string& prefix, const G& getter, const S& setter, CategoryProperty* parent, void* user_data = NULL)
  {
    T* property = new T( name, prefix, grid_, parent ? parent->getPGProperty() : 0, getter, setter );
    bool inserted = properties_.insert( std::make_pair( std::make_pair(name, prefix), property ) ).second;
    ROS_ASSERT(inserted);

    property->writeToGrid();
    property->getPGProperty()->SetClientData( property );

    property->addChangedListener( boost::bind( &PropertyManager::propertySet, this, _1 ) );

    property->setUserData( user_data );

    return property;
  }

  /**
   * \brief Create a category property
   * @param name Name of the category
   * @param parent Parent category (may be NULL)
   * @return The new category property
   */
  CategoryProperty* createCategory(const std::string& name, CategoryProperty* parent);

  /**
   * \brief Delete a property
   * @param property The property to delete
   */
  void deleteProperty( PropertyBase* property );
  /**
   * \brief Delete a property, by name/prefix
   * @param name Name of the property
   * @param prefix Prefix of the property
   */
  void deleteProperty( const std::string& name, const std::string& prefix );
  /**
   * \brief Delete all properties that have a given user data
   * @param user_data The user data to compare against
   */
  void deleteByUserData( void* user_data );

  /**
   * \brief Called when a property in the property grid is changing.
   * @param event The event
   */
  void propertyChanging( wxPropertyGridEvent& event );
  /**
   * \brief Called when a property in the property grid has changed.
   * @param event The event
   */
  void propertyChanged( wxPropertyGridEvent& event );

  /**
   * \brief Called when a property has been set (ie, Property::changed() has been called)
   * @param property The property that was set
   */
  void propertySet( PropertyBase* property );

  /**
   * \brief Save all properties into a wxConfig
   * @param config The config to save to
   */
  void save( wxConfigBase* config );
  /**
   * \brief Load all existing properties' values from a wxConfig
   * @param config The config to load from
   */
  void load( wxConfigBase* config );

protected:
  wxPropertyGrid* grid_;        //< The property grid associated with our properties

  typedef std::map< std::pair<std::string, std::string>, PropertyBase* > M_Property;
  M_Property properties_;       //< The properties, mapped by name + prefix
};

} // namespace ogre_vis

#endif

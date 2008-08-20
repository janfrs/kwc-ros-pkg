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

#ifndef OGRE_VISUALIZER_ROS_TOPIC_PROPERTY_H
#define OGRE_VISUALIZER_ROS_TOPIC_PROPERTY_H

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/propdev.h>

namespace ros
{
class node;
}

namespace ogre_vis
{
class ROSTopicDialogAdapter : public wxPGEditorDialogAdapter
{
public:

  ROSTopicDialogAdapter( ros::node* node )
  : wxPGEditorDialogAdapter()
  , m_ROSNode( node )
  {
  }

  virtual bool DoShowDialog( wxPropertyGrid* WXUNUSED(propGrid), wxPGProperty* WXUNUSED(property) );

protected:
  ros::node* m_ROSNode;
};

//
// wxLongStringProperty has wxString as value type and TextCtrlAndButton as editor.
// Here we will derive a new property class that will show single choice dialog
// on button click.
//

class ROSTopicProperty : public wxLongStringProperty
{
  DECLARE_DYNAMIC_CLASS(ROSTopicProperty)
public:

  // Normal property constructor.
  ROSTopicProperty(ros::node* node, const wxString& label, const wxString& name = wxPG_LABEL, const wxString& value = wxEmptyString);

  // Do something special when button is clicked.
  virtual wxPGEditorDialogAdapter* GetEditorDialog() const
  {
    return new ROSTopicDialogAdapter( m_ROSNode );
  }

protected:
  ROSTopicProperty();

  ros::node* m_ROSNode;
};

} // namespace ogre_vis

#endif

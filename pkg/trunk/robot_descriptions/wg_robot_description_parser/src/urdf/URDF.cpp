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
 *   * Redistributions of source code must retain the above copyright
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

#include <urdf/URDF.h>
#include <math_utils/MathExpression.h>
#include <string_utils/string_utils.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <queue>

namespace robot_desc {
    
    /** Macro to mark the fact a certain member variable was set. Also
	prints a warning if the same member was set multiple times. */
#define MARK_SET(node, owner, variable)					\
    {									\
	if (owner->isSet[#variable]) {					\
	    errorMessage("'" + std::string(#variable) +			\
			 "' already set");				\
	    errorLocation(node); }					\
	else								\
	    owner->isSet[#variable] = true;				\
    }
    

    /** Operator for sorting objects by name */
    template<typename T>
    struct SortByName
    {
	bool operator()(const T *a, const T *b) const
	{
	    return a->name < b->name;
	}
    };
    
    void URDF::freeMemory(void)
    {
	clearDocs();
	
	for (std::map<std::string, Link*>::iterator i = m_links.begin() ; i != m_links.end() ; i++)
	    delete i->second;
	for (std::map<std::string, Group*>::iterator i = m_groups.begin() ; i != m_groups.end() ; i++)
	    delete i->second;
	for (std::map<std::string, Actuator*>::iterator i = m_actuators.begin() ; i != m_actuators.end() ; i++)
	    delete i->second;
	for (std::map<std::string, Transmission*>::iterator i = m_transmissions.begin() ; i != m_transmissions.end() ; i++)
	    delete i->second;
	for (std::map<std::string, Frame*>::iterator i = m_frames.begin() ; i != m_frames.end() ; i++)
	    delete i->second;
    }
    
    void URDF::clear(void)
    {
	freeMemory();
	m_name.clear();
	m_source.clear();
	m_location.clear();
	m_links.clear();
	m_actuators.clear();
	m_transmissions.clear();
	m_frames.clear();
	m_groups.clear();
	m_paths.clear();
	m_paths.push_back("");
	m_linkRoots.clear();
	clearTemporaryData();
	m_errorCount = 0;
    }
    
    unsigned int URDF::getErrorCount(void) const
    {
	return m_errorCount;	
    }
    
    const std::string& URDF::getRobotName(void) const
    {
	return m_name;
    }
    
    URDF::Link* URDF::getLink(const std::string &name) const
    {
	std::map<std::string, Link*>::const_iterator it = m_links.find(name);
	return it == m_links.end() ? NULL : it->second;
    }
    
    void URDF::getLinks(std::vector<Link*> &links) const
    {
	std::vector<Link*> localLinks;
	for (std::map<std::string, Link*>::const_iterator i = m_links.begin() ; i != m_links.end() ; i++)
	    localLinks.push_back(i->second);
	std::sort(localLinks.begin(), localLinks.end(), SortByName<Link>());
	links.insert(links.end(), localLinks.begin(), localLinks.end());
    }

    URDF::Frame* URDF::getFrame(const std::string &name) const
    {
	std::map<std::string, Frame*>::const_iterator it = m_frames.find(name);
	return it == m_frames.end() ? NULL : it->second;
    }
    
    void URDF::getFrames(std::vector<Frame*> &frames) const
    {
	std::vector<Frame*> localFrames;
	for (std::map<std::string, Frame*>::const_iterator i = m_frames.begin() ; i != m_frames.end() ; i++)
	    localFrames.push_back(i->second);
	std::sort(localFrames.begin(), localFrames.end(), SortByName<Frame>());
	frames.insert(frames.end(), localFrames.begin(), localFrames.end());
    }
    
    URDF::Actuator* URDF::getActuator(const std::string &name) const
    {
	std::map<std::string, Actuator*>::const_iterator it = m_actuators.find(name);
	return it == m_actuators.end() ? NULL : it->second;
    }

    void URDF::getActuators(std::vector<Actuator*> &actuators) const
    {
	std::vector<Actuator*> localActuators;
	for (std::map<std::string, Actuator*>::const_iterator i = m_actuators.begin() ; i != m_actuators.end() ; i++)
	    localActuators.push_back(i->second);
	std::sort(localActuators.begin(), localActuators.end(), SortByName<Actuator>());
	actuators.insert(actuators.end(), localActuators.begin(), localActuators.end());
    }

    URDF::Transmission* URDF::getTransmission(const std::string &name) const
    {
	std::map<std::string, Transmission*>::const_iterator it = m_transmissions.find(name);
	return it == m_transmissions.end() ? NULL : it->second;
    }
    
    void URDF::getTransmissions(std::vector<Transmission*> &transmissions) const
    {
	std::vector<Transmission*> localTransmissions;
	for (std::map<std::string, Transmission*>::const_iterator i = m_transmissions.begin() ; i != m_transmissions.end() ; i++)
	    localTransmissions.push_back(i->second);
	std::sort(localTransmissions.begin(), localTransmissions.end(), SortByName<Transmission>());
	transmissions.insert(transmissions.end(), localTransmissions.begin(), localTransmissions.end());
    }
    
    unsigned int URDF::getDisjointPartCount(void) const
    {
	return m_linkRoots.size();
    }
    
    URDF::Link* URDF::getDisjointPart(unsigned int index) const
    {
	if (index < m_linkRoots.size())
	    return m_linkRoots[index];
	else
	    return NULL;
    }
    
    bool URDF::isRoot(const Link* link) const
    {
	for (unsigned int i = 0 ; i < m_linkRoots.size() ; ++i)
	    if (link == m_linkRoots[i])
		return true;
	return false;
    }
    
    void URDF::getGroupNames(std::vector<std::string> &groups) const
    {
	std::vector<std::string> localGroups;
	for (std::map<std::string, Group*>::const_iterator i = m_groups.begin() ; i != m_groups.end() ; i++)
	    localGroups.push_back(i->first);
	std::sort(localGroups.begin(), localGroups.end());
	groups.insert(groups.end(), localGroups.begin(), localGroups.end());
    }
    
    URDF::Group* URDF::getGroup(const std::string &name) const
    {
	std::map<std::string, Group*>::const_iterator it = m_groups.find(name);
	return (it == m_groups.end()) ? NULL : it->second;
    }
    
    void URDF::getGroups(std::vector<Group*> &groups) const
    {
	/* To maintain the same ordering as getGroupNames, we do this in a slightly more cumbersome way */
	std::vector<std::string> names;
	getGroupNames(names);
	for (unsigned int i = 0 ; i < names.size() ; ++i)
	    groups.push_back(getGroup(names[i]));
    }
    
    const URDF::Data& URDF::getData(void) const
    {
	return m_data;
    }
    
    void URDF::Data::add(const std::string &type, const std::string &name, const std::string &key, const std::string &value)
    {
	if (!m_data[type][name][key].str)
	    m_data[type][name][key].str = new std::string();
	*(m_data[type][name][key].str) = value;
    }
    
    void URDF::Data::add(const std::string &type, const std::string &name, const std::string &key, const TiXmlElement *value)
    {
	m_data[type][name][key].xml = value;
    }
    
    bool URDF::Data::hasDefault(const std::string &key) const
    {
	std::map<std::string, std::string> m = getDataTagValues("", "");
	return m.find(key) != m.end();
    }
    
    std::string URDF::Data::getDefaultValue(const std::string &key) const
    {
	return getDataTagValues("", "")[key];
    }
    
    const TiXmlElement* URDF::Data::getDefaultXML(const std::string &key) const
    {
	return getDataTagXML("", "")[key];
    }
    
    void URDF::Data::getDataTagTypes(std::vector<std::string> &types) const
    {
	for (std::map<std::string, std::map<std::string, std::map<std::string, Element > > >::const_iterator i = m_data.begin() ; i != m_data.end() ; ++i)
	    types.push_back(i->first);
    }
    
    void URDF::Data::getDataTagNames(const std::string &type, std::vector<std::string> &names) const
    {
	std::map<std::string, std::map<std::string, std::map<std::string, Element > > >::const_iterator pos = m_data.find(type);
	if (pos != m_data.end())
	{
	    for (std::map<std::string, std::map<std::string, Element > >::const_iterator i = pos->second.begin() ; i != pos->second.end() ; ++i)
		names.push_back(i->first);
	}
    }
    std::map<std::string, std::string> URDF::Data::getDataTagValues(const std::string &type, const std::string &name) const
    {    
	std::map<std::string, std::string> result;
	
	std::map<std::string, std::map<std::string, std::map<std::string, Element > > >::const_iterator pos = m_data.find(type);
	if (pos != m_data.end())
	{
	    std::map<std::string, std::map<std::string, Element > >::const_iterator m = pos->second.find(name);
	    if (m != pos->second.end())
	    {
		for (std::map<std::string, Element>::const_iterator it = m->second.begin() ; it != m->second.end() ; it++)
		    if (it->second.str)
			result[it->first] = *(it->second.str);
	    }
	}
	return result;
    }
    
    std::map<std::string, const TiXmlElement*> URDF::Data::getDataTagXML(const std::string &type, const std::string &name) const
    {    
	std::map<std::string, const TiXmlElement*> result;
	
	std::map<std::string, std::map<std::string, std::map<std::string, Element > > >::const_iterator pos = m_data.find(type);
	if (pos != m_data.end())
	{
	    std::map<std::string, std::map<std::string, Element > >::const_iterator m = pos->second.find(name);
	    if (m != pos->second.end())
	    {
		for (std::map<std::string, Element>::const_iterator it = m->second.begin() ; it != m->second.end() ; it++)
		    if (it->second.xml)
			result[it->first] = it->second.xml;
	    }
	}
	return result;
    }
    
    bool URDF::containsCycle(unsigned int index) const
    {
	if (index >= m_linkRoots.size())
	    return false;
	
	std::map<Link*, bool> seen;
	
	std::queue<Link*> queue;
	queue.push(m_linkRoots[index]);
	
	while (!queue.empty())
	{
	    Link* link = queue.front();
	    queue.pop();
	    seen[link] = true;
	    for (unsigned int i = 0 ; i < link->children.size() ; ++i) {
		if (seen.find(link->children[i]) != seen.end())
		    return true;
		else
		    queue.push(link->children[i]);
	    }
	}
	
	return false;
    }
    
    void URDF::print(std::ostream &out) const
    {
	out << std::endl << "List of root links in robot '"<< m_name << "' (" << m_linkRoots.size() << ") :" << std::endl;
	for (unsigned int i = 0 ; i < m_linkRoots.size() ; ++i)
	    m_linkRoots[i]->print(out, "  ");
	out << std::endl << "Frames:" << std::endl;
	for (std::map<std::string, Frame*>::const_iterator i = m_frames.begin() ; i != m_frames.end() ; i++)
	    i->second->print(out, "  ");
	out << std::endl << "Actuators:" << std::endl;
	for (std::map<std::string, Actuator*>::const_iterator i = m_actuators.begin() ; i != m_actuators.end() ; i++)
	    i->second->print(out, "  "); 
	out << std::endl << "Transmissions:" << std::endl;
	for (std::map<std::string, Transmission*>::const_iterator i = m_transmissions.begin() ; i != m_transmissions.end() ; i++)
	    i->second->print(out, "  ");
	out << std::endl << "Data types:" << std::endl;
	m_data.print(out, "  ");
    }
    
    void URDF::Data::print(std::ostream &out, std::string indent) const
    {
	for (std::map<std::string, std::map<std::string, std::map<std::string, Element > > >::const_iterator i = m_data.begin() ; i != m_data.end() ; ++i)
	{
	    out << indent << "Data of type '" << i->first << "':" << std::endl;
	    for (std::map<std::string, std::map<std::string, Element > >::const_iterator j = i->second.begin() ; j != i->second.end() ; ++j)
	    {
		out << indent << "  [" << j->first << "]" << std::endl;
		for (std::map<std::string, Element>::const_iterator k = j->second.begin() ; k != j->second.end() ; ++k)
		    out << indent << "    " << k->first << " = " << (k->second.str != NULL ? *(k->second.str) : "") << (k->second.xml != NULL ? " [XML]" : "") << std::endl;
	    }
	}
    }
    
    void URDF::Frame::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Frame [" << name << "]:" << std::endl;
	out << indent << "  - type: " << type << std::endl;
	out << indent << "  - rpy: (" <<  rpy[0] << ", " << rpy[1] << ", " << rpy[2] << ")" << std::endl;
	out << indent << "  - xyz: (" <<  xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" << std::endl;
	out << indent << "  - link: " << (link ? link->name : "") << std::endl;

	out << indent << "  - groups: ";
	for (unsigned int i = 0 ; i < groups.size() ; ++i)
	{
	    out << groups[i]->name << " ( ";
	    for (unsigned int j = 0 ; j < groups[i]->flags.size() ; ++j)
		out << groups[i]->flags[j] << " ";
	    out << ") ";
	}
	out << std::endl;
	data.print(out, indent + "  ");
    }
    
    void URDF::Transmission::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Transmission [" << name << "]:" << std::endl;
	data.print(out, indent + "  ");
    }
    
    void URDF::Actuator::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Actuator [" << name << "]:" << std::endl;
	data.print(out, indent + "  ");
    }
     
    void URDF::Link::Geometry::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Geometry [" << name << "]:" << std::endl;
	out << indent << "  - type: " << type << std::endl;
	out << indent << "  - size: ( ";
	for (int i = 0 ; i < nsize ; ++i)
	    out << size[i] << " ";
	out << ")" << std::endl;
	out << indent << "  - filename: " << filename << std::endl;
	data.print(out, indent + "  ");
    }
    
    void URDF::Link::Joint::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Joint [" << name << "]:" << std::endl;
	out << indent << "  - type: " << type << std::endl;
	out << indent << "  - axis: (" <<  axis[0] << ", " << axis[1] << ", " << axis[2] << ")" << std::endl;
	out << indent << "  - anchor: (" <<  anchor[0] << ", " << anchor[1] << ", " << anchor[2] << ")" << std::endl;
	out << indent << "  - limit: (" <<  limit[0] << ", " << limit[1] << ")" << std::endl;
	out << indent << "  - effortLimit: " << effortLimit << std::endl;
	out << indent << "  - velocityLimit: " << velocityLimit << std::endl;
	out << indent << "  - calibration: " << calibration << std::endl;
	data.print(out, indent + "  ");
    }
    
    void URDF::Link::Collision::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Collision [" << name << "]:" << std::endl;
	out << indent << "  - verbose: " << (verbose ? "Yes" : "No") << std::endl;
	out << indent << "  - material: " << material << std::endl;
	out << indent << "  - rpy: (" <<  rpy[0] << ", " << rpy[1] << ", " << rpy[2] << ")" << std::endl;
	out << indent << "  - xyz: (" <<  xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" << std::endl;
	geometry->print(out, indent + "  ");
	data.print(out, indent + "  ");
    }
    
    void URDF::Link::Inertial::print(std::ostream &out, std::string indent) const
    {	
	out << indent << "Inertial [" << name << "]:" << std::endl;
	out << indent << "  - mass: " << mass << std::endl;
	out << indent << "  - com: (" <<  com[0] << ", " << com[1] << ", " << com[2] << ")" << std::endl;
	out << indent << "  - inertia: (" <<  inertia[0] << ", " << inertia[1] << ", " << inertia[2] << ", " << inertia[3] << ", " << inertia[4] << ", " << inertia[5] << ")" << std::endl;
	data.print(out, indent + "  ");
    }
    
    void URDF::Link::Visual::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Visual [" << name << "]:" << std::endl;
	out << indent << "  - material: " << material << std::endl;
	out << indent << "  - scale: " << scale[0] << ", " << scale[1] << ", " << scale[2] << ")" << std::endl;
	out << indent << "  - rpy: (" <<  rpy[0] << ", " << rpy[1] << ", " << rpy[2] << ")" << std::endl;
	out << indent << "  - xyz: (" <<  xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" << std::endl;
	geometry->print(out, indent + "  ");
	data.print(out, indent + "  ");
    }
    
    void URDF::Link::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Link [" << name << "]:" << std::endl;
	out << indent << "  - parent link: " << parentName << std::endl;
	out << indent << "  - rpy: (" <<  rpy[0] << ", " << rpy[1] << ", " << rpy[2] << ")" << std::endl;
	out << indent << "  - xyz: (" <<  xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")" << std::endl;
	joint->print(out, indent+ "  ");
	collision->print(out, indent+ "  ");
	inertial->print(out, indent+ "  ");
	visual->print(out, indent+ "  ");
	
	out << indent << "  - groups: ";
	for (unsigned int i = 0 ; i < groups.size() ; ++i)
	{      
	    out << groups[i]->name <<  " ( ";
	    for (unsigned int j = 0 ; j < groups[i]->flags.size() ; ++j)
		out << groups[i]->flags[j] << " ";
	    out << ") ";
	}
	out << std::endl;
	
	out << indent << "  - children links: ";
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	    out << children[i]->name << " ";
	out << std::endl;

	for (unsigned int i = 0 ; i < children.size() ; ++i)
	    children[i]->print(out, indent + "  ");
	data.print(out, indent + "  ");
    }
    
    void URDF::Sensor::print(std::ostream &out, std::string indent) const
    {
	out << indent << "Sensor:" << std::endl;
	out << indent << "  - type: " << type << std::endl;
	out << indent << "  - calibration: " << calibration << std::endl;
	Link::print(out, indent + "  ");
    }
    
    bool URDF::Link::canSense(void) const
    {
	return false;
    }
    
    bool URDF::Sensor::canSense(void) const
    {
	return true;
    }
    
    bool URDF::Link::insideGroup(Group *group) const
    {
	for (unsigned int i = 0 ; i < groups.size() ; ++i)
	    if (groups[i] == group)
		return true;
	return false;
    }

    bool URDF::Link::insideGroup(const std::string &group) const
    {
	for (unsigned int i = 0 ; i < groups.size() ; ++i)
	    if (groups[i]->name == group)
		return true;
	return false;
    }
    
    bool URDF::Group::hasFlag(const std::string &flag) const
    {
	for (unsigned int i = 0 ; i < flags.size() ; ++i)
	    if (flags[i] == flag)
		return true;
	return false;	  
    }    
    
    bool URDF::Group::isRoot(const Link* link) const
    {
	for (unsigned int i = 0 ; i < linkRoots.size() ; ++i)
	    if (linkRoots[i] == link)
		return true;
	return false;    
    }
    
    void URDF::errorMessage(const std::string &msg) const
    {
	std::cerr << msg << std::endl;
	m_errorCount++;
    }
    
    void URDF::errorLocation(const TiXmlNode* node) const
    {
	if (!m_location.empty())
	{
	    std::cerr << "  ... at " << m_location;
	    if (!node)
		std::cerr << std::endl;
	}
	
	if (node)
	{
	    /* find the document the node is part of */
	    const TiXmlNode* doc = node;
	    while (doc && doc->Type() != TiXmlNode::DOCUMENT)
		doc = doc->Parent();
	    const char *filename = doc ? reinterpret_cast<const char*>(doc->GetUserData()) : NULL;
	    std::cerr << (m_location.empty() ? "  ..." : ",") << " line " << node->Row() << ", column " << node->Column();
	    if (filename)
		std::cerr << " (" << filename << ")";
	    std::cerr << std::endl;
	}
    }
    
    void URDF::ignoreNode(const TiXmlNode* node)
    {
	switch (node->Type())
	{
	case TiXmlNode::ELEMENT:
	    errorMessage("Ignoring element node '" + node->ValueStr() + "'");
	    errorLocation(node);  
	    break;
	case TiXmlNode::TEXT:
	    errorMessage("Ignoring text node with content '" + node->ValueStr() + "'");
	    errorLocation(node);  
	    break;
	case TiXmlNode::COMMENT:
	case TiXmlNode::DECLARATION:
	    break;            
	case TiXmlNode::UNKNOWN:
	default:
	    errorMessage("Ignoring unknown node '" + node->ValueStr() + "'");
	    errorLocation(node);  
	    break;
	}
    }
    
    void URDF::getChildrenAndAttributes(const TiXmlNode *node, std::vector<const TiXmlNode*> &children, std::vector<const TiXmlAttribute*> &attributes) const
    {
	for (const TiXmlNode *child = node->FirstChild() ; child ; child = child->NextSibling())
	    children.push_back(child);
	if (node->Type() == TiXmlNode::ELEMENT)
	{
	    for (const TiXmlAttribute *attr = node->ToElement()->FirstAttribute() ; attr ; attr = attr->Next())
	    {
		if (strcmp(attr->Name(), "clone") == 0)
		{
		    std::map<std::string, const TiXmlNode*>::const_iterator pos = m_templates.find(attr->ValueStr());
		    if (pos == m_templates.end())
		    {
			errorMessage("Template '" + attr->ValueStr() + "' is not defined");
			errorLocation(node);
		    }	
		    else
			getChildrenAndAttributes(pos->second, children, attributes);
		}
		else
		    attributes.push_back(attr);
	    }
	}
    }
    
    void URDF::defaultConstants(void)
    {
	m_constants["M_PI"] = "3.14159265358979323846";
    }
    
    void URDF::clearDocs(void)
    {
	/* clear memory allocated for loaded documents */
	for (unsigned int i = 0 ; i < m_docs.size() ; ++i)
	{
	    char *filename = reinterpret_cast<char*>(m_docs[i]->GetUserData());
	    if (filename)
		free(filename);
	    delete m_docs[i];
	}	
	m_docs.clear();
    }
    
    void URDF::clearTemporaryData(void)
    {	
	m_collision.clear();
	m_joints.clear();
	m_inertial.clear();
	m_visual.clear();
	m_geoms.clear();
	
	m_constants.clear();
	m_templates.clear();
	m_stage2.clear();
    }
    
    bool URDF::loadStream(std::istream &is)
    {
	if (!is.good())
	    return false;
	
	is.seekg(0, std::ios::end);
	std::streampos length = is.tellg();
	is.seekg(0, std::ios::beg);
	if (length >= 0)
	{
	    char *buffer = new char[length];
	    is.read(buffer, length);
	    bool result = (is.gcount() == length) ? loadString(buffer) : false;
	    delete[] buffer;
	    return result;
	}
	else
	    return false;
    }
    
    bool URDF::loadString(const char *data)
    {
	clear();
	bool result = false;
	
	TiXmlDocument *doc = new TiXmlDocument();
	doc->SetUserData(NULL);	
	m_docs.push_back(doc);
	if (doc->Parse(data))
	{
	    defaultConstants();
	    result = parse(dynamic_cast<const TiXmlNode*>(doc));
	}
	else
	    errorMessage(doc->ErrorDesc());
	
	return result;
    }  
    
    bool URDF::loadFile(FILE *file)
    {
	clear();
	bool result = false;
	
	TiXmlDocument *doc = new TiXmlDocument();
	doc->SetUserData(NULL);	
	m_docs.push_back(doc);
	if (doc->LoadFile(file))
	{
	    defaultConstants();
	    result = parse(dynamic_cast<const TiXmlNode*>(doc));
	}
	else
	    errorMessage(doc->ErrorDesc());
	
	return result;
    }
    
    bool URDF::loadFile(const char *filename)
    {
	clear();
	bool result = false;
	m_source = filename;
	
	TiXmlDocument *doc = new TiXmlDocument(filename);
	doc->SetUserData(filename ? reinterpret_cast<void*>(strdup(filename)) : NULL);	
	m_docs.push_back(doc);
	if (doc->LoadFile())
	{
	    addPath(filename);
	    defaultConstants();
	    result = parse(dynamic_cast<const TiXmlNode*>(doc));
	}
	else
	    errorMessage(doc->ErrorDesc());
	
	return result;
    }
    
    void URDF::addPath(const char *filename)
    {
	if (!filename)
	    return;
	
	std::string name = filename;
	std::string::size_type pos = name.find_last_of("/\\");
	if (pos != std::string::npos)
	{
	    char sep = name[pos];
	    name.erase(pos);
	    m_paths.push_back(name + sep);
	}    
    }
    
    char* URDF::findFile(const char *filename)
    {
	for (unsigned int i = 0 ; i < m_paths.size() ; ++i)
	{
	    std::string name = m_paths[i] + filename;
	    std::fstream fin;
	    fin.open(name.c_str(), std::ios::in);
	    bool good = fin.is_open();
	    fin.close();
	    if (good)
		return strdup(name.c_str());        
	}
	return NULL;
    }
    
    std::string URDF::extractName(std::vector<const TiXmlAttribute*> &attributes, const std::string &defaultName) const
    { 
	std::string name = defaultName;
	for (unsigned int i = 0 ; i < attributes.size() ; ++i)
	{
	    if (strcmp(attributes[i]->Name(), "name") == 0)
	    {
		name = attributes[i]->ValueStr();
		attributes.erase(attributes.begin() + i);
		break;
	    }
	}
	return name;
    }
    
    struct getConstantData
    {
	getConstantData(void)
	{
	    m = NULL;
	    errorCount = 0;
	}
	
	std::map<std::string, std::string> *m;
	std::map<std::string, bool>         s;
	
	unsigned int                        errorCount;
	std::vector<std::string>            errorMsg;
    };
    
    static double getConstant(void *data, std::string &name)
    {
	getConstantData *d = reinterpret_cast<getConstantData*>(data);
	std::map<std::string, std::string> *m = d->m;
	assert(m);
	if (m->find(name) == m->end())
	{
	    d->errorMsg.push_back("Request for undefined constant: '" + name + "'");
	    d->errorCount++;
	    return 0.0;
	}
	else
	{
	    if (meval::ContainsOperators((*m)[name]))
	    {
		std::map<std::string, bool>::iterator pos = d->s.find((*m)[name]);
		if (pos != d->s.end() && pos->second == true)
		{
		    d->errorMsg.push_back("Recursive definition of constant '" + name + "'");
		    d->errorCount++;
		    return 0.0;
		}
		d->s[(*m)[name]] = true;
		double result = meval::EvaluateMathExpression((*m)[name], &getConstant, data);
		d->s[(*m)[name]] = false;
		return result;
	    }
	    else
		return atof((*m)[name].c_str());
	}
    }
    
    unsigned int URDF::loadDoubleValues(const TiXmlNode *node, unsigned int count, double *vals)
    {
	if (node && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
	    node = node->FirstChild();
	if (node && node->Type() == TiXmlNode::TEXT)
	    return loadDoubleValues(node->ValueStr(), count, vals, node);
	else
	    return 0;
    }
    
    unsigned int URDF::loadDoubleValues(const std::string &data, unsigned int count, double *vals, const TiXmlNode *node)
    {
	std::stringstream ss(data);
	unsigned int read = 0;
	
	for (unsigned int i = 0 ; ss.good() && i < count ; ++i)
	{
	    std::string value;
	    ss >> value;
	    getConstantData data;
	    data.m = &m_constants;
	    vals[i] = meval::EvaluateMathExpression(value, &getConstant, reinterpret_cast<void*>(&data));
	    read++;
	    for (unsigned int k = 0 ; k < data.errorMsg.size() ; ++k)
		errorMessage(data.errorMsg[k]);
	    if (data.errorCount)
	    {
		m_errorCount += data.errorCount;
		errorLocation(node);
	    } 
	}

	if (ss.good())
	{
	    std::string extra = ss.str();
	    while (!extra.empty())
	    {
		char last = extra[extra.size() - 1];
		if (last == ' ' || last == '\n' || last == '\t')
		    extra.erase(extra.size() - 1);
		else
		    break;	  
	    }
	    if (!extra.empty())
	    {
		errorMessage("More data available (" + string_utils::convert2str(read) + " read, rest is ignored): '" + data + "'");
		errorLocation(node);
	    }	    
	}
	
	if (read != count)
	{
	    errorMessage("Not all values were read: '" + data + "'");
	    errorLocation(node);
	}  
	
	return read;
    }
    
    unsigned int URDF::loadBoolValues(const TiXmlNode *node, unsigned int count, bool *vals)
    {
	if (node && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
	    node = node->FirstChild();
	if (node && node->Type() == TiXmlNode::TEXT)
	    return loadBoolValues(node->ValueStr(), count, vals, node);
	else
	    return 0;
    }
    
    unsigned int URDF::loadBoolValues(const std::string& data, unsigned int count, bool *vals, const TiXmlNode *node)
    {
	std::stringstream ss(data);
	unsigned int read = 0;
	
	for (unsigned int i = 0 ; ss.good() && i < count ; ++i)
	{
	    std::string value;
	    ss >> value;
	    const unsigned int length = value.length();
	    for(unsigned int j = 0 ; j < length ; ++j)
		value[j] = std::tolower(value[j]);        
	    vals[i] = (value == "true" || value == "yes" || value == "1");
	    read++;
	}

	if (ss.good())
	{
	    std::string extra = ss.str();
	    while (!extra.empty())
	    {
		char last = extra[extra.size() - 1];
		if (last == ' ' || last == '\n' || last == '\t')
		    extra.erase(extra.size() - 1);
		else
		    break;	  
	    }
	    if (!extra.empty())
	    {	
		errorMessage("More data available (" + string_utils::convert2str(read) + " read, rest is ignored): '" + data + "'");
		errorLocation(node);
	    }	    
	}
	
	if (read != count)
	{
	    errorMessage("Not all values were read: '" + data + "'");
	    errorLocation(node);  
	}
	
	return read;
    }
    
    void URDF::loadTransmission(const TiXmlNode *node)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, "");    
	Transmission *transmission = (m_transmissions.find(name) != m_transmissions.end()) ? m_transmissions[name] : new Transmission();
	transmission->name = name;
	if (transmission->name.empty())
	{
	    errorMessage("No transmission name given");
	    errorLocation(node);
	}	
	else
	    MARK_SET(node, transmission, name);
	
	m_transmissions[transmission->name] = transmission;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "data")
		    loadData(node, &transmission->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}    
    }
    
    void URDF::loadActuator(const TiXmlNode *node)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, "");    
	Actuator *actuator = (m_actuators.find(name) != m_actuators.end()) ? m_actuators[name] : new Actuator();
	actuator->name = name;
	if (actuator->name.empty())
	{
	    errorMessage("No actuator name given");
	    errorLocation(node);
	}
	else
	    MARK_SET(node, actuator, name);
	
	m_actuators[actuator->name] = actuator;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "data")
		    loadData(node, &actuator->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}    
    }
    
    void URDF::loadFrame(const TiXmlNode *node)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, "");    
	Frame *frame = (m_frames.find(name) != m_frames.end()) ? m_frames[name] : new Frame();
	frame->name = name;
	if (frame->name.empty())
	{
	    errorMessage("No frame name given");
	    errorLocation(node);
	}
	else
	    MARK_SET(node, frame, name);
	
	m_frames[frame->name] = frame;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "rpy")
		{
		    loadDoubleValues(node, 3, frame->rpy);
		    MARK_SET(node, frame, rpy);		    
		}		
		else if (node->ValueStr() == "xyz")
		{
		    loadDoubleValues(node, 3, frame->xyz);
		    MARK_SET(node, frame, xyz);
		}
		else if (node->ValueStr() == "parent" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    frame->linkName = node->FirstChild()->ValueStr();
		    MARK_SET(node, frame, parent);
		    if (frame->type == Frame::CHILD)
			errorMessage("Frame '" + frame->name + "' can only have either a child or a parent link");
		    frame->type = Frame::PARENT;
		}
		else if (node->ValueStr() == "child" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    frame->linkName = node->FirstChild()->ValueStr();
		    MARK_SET(node, frame, child);
		    if (frame->type == Frame::PARENT)
			errorMessage("Frame '" + frame->name + "' can only have either a child or a parent link");
		    frame->type = Frame::CHILD;
		}
		else
		if (node->ValueStr() == "data")
		    loadData(node, &frame->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}    
    }
    
    void URDF::loadJoint(const TiXmlNode *node, const std::string& defaultName, Link::Joint *joint)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, defaultName);
	if (joint && !name.empty())
	    MARK_SET(node, joint, name);
	
	if (!joint)
	{
	    if (m_joints.find(name) == m_joints.end())
	    {
		errorMessage("Attempting to add information to an undefined joint: '" + name + "'");
		errorLocation(node);		
		return;
	    }
	    else
		joint = m_joints[name];
	}
	
	joint->name = name;
	m_joints[name] = joint;
	
	for (unsigned int i = 0 ; i < attributes.size() ; ++i)
	{
	    const TiXmlAttribute *attr = attributes[i];
	    if (strcmp(attr->Name(), "type") == 0)
	    {
		if (attr->ValueStr() == "fixed")
		    joint->type = Link::Joint::FIXED;
		else if (attr->ValueStr() == "revolute")
		    joint->type = Link::Joint::REVOLUTE;
		else if (attr->ValueStr() == "prismatic")
		    joint->type = Link::Joint::PRISMATIC;
		else if (attr->ValueStr() == "floating")
		    joint->type = Link::Joint::FLOATING;
		else if (attr->ValueStr() == "planar")
		    joint->type = Link::Joint::PLANAR;
		else
		{
		    errorMessage("Unknown joint type: '" + attr->ValueStr() + "'");
		    errorLocation(node);
		}
		MARK_SET(node, joint, type);
	    }
	}
	
	bool free = joint->type == Link::Joint::PLANAR || joint->type == Link::Joint::FLOATING;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "axis" && !free)
		{
		    loadDoubleValues(node, 3, joint->axis);
		    MARK_SET(node, joint, axis);
		}		
		else if (node->ValueStr() == "anchor" && !free)
		{
		    loadDoubleValues(node, 3, joint->anchor);
		    MARK_SET(node, joint, anchor);
		}		
		else if (node->ValueStr() == "limit" && !free)
		{
		    loadDoubleValues(node, 2, joint->limit);
		    MARK_SET(node, joint, limit);
		}
		else if (node->ValueStr() == "effortLimit" && !free)
		{
		    loadDoubleValues(node, 1, &joint->effortLimit);
		    MARK_SET(node, joint, effortLimit);
		}
		else if (node->ValueStr() == "velocityLimit")
		{
		    loadDoubleValues(node, 1, &joint->velocityLimit);
		    MARK_SET(node, joint, velocityLimit);
		}
		else if (node->ValueStr() == "calibration" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    joint->calibration = node->FirstChild()->ValueStr();
		    MARK_SET(node, joint, calibration);		    
		}	
		else if (node->ValueStr() == "data")
		    loadData(node, &joint->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}
	
    }
    
    void URDF::loadGeometry(const TiXmlNode *node, const std::string &defaultName, Link::Geometry *geometry)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, defaultName);
	if (geometry && !name.empty())
	    MARK_SET(node, geometry, name);

	if (!geometry)
	{
	    if (m_geoms.find(name) == m_geoms.end())
	    {
		errorMessage("Attempting to add information to an undefined geometry: '" + name + "'");
		errorLocation(node);		
		return;
	    }
	    else
		geometry = m_geoms[name];
	}
	
	geometry->name = name;
	m_geoms[name] = geometry;
	
	for (unsigned int i = 0 ; i < attributes.size() ; ++i)
	{
	    const TiXmlAttribute *attr = attributes[i];
	    if (strcmp(attr->Name(), "type") == 0)
	    {
		if (attr->ValueStr() == "box")
		{
		    geometry->type = Link::Geometry::BOX;
		    geometry->nsize = 3;	  
		}      
		else if (attr->ValueStr() == "cylinder")
		{
		    geometry->type = Link::Geometry::CYLINDER;
		    geometry->nsize = 2;	  
		}
		else if (attr->ValueStr() == "sphere")
		{
		    geometry->type = Link::Geometry::SPHERE;
		    geometry->nsize = 1;	  
		}
		else if (attr->ValueStr() == "mesh")
		{
		    geometry->type = Link::Geometry::MESH;
		    geometry->nsize = 3;
		}      
		else
		{
		    errorMessage("Unknown geometry type: '" + attr->ValueStr() + "'");
		    errorLocation(node);
		}
		MARK_SET(node, geometry, type);
	    }
	}
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "size")
		{
		    if (geometry->nsize > 0)
			loadDoubleValues(node, geometry->nsize, geometry->size);
		    else
			ignoreNode(node);
		    MARK_SET(node, geometry, size);
		}
		else if (node->ValueStr() == "data")
		    loadData(node, &geometry->data);
		else if (node->ValueStr() == "filename" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    geometry->filename = node->FirstChild()->ValueStr();
		    MARK_SET(node, geometry, filename);
		}		
		else                
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}
    }
    
    void URDF::loadCollision(const TiXmlNode *node, const std::string &defaultName, Link::Collision *collision)
    {  
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, defaultName);
	if (collision && !name.empty())
	    MARK_SET(node, collision, name);
	
	if (!collision)
	{
	    if (m_collision.find(name) == m_collision.end())
	    {
		errorMessage("Attempting to add information to an undefined collision: '" + name + "'");
		errorLocation(node);
		return;
	    }
	    else
		collision = m_collision[name];
	}
	
	collision->name = name;
	m_collision[name] = collision;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "rpy")
		{
		    loadDoubleValues(node, 3, collision->rpy);
		    MARK_SET(node, collision, rpy);		    
		}		
		else if (node->ValueStr() == "xyz")
		{
		    loadDoubleValues(node, 3, collision->xyz);
		    MARK_SET(node, collision, xyz);
		}		
		else if (node->ValueStr() == "verbose")
		{
		    loadBoolValues(node, 1, &collision->verbose);
		    MARK_SET(node, collision, verbose);
		}		
		else if (node->ValueStr() == "material" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    collision->material = node->FirstChild()->ValueStr();
		    MARK_SET(node, collision, material);
		}		
		else if (node->ValueStr() == "geometry")
		{
		    loadGeometry(node, name + "_geom", collision->geometry);
		    MARK_SET(node, collision, geometry);		    
		}		
		else if (node->ValueStr() == "data")
		    loadData(node, &collision->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}    
    }
    
    void URDF::loadVisual(const TiXmlNode *node, const std::string &defaultName, Link::Visual *visual)
    {  
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, defaultName);
	if (visual && !name.empty())
	    MARK_SET(node, visual, name);

	if (!visual)
	{
	    if (m_visual.find(name) == m_visual.end())
	    {
		errorMessage("Attempting to add information to an undefined visual: '" + name + "'");
		errorLocation(node);
		return;
	    }
	    else
		visual = m_visual[name];
	}
	
	visual->name = name;
	m_visual[name] = visual;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "rpy")
		{
		    loadDoubleValues(node, 3, visual->rpy);
		    MARK_SET(node, visual, rpy);
		}
		else if (node->ValueStr() == "xyz")
		{
		    loadDoubleValues(node, 3, visual->xyz);
		    MARK_SET(node, visual, xyz);		    
		}		
		else if (node->ValueStr() == "scale")
		{
		    loadDoubleValues(node, 3, visual->scale);
		    MARK_SET(node, visual, scale);
		}		
		else if (node->ValueStr() == "material" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    visual->material = node->FirstChild()->ValueStr();
		    MARK_SET(node, visual, material);
		}		
		else if (node->ValueStr() == "geometry")
		{
		    loadGeometry(node, name + "_geom", visual->geometry);
		    MARK_SET(node, visual, geometry);
		}
		else if (node->ValueStr() == "data")
		    loadData(node, &visual->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}   
    }
    
    void URDF::loadInertial(const TiXmlNode *node, const std::string &defaultName, Link::Inertial *inertial)
    { 
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, defaultName);
	if (inertial && !name.empty())
	    MARK_SET(node, inertial, name);
	
	if (!inertial)
	{
	    if (m_inertial.find(name) == m_inertial.end())
	    {
		errorMessage("Attempting to add information to an undefined inertial component: '" + name + "'");
		errorLocation(node);
		return;
	    }
	    else
		inertial = m_inertial[name];
	}
	
	inertial->name = name;
	m_inertial[name] = inertial;
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "mass")
		{
		    loadDoubleValues(node, 1, &inertial->mass);
		    MARK_SET(node, inertial, mass);		    
		}		
		else if (node->ValueStr() == "com")
		{
		    loadDoubleValues(node, 3, inertial->com);
		    MARK_SET(node, inertial, com);
		}
		else if (node->ValueStr() == "inertia")
		{
		    loadDoubleValues(node, 6, inertial->inertia);
		    MARK_SET(node, inertial, inertia);		    
		}		
		else if (node->ValueStr() == "data")
		    loadData(node, &inertial->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}
    }
    
    void URDF::loadLink(const TiXmlNode *node)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, "");    
	Link *link = (m_links.find(name) != m_links.end()) ? m_links[name] : new Link();
	link->name = name;
	if (link->name.empty())
	{
	    errorMessage("No link name given");
	    errorLocation(node);
	}
	else
	    MARK_SET(node, link, name);
	m_links[link->name] = link;
	
	if (link->canSense())
	    errorMessage("Link '" + link->name + "' was already defined as a sensor");

	std::string currentLocation = m_location;
	m_location = "link '" + name + "'";
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "parent" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    link->parentName = node->FirstChild()->ValueStr();
		    MARK_SET(node, link, parent);
		}		
		else if (node->ValueStr() == "rpy")
		{
		    loadDoubleValues(node, 3, link->rpy);
		    MARK_SET(node, link, rpy);
		}		
		else if (node->ValueStr() == "xyz")
		{
		    loadDoubleValues(node, 3, link->xyz);
		    MARK_SET(node, link, xyz);
		}		
		else if (node->ValueStr() == "joint")
		{
		    loadJoint(node, name + "_joint", link->joint);
		    MARK_SET(node, link, joint);
		}		
		else if (node->ValueStr() == "collision")
		{
		    loadCollision(node, name + "_collision", link->collision);
		    MARK_SET(node, link, collision);
		}
		else if (node->ValueStr() == "inertial")
		{
		    loadInertial(node, name + "_inertial", link->inertial);
		    MARK_SET(node, link, inertial);
		}		
		else if (node->ValueStr() == "visual")
		{
		    loadVisual(node, name + "_visual", link->visual);
		    MARK_SET(node, link, visual);
		}		
		else if (node->ValueStr() == "data")
		    loadData(node, &link->data);
		else
		    ignoreNode(node);
	    }
	    else
		ignoreNode(node);
	}
	
	m_location = currentLocation;  
    }
    
    void URDF::loadSensor(const TiXmlNode *node)
    {
	std::vector<const TiXmlNode*> children;
	std::vector<const TiXmlAttribute*> attributes;
	getChildrenAndAttributes(node, children, attributes);
	
	std::string name = extractName(attributes, "");    
	Sensor *sensor = (m_links.find(name) != m_links.end()) ? dynamic_cast<Sensor*>(m_links[name]) : new Sensor();
	if (!sensor)
	{
	    errorMessage("Link with name '" + name + "' has already been defined");
	    sensor = new Sensor();
	}
	
	sensor->name = name;
	if (sensor->name.empty())
	    errorMessage("No sensor name given");
	else
	    MARK_SET(node, sensor, name);
	if (m_links.find(sensor->name) != m_links.end())
	    errorMessage("Sensor '" + sensor->name + "' redefined");
	m_links[sensor->name] = dynamic_cast<Link*>(sensor);
	
	std::string currentLocation = m_location;
	m_location = "sensor '" + name + "'";
	
	for (unsigned int i = 0 ; i < attributes.size() ; ++i)
	{
	    const TiXmlAttribute *attr = attributes[i];
	    if (strcmp(attr->Name(), "type") == 0)
	    {
		if (attr->ValueStr() == "camera")
		    sensor->type = Sensor::CAMERA;
		else if (attr->ValueStr() == "laser")
		    sensor->type = Sensor::LASER;
		else if (attr->ValueStr() == "stereocamera")
		    sensor->type = Sensor::STEREO_CAMERA;
		else
		{
		    errorMessage("Unknown sensor type: '" + attr->ValueStr() + "'");
		    errorLocation(node);
		}
		MARK_SET(node, sensor, type);
	    }
	}
	
	for (unsigned int i = 0 ; i < children.size() ; ++i)
	{
	    const TiXmlNode *node = children[i];
	    if (node->Type() == TiXmlNode::ELEMENT)
	    {
		if (node->ValueStr() == "parent" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    sensor->parentName = node->FirstChild()->ValueStr();
		    MARK_SET(node, sensor, parent);		    
		}		
		else if (node->ValueStr() == "rpy")
		{
		    loadDoubleValues(node, 3, sensor->rpy);
		    MARK_SET(node, sensor, rpy);		    
		}		
		else if (node->ValueStr() == "xyz")
		{
		    loadDoubleValues(node, 3, sensor->xyz);
		    MARK_SET(node, sensor, xyz);		    
		}		
		else if (node->ValueStr() == "joint")
		{
		    loadJoint(node, name + "_joint", sensor->joint);
		    MARK_SET(node, sensor, joint);
		}
		else if (node->ValueStr() == "collision")
		{
		    loadCollision(node, name + "_collision", sensor->collision);
		    MARK_SET(node, sensor, collision);
		}		
		else if (node->ValueStr() == "inertial")
		{
		    loadInertial(node, name + "_inertial", sensor->inertial);
		    MARK_SET(node, sensor, inertial);
		}		
		else if (node->ValueStr() == "visual")
		{
		    loadVisual(node, name + "_visual", sensor->visual);
		    MARK_SET(node, sensor, visual);
		}		
		else if (node->ValueStr() == "data")
		    loadData(node, &sensor->data);
		else if (node->ValueStr() == "calibration" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		{
		    sensor->calibration =  node->FirstChild()->ValueStr();
		    MARK_SET(node, sensor, calibration);
		}		
		else
		    ignoreNode(node); 
	    }
	    else
		ignoreNode(node);
	}
	
	m_location = currentLocation;  
    }
    
    bool URDF::replaceIncludes(TiXmlElement *elem)
    {
	if (elem->ValueStr() == "include" && elem->FirstChild() && elem->FirstChild()->Type() == TiXmlNode::TEXT)
        {
            char* filename = findFile(elem->FirstChild()->Value());
	    bool change = false;
	    if (filename)
            {
                TiXmlDocument *doc = new TiXmlDocument(filename);
                if (doc->LoadFile())
                {
                    addPath(filename);
                    TiXmlNode *parent = elem->Parent();
                    if (parent)
		    {
			parent->ReplaceChild(dynamic_cast<TiXmlNode*>(elem), *doc->RootElement())->ToElement();
			change = true;
		    }
		}
                else
                    errorMessage("Unable to load " + std::string(filename));
		delete doc;
                free(filename);
	    }
            else
                errorMessage("Unable to find " + elem->FirstChild()->ValueStr());
	    if (change)
		return true;
        }
	
	bool restart = true;
	while (restart)
	{
	    restart = false;
	    for (TiXmlNode *child = elem->FirstChild() ; child ; child = child->NextSibling())
		if (child->Type() == TiXmlNode::ELEMENT)
		    if (replaceIncludes(child->ToElement()))
		    {
			restart = true;
			break;
		    }
	}
	return false;
	
    }

    void URDF::loadData(const TiXmlNode *node, Data *data)
    {
	std::string name;
	std::string type;
	
	for (const TiXmlAttribute *attr = node->ToElement()->FirstAttribute() ; attr ; attr = attr->Next())
	{
	    if (strcmp(attr->Name(), "name") == 0)
		name = attr->ValueStr();
	    else
		if (strcmp(attr->Name(), "type") == 0)
		    type = attr->ValueStr();
	}
	
	for (const TiXmlNode *child = node->FirstChild() ; child ; child = child->NextSibling())
	    if (child->Type() == TiXmlNode::ELEMENT && child->ValueStr() == "elem")
	    {
		std::string key;
		std::string value;
		for (const TiXmlAttribute *attr = child->ToElement()->FirstAttribute() ; attr ; attr = attr->Next())
		{
		    if (strcmp(attr->Name(), "key") == 0)
			key = attr->ValueStr();
		    else
			if (strcmp(attr->Name(), "value") == 0)
			    value = attr->ValueStr();
		}
		data->add(type, name, key, value);
	    }
	    else if (child->Type() == TiXmlNode::ELEMENT && child->ValueStr() == "verbatim")
	    {
		std::string key;
		bool includes = false;
		for (const TiXmlAttribute *attr = child->ToElement()->FirstAttribute() ; attr ; attr = attr->Next())
		{
		    if (strcmp(attr->Name(), "key") == 0)
			key = attr->ValueStr();
		    if (strcmp(attr->Name(), "includes") == 0)
			loadBoolValues(attr->ValueStr(), 1, &includes, child);
		}
		if (includes)
		    replaceIncludes(const_cast<TiXmlElement*>(child->ToElement()));
		data->add(type, name, key, child->ToElement());
	    }
	    else
		ignoreNode(child);
    }
     

    void URDF::linkDatastructure(void)
    {
	
	/* compute the proper pointers for parent nodes and children */
	for (std::map<std::string, Link*>::iterator i = m_links.begin() ; i != m_links.end() ; i++)
	{
	    if (i->second->parentName.empty() || i->second->parentName == "world")
	    {
		m_linkRoots.push_back(i->second);
		continue;
	    }
	    if (i->second->joint->type == Link::Joint::FLOATING || i->second->joint->type == Link::Joint::PLANAR)
		errorMessage("Link '" + i->second->name + "' uses a free joint (floating or planar) but its parent is not the environment!");
	    if (m_links.find(i->second->parentName) == m_links.end())
		errorMessage("Parent of link '" + i->second->name + "' is undefined: '" + i->second->parentName + "'");
	    else
	    {
		Link *parent =  m_links[i->second->parentName];
		i->second->parent = parent;
		parent->children.push_back(i->second);
	    }
	}
	
	/* compute the pointers to links inside frames */
	for (std::map<std::string, Frame*>::iterator i = m_frames.begin() ; i != m_frames.end() ; i++)
	{
	    if (m_links.find(i->second->linkName) != m_links.end())
		i->second->link = m_links[i->second->linkName];
	    else
		errorMessage("Frame '" + i->first + "' refers to unknown link ('" + i->second->linkName + "')");	    
	}
	
	/* for each group, compute the pointers to the links they contain, and for every link,
	 * compute the list of pointers to the groups they are part of 
	 * do the same for frames */
	for (std::map<std::string, Group*>::iterator i = m_groups.begin() ; i != m_groups.end() ; i++)
	{
	    std::sort(i->second->linkNames.begin(), i->second->linkNames.end());
	    
	    for (unsigned int j = 0 ; j < i->second->linkNames.size() ; ++j)
		if (m_links.find(i->second->linkNames[j]) == m_links.end())
		{
		    if (m_frames.find(i->second->linkNames[j]) == m_frames.end())
			errorMessage("Group '" + i->first + "': '" + i->second->linkNames[j] + "' is not defined as a link or frame");
		    else
		    {
			/* name is a frame */
			i->second->frameNames.push_back(i->second->linkNames[j]);
			Frame* f = m_frames[i->second->linkNames[j]];
			f->groups.push_back(i->second);
			i->second->frames.push_back(f);
		    }			    
		}
		else
		{
		    /* name is a link */
		    if (m_frames.find(i->second->linkNames[j]) != m_frames.end())
			errorMessage("Name '" + i->second->linkNames[j] + "' is used both for a link and a frame");
		    
		    Link* l = m_links[i->second->linkNames[j]];
		    l->groups.push_back(i->second);
		    i->second->links.push_back(l);
		}
	    
	    /* remove the link names that are in fact frame names */
	    for (unsigned int j = 0 ; j < i->second->frameNames.size() ; ++j)
		for (unsigned int k = 0 ; k < i->second->linkNames.size() ; ++k)
		    if (i->second->linkNames[k] == i->second->frameNames[j])
		    {
			i->second->linkNames.erase(i->second->linkNames.begin() + k);
			break;
		    }
	}
	
	/* sort the links by name to reduce variance in the output of the parser */
	std::sort(m_linkRoots.begin(), m_linkRoots.end(), SortByName<Link>());
	for (std::map<std::string, Link*>::iterator i = m_links.begin() ; i != m_links.end() ; i++)
	    std::sort(i->second->children.begin(), i->second->children.end(), SortByName<Link>());
	
	/* for every group, find the set of links that are roots in this group (their parent is not in the group) */
	for (std::map<std::string, Group*>::iterator i = m_groups.begin() ; i != m_groups.end() ; i++)
	{
	    for (unsigned int j = 0 ; j < i->second->links.size() ; ++j)
	    {
		Link *parent = i->second->links[j]->parent;
		bool outside = true;
		if (parent)
		    for (unsigned int k = 0 ; k < parent->groups.size() ; ++k)
			if (parent->groups[k] == i->second)
			{
			    outside = false;
			    break;
			}
		if (outside)
		    i->second->linkRoots.push_back(i->second->links[j]);
	    }
	    std::sort(i->second->linkRoots.begin(), i->second->linkRoots.end(), SortByName<Link>());
	}
	
	/* construct inGroup for every link */
	std::vector<std::string> grps;
	getGroupNames(grps);
	std::map<std::string, unsigned int> grpmap;
	for (unsigned int i = 0 ; i < grps.size() ; ++i)
	    grpmap[grps[i]] = i;
	
	for (std::map<std::string, Link*>::iterator i = m_links.begin() ; i != m_links.end() ; i++)
	{
	    i->second->inGroup.resize(grps.size(), false);
	    for (unsigned int j = 0 ; j < i->second->groups.size() ; ++j)
		i->second->inGroup[grpmap[i->second->groups[j]->name]] = true;
	}
    }
    
    

    bool URDF::parse(const TiXmlNode *node)
    {
	if (!node) return false;
	
	int type =  node->Type();
	switch (type)
	{
	case TiXmlNode::DOCUMENT:
	    if (dynamic_cast<const TiXmlDocument*>(node)->RootElement()->ValueStr() != "robot")
		errorMessage("File '" + m_source + "' does not start with the <robot> tag");
	    
	    /* stage 1: extract templates, constants, groups */
	    parse(dynamic_cast<const TiXmlNode*>(dynamic_cast<const TiXmlDocument*>(node)->RootElement()));
	    
	    /* stage 2: parse the rest of the data (that depends on templates & constants) */
	    {		
		for (unsigned int i = 0 ; i < m_stage2.size() ; ++i)
		{
		    const TiXmlElement *elem = m_stage2[i]->ToElement(); 
		    if (!elem)
			errorMessage("Non-element node found in second stage of parsing. This should NOT happen");
		    else
		    {
			std::string name = elem->ValueStr();
			
			if (name == "link")
			    loadLink(m_stage2[i]);
			else if (name == "sensor")
			    loadSensor(m_stage2[i]);
			else if (name == "frame")
			    loadFrame(m_stage2[i]);
			else if (name == "actuator")
			    loadActuator(m_stage2[i]);
			else if (name == "transmission")
			    loadTransmission(m_stage2[i]);
			else if (name == "joint")
			    loadJoint(m_stage2[i], "", NULL);
			else if (name == "geometry")
			    loadGeometry(m_stage2[i], "", NULL);
			else if (name == "collision")
			    loadCollision(m_stage2[i], "", NULL);
			else if (name == "visual")
			    loadVisual(m_stage2[i], "", NULL);
			else if (name == "inertial")
			    loadInertial(m_stage2[i], "", NULL);
			else
			    ignoreNode(m_stage2[i]);
		    }
		}
	    }
	    
	    /* stage 3: 'link' datastructures -- provide easy access pointers */
	    linkDatastructure();
	    
	    /* clear temporary data */
	    clearTemporaryData();
	    
	    break;
	case TiXmlNode::ELEMENT:
	    if (node->ValueStr() == "robot")
	    {
		const char *name = node->ToElement()->Attribute("name");
		if (name)
		{
		    std::string nameStr = name;
		    if (!m_name.empty() && nameStr != m_name)
			errorMessage("Loading a file with contradicting robot name: '" + m_name + "' - '" + name + "'");
		    m_name = nameStr;
		}
		for (const TiXmlNode *child = node->FirstChild() ; child ; child = child->NextSibling())
		    parse(child);
	    }
	    else
		if (node->ValueStr() == "include")
		{
		    if (node->Type() == TiXmlNode::ELEMENT && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		    {
			char* filename = findFile(node->FirstChild()->Value());
			if (filename)
			{
			    TiXmlDocument *doc = new TiXmlDocument(filename);
			    doc->SetUserData(reinterpret_cast<void*>(filename));
			    m_docs.push_back(doc);
			    if (doc->LoadFile())
			    {
				addPath(filename);
				if (doc->RootElement()->ValueStr() != "robot")
				    errorMessage("Included file '" + std::string(filename) + "' does not start with the <robot> tag");
				
				parse(dynamic_cast<const TiXmlNode*>(doc->RootElement()));
			    }
			    else
				errorMessage("Unable to load " + std::string(filename));
			}
			else
			    errorMessage("Unable to find " + node->FirstChild()->ValueStr());
		    }
		    else 
			ignoreNode(node);
		}
		else if (node->ValueStr() == "const")
		{       
		    std::string name;
		    std::string value;
		    
		    for (const TiXmlAttribute *attr = node->ToElement()->FirstAttribute() ; attr ; attr = attr->Next())
		    {
			if (strcmp(attr->Name(), "name") == 0)
			    name = attr->ValueStr();
			else
			    if (strcmp(attr->Name(), "value") == 0)
				value = attr->ValueStr();
		    }
		    
		    if (!node->NoChildren())
		    {
			errorMessage("Constant '" + name + "' appears to contain tags. This should not be the case.");
			errorLocation(node);
		    }
		    
		    m_constants[name] = value;
		}
		else
		    if (node->ValueStr() == "templates")
		    {
			for (const TiXmlNode *child = node->FirstChild() ; child ; child = child->NextSibling())
			    if (child->Type() == TiXmlNode::ELEMENT && child->ValueStr() == "define")
			    {
				const char *name = child->ToElement()->Attribute("template");
				if (name)
				    m_templates[name] = child;
				else
				    ignoreNode(child);
			    }
			    else
				ignoreNode(child);
		    }
		    else if (node->ValueStr() == "group" && node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TEXT)
		    {
			std::string group;
			std::string flags;
			
			for (const TiXmlAttribute *attr = node->ToElement()->FirstAttribute() ; attr ; attr = attr->Next())
			{
			    if (strcmp(attr->Name(), "name") == 0)
				group = attr->ValueStr();
			    else
				if (strcmp(attr->Name(), "flags") == 0)
				    flags = attr->ValueStr();
			}
			Group *g = NULL;
			if (m_groups.find(group) == m_groups.end())
			{
			    g = new Group();
			    g->name = group;
			    m_groups[group] = g;
			}
			else
			    g = m_groups[group];
			
			std::stringstream ssflags(flags);
			while (ssflags.good())
			{
			    std::string flag;
			    ssflags >> flag;
			    g->flags.push_back(flag);	      
			}
			
			std::stringstream ss(node->FirstChild()->ValueStr());
			while (ss.good())
			{
			    std::string value; ss >> value;
			    g->linkNames.push_back(value);
			}
			
			if (g->linkNames.empty())
			{
			    errorMessage("Group '" + g->name + "' is empty. Not adding to list of groups.");
			    m_groups.erase(m_groups.find(g->name));
			    delete g;
			}			
		    }
		    else if (node->ValueStr() == "data")
			loadData(node, &m_data);
		    else
			m_stage2.push_back(node);
	    break;
	default:
	    ignoreNode(node);
	}
	
	return true;
    }
    
    void URDF::sanityCheck(void) const
    {
	std::vector<Link*> links;
	getLinks(links);
	for (unsigned int i = 0 ; i < links.size() ; ++i)
	{
	    
	    Link::Joint *joint = links[i]->joint;
	    if (joint->type == Link::Joint::UNKNOWN)
		errorMessage("Joint '" + joint->name + "' in link '" + links[i]->name + "' is of unknown type");
	    if (joint->type == Link::Joint::REVOLUTE || joint->type == Link::Joint::PRISMATIC)
	    {
		if (joint->axis[0] == 0.0 && joint->axis[1] == 0.0 && joint->axis[2] == 0.0)
		    errorMessage("Joint '" + joint->name + "' in link '" + links[i]->name + "' does not seem to have its axis properly set");
		if ((joint->isSet["limit"] || joint->type == Link::Joint::PRISMATIC) && joint->limit[0] == 0.0 && joint->limit[1] == 0.0)
		    errorMessage("Joint '" + joint->name + "' in link '" + links[i]->name + "' does not seem to have its limits properly set");
	    }
	    
	    Link::Geometry *cgeom = links[i]->collision->geometry;
	    if (cgeom->type == Link::Geometry::UNKNOWN)
		errorMessage("Collision geometry '" + cgeom->name + "' in link '" + links[i]->name + "' is of unknown type");
	    if (cgeom->type != Link::Geometry::UNKNOWN && cgeom->type != Link::Geometry::MESH)
	    {
		int nzero = 0;
		for (int k = 0 ; k < cgeom->nsize ; ++k)
		    nzero += cgeom->size[k] == 0.0 ? 1 : 0;
		if (nzero > 0)
		    errorMessage("Collision geometry '" + cgeom->name + "' in link '" + links[i]->name + "' does not seem to have its size properly set");
	    }
	    
	    Link::Geometry *vgeom = links[i]->visual->geometry;
	    if (vgeom->type == Link::Geometry::UNKNOWN)
		errorMessage("Visual geometry '" + vgeom->name + "' in link '" + links[i]->name + "' is of unknown type");
	    if (vgeom->type != Link::Geometry::UNKNOWN && vgeom->type != Link::Geometry::MESH)
	    {
		int nzero = 0;
		for (int k = 0 ; k < vgeom->nsize ; ++k)
		    nzero += vgeom->size[k] == 0.0 ? 1 : 0;
		if (nzero > 0)
		    errorMessage("Visual geometry '" + vgeom->name + "' in link '" + links[i]->name + "' does not seem to have its size properly set");
	    }	
	}
    }
    
    
} // namespace robot_desc

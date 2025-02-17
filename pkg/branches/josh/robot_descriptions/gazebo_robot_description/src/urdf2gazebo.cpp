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

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <vector>
#include <string>
#include <sstream>

#include <urdf/URDF.h>
#include <libTF/Pose3D.h>

std::string values2str(unsigned int count, const double *values, double (*conv)(double) = NULL)
{
    std::stringstream ss;
    for (unsigned int i = 0 ; i < count ; i++)
    {
	if (i > 0)
	    ss << " ";
	ss << (conv ? conv(values[i]) : values[i]);
    }
    return ss.str();
}

double rad2deg(double v)
{
    return v * 180.0 / M_PI;
}

void setupTransform(libTF::Pose3D &transform, const double *xyz, const double *rpy)
{
    transform.setFromEuler(xyz[0], xyz[1], xyz[2], rpy[2], rpy[1], rpy[0]);
}

void addKeyValue(TiXmlElement *elem, const std::string& key, const std::string &value)
{
    TiXmlElement *ekey      = new TiXmlElement(key);
    TiXmlText    *text_ekey = new TiXmlText(value);
    ekey->LinkEndChild(text_ekey);    
    elem->LinkEndChild(ekey); 
}

void addTransform(TiXmlElement *elem, const::libTF::Pose3D& transform)
{
    libTF::Position pz;
    transform.getPosition(pz);
    double cpos[3] = { pz.x, pz.y, pz.z };
    
    libTF::Euler eu;
    transform.getEuler(eu);
    double crot[3] = { eu.roll, eu.pitch, eu.yaw };        
    
    /* set geometry transform */
    addKeyValue(elem, "xyz", values2str(3, cpos));
    addKeyValue(elem, "rpy", values2str(3, crot, rad2deg));  
}

void copyGazeboMap(const robot_desc::URDF::Map& data, TiXmlElement *elem, const std::vector<std::string> *tags = NULL)
{
    std::vector<std::string> gazebo_names;
    data.getMapTagNames("gazebo", gazebo_names);
    for (unsigned int k = 0 ; k < gazebo_names.size() ; ++k)
    {
	std::map<std::string, std::string> m = data.getMapTagValues("gazebo", gazebo_names[k]);
	std::vector<std::string> accepted_tags;
	if (tags)
	    accepted_tags = *tags;
	else
	    for (std::map<std::string, std::string>::iterator it = m.begin() ; it != m.end() ; it++)
		accepted_tags.push_back(it->first);
	
	for (unsigned int i = 0 ; i < accepted_tags.size() ; ++i)
	    if (m.find(accepted_tags[i]) != m.end())
		addKeyValue(elem, accepted_tags[i], m[accepted_tags[i]]);
	
	std::map<std::string, const TiXmlElement*> x = data.getMapTagXML("gazebo", gazebo_names[k]);
	for (std::map<std::string, const TiXmlElement*>::iterator it = x.begin() ; it != x.end() ; it++)
	{
	    for (const TiXmlNode *child = it->second->FirstChild() ; child ; child = child->NextSibling())
		elem->LinkEndChild(child->Clone());
	}	      
    }
}

std::string getGeometrySize(robot_desc::URDF::Link::Geometry* geometry, int *sizeCount, double *sizeVals)
{
    std::string type;
    
    switch (geometry->type)
    {
    case robot_desc::URDF::Link::Geometry::BOX:
	type = "box";
	*sizeCount = 3;
	{
	    robot_desc::URDF::Link::Geometry::Box* box = static_cast<robot_desc::URDF::Link::Geometry::Box*>(geometry->shape);
	    sizeVals[0] = box->size[0];
	    sizeVals[1] = box->size[1];
	    sizeVals[2] = box->size[2];
	}
	break;
    case robot_desc::URDF::Link::Geometry::CYLINDER:
	type = "cylinder";
	*sizeCount = 2;
	{
	    robot_desc::URDF::Link::Geometry::Cylinder* cylinder = static_cast<robot_desc::URDF::Link::Geometry::Cylinder*>(geometry->shape);
	    sizeVals[0] = cylinder->radius;
	    sizeVals[1] = cylinder->length;
	}
	break;
    case robot_desc::URDF::Link::Geometry::SPHERE:
	type = "sphere";
	*sizeCount = 1;
	sizeVals[0] = static_cast<robot_desc::URDF::Link::Geometry::Sphere*>(geometry->shape)->radius;
	break;
    default:
	*sizeCount = 0;
	printf("Unknown body type: %d in geometry '%s'\n", geometry->type, geometry->name.c_str());
	break;
    }
    
    return type;
}

void convertLink(TiXmlElement *root, robot_desc::URDF::Link *link, const libTF::Pose3D &transform, bool enforce_limits)
{
    libTF::Pose3D currentTransform = transform;
    
    int linkGeomSize;
    double linkSize[3];
    std::string type = getGeometrySize(link->collision->geometry, &linkGeomSize, linkSize);
    
    if (!type.empty())
    {
	/* create new body */
	TiXmlElement *elem     = new TiXmlElement("body:" + type);
	
	/* set body name */
	elem->SetAttribute("name", link->name);
	
	/* compute global transform */
	libTF::Pose3D localTransform;
	setupTransform(localTransform, link->xyz, link->rpy);
	currentTransform.multiplyPose(localTransform);
	addTransform(elem, currentTransform);
	
	if (link->collision->verbose)
	    addKeyValue(elem, "reportStaticCollision", "true");
	
	/* begin create geometry node */
	TiXmlElement *geom     = new TiXmlElement("geom:" + type);
	
	{    
	    /* set its name */
	    geom->SetAttribute("name", link->collision->geometry->name);
	    
	    /* set transform */
	    addKeyValue(geom, "xyz", values2str(3, link->collision->xyz));
	    addKeyValue(geom, "rpy", values2str(3, link->collision->rpy, rad2deg));
	    
	    /* set mass properties */
	    addKeyValue(geom, "massMatrix", "true");
	    addKeyValue(geom, "mass", values2str(1, &link->inertial->mass));
	    
	    static const char tagList1[6][4] = {"ixx", "ixy", "ixz", "iyy", "iyz", "izz"};
	    for (int j = 0 ; j < 6 ; ++j)
		addKeyValue(geom, tagList1[j], values2str(1, link->inertial->inertia + j));
	    
	    static const char tagList2[3][3] = {"cx", "cy", "cz"};
	    for (int j = 0 ; j < 3 ; ++j)
		addKeyValue(geom, tagList2[j], values2str(1, link->inertial->com + j));
	    
	    /* set geometry size */
	    addKeyValue(geom, "size", values2str(linkGeomSize, linkSize));
	    
	    /* set additional data */      
	    copyGazeboMap(link->collision->data, geom);
	    
	    /* begin create visual node */
	    TiXmlElement *visual = new TiXmlElement("visual");
	    {
		/* compute the visualisation transfrom */
		libTF::Pose3D coll;
		setupTransform(coll, link->collision->xyz, link->collision->rpy);
		coll.invert();
		
		libTF::Pose3D vis;
		setupTransform(vis, link->visual->xyz, link->visual->rpy);
		coll.multiplyPose(vis);
		
		addTransform(visual, coll);
		
		/* set geometry size */		
		
		if (link->visual->geometry->type == robot_desc::URDF::Link::Geometry::MESH)
		{  
		    robot_desc::URDF::Link::Geometry::Mesh* mesh = static_cast<robot_desc::URDF::Link::Geometry::Mesh*>(link->visual->geometry->shape);
		    /* set mesh size or scale */
		    /*
		    if (link->visual->geometry->isSet["size"])
			addKeyValue(visual, "size", values2str(3, mesh->size));	
		    else
			addKeyValue(visual, "scale", values2str(3, mesh->scale));	
		    */
		    addKeyValue(visual, "scale", values2str(3, mesh->scale));

		    /* set mesh file */
		    if (mesh->filename.empty())
			addKeyValue(visual, "mesh", "unit_" + type);
		    else
			addKeyValue(visual, "mesh", "models/pr2_new/" + mesh->filename + "_hi.mesh");
		    
		}
		else
		{
		    int visualGeomSize;
		    double visualSize[3];
		    getGeometrySize(link->visual->geometry, &visualGeomSize, visualSize);
		    addKeyValue(visual, "size", values2str(visualGeomSize, visualSize));
		}
		
		copyGazeboMap(link->visual->data, visual);
	    }
	    /* end create visual node */
	    
	    geom->LinkEndChild(visual);
	}
	/* end create geometry node */
	
	/* add geometry to body */
	elem->LinkEndChild(geom);      
	
	/* copy gazebo data */
	copyGazeboMap(link->data, elem);
	
	/* add body to document */
	root->LinkEndChild(elem);
	
	/* compute the joint tag */
	bool fixed = false;
	std::string jtype;
	switch (link->joint->type)
	{
	case robot_desc::URDF::Link::Joint::REVOLUTE:
	    jtype = "hinge";
	    break;
	case robot_desc::URDF::Link::Joint::PRISMATIC:
	    jtype = "slider";
	    break;
	case robot_desc::URDF::Link::Joint::FLOATING:
	case robot_desc::URDF::Link::Joint::PLANAR:
	    break;
	case robot_desc::URDF::Link::Joint::FIXED:
	    jtype = "hinge";
	    fixed = true;
	    break;
	default:
	    printf("Unknown joint type: %d in link '%s'\n", link->joint->type, link->name.c_str());
	    break;
	}
	
	if (!jtype.empty())
	{
	    TiXmlElement *joint = new TiXmlElement("joint:" + jtype);
	    joint->SetAttribute("name", link->joint->name);
	    
	    addKeyValue(joint, "body1", link->name);
	    addKeyValue(joint, "body2", link->parentName);
	    addKeyValue(joint, "anchor", link->name);
	    
	    if (fixed)
	    {
		addKeyValue(joint, "lowStop", "0");
		addKeyValue(joint, "highStop", "0");
		addKeyValue(joint, "axis", "1 0 0");
	    }
	    else
	    {        
		addKeyValue(joint, "axis", values2str(3, link->joint->axis));
		
		double tmpAnchor[3];
		
		for (int j = 0 ; j < 3 ; ++j)
		{
		    // undo Gazebo's shift of object anchor to geom xyz, stay in body cs
		    tmpAnchor[j] = (link->joint->anchor)[j] - 1.0*(link->inertial->com)[j] - 0.0*(link->collision->xyz)[j]; /// @todo compensate for gazebo's error.  John is fixing this one
		}
		
		addKeyValue(joint, "anchorOffset", values2str(3, tmpAnchor));
		
		if (enforce_limits && link->joint->isSet["limit"])
		{
		    if (jtype == "slider")
		    {
			addKeyValue(joint, "lowStop",  values2str(1, link->joint->limit             ));
			addKeyValue(joint, "highStop", values2str(1, link->joint->limit + 1         ));
		    }
		    else
		    {
			addKeyValue(joint, "lowStop",  values2str(1, link->joint->limit    , rad2deg));
			addKeyValue(joint, "highStop", values2str(1, link->joint->limit + 1, rad2deg));
		    }
		}
	    }
	    
	    /* add joint to document */
	    root->LinkEndChild(joint);
	}
    }
    
    for (unsigned int i = 0 ; i < link->children.size() ; ++i)
	convertLink(root, link->children[i], currentTransform, enforce_limits);
}

void convert(robot_desc::URDF &wgxml, TiXmlDocument &doc, bool enforce_limits)
{
    TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
    doc.LinkEndChild(decl);
    
    /* create root element and define needed namespaces */
    TiXmlElement *robot = new TiXmlElement("model:physical");
    robot->SetAttribute("xmlns:gazebo", "http://playerstage.sourceforge.net/gazebo/xmlschema/#gz");
    robot->SetAttribute("xmlns:model", "http://playerstage.sourceforge.net/gazebo/xmlschema/#model");
    robot->SetAttribute("xmlns:sensor", "http://playerstage.sourceforge.net/gazebo/xmlschema/#sensor");
    robot->SetAttribute("xmlns:body", "http://playerstage.sourceforge.net/gazebo/xmlschema/#body");
    robot->SetAttribute("xmlns:geom", "http://playerstage.sourceforge.net/gazebo/xmlschema/#geom");
    robot->SetAttribute("xmlns:joint", "http://playerstage.sourceforge.net/gazebo/xmlschema/#joint");
    robot->SetAttribute("xmlns:controller", "http://playerstage.sourceforge.net/gazebo/xmlschema/#controller");
    robot->SetAttribute("xmlns:interface", "http://playerstage.sourceforge.net/gazebo/xmlschema/#interface");
    robot->SetAttribute("xmlns:rendering", "http://playerstage.sourceforge.net/gazebo/xmlschema/#rendering");
    robot->SetAttribute("xmlns:physics", "http://playerstage.sourceforge.net/gazebo/xmlschema/#physics");
    
    // Create a node to enclose the robot body
    robot->SetAttribute("name", "pr2_model");
    
    /* set the transform for the whole model to identity */
    addKeyValue(robot, "xyz", "0 0 0");
    addKeyValue(robot, "rpy", "0 0 0");
    libTF::Pose3D transform;    
    
    for (unsigned int k = 0 ; k < wgxml.getDisjointPartCount() ; ++k)
	convertLink(robot, wgxml.getDisjointPart(k), transform, enforce_limits);
    
    /* find all data item types */
    copyGazeboMap(wgxml.getMap(), robot);
    
    doc.LinkEndChild(robot);
}

void usage(const char *progname)
{
    printf("\nUsage: %s URDF.xml Gazebo.model\n", progname);
    printf("       where URDF.xml is the file containing a robot description in the Willow Garage format (URDF)\n");
    printf("       and Gazebo.model is the file where the Gazebo model should be written\n\n");
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
	usage(argv[0]);
	exit(1);
    }
    
    // Experimental: take in optional argument to disable passing the limit for grasping test
    bool enforce_limits = true;
    if (argc == 4)
    {
	printf("Not enforcing limits\n");
	enforce_limits = false;
    }
    
    robot_desc::URDF wgxml;
    
    if (!wgxml.loadFile(argv[1]))
    {
	printf("Unable to load robot model from %s\n", argv[1]);  
	exit(2);
    }
    
    TiXmlDocument doc;
    
    convert(wgxml, doc, enforce_limits);
    
    if (!doc.SaveFile(argv[2]))
    {
	printf("Unable to save gazebo model in %s\n", argv[2]);  
	exit(3);
    }
    
    return 0;
}

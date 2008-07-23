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

#include <tinyxml-2.5.3/tinyxml.h>
#include <vector>
#include <string>
#include <fstream>

class Merge
{
public:
    Merge(void)
    {
	m_doc = new TiXmlDocument();
	m_paths.push_back("");
    }
    
    ~Merge(void)
    {  
	delete m_doc;
    }

    bool Execute(const char *filename)
    {
	if (!m_doc->LoadFile(filename))
	{
	    fprintf(stderr, "%s\n", m_doc->ErrorDesc());
	    return false;
	}
	addPath(filename);
	fixIncludes(m_doc->RootElement());
	return true;	
    }
    
    TiXmlDocument* getDocument(void) const
    {
	return m_doc;
    }
    
private:
    
    TiXmlDocument*           m_doc;
    std::vector<std::string> m_paths;
    
    void addPath(const char *filename)
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

    char* findFile(const char *filename)
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

    void fixIncludes(TiXmlElement *elem)
    {
	if (elem->ValueStr() == "include" && elem->FirstChild() && elem->FirstChild()->Type() == TiXmlNode::TEXT)
	{
	    char* filename = findFile(elem->FirstChild()->Value());
	    if (filename)
	    {
		TiXmlDocument *doc = new TiXmlDocument(filename);
		if (doc->LoadFile())
		{
		    addPath(filename);
		    TiXmlNode *parent = elem->Parent();
		    if (parent)
			parent->ReplaceChild(dynamic_cast<TiXmlNode*>(elem), *doc->RootElement()->Clone());
		}
		else
		    fprintf(stderr, "Unable to load %s\n", filename);
		delete doc;
		free(filename);
	    }
	    else
		fprintf(stderr, "Unable to find %s\n", elem->FirstChild()->Value());	
	}
	
	for (TiXmlNode *child = elem->FirstChild() ; child ; child = child->NextSibling())
	    if (child->Type() == TiXmlNode::ELEMENT)
		fixIncludes(child->ToElement());
    }
};

void usage(const char *pname)
{
    printf("\nUsage: %s <URDF file>\n\n", pname);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
	usage(argv[0]);
	exit(1);	
    }
    
    Merge m;
    m.Execute(argv[1]);
    m.getDocument()->Print();
        
    return 0;    
}

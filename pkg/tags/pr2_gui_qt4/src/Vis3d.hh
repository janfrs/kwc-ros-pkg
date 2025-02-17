///////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) 2008, Willow Garage Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//   
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////
/**
@mainpage

@htmlinclude manifest.html

@b Vis3d is a 3D visualization of the robot's current state and sensor feedback using the Irrlicht rendering engine.

<hr>

@section topic ROS topics

Subscribes to (name/type):
- @b "cloud"/std_msgs::PointCloudFloat32 : Point cloud received from head Hokuyo
- @b "cloudFloor"/std_msgs::PointCloudFloat32 : Point cloud received from base Hokuyo (type may be changed soon)
- @b "cloudStereo"/std_msgs::PointCloudFloat32 : Point cloud received from stereo vision (type may be changed soon)
- @b "shutter"/std_msgs::Empty : Cue to erase "cloud" information
- @b "shutterFloor"/std_msgs::Empty : Cue to erase "cloudFloor" information
- @b "shutterStereo"/std_msgs::Empty : Cue to erase "cloudStereo" information

@todo Start using libTF for transform management:
  - Use libTF to place models
  - Read in joint angles from ros
  - Ability to place random objects (sphere, cube, cylinder, etc) to mark suspected object locations
**/

#ifndef __PP_IL_RENDER_HH
#define __PP_IL_RENDER_HH
#define cloudArrayLength 400
#include <rosthread/member_thread.h>
//#include <irrlicht.h>
#include "ILClient.hh"
#include "ILRender.hh"
#include "CustomNodes/ILPointCloud.hh"
#include "CustomNodes/ILGrid.hh"
#include "ILModel.cpp"
#include "ILUCS.cpp"

#include <ros/node.h>
#include <std_msgs/PointCloudFloat32.h>
#include <std_msgs/Empty.h>


#include <pr2Core/pr2Core.h> /* Contains enumeration definitions for PR2 bodies and joints, must also include ros package pr2Core in manifest.xml */

//using namespace PR2;

class Vis3d
{
public:
	enum modelParts{base,body,wheelFL,wheelRL,wheelFR,wheelRR,modelPartsCount};
	enum viewEnum{Maya,FPS,TFL,TFR,TRL,TRR,Top,Bottom,Front,Rear,Left,Right,viewCount};
	ros::node *myNode;
	std_msgs::Empty shutHead;
	std_msgs::Empty shutFloor;
	std_msgs::Empty shutStereo;
	std_msgs::PointCloudFloat32 ptCldHead;
	std_msgs::PointCloudFloat32 ptCldFloor;
	std_msgs::PointCloudFloat32 ptCldStereo;

	int headVertScanCount;
	ILClient *localClient;
	ILRender *pLocalRenderer;
	ILPointCloud *ilHeadCloud[cloudArrayLength];
	ILPointCloud *ilFloorCloud;
	ILPointCloud *ilStereoCloud;
	ILGrid *ilGrid;
	ILModel *model[modelPartsCount];
	ILUCS *ilucs;
	
	irr::scene::ILightSceneNode *light[3];
	irr::scene::ICameraSceneNode *cameras[viewCount];
	

	Vis3d(ros::node *aNode) //: localClient(NULL)
	{
		headVertScanCount = 0;
		for(int i = 0; i < modelPartsCount; i++)
		{
			model[i] = 0;
		}
		myNode = aNode;
		localClient = new ILClient();
		pLocalRenderer = ILClient::getSingleton();
		pLocalRenderer->lock();

		ilFloorCloud = new ILPointCloud(pLocalRenderer->manager()->getRootSceneNode(),pLocalRenderer->manager(),667);
		ilStereoCloud = new ILPointCloud(pLocalRenderer->manager()->getRootSceneNode(),pLocalRenderer->manager(),666);
		ilGrid = new ILGrid(pLocalRenderer->manager()->getRootSceneNode(), pLocalRenderer->manager(), 668);
		ilucs = new ILUCS(pLocalRenderer->manager()->getRootSceneNode(), pLocalRenderer->manager(),true);
		for(int i = 0; i < cloudArrayLength; i++)
		{
			ilHeadCloud[i] = new ILPointCloud(pLocalRenderer->manager()->getRootSceneNode(),pLocalRenderer->manager(),669 + i);
		}
		pLocalRenderer->unlock();
		pLocalRenderer->addNode(ilFloorCloud);
		pLocalRenderer->addNode(ilStereoCloud);
		for(int i = 0; i < cloudArrayLength; i++)
		{
			pLocalRenderer->addNode(ilHeadCloud[i]);
		}
		ilGrid->makegrid(100,1.0f,50,50,50);
		pLocalRenderer->addNode(ilGrid);
		irr::SKeyMap keyMap[8];
		{
                 keyMap[0].Action = irr::EKA_MOVE_FORWARD;
                 keyMap[0].KeyCode = irr::KEY_UP;
                 keyMap[1].Action = irr::EKA_MOVE_FORWARD;
                 keyMap[1].KeyCode = irr::KEY_KEY_W;

                 keyMap[2].Action = irr::EKA_MOVE_BACKWARD;
                 keyMap[2].KeyCode = irr::KEY_DOWN;
                 keyMap[3].Action = irr::EKA_MOVE_BACKWARD;
                 keyMap[3].KeyCode = irr::KEY_KEY_S;

                 keyMap[4].Action = irr::EKA_STRAFE_LEFT;
                 keyMap[4].KeyCode = irr::KEY_LEFT;
                 keyMap[5].Action = irr::EKA_STRAFE_LEFT;
                 keyMap[5].KeyCode = irr::KEY_KEY_A;

                 keyMap[6].Action = irr::EKA_STRAFE_RIGHT;
                 keyMap[6].KeyCode = irr::KEY_RIGHT;
                 keyMap[7].Action = irr::EKA_STRAFE_RIGHT;
                 keyMap[7].KeyCode = irr::KEY_KEY_D;
		}
		cameras[Maya] = pLocalRenderer->manager()->addCameraSceneNodeMaya(NULL,-150.0f,50.0f,25.0f,Maya);
		cameras[FPS] = pLocalRenderer->manager()->addCameraSceneNodeFPS(NULL,50,5,FPS,keyMap,8);
		cameras[TFL] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(-3,3,3),irr::core::vector3df(-1,1,1),TFL);
		cameras[TFR] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(3,3,3),irr::core::vector3df(1,1,1),TFR);
		cameras[TRL] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(-3,3,-3),irr::core::vector3df(-1,1,-1),TRL);
		cameras[TRR] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(3,3,-3),irr::core::vector3df(1,1,-1),TRR);
		cameras[Front] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(0,1,3),irr::core::vector3df(0,1,0),Front);
		cameras[Rear] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(0,1,-3),irr::core::vector3df(0,1,0),Rear);
		cameras[Top] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(0,3,0),irr::core::vector3df(0,0,0),Top);
		cameras[Bottom] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(0,-3,0),irr::core::vector3df(0,0,0),Bottom);
		cameras[Left] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(-3,1,0),irr::core::vector3df(0,1,0),Left);
		cameras[Right] = pLocalRenderer->manager()->addCameraSceneNode(NULL,irr::core::vector3df(3,1,0),irr::core::vector3df(0,1,0),Right);
		pLocalRenderer->manager()->setActiveCamera(cameras[Maya]);
		
		//light[0] = pLocalRenderer->manager()->addLightSceneNode(NULL,irr::core::vector3df(50,50,50),irr::video::SColorf(1.0f,1.0f,1.0f,1.0f));
		//light[1] = pLocalRenderer->manager()->addLightSceneNode(NULL,irr::core::vector3df(-50,-50,-50),irr::video::SColorf(.5f,.5f,.5f,1.0f));
		
		std::cerr<<"Done Constructing Vis3D"<<std::endl;
	}

	~Vis3d()
	{
	    std::cout << "destroying Vis3D\n";
		if(model[0])
			disableModel();
		disableHead();
		disableStereo();
		disableFloor();
		delete localClient;
	    for(int i = 0; i < cloudArrayLength; i++)
	    {
		    //if(ilHeadCloud[i])
			delete ilHeadCloud[i];
	    }
	    //if(ilFloorCloud)
		delete ilFloorCloud;
	    //if(ilStereoCloud)
		delete ilStereoCloud;
	    //if(ilGrid)
		delete ilGrid;
		
		
	    std::cout << "destroyed Vis3D\n";
	}

	bool isEnabled()
	{
	    return pLocalRenderer->isEnabled();
	}
	
	void changeView(int id)
	{
		irr::core::vector3df pos = pLocalRenderer->manager()->getActiveCamera()->getPosition();
		irr::core::vector3df tar = pLocalRenderer->manager()->getActiveCamera()->getTarget();
		switch(id)
		{
			case Maya:
			case FPS:
				cameras[id]->setTarget(tar);
				cameras[id]->setPosition(pos);
				break;
				
		}
		pLocalRenderer->manager()->setActiveCamera(cameras[id]);
	}

	void enableHead()
	{
	    myNode->subscribe("cloud", ptCldHead, &Vis3d::addHeadCloud,this);
	    myNode->subscribe("shutter", shutHead, &Vis3d::shutterHead,this);
	    for(int i = 0; i < cloudArrayLength; i++)
	    {
			pLocalRenderer->enable(ilHeadCloud[i]);
			ilHeadCloud[i]->setVisible(true);
	    }   
	}

	void enableModel()
	{
	
		static char *modelPaths[] = {"../pr2_models/base1000.3DS","../pr2_models/body1000.3DS","../pr2_models/caster1000r2.3DS","../pr2_models/caster1000r2.3DS","../pr2_models/caster1000r2.3DS","../pr2_models/caster1000r2.3DS"};
		pLocalRenderer->lock();
		//int i = 2;
		for(int i = 0; i < modelPartsCount; i++)
		{
			//delete model[i];
			model[i] = new ILModel(pLocalRenderer->manager(), modelPaths[i], true);
			model[i]->getNode()->setMaterialFlag(irr::video::EMF_LIGHTING,false);
			model[i]->getNode()->setMaterialFlag(irr::video::EMF_WIREFRAME,true);
		}
		pLocalRenderer->unlock();
	}

	void enableUCS()
	{
		pLocalRenderer->lock();
		/*if(!ilucs)
  			ilucs = new ILUCS(pLocalRenderer->manager(),true);*/
  		ilucs->setVisible(true);
  		std::cout<<"Enabling UCS\n";
  		pLocalRenderer->unlock();
	}
	
	void enableGrid()
	{
		pLocalRenderer->lock();
		pLocalRenderer->enable(ilGrid);
		ilGrid->setVisible(true);
		std::cout<<"Enabling Grid\n";
		pLocalRenderer->unlock();
	}
	
	void enableFloor()
	{
	    myNode->subscribe("cloudFloor", ptCldFloor, &Vis3d::addFloorCloud,this);
	    myNode->subscribe("shutterFloor", shutFloor, &Vis3d::shutterFloor,this);
	    pLocalRenderer->enable(ilFloorCloud);
	    ilFloorCloud->setVisible(true);
	}

	void enableStereo()
	{
	    myNode->subscribe("cloudStereo", ptCldStereo, &Vis3d::addStereoCloud,this);
	    myNode->subscribe("shutterStereo", shutStereo, &Vis3d::shutterStereo,this);
	    pLocalRenderer->enable(ilStereoCloud);
	    ilStereoCloud->setVisible(true);
	}

	void disableHead()
	{
	    myNode->unsubscribe("cloud");
	    myNode->unsubscribe("shutter");
	    shutterHead();
		pLocalRenderer->lock();
	    for(int i = 0; i < cloudArrayLength; i++)
	    {
		    
			pLocalRenderer->disable(ilHeadCloud[i]);
		    ilHeadCloud[i]->setVisible(false);
	    }  
		pLocalRenderer->unlock();	    
	}

	void disableFloor()
	{
	    myNode->unsubscribe("cloudFloor");
	    myNode->unsubscribe("shutterFloor");
	    shutterFloor();
		pLocalRenderer->lock();
	    pLocalRenderer->disable(ilFloorCloud);
	    ilFloorCloud->setVisible(false);
		pLocalRenderer->unlock();
	}

	void disableStereo()
	{
	    myNode->unsubscribe("cloudStereo");
	    myNode->unsubscribe("shutterStereo");
	    shutterStereo();
		pLocalRenderer->lock();
	    pLocalRenderer->disable(ilStereoCloud);
	    ilStereoCloud->setVisible(false);
		pLocalRenderer->unlock();
	}
	
	void disableUCS()
	{
		pLocalRenderer->lock();
		/*if(ilucs)
		{
			delete ilucs;
			ilucs = 0;
		}*/
		ilucs->setVisible(false);
		std::cout<<"Disabling UCS\n";
		pLocalRenderer->unlock();
	}
	
	void disableGrid()
	{
		pLocalRenderer->lock();
		pLocalRenderer->disable(ilGrid);
		ilGrid->setVisible(false);
		std::cout<<"Disabling Grid\n";
		pLocalRenderer->unlock();
	}
	
	void disableModel()
	{
		pLocalRenderer->lock();
		for(int i = 0; i < modelPartsCount; i++)
		{
			delete model[i];
			model[i] = 0;
		}
		pLocalRenderer->unlock();
	}

	void shutterHead()
	{
	    pLocalRenderer->lock();
	    for(int i = 0; i < cloudArrayLength; i++)
	    {
		ilHeadCloud[i]->resetCount();
	    }
	    pLocalRenderer->unlock();
	    headVertScanCount = 0;
	}

	void shutterFloor()
	{
	    pLocalRenderer->lock();
	    ilFloorCloud->resetCount();
	    pLocalRenderer->unlock();
	}

	void shutterStereo()
	{
	    pLocalRenderer->lock();
	    ilStereoCloud->resetCount();
	    pLocalRenderer->unlock();
	}

	void addHeadCloud()
	{
	    pLocalRenderer->lock();
	    if(headVertScanCount < cloudArrayLength)
	    {
		if(ptCldHead.get_pts_size() > 65535)
		{
		    for(int i = 0; i < 65535; i++)
		    {
			ilHeadCloud[headVertScanCount]->addPoint(ptCldHead.pts[i].x, ptCldHead.pts[i].y, ptCldHead.pts[i].z, 255 ,(int)(ptCldHead.chan[0].vals[i]/16.0),(int)(ptCldHead.chan[0].vals[i]/16.0));
		    }
		}
		else
		{
		    for(size_t i = 0; i < ptCldHead.get_pts_size(); i++)
		    {
			ilHeadCloud[headVertScanCount]->addPoint(ptCldHead.pts[i].x, ptCldHead.pts[i].y, ptCldHead.pts[i].z, 255,(int)(ptCldHead.chan[0].vals[i]/16.0),(int)(ptCldHead.chan[0].vals[i]/16.0));
		    }
		}
		headVertScanCount++;
	    }
	    pLocalRenderer->unlock();
	}

	void addFloorCloud()
	{
	    pLocalRenderer->lock();
	    if(ptCldFloor.get_pts_size() > 65535)
	    {
		for(int i = 0; i < 65535; i++)
		{
		    ilFloorCloud->addPoint(ptCldFloor.pts[i].x, ptCldFloor.pts[i].y, ptCldFloor.pts[i].z, (int)(ptCldFloor.chan[0].vals[i]/16.0),255,(int)(ptCldFloor.chan[0].vals[i]/16.0));
		}
	    }
	    else
	    {
		for(size_t i = 0; i < ptCldFloor.get_pts_size(); i++)
		{
		    ilFloorCloud->addPoint(ptCldFloor.pts[i].x, ptCldFloor.pts[i].y, ptCldFloor.pts[i].z, (int)(ptCldFloor.chan[0].vals[i]/16.0),255,(int)(ptCldFloor.chan[0].vals[i]/16.0));
		}
	    }
	    pLocalRenderer->unlock();
	}

	void addStereoCloud()
	{
	    pLocalRenderer->lock();
	    if(ptCldStereo.get_pts_size() > 65535)
	    {
		for(int i = 0; i < 65535; i++)
		{
		    ilStereoCloud->addPoint(ptCldStereo.pts[i].x, ptCldStereo.pts[i].y, ptCldStereo.pts[i].z, (int)(ptCldStereo.chan[0].vals[i]/16.0),(int)(ptCldStereo.chan[0].vals[i]/16.0),255);
		}
	    }
	    else
	    {
		for(size_t i = 0; i < ptCldStereo.get_pts_size(); i++)
		{
		    ilStereoCloud->addPoint(ptCldStereo.pts[i].x, ptCldStereo.pts[i].y, ptCldStereo.pts[i].z, (int)(ptCldStereo.chan[0].vals[i]/16.0),(int)(ptCldStereo.chan[0].vals[i]/16.0),255);
		}
	    }
	    pLocalRenderer->unlock();
	}

};
    
    
#endif // ifndef __PP_IL_RENDER_HH
    

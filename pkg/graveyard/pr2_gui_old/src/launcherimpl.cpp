#include "launcherimpl.h"

LauncherImpl::LauncherImpl( wxWindow* parent )
:
launcher( parent )
{
	PTZLCodec = new ImageCodec<std_msgs::Image>(&PTZLImage);
	PTZRCodec = new ImageCodec<std_msgs::Image>(&PTZRImage);
	WristLCodec = new ImageCodec<std_msgs::Image>(&WristLImage);
	WristRCodec = new ImageCodec<std_msgs::Image>(&WristRImage);

	wxInitAllImageHandlers();
	LeftDock_FGS->Hide(HeadLaser_RB,true);
	LeftDock_FGS->Hide(Visualization_SBS,true);
	LeftDock_FGS->Hide(PTZL_SBS,true);
	LeftDock_FGS->Hide(WristL_SBS,true);
	RightDock_FGS->Hide(Topdown_SBS,true);
	RightDock_FGS->Hide(PTZR_SBS,true);
	RightDock_FGS->Hide(WristR_SBS,true);
	Layout();
	Fit();


	PTZL_GET_NEW_IMAGE = true;
	PTZR_GET_NEW_IMAGE = true;
	WristR_GET_NEW_IMAGE = true;
	WristL_GET_NEW_IMAGE = true;
	vis3d_Window = NULL;
	myNode = new ros::node("guiNode");
	myNode->subscribe("/roserr",rosErrMsg, &LauncherImpl::errorOut);
	this->Connect(PTZL_B->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LauncherImpl::PTZLDrawPic));
	this->Connect(PTZR_B->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LauncherImpl::PTZRDrawPic));
	this->Connect(WristL_B->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LauncherImpl::WristLDrawPic));
	this->Connect(WristR_B->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LauncherImpl::WristRDrawPic));
}

void LauncherImpl::errorOut()
{
	//std::cerr << "errorOut!!!\n";
	long first = Ros_TC->GetLastPosition();
	wxString line = wxString::FromAscii( rosErrMsg.msg.c_str() );
	Ros_TC->AppendText(line);
}

void LauncherImpl::consoleOut(wxString Line)
{
    Console_TC->AppendText(Line);
}

void LauncherImpl::startStopHeadPtCld( wxCommandEvent& event )
{
	if(HeadLaser_CB->IsChecked())
    {
		consoleOut(wxT("Enabling Head Laser Cloud\n"));
		vis3d_Window->enableHead();
		HeadLaser_RB->Show(true);
		Layout();
		Fit();
    }
    else
    {
		consoleOut(wxT("Disabling Head Laser Cloud\n"));
		vis3d_Window->disableHead();
		HeadLaser_RB->Show(false);
		Layout();
		Fit();
    }
}

void LauncherImpl::startStopFloorPtCld( wxCommandEvent& event )
{
	if(FloorLaser_CB->IsChecked())
    {
		consoleOut(wxT("Enabling Floor Laser Cloud\n"));
		vis3d_Window->enableFloor();
    }
    else
    {
		consoleOut(wxT("Disabling Floor Laser Cloud\n"));
		vis3d_Window->disableFloor();
    }
}

void LauncherImpl::startStopStereoPtCld( wxCommandEvent& event )
{
	if(Stereo_CB->IsChecked())
    {
		consoleOut(wxT("Enabling Stereo Laser Cloud\n"));
		vis3d_Window->enableStereo();
    }
    else
    {
		consoleOut(wxT("Disabling Stereo Laser Cloud\n"));
		vis3d_Window->disableStereo();
    }
}

void LauncherImpl::startStopModel( wxCommandEvent& event )
{
	if(Model_CB->IsChecked())
    {
		consoleOut(wxT("Enabling 3D Model\n"));
		vis3d_Window->enableModel();
    }
    else
    {
		consoleOut(wxT("Disabling 3D Model\n"));
		vis3d_Window->disableModel();
    }
}

void LauncherImpl::startStopUCS( wxCommandEvent& event )
{
	if(UCS_CB->IsChecked())
	{
		consoleOut(wxT("Enabling UCS\n"));
		vis3d_Window->enableUCS();
	}
	else
	{
		consoleOut(wxT("Disabling UCS\n"));
		vis3d_Window->disableUCS();
	}
}

void LauncherImpl::startStopGrid( wxCommandEvent& event )
{
	if(Grid_CB->IsChecked())
	{
		consoleOut(wxT("Enabling Grid\n"));
		vis3d_Window->enableGrid();
	}
	else
	{
		consoleOut(wxT("Disabling Grid\n"));
		vis3d_Window->disableGrid();
	}
}

void LauncherImpl::startStopObjects( wxCommandEvent& event )
{
	if(Objects_CB->IsChecked())
	{
		consoleOut(wxT("Enabling Objects\n"));
		vis3d_Window->enableObjects();
	}
	else
	{
		consoleOut(wxT("Disabling Objects\n"));
		vis3d_Window->disableObjects();
	}
}

void LauncherImpl::deleteObjects(wxMouseEvent& event)
{
	vis3d_Window->deleteObjects();
}

void LauncherImpl::viewChanged( wxCommandEvent& event )
{
	if(vis3d_Window)
	{
		vis3d_Window->changeView(Views_RB->GetSelection());
	}
	else
	{
		consoleOut(wxT("Cannot change view.  3D window does not exist.\n"));
	}
}

void LauncherImpl::HeadLaserChanged( wxCommandEvent& event )
{
	if(vis3d_Window)
	{
		std::cout << "Selection: " << HeadLaser_RB->GetSelection() << std::endl;
		vis3d_Window->changeHeadLaser(HeadLaser_RB->GetSelection());
	}
	else
		consoleOut(wxT("Cannot change shutter type.  3D window does not exist.\n"));
}

void LauncherImpl::startStop_Visualization( wxCommandEvent& event )
{
	if(Visualization_CB->IsChecked())
    {
		consoleOut(wxT("Opening Visualizer\n"));
		HeadLaser_CB->SetValue(false);
		FloorLaser_CB->SetValue(false);
		Stereo_CB->SetValue(false);
		Model_CB->SetValue(false);
		UCS_CB->SetValue(false);
		Grid_CB->SetValue(true);
		Objects_CB->SetValue(true);
		LeftDock_FGS->Show(Visualization_SBS,true);
		HeadLaser_RB->Show(false);
		Layout();
		Fit();
		HeadLaser_CB->Enable(true);
		FloorLaser_CB->Enable(true);
		Stereo_CB->Enable(true);
		Model_CB->Enable(true);
		UCS_CB->Enable(true);
		Grid_CB->Enable(true);
		Objects_CB->Enable(true);
		Views_RB->Enable(true);
		if(!vis3d_Window)
			vis3d_Window = new Vis3d(myNode);
    }
    else
    {
		consoleOut(wxT("Closing Visualizer\n"));
		delete vis3d_Window;
		vis3d_Window = 0;
		LeftDock_FGS->Hide(Visualization_SBS,true);
		Layout();
		Fit();
		HeadLaser_CB->Enable(false);
		FloorLaser_CB->Enable(false);
		Stereo_CB->Enable(false);
		Model_CB->Enable(false);
		UCS_CB->Enable(false);
		Grid_CB->Enable(false);
		Objects_CB->Enable(false);
		Views_RB->Enable(false);
		vis3d_Window->disable();
    }
}

void LauncherImpl::startStop_Topdown( wxCommandEvent& event )
{
	if(Topdown_CB->IsChecked())
	{
		consoleOut(wxT("Opening Topdown\n"));
		RightDock_FGS->Show(Topdown_SBS,true);
		Layout();
		Fit();
		PLACEHOLDER_B->Enable(true);
	}
	else
	{
		consoleOut(wxT("Closing Topdown\n"));
		RightDock_FGS->Hide(Topdown_SBS,true);
		Layout();
		Fit();
		PLACEHOLDER_B->Enable(false);
	}
}

void LauncherImpl::startStop_PTZL( wxCommandEvent& event )
{
	if(PTZL_CB->IsChecked())
	{
		consoleOut(wxT("Opening Left Pan-Tilt-Zoom\n"));
		LeftDock_FGS->Show(PTZL_SBS,true);
		Layout();
		Fit();
		panPTZL_S->Enable(true);
		tiltPTZL_S->Enable(true);
		zoomPTZL_S->Enable(true);
		PTZL_B->Enable(true);
		myNode->subscribe("image_ptz_left", PTZLImage, &LauncherImpl::incomingPTZLImageConn,this);
		myNode->subscribe("PTZL_state", PTZL_state, &LauncherImpl::incomingPTZLState,this);
		myNode->advertise<axis_cam::PTZActuatorCmd>("PTZL_cmd");
	}
	else
	{
		consoleOut(wxT("Closing Left Pan-Tilt-Zoom\n"));
		LeftDock_FGS->Hide(PTZL_SBS,true);
		myNode->unsubscribe("image_ptz_left");
		panPTZL_S->Enable(false);
		tiltPTZL_S->Enable(false);
		zoomPTZL_S->Enable(false);
		PTZL_B->Enable(false);
		wxSize size(0,0);
		PTZL_B->SetMinSize(size);
		Layout();
		Fit();
		PTZL_bmp == NULL;
	}
}

void LauncherImpl::PTZL_click( wxMouseEvent& event)
{
	ptz_cmd.pan.valid = 1;
	float h_mid = ((float)PTZL_B->GetSize().GetWidth())/2.0f;
	float v_mid = ((float)PTZL_B->GetSize().GetHeight())/2.0f;
	float delH = (event.m_x - h_mid)/h_mid*(21.0f-((float)zoomPTZL_S->GetValue())/500.0f);
	float delV = (event.m_y - v_mid)/v_mid*(15.0f-((float)zoomPTZL_S->GetValue())/700.0f);
	ptz_cmd.pan.cmd = panPTZL + delH;
	ptz_cmd.tilt.valid = 1;
	ptz_cmd.tilt.cmd = tiltPTZL - delV;
	ptz_cmd.zoom.valid = 0;
	myNode->publish("PTZL_cmd",ptz_cmd);
}

void LauncherImpl::incomingPTZLState()
{
	if(PTZL_state.zoom.pos_valid)
		zoomPTZL_S->SetValue(round(PTZL_state.zoom.pos));
	if(PTZL_state.tilt.pos_valid)
		tiltPTZL_S->SetValue(round(PTZL_state.tilt.pos));
	if(PTZL_state.pan.pos_valid)
		panPTZL_S->SetValue(round(PTZL_state.pan.pos));
}

void LauncherImpl::incomingPTZLImageConn()
{
    if(PTZL_GET_NEW_IMAGE)
    {
		PTZL_GET_NEW_IMAGE = false;
		if(PTZLImage.compression == string("raw"))
		{
			PTZLCodec->inflate();
			PTZLImage.compression = string("jpeg");
			if(!PTZLCodec->deflate(100))
				return;
		}
		const uint32_t count = PTZLImage.get_data_size();
		delete PTZLImageData;
		PTZLImageData = new uint8_t[count];
		memcpy(PTZLImageData, PTZLImage.data, sizeof(uint8_t) * count);
		wxMemoryInputStream mis(PTZLImageData,PTZLImage.get_data_size());
		delete PTZL_im;
		PTZL_im = new wxImage(mis,wxBITMAP_TYPE_ANY,-1);

		if(PTZL_bmp == NULL){
			std::cout << "Layout\n";
			wxSize size(PTZLImage.width,PTZLImage.height);
			PTZL_B->SetMinSize(size);
			PTZL_bmp = new wxBitmap();
			Layout();
			Fit();
		}

    	//Event stuff
		wxCommandEvent PTZL_Event(wxEVT_COMMAND_BUTTON_CLICKED, PTZL_B->GetId());
		PTZL_Event.SetEventObject(this);
		this->AddPendingEvent(PTZL_Event);
    }
}

void LauncherImpl::PTZLDrawPic( wxCommandEvent& event )
{
		delete PTZL_bmp;
		PTZL_bmp = new wxBitmap(*PTZL_im);
		wxClientDC dc( PTZL_B );
		dc.DrawBitmap( *PTZL_bmp, 0, 0, false );
		PTZL_GET_NEW_IMAGE = true;
}
//PTZR
void LauncherImpl::startStop_PTZR( wxCommandEvent& event )
{
	if(PTZR_CB->IsChecked())
	{
		consoleOut(wxT("Opening Right Pan-Tilt-Zoom\n"));
		RightDock_FGS->Show(PTZR_SBS,true);
		Layout();
		Fit();
		panPTZR_S->Enable(true);
		tiltPTZR_S->Enable(true);
		zoomPTZR_S->Enable(true);
		PTZR_B->Enable(true);
		myNode->subscribe("image_ptz_right", PTZRImage, &LauncherImpl::incomingPTZRImageConn,this);
		myNode->subscribe("PTZR_state", PTZR_state, &LauncherImpl::incomingPTZRState,this);
		myNode->advertise<axis_cam::PTZActuatorCmd>("PTZR_cmd");
	}
	else
	{
		consoleOut(wxT("Closing Right Pan-Tilt-Zoom\n"));
		RightDock_FGS->Hide(PTZR_SBS,true);
		myNode->unsubscribe("image_ptz_right");
		panPTZR_S->Enable(false);
		tiltPTZR_S->Enable(false);
		zoomPTZR_S->Enable(false);
		PTZR_B->Enable(false);
		wxSize size(0,0);
		PTZR_B->SetMinSize(size);
		Layout();
		Fit();
		PTZR_bmp = NULL;
	}
}

void LauncherImpl::PTZR_click( wxMouseEvent& event)
{
	ptz_cmd.pan.valid = 1;
	float h_mid = ((float)PTZR_B->GetSize().GetWidth())/2.0f;
	float v_mid = ((float)PTZR_B->GetSize().GetHeight())/2.0f;
	float delH = (event.m_x - h_mid)/h_mid*(21.0f-((float)zoomPTZR_S->GetValue())/500.0f);
	float delV = (event.m_y - v_mid)/v_mid*(15.0f-((float)zoomPTZR_S->GetValue())/700.0f);
	ptz_cmd.pan.cmd = panPTZR + delH;
	ptz_cmd.tilt.valid = 1;
	ptz_cmd.tilt.cmd = tiltPTZR - delV;
	ptz_cmd.zoom.valid = 0;
	myNode->publish("PTZR_cmd",ptz_cmd);
}

void LauncherImpl::incomingPTZRState()
{
	if(PTZR_state.zoom.pos_valid)
		zoomPTZR_S->SetValue(round(PTZR_state.zoom.pos));
	if(PTZR_state.tilt.pos_valid)
	{
		tiltPTZR = PTZR_state.tilt.pos;
		tiltPTZR_S->SetValue(round(PTZR_state.tilt.pos));
	}
	if(PTZR_state.pan.pos_valid)
	{
		panPTZR = PTZR_state.pan.pos;
		panPTZR_S->SetValue(round(PTZR_state.pan.pos));
	}
}

void LauncherImpl::incomingPTZRImageConn()
{
    if(PTZR_GET_NEW_IMAGE)
    {
    	PTZR_GET_NEW_IMAGE = false;
    	if(PTZRImage.compression == string("raw"))
		{
			PTZRCodec->inflate();
			PTZRImage.compression = string("jpeg");
			if(!PTZRCodec->deflate(100))
				return;
		}
    	const uint32_t count = PTZRImage.get_data_size();
    	delete PTZRImageData;
		PTZRImageData = new uint8_t[count];
		memcpy(PTZRImageData, PTZRImage.data, sizeof(uint8_t) * count);
		wxMemoryInputStream mis(PTZRImageData,PTZRImage.get_data_size());
		delete PTZR_im;
		PTZR_im = new wxImage(mis,wxBITMAP_TYPE_ANY,-1);
    	if(PTZR_bmp == NULL){
			std::cout << "Layout\n";
			wxSize size(PTZRImage.width,PTZRImage.height);
			PTZR_B->SetMinSize(size);
			PTZR_bmp = new wxBitmap();
			Layout();
			Fit();
		}
    	//Event stuff
		wxCommandEvent PTZR_Event(wxEVT_COMMAND_BUTTON_CLICKED, PTZR_B->GetId());
		PTZR_Event.SetEventObject(this);
		this->AddPendingEvent(PTZR_Event);
    }
}

void LauncherImpl::PTZRDrawPic( wxCommandEvent& event )
{
		delete PTZR_bmp;
		PTZR_bmp = new wxBitmap(*PTZR_im);
		wxClientDC dc( PTZR_B );
		dc.DrawBitmap( *PTZR_bmp, 0, 0, false );
		PTZR_GET_NEW_IMAGE = true;
}
//Left Wrist
void LauncherImpl::startStop_WristL( wxCommandEvent& event )
{
	if(WristL_CB->IsChecked())
	{
		consoleOut(wxT("Opening Left Wrist\n"));
		LeftDock_FGS->Show(WristL_SBS,true);
		Layout();
		Fit();
		myNode->subscribe("image_wrist_left", WristLImage, &LauncherImpl::incomingWristLImageConn,this);
	}
	else
	{
		consoleOut(wxT("Closing Left Wrist\n"));
		LeftDock_FGS->Hide(WristL_SBS,true);
		myNode->unsubscribe("image_wrist_left");
		wxSize size(0,0);
		WristL_B->SetMinSize(size);
		Layout();
		Fit();
		WristL_bmp = NULL;
	}
}

void LauncherImpl::incomingWristLImageConn()
{
    if(WristL_GET_NEW_IMAGE)
    {
    	WristL_GET_NEW_IMAGE = false;
    	if(WristLImage.compression == string("raw"))
		{
			WristLCodec->inflate();
			WristLImage.compression = string("jpeg");
			if(!WristLCodec->deflate(100))
				return;
		}
    	const uint32_t count = WristLImage.get_data_size();
    	delete WristLImageData;
		WristLImageData = new uint8_t[count];
		memcpy(WristLImageData, WristLImage.data, sizeof(uint8_t) * count);
		wxMemoryInputStream mis(WristLImageData,WristLImage.get_data_size());
		delete WristL_im;
		WristL_im = new wxImage(mis,wxBITMAP_TYPE_ANY,-1);
    	if(WristL_bmp == NULL){
			std::cout << "Layout\n";
			wxSize size(WristLImage.width,WristLImage.height);
			WristL_B->SetMinSize(size);
			WristL_bmp = new wxBitmap();
			Layout();
			Fit();
		}
    	//Event stuff
		wxCommandEvent WristL_Event(wxEVT_COMMAND_BUTTON_CLICKED, WristL_B->GetId());
		WristL_Event.SetEventObject(this);
		this->AddPendingEvent(WristL_Event);

    }
}

void LauncherImpl::WristLDrawPic( wxCommandEvent& event )
{
		delete WristL_bmp;
		WristL_bmp = new wxBitmap(*WristL_im);
		wxClientDC dc( WristL_B );
		dc.DrawBitmap( *WristL_bmp, 0, 0, false );
		WristL_GET_NEW_IMAGE = true;
}

//Right Wrist
void LauncherImpl::startStop_WristR( wxCommandEvent& event )
{
	if(WristR_CB->IsChecked())
	{
		consoleOut(wxT("Opening Right Wrist\n"));
		RightDock_FGS->Show(WristR_SBS,true);
		Layout();
		Fit();
		myNode->subscribe("image_wrist_right", WristRImage, &LauncherImpl::incomingWristRImageConn,this);
	}
	else
	{
		consoleOut(wxT("Closing Right Wrist\n"));
		RightDock_FGS->Hide(WristR_SBS,true);
		myNode->unsubscribe("image_wrist_right");
		wxSize size(0,0);
		WristR_B->SetMinSize(size);
		Layout();
		Fit();
		WristR_bmp = NULL;
	}
}

void LauncherImpl::incomingWristRImageConn()
{
    if(WristR_GET_NEW_IMAGE)
    {
    	WristR_GET_NEW_IMAGE = false;
    	if(WristRImage.compression == string("raw"))
		{
			WristRCodec->inflate();
			WristRImage.compression = string("jpeg");
			if(!WristRCodec->deflate(100))
				return;
		}
    	const uint32_t count = WristRImage.get_data_size();
    	delete WristRImageData;
		WristRImageData = new uint8_t[count];
		memcpy(WristRImageData, WristRImage.data, sizeof(uint8_t) * count);
		wxMemoryInputStream mis(WristRImageData,WristRImage.get_data_size());
		delete WristR_im;
		WristR_im = new wxImage(mis,wxBITMAP_TYPE_ANY,-1);
    	if(WristR_bmp == NULL){
			std::cout << "Layout\n";
			wxSize size(WristRImage.width,WristRImage.height);
			WristR_B->SetMinSize(size);
			WristR_bmp = new wxBitmap();
			Layout();
			Fit();
		}
    	//Event stuff
		wxCommandEvent WristR_Event(wxEVT_COMMAND_BUTTON_CLICKED, WristR_B->GetId());
		WristR_Event.SetEventObject(this);
		this->AddPendingEvent(WristR_Event);

    }
}

void LauncherImpl::WristRDrawPic( wxCommandEvent& event )
{
		delete WristR_bmp;
		WristR_bmp = new wxBitmap(*WristR_im);
		wxClientDC dc( WristR_B );
		dc.DrawBitmap( *WristR_bmp, 0, 0, false );
		WristR_GET_NEW_IMAGE = true;
}

void LauncherImpl::PTZL_ptzChanged(wxScrollEvent& event)
{
	ptz_cmd.pan.valid = 1;
	ptz_cmd.pan.cmd = panPTZL_S->GetValue();
	ptz_cmd.tilt.valid = 1;
	ptz_cmd.tilt.cmd = tiltPTZL_S->GetValue();
	ptz_cmd.zoom.valid = 1;
	ptz_cmd.zoom.cmd = zoomPTZL_S->GetValue();
	myNode->publish("PTZL_cmd",ptz_cmd);
}

void LauncherImpl::PTZR_ptzChanged(wxScrollEvent& event)
{
	ptz_cmd.pan.valid = 1;
	ptz_cmd.pan.cmd = panPTZR_S->GetValue();
	ptz_cmd.tilt.valid = 1;
	ptz_cmd.tilt.cmd = tiltPTZR_S->GetValue();
	ptz_cmd.zoom.valid = 1;
	ptz_cmd.zoom.cmd = zoomPTZR_S->GetValue();
	myNode->publish("PTZR_cmd",ptz_cmd);
}

void LauncherImpl::EmergencyStop( wxCommandEvent& event )
{
	// TODO: Implement EmergencyStop
}

template <class T> const T& max ( const T& a, const T& b ) {
	return (b<a)?a:b;
}

template <class T> const T& min ( const T& a, const T& b ) {
	return (a<b)?a:b;
}

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __visualization_panel_generated__
#define __visualization_panel_generated__

#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/toolbar.h>
#include <wx/splitter.h>
#include <wx/listbox.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class VisualizationPanelGenerated
///////////////////////////////////////////////////////////////////////////////
class VisualizationPanelGenerated : public wxPanel 
{
	private:
	
	protected:
		wxSplitterWindow* main_splitter_;
		wxPanel* m_panel3;
		wxPanel* displays_panel_;
		wxPanel* properties_panel_;
		wxBoxSizer* properties_panel_sizer_;
		wxButton* new_display_;
		wxButton* delete_display_;
		wxPanel* render_panel_;
		wxBoxSizer* render_sizer_;
		wxToolBar* views_;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onNewDisplay( wxCommandEvent& event ){ event.Skip(); }
		virtual void onDeleteDisplay( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		VisualizationPanelGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 843,632 ), long style = wxTAB_TRAVERSAL );
		~VisualizationPanelGenerated();
		void main_splitter_OnIdle( wxIdleEvent& )
		{
		main_splitter_->SetSashPosition( 265 );
		main_splitter_->Disconnect( wxEVT_IDLE, wxIdleEventHandler( VisualizationPanelGenerated::main_splitter_OnIdle ), NULL, this );
		}
		
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class NewDisplayDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class NewDisplayDialogGenerated : public wxDialog 
{
	private:
	
	protected:
		wxListBox* types_;
		wxTextCtrl* name_;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onNameEnter( wxCommandEvent& event ){ event.Skip(); }
		virtual void onCancel( wxCommandEvent& event ){ event.Skip(); }
		virtual void onOK( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		NewDisplayDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("New Display"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 394,351 ), long style = wxDEFAULT_DIALOG_STYLE );
		~NewDisplayDialogGenerated();
	
};

#endif //__visualization_panel_generated__

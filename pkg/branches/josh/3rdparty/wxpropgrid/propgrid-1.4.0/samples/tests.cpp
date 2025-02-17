/////////////////////////////////////////////////////////////////////////////
// Name:        tests.cpp
// Purpose:     wxPropertyGrid tests
// Author:      Jaakko Salli
// Modified by:
// Created:     May-16-2007
// RCS-ID:      $Id:
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/propdev.h>
#include <wx/propgrid/advprops.h>
#include <wx/propgrid/manager.h>

#include "propgridsample.h"
#include "sampleprops.h"


// -----------------------------------------------------------------------
// wxTestCustomFlagsProperty
// -----------------------------------------------------------------------

//
// Constant definitions required by wxFlagsProperty examples.
//

static const wxChar* _fs_framestyle_labels[] = {
    wxT("wxCAPTION"),
    wxT("wxMINIMIZE"),
    wxT("wxMAXIMIZE"),
    wxT("wxCLOSE_BOX"),
    wxT("wxSTAY_ON_TOP"),
    wxT("wxSYSTEM_MENU"),
    wxT("wxRESIZE_BORDER"),
    wxT("wxFRAME_TOOL_WINDOW"),
    wxT("wxFRAME_NO_TASKBAR"),
    wxT("wxFRAME_FLOAT_ON_PARENT"),
    wxT("wxFRAME_SHAPED"),
    (const wxChar*) NULL
};

static const long _fs_framestyle_values[] = {
    wxCAPTION,
    wxMINIMIZE,
    wxMAXIMIZE,
    wxCLOSE_BOX,
    wxSTAY_ON_TOP,
    wxSYSTEM_MENU,
    wxRESIZE_BORDER,
    wxFRAME_TOOL_WINDOW,
    wxFRAME_NO_TASKBAR,
    wxFRAME_FLOAT_ON_PARENT,
    wxFRAME_SHAPED
};


WX_PG_IMPLEMENT_CUSTOM_FLAGS_PROPERTY(wxTestCustomFlagsProperty,
                                      _fs_framestyle_labels,
                                      _fs_framestyle_values,
                                      wxDEFAULT_FRAME_STYLE)

WX_PG_IMPLEMENT_CUSTOM_ENUM_PROPERTY(wxTestCustomEnumProperty,
                                      _fs_framestyle_labels,
                                      _fs_framestyle_values,
                                      wxCAPTION)


// Colour labels. Last (before NULL, if any) must be Custom.
static const wxChar* mycolprop_labels[] = {
    wxT("Black"),
    wxT("Blue"),
    wxT("Brown"),
    wxT("Custom"),
    (const wxChar*) NULL
};

// Relevant colour values as unsigned longs.
static unsigned long mycolprop_colours[] = {
    wxPG_COLOUR(0,0,0),
    wxPG_COLOUR(0,0,255),
    wxPG_COLOUR(166,124,81),
    wxPG_COLOUR(0,0,0)
};

// Implement property class. Third argument is optional values array,
// but in this example we are only interested in creating a shortcut
// for user to access the colour values. Last arg is itemcount, but
// it will be deprecated in the future.
WX_PG_DECLARE_CUSTOM_COLOUR_PROPERTY_USES_WXCOLOUR(wxMyColourProperty)
WX_PG_IMPLEMENT_CUSTOM_COLOUR_PROPERTY_USES_WXCOLOUR(wxMyColourProperty,
                                                     mycolprop_labels,
                                                     (long*)NULL,
                                                     mycolprop_colours)


WX_PG_DECLARE_CUSTOM_COLOUR_PROPERTY(wxMyColour2Property)
WX_PG_IMPLEMENT_CUSTOM_COLOUR_PROPERTY(wxMyColour2Property,
                                       mycolprop_labels,
                                       (long*)NULL,
                                       mycolprop_colours)



// Just testing the macros
WX_PG_DECLARE_STRING_PROPERTY(wxTestStringProperty)
WX_PG_IMPLEMENT_STRING_PROPERTY(wxTestStringProperty,wxPG_NO_ESCAPE)
bool wxTestStringProperty::OnButtonClick( wxPropertyGrid*,
                                          wxString& )
{
    ::wxMessageBox(wxT("Button Clicked"));
    return true;
}

WX_PG_DECLARE_STRING_PROPERTY(wxTextStringPropertyWithValidator)
WX_PG_IMPLEMENT_STRING_PROPERTY_WITH_VALIDATOR(wxTextStringPropertyWithValidator,
                                               wxPG_NO_ESCAPE)

bool wxTextStringPropertyWithValidator::OnButtonClick( wxPropertyGrid* WXUNUSED(propgrid),
                                                       wxString& WXUNUSED(value) )
{
    ::wxMessageBox(wxT("Button Clicked"));
    return true;
}

wxValidator* wxTextStringPropertyWithValidator::DoGetValidator() const
{
#if wxUSE_VALIDATORS
    WX_PG_DOGETVALIDATOR_ENTRY()
    wxTextValidator* validator = new
        wxTextValidator(wxFILTER_INCLUDE_CHAR_LIST);
    wxArrayString oValid;
    oValid.Add(wxT("0"));
    oValid.Add(wxT("1"));
    oValid.Add(wxT("2"));
    oValid.Add(wxT("3"));
    oValid.Add(wxT("4"));
    oValid.Add(wxT("5"));
    oValid.Add(wxT("6"));
    oValid.Add(wxT("7"));
    oValid.Add(wxT("8"));
    oValid.Add(wxT("9"));
    oValid.Add(wxT("$"));
    validator->SetIncludes(oValid);
    WX_PG_DOGETVALIDATOR_EXIT(validator)
#else
    return NULL;
#endif
}

// -----------------------------------------------------------------------

//
// Test customizing wxColourProperty via subclassing
//
// * Includes custom colour entry.
// * Includes extra custom entry.
//
class MyColourProperty3 : public wxColourProperty
{
public:
    MyColourProperty3( const wxString& label = wxPG_LABEL,
                       const wxString& name = wxPG_LABEL,
                       const wxColour& value = *wxWHITE )
        : wxColourProperty(label, name, value)
    {
        wxPGChoices colours;
        colours.Add(wxT("White"));
        colours.Add(wxT("Black"));
        colours.Add(wxT("Red"));
        colours.Add(wxT("Green"));
        colours.Add(wxT("Blue"));
        colours.Add(wxT("Custom"));
        colours.Add(wxT("None"));
        m_choices = colours;
        SetIndex(0);
        wxVariant variant;
        variant << value;
        SetValue(variant);
    }

    virtual ~MyColourProperty3()
    {
    }

    virtual wxColour GetColour( int index ) const
    {
        switch (index)
        {
            case 0: return *wxWHITE;
            case 1: return *wxBLACK;
            case 2: return *wxRED;
            case 3: return *wxGREEN;
            case 4: return *wxBLUE;
            case 5:
                // Return current colour for the custom entry
                wxColour col;
                if ( GetIndex() == GetCustomColourIndex() )
                {
                    if ( m_value.IsNull() )
                        return col;
                    col << m_value;
                    return col;
                }
                return *wxWHITE;
        };
        return wxColour();
    }

    virtual wxString ColourToString( const wxColour& col, int index ) const
    {
        if ( index == (int)(m_choices.GetCount()-1) )
            return wxT("");

        return wxColourProperty::ColourToString(col, index);
    }

    virtual int GetCustomColourIndex() const
    {
        return m_choices.GetCount()-2;
    }
};


void FormMain::AddTestProperties( wxPropertyGridPage* pg )
{
    pg->Append( new wxMyColourProperty(wxT("CustomColourProperty1")) );

    pg->SetPropertyHelpString(wxT("CustomColourProperty1"),
        wxT("This is a wxMyColourProperty from the sample app. ")
        wxT("It is built with WX_PG_IMPLEMENT_CUSTOM_COLOUR_PROPERTY_USES_WXCOLOUR macro ")
        wxT("and has wxColour as its data type"));

    pg->Append( new wxMyColour2Property(wxT("CustomColourProperty2")) );

    pg->SetPropertyHelpString(wxT("CustomColourProperty2"),
        wxT("This is a wxMyColour2Property from the sample app. ")
        wxT("It is built with WX_PG_IMPLEMENT_CUSTOM_COLOUR_PROPERTY macro ")
        wxT("and has wxColourPropertyValue as its data type"));

    pg->Append( new MyColourProperty3(wxT("CustomColourProperty3"), wxPG_LABEL, *wxGREEN) );
    pg->GetProperty(wxT("CustomColourProperty3"))->SetFlag(wxPG_PROP_AUTO_UNSPECIFIED);
    pg->SetPropertyEditor( wxT("CustomColourProperty3"), wxPG_EDITOR(ComboBox) );

    pg->SetPropertyHelpString(wxT("CustomColourProperty3"),
        wxT("This is a MyColourProperty3 from the sample app. ")
        wxT("It is built by subclassing wxColourProperty."));

    pg->Append( new wxTextStringPropertyWithValidator(wxT("TestProp1"), wxPG_LABEL) );
}

// -----------------------------------------------------------------------

void FormMain::OnDumpList( wxCommandEvent& WXUNUSED(event) )
{
    wxVariant values = m_pPropGridManager->GetPropertyValues(wxT("list"), wxNullProperty, wxPG_INC_ATTRIBUTES);
    wxString text = wxT("This only tests that wxVariant related routines do not crash.");
    wxString t;

    wxDialog* dlg = new wxDialog(this,-1,wxT("wxVariant Test"),
        wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

    wxPGIndex i;
    for ( i = 0; i < (wxPGIndex)values.GetCount(); i++ )
    {
        wxVariant& v = values[i];

        wxString strValue = v.GetString();

#if wxCHECK_VERSION(2,8,0)
        if ( v.GetName().EndsWith(wxT("@attr")) )
#else
        if ( v.GetName().Right(5) == wxT("@attr") )
#endif
        {
            text += wxString::Format(wxT("Attributes:\n"));

            wxPGIndex n;
            for ( n = 0; n < (wxPGIndex)v.GetCount(); n++ )
            {
                wxVariant& a = v[n];

                t.Printf(wxT("  atribute %i: name=\"%s\"  (type=\"%s\"  value=\"%s\")\n"),(int)n,
                    a.GetName().c_str(),a.GetType().c_str(),a.GetString().c_str());
                text += t;
            }
        }
        else
        {
            t.Printf(wxT("%i: name=\"%s\"  type=\"%s\"  value=\"%s\"\n"),(int)i,
                v.GetName().c_str(),v.GetType().c_str(),strValue.c_str());
            text += t;
        }
    }

    // multi-line text editor dialog
    const int spacing = 8;
    wxBoxSizer* topsizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* rowsizer = new wxBoxSizer( wxHORIZONTAL );
    wxTextCtrl* ed = new wxTextCtrl(dlg,11,text,
        wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY);
    rowsizer->Add( ed, 1, wxEXPAND|wxALL, spacing );
    topsizer->Add( rowsizer, 1, wxEXPAND, 0 );
    rowsizer = new wxBoxSizer( wxHORIZONTAL );
    const int butSzFlags =
        wxALIGN_CENTRE_HORIZONTAL|wxALIGN_CENTRE_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT;
    rowsizer->Add( new wxButton(dlg,wxID_OK,wxT("Ok")),
        0, butSzFlags, spacing );
    topsizer->Add( rowsizer, 0, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL, 0 );

    dlg->SetSizer( topsizer );
    topsizer->SetSizeHints( dlg );

    dlg->SetSize(400,300);
    dlg->Centre();
    dlg->ShowModal();
}

// -----------------------------------------------------------------------

class TestRunner
{
public:

    TestRunner( const wxString& name, wxPropertyGridManager* man, wxTextCtrl* ed, wxArrayString* errorMessages )
    {
        m_name = name;
        m_man = man;
        m_ed = ed;
        m_errorMessages = errorMessages;
#ifdef __WXDEBUG__
        m_preWarnings = wxPGGlobalVars->m_warnings;
#endif

        if ( name != wxT("none") )
            Msg(name+wxT("\n"));
    }

    ~TestRunner()
    {
#ifdef __WXDEBUG__
        int warningsOccurred = wxPGGlobalVars->m_warnings - m_preWarnings;
        if ( warningsOccurred )
        {
            wxString s = wxString::Format(wxT("%i warnings occurred during test '%s'"), warningsOccurred, m_name.c_str());
            m_errorMessages->push_back(s);
            Msg(s);
        }
#endif
    }

    void Msg( const wxString& text )
    {
        if ( m_ed )
        {
            m_ed->AppendText(text);
            m_ed->AppendText(wxT("\n"));
        }
        wxLogDebug(text);
    }

protected:
    wxPropertyGridManager* m_man;
    wxTextCtrl* m_ed;
    wxArrayString* m_errorMessages;
    wxString m_name;
#ifdef __WXDEBUG__
    int m_preWarnings;
#endif
};


#define RT_START_TEST(TESTNAME) \
    TestRunner tr(wxT(#TESTNAME), pgman, ed, &errorMessages);

#define RT_MSG(S) \
    tr.Msg(S);

#define RT_FAILURE() \
    { \
        wxString s1 = wxString::Format(wxT("Test failure in tests.cpp, line %i."),__LINE__-1); \
        errorMessages.push_back(s1); \
        wxLogDebug(s1); \
        failures++; \
    }

#define RT_FAILURE_MSG(MSG) \
    { \
        wxString s1 = wxString::Format(wxT("Test failure in tests.cpp, line %i."),__LINE__-1); \
        errorMessages.push_back(s1); \
        wxLogDebug(s1); \
        wxString s2 = wxString::Format(wxT("Message: %s"),MSG); \
        errorMessages.push_back(s2); \
        wxLogDebug(s2); \
        failures++; \
    }

#define RT_VALIDATE_VIRTUAL_HEIGHT(PROPS, EXTRATEXT) \
    { \
        unsigned int h1_ = PROPS->GetVirtualHeight(); \
        unsigned int h2_ = PROPS->GetActualVirtualHeight(); \
        if ( h1_ != h2_ ) \
        { \
            wxString s_ = wxString::Format(wxT("VirtualHeight = %i, should be %i (%s)"), h1_, h2_, EXTRATEXT.c_str()); \
            RT_FAILURE_MSG(s_.c_str()); \
            _failed_ = true; \
        } \
        else \
        { \
            _failed_ = false; \
        } \
    }


int gpiro_cmpfunc(const void* a, const void* b)
{
    const wxPGProperty* p1 = (const wxPGProperty*) a;
    const wxPGProperty* p2 = (const wxPGProperty*) b;
    return (int) (((size_t)p1->GetClientData()) - ((size_t)p2->GetClientData()));
}

wxArrayPGProperty GetPropertiesInRandomOrder( wxPropertyGridInterface* props, int iterationFlags = wxPG_ITERATE_ALL )
{
    wxArrayPGProperty arr;

    wxPropertyGridIterator it;

    for ( it = props->GetIterator(iterationFlags);
          !it.AtEnd();
          it++ )
    {
        wxPGProperty* p = *it;
        p->SetClientData((void*)rand());
        arr.push_back(p);
    }

    wxPGProperty** firstEntry = &arr[0];
    qsort(firstEntry, arr.size(), sizeof(wxPGProperty*), gpiro_cmpfunc);

    return arr;
}


bool FormMain::RunTests( bool fullTest, bool interactive )
{
    wxString t;

    wxPropertyGridManager* pgman = m_pPropGridManager;
    wxPropertyGrid* pg;

    size_t i;

    pgman->ClearSelection();

    int failures = 0;
    bool _failed_ = false;
    wxArrayString errorMessages;
    wxDialog* dlg = NULL;

    dlg = new wxDialog(this,-1,wxT("wxPropertyGrid Regression Tests"),
        wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

    // multi-line text editor dialog
    const int spacing = 8;
    wxBoxSizer* topsizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* rowsizer = new wxBoxSizer( wxHORIZONTAL );
    wxTextCtrl* ed = new wxTextCtrl(dlg,11,wxEmptyString,
        wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY);
    rowsizer->Add( ed, 1, wxEXPAND|wxALL, spacing );
    topsizer->Add( rowsizer, 1, wxEXPAND, 0 );
    rowsizer = new wxBoxSizer( wxHORIZONTAL );
    const int butSzFlags =
        wxALIGN_CENTRE_HORIZONTAL|wxALIGN_CENTRE_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT;
    rowsizer->Add( new wxButton(dlg,wxID_OK,wxT("Ok")),
        0, butSzFlags, spacing );
    topsizer->Add( rowsizer, 0, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL, 0 );

    dlg->SetSizer( topsizer );
    topsizer->SetSizeHints( dlg );

    dlg->SetSize(400,300);
    dlg->Move(wxSystemSettings::GetMetric(wxSYS_SCREEN_X)-dlg->GetSize().x,
              wxSystemSettings::GetMetric(wxSYS_SCREEN_Y)-dlg->GetSize().y);
    dlg->Show();

    {
        //
        // Basic iterator tests
        RT_START_TEST(GetIterator)

        wxPGVIterator it;
        int count;

        count = 0;
        for ( it = pgman->GetVIterator(wxPG_ITERATE_PROPERTIES);
              !it.AtEnd();
              it.Next() )
        {
            wxPGProperty* p = it.GetProperty();
            if ( p->IsCategory() )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' is a category (non-private child property expected)"),p->GetLabel().c_str()).c_str())
            else if ( p->GetParent()->HasFlag(wxPG_PROP_AGGREGATE) )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' is a private child (non-private child property expected)"),p->GetLabel().c_str()).c_str())
            count++;
        }

        RT_MSG(wxString::Format(wxT("GetVIterator(wxPG_ITERATE_PROPERTIES) -> %i entries"), count));

        count = 0;
        for ( it = pgman->GetVIterator(wxPG_ITERATE_CATEGORIES);
              !it.AtEnd();
              it.Next() )
        {
            wxPGProperty* p = it.GetProperty();
            if ( !p->IsCategory() )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' is not a category (only category was expected)"),p->GetLabel().c_str()).c_str())
            count++;
        }

        RT_MSG(wxString::Format(wxT("GetVIterator(wxPG_ITERATE_CATEGORIES) -> %i entries"), count));

        count = 0;
        for ( it = pgman->GetVIterator(wxPG_ITERATE_PROPERTIES|wxPG_ITERATE_CATEGORIES);
              !it.AtEnd();
              it.Next() )
        {
            wxPGProperty* p = it.GetProperty();
            if ( p->GetParent()->HasFlag(wxPG_PROP_AGGREGATE) )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' is a private child (non-private child property or category expected)"),p->GetLabel().c_str()).c_str())
            count++;
        }

        RT_MSG(wxString::Format(wxT("GetVIterator(wxPG_ITERATE_PROPERTIES|wxPG_ITERATE_CATEGORIES) -> %i entries"), count));

        count = 0;
        for ( it = pgman->GetVIterator(wxPG_ITERATE_VISIBLE);
              !it.AtEnd();
              it.Next() )
        {
            wxPGProperty* p = it.GetProperty();
            if ( (p->GetParent() != p->GetParentState()->DoGetRoot() && !p->GetParent()->IsExpanded()) )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' had collapsed parent (only visible properties expected)"),p->GetLabel().c_str()).c_str())
            else if ( p->HasFlag(wxPG_PROP_HIDDEN) )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' was hidden (only visible properties expected)"),p->GetLabel().c_str()).c_str())
            count++;
        }

        RT_MSG(wxString::Format(wxT("GetVIterator(wxPG_ITERATE_VISIBLE) -> %i entries"), count));
    }

    if ( fullTest )
    {
        // Test that setting focus to properties does not crash things
        RT_START_TEST(SelectProperty)

        wxPropertyGridIterator it;
        size_t ind;

        for ( ind=0; ind<pgman->GetPageCount(); ind++ )
        {
            wxPropertyGridPage* page = pgman->GetPage(ind);
            pgman->SelectPage(page);

            for ( it = page->GetIterator(wxPG_ITERATE_VISIBLE);
                  !it.AtEnd();
                  it++ )
            {
                wxPGProperty* p = *it;
                RT_MSG(p->GetLabel());
                pgman->GetGrid()->SelectProperty(p, true);
                ::wxMilliSleep(150);
                Update();
            }
        }
    }

    {
        RT_START_TEST(GetPropertiesWithFlag)

        //
        // Get list of expanded properties
        wxArrayPGProperty array = pgman->GetExpandedProperties();

        // Make sure list only has items with children
        for ( i=0; i<array.size(); i++ )
        {
            wxPGProperty* p = array[i];
            if ( !p->IsKindOf(CLASSINFO(wxPGProperty)) )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s' was returned by GetExpandedProperties(), but was not a parent"),p->GetName().c_str()).c_str());
        }

        wxArrayString names;
        pgman->PropertiesToNames( &names, array );

        //
        // ... and then collapse them
        wxArrayPGProperty array2;
        pgman->NamesToProperties( &array2, names );
        pgman->SetExpandedProperties( array2, false );

        // Make sure everything is collapsed
        wxPGVIterator it;

        for ( it = pgman->GetVIterator(wxPG_ITERATE_ALL);
              !it.AtEnd();
              it.Next() )
        {
            wxPGProperty* p = it.GetProperty();
            if ( p->IsExpanded() )
                RT_FAILURE_MSG(wxString::Format(wxT("'%s.%s' was expanded"),p->GetParent()->GetName().c_str(),p->GetName().c_str()).c_str());
        }

        pgman->Refresh();
    }

    {
        //
        // Delete everything in reverse order
        RT_START_TEST(DeleteProperty)

        wxPGVIterator it;
        wxArrayPGProperty array;

        for ( it = pgman->GetVIterator(wxPG_ITERATE_ALL&~(wxPG_IT_CHILDREN(wxPG_PROP_AGGREGATE)));
              !it.AtEnd();
              it.Next() )
            array.push_back(it.GetProperty());

        wxArrayPGProperty::reverse_iterator it2;

        for ( it2 = array.rbegin(); it2 != array.rend(); it2++ )
        {
            wxPGProperty* p = (wxPGProperty*)*it2;
            RT_MSG(wxString::Format(wxT("Deleting '%s' ('%s')"),p->GetLabel().c_str(),p->GetName().c_str()));
            pgman->DeleteProperty(p);
        }

        // Recreate grid
        CreateGrid( -1, -1 );
        pgman = m_pPropGridManager;
    }

    {
        //
        // Clear property value
        RT_START_TEST(ClearPropertyValue)

        wxPGVIterator it;

        for ( it = pgman->GetVIterator(wxPG_ITERATE_PROPERTIES);
              !it.AtEnd();
              it.Next() )
        {
            RT_MSG(wxString::Format(wxT("Clearing value of '%s'"),it.GetProperty()->GetLabel().c_str()));
            pgman->ClearPropertyValue(it.GetProperty());
        }

        // Recreate grid
        CreateGrid( -1, -1 );
        pgman = m_pPropGridManager;
    }

    {
        RT_START_TEST(GetPropertyValues)

        for ( i=0; i<3; i++ )
        {
            wxString text;

            wxPropertyGridPage* page = pgman->GetPage(i);

            wxVariant values = page->GetPropertyValues();

            wxPGIndex i;
            for ( i = 0; i < (wxPGIndex)values.GetCount(); i++ )
            {
                wxVariant& v = values[i];

                t.Printf(wxT("%i: name=\"%s\"  type=\"%s\"\n"),(int)i,
                    v.GetName().c_str(),v.GetType().c_str());

                text += t;
            }
            ed->AppendText(text);
        }
    }

    {
        RT_START_TEST(SetPropertyValue_and_GetPropertyValue)

        //pg = (wxPropertyGrid*) NULL;

        wxArrayString test_arrstr_1;
        test_arrstr_1.Add(wxT("Apple"));
        test_arrstr_1.Add(wxT("Orange"));
        test_arrstr_1.Add(wxT("Lemon"));

        wxArrayString test_arrstr_2;
        test_arrstr_2.Add(wxT("Potato"));
        test_arrstr_2.Add(wxT("Cabbage"));
        test_arrstr_2.Add(wxT("Cucumber"));

        wxArrayInt test_arrint_1;
        test_arrint_1.Add(1);
        test_arrint_1.Add(2);
        test_arrint_1.Add(3);

        wxArrayInt test_arrint_2;
        test_arrint_2.Add(0);
        test_arrint_2.Add(1);
        test_arrint_2.Add(4);

#if wxUSE_DATETIME
        wxDateTime dt1 = wxDateTime::Now();
        dt1.SetYear(dt1.GetYear()-1);

        wxDateTime dt2 = wxDateTime::Now();
        dt2.SetYear(dt2.GetYear()-10);
#endif

#define FLAG_TEST_SET1 (wxCAPTION|wxCLOSE_BOX|wxSYSTEM_MENU|wxRESIZE_BORDER)
#define FLAG_TEST_SET2 (wxSTAY_ON_TOP|wxCAPTION|wxICONIZE|wxSYSTEM_MENU)

        pgman->SetPropertyValue(wxT("StringProperty"),wxT("Text1"));
        pgman->SetPropertyValue(wxT("IntProperty"),1024);
        pgman->SetPropertyValue(wxT("FloatProperty"),1024.0000000001);
        pgman->SetPropertyValue(wxT("BoolProperty"),FALSE);
        pgman->SetPropertyValue(wxT("EnumProperty"),120);
        pgman->SetPropertyValue(wxT("Custom FlagsProperty"),FLAG_TEST_SET1);
        pgman->SetPropertyValue(wxT("ArrayStringProperty"),test_arrstr_1);
        wxColour emptyCol;
        pgman->SetPropertyValue(wxT("ColourProperty"),emptyCol);
        pgman->SetPropertyValue(wxT("ColourProperty"),(wxObject*)wxBLACK);
        pgman->SetPropertyValue(wxT("Size"),wxSize(150,150));
        pgman->SetPropertyValue(wxT("Position"),wxPoint(150,150));
        pgman->SetPropertyValue(wxT("MultiChoiceProperty"),test_arrint_1);
#if wxUSE_DATETIME
        pgman->SetPropertyValue(wxT("DateProperty"),dt1);
#endif

        pgman->SelectPage(2);
        pg = pgman->GetGrid();

        if ( pg->GetPropertyValueAsString(wxT("StringProperty")) != wxT("Text1") )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsInt(wxT("IntProperty")) != 1024 )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsDouble(wxT("FloatProperty")) != 1024.0000000001 )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsBool(wxT("BoolProperty")) != FALSE )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsLong(wxT("EnumProperty")) != 120 )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsArrayString(wxT("ArrayStringProperty")) != test_arrstr_1 )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsLong(wxT("Custom FlagsProperty")) != FLAG_TEST_SET1 )
            RT_FAILURE();
        wxColour col;
        col << pgman->GetPropertyValue(wxT("ColourProperty"));
        if ( col != *wxBLACK )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsSize(wxT("Size")) != wxSize(150,150) )
            RT_FAILURE();
        if ( pg->GetPropertyValueAsPoint(wxT("Position")) != wxPoint(150,150) )
            RT_FAILURE();
        if ( !(pg->GetPropertyValueAsArrayInt(wxT("MultiChoiceProperty")) == test_arrint_1) )
            RT_FAILURE();
#if wxUSE_DATETIME
        if ( !(pg->GetPropertyValueAsDateTime(wxT("DateProperty")) == dt1) )
            RT_FAILURE();
#endif

        pgman->SetPropertyValue(wxT("IntProperty"),wxLL(10000000000));
        if ( pg->GetPropertyValueAsLongLong(wxT("IntProperty")) != wxLL(10000000000) )
            RT_FAILURE();

        pg->SetPropertyValue(wxT("StringProperty"),wxT("Text2"));
        pg->SetPropertyValue(wxT("IntProperty"),512);
        pg->SetPropertyValue(wxT("FloatProperty"),512.0);
        pg->SetPropertyValue(wxT("BoolProperty"),TRUE);
        pg->SetPropertyValue(wxT("EnumProperty"),80);
        pg->SetPropertyValue(wxT("ArrayStringProperty"),test_arrstr_2);
        pg->SetPropertyValue(wxT("Custom FlagsProperty"),FLAG_TEST_SET2);
        pg->SetPropertyValue(wxT("ColourProperty"),(wxObject*)wxWHITE);
        pg->SetPropertyValue(wxT("Size"),wxSize(300,300));
        pg->SetPropertyValue(wxT("Position"),wxPoint(300,300));
        pg->SetPropertyValue(wxT("MultiChoiceProperty"),test_arrint_2);
#if wxUSE_DATETIME
        pg->SetPropertyValue(wxT("DateProperty"),dt2);
#endif

        //pg = (wxPropertyGrid*) NULL;

        pgman->SelectPage(0);

        if ( pgman->GetPropertyValueAsString(wxT("StringProperty")) != wxT("Text2") )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsInt(wxT("IntProperty")) != 512 )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsDouble(wxT("FloatProperty")) != 512.0 )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsBool(wxT("BoolProperty")) != TRUE )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsLong(wxT("EnumProperty")) != 80 )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsArrayString(wxT("ArrayStringProperty")) != test_arrstr_2 )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsLong(wxT("Custom FlagsProperty")) != FLAG_TEST_SET2 )
            RT_FAILURE();
        col << pgman->GetPropertyValue(wxT("ColourProperty"));
        if ( col != *wxWHITE )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsSize(wxT("Size")) != wxSize(300,300) )
            RT_FAILURE();
        if ( pgman->GetPropertyValueAsPoint(wxT("Position")) != wxPoint(300,300) )
            RT_FAILURE();
        if ( !(pgman->GetPropertyValueAsArrayInt(wxT("MultiChoiceProperty")) == test_arrint_2) )
            RT_FAILURE();
#if wxUSE_DATETIME
        if ( !(pgman->GetPropertyValueAsDateTime(wxT("DateProperty")) == dt2) )
            RT_FAILURE();
#endif

        pgman->SetPropertyValue(wxT("IntProperty"),wxLL(-80000000000));
        if ( pgman->GetPropertyValueAsLongLong(wxT("IntProperty")) != wxLL(-80000000000) )
            RT_FAILURE();

        // Make sure children of composite parent get updated as well
        // Original string value: "Lamborghini Diablo SV; [300; 3.9; 8.6]; 300000"

        // This updates children as well
        wxString nvs = wxT("Lamborghini Diablo XYZ; 5707; [100; 3.9; 8.6] 3000002");
        pgman->SetPropertyValue(wxT("Car"), nvs);

        if ( pgman->GetProperty(wxT("Car"))->GetDisplayedString() != nvs )
        {
            wxLogDebug(wxT("Did not match: Car=%s"), pgman->GetProperty(wxT("Car"))->GetDisplayedString().c_str());
            RT_FAILURE();
        }

        if ( pgman->GetPropertyValueAsString(wxT("Car.Model")) != wxT("Lamborghini Diablo XYZ") )
        {
            wxLogDebug(wxT("Did not match: Car.Model=%s"), pgman->GetPropertyValueAsString(wxT("Car.Model")).c_str());
            RT_FAILURE();
        }

        if ( pgman->GetPropertyValueAsInt(wxT("Car.Speeds.Max. Speed (mph)")) != 100 )
        {
            wxLogDebug(wxT("Did not match: Car.Speeds.Max. Speed (mph)=%s"), pgman->GetPropertyValueAsString(wxT("Car.Speeds.Max. Speed (mph)")).c_str());
            RT_FAILURE();
        }

        if ( pgman->GetPropertyValueAsInt(wxT("Car.Price ($)")) != 3000002 )
        {
            wxLogDebug(wxT("Did not match: Car.Price ($)=%s"), pgman->GetPropertyValueAsString(wxT("Car.Price ($)")).c_str());
            RT_FAILURE();
        }
    }

    {
        RT_START_TEST(SetPropertyValueUnspecified)

        // Null variant setter tests
        pgman->SetPropertyValueUnspecified(wxT("StringProperty"));
        pgman->SetPropertyValueUnspecified(wxT("IntProperty"));
        pgman->SetPropertyValueUnspecified(wxT("FloatProperty"));
        pgman->SetPropertyValueUnspecified(wxT("BoolProperty"));
        pgman->SetPropertyValueUnspecified(wxT("EnumProperty"));
        pgman->SetPropertyValueUnspecified(wxT("ArrayStringProperty"));
        pgman->SetPropertyValueUnspecified(wxT("Custom FlagsProperty"));
        pgman->SetPropertyValueUnspecified(wxT("ColourProperty"));
        pgman->SetPropertyValueUnspecified(wxT("Size"));
        pgman->SetPropertyValueUnspecified(wxT("Position"));
        pgman->SetPropertyValueUnspecified(wxT("MultiChoiceProperty"));
#if wxUSE_DATETIME
        pgman->SetPropertyValueUnspecified(wxT("DateProperty"));
#endif
    }

    {
        RT_START_TEST(Attributes)

        wxPGProperty* prop = pgman->GetProperty(wxT("StringProperty"));
        prop->SetAttribute(wxT("Dummy Attribute"), (long)15);

        if ( prop->GetAttribute(wxT("Dummy Attribute")).GetLong() != 15 )
            RT_FAILURE();

        prop->SetAttribute(wxT("Dummy Attribute"), wxVariant());

        if ( !prop->GetAttribute(wxT("Dummy Attribute")).IsNull() )
            RT_FAILURE();
    }

    {
        wxPropertyGridPage* page1;
        wxPropertyGridPage* page2;
        wxPropertyGridPage* page3;
        wxVariant pg1_values;
        wxVariant pg2_values;
        wxVariant pg3_values;

        {
            RT_START_TEST(GetPropertyValues)

            page1 = pgman->GetPage(0);
            pg1_values = page1->GetPropertyValues(wxT("Page1"),NULL,wxPG_KEEP_STRUCTURE);
            page2 = pgman->GetPage(1);
            pg2_values = page2->GetPropertyValues(wxT("Page2"),NULL,wxPG_KEEP_STRUCTURE);
            page3 = pgman->GetPage(2);
            pg3_values = page3->GetPropertyValues(wxT("Page3"),NULL,wxPG_KEEP_STRUCTURE);
        }

        {
            RT_START_TEST(SetPropertyValues)

            page1->SetPropertyValues(pg3_values);
            page2->SetPropertyValues(pg1_values);
            page3->SetPropertyValues(pg2_values);
        }
    }

    if ( !(pgman->GetWindowStyleFlag()&wxPG_HIDE_CATEGORIES) )
    {
        RT_START_TEST(Collapse_and_GetFirstCategory_and_GetNextCategory)

        for ( i=0; i<3; i++ )
        {
            wxPropertyGridPage* page = pgman->GetPage(i);

            wxPropertyGridIterator it;

            for ( it = page->GetIterator( wxPG_ITERATE_CATEGORIES );
                  !it.AtEnd();
                  it++ )
            {
                wxPGProperty* p = *it;

                if ( !page->IsPropertyCategory(p) )
                    RT_FAILURE();

                page->Collapse( p );

                t.Printf(wxT("Collapsing: %s\n"),page->GetPropertyLabel(p).c_str());
                ed->AppendText(t);
            }
        }
    }

    {
        RT_START_TEST(Save_And_RestoreEditableState)

        for ( i=0; i<3; i++ )
        {
            pgman->SelectPage(i);

            wxString stringState = pgman->SaveEditableState();
            bool res = pgman->RestoreEditableState(stringState);
            if ( !res )
                RT_FAILURE();
        }
    }

    //if ( !(pgman->GetWindowStyleFlag()&wxPG_HIDE_CATEGORIES) )
    {
        RT_START_TEST(Expand_and_GetFirstCategory_and_GetNextCategory)

        for ( i=0; i<3; i++ )
        {
            wxPropertyGridPage* page = pgman->GetPage(i);

            wxPropertyGridIterator it;

            for ( it = page->GetIterator( wxPG_ITERATE_CATEGORIES );
                  !it.AtEnd();
                  it++ )
            {
                wxPGProperty* p = *it;

                if ( !page->IsPropertyCategory(p) )
                    RT_FAILURE();

                page->Expand( p );

                t.Printf(wxT("Expand: %s\n"),page->GetPropertyLabel(p).c_str());
                ed->AppendText(t);
            }
        }
    }

    //if ( !(pgman->GetWindowStyleFlag()&wxPG_HIDE_CATEGORIES) )
    {
        RT_START_TEST(RandomCollapse)

        // Select the most error prone page as visible.
        pgman->SelectPage(1);

        for ( i=0; i<3; i++ )
        {
            wxArrayPtrVoid arr;

            wxPropertyGridPage* page = pgman->GetPage(i);

            wxPropertyGridIterator it;

            for ( it = page->GetIterator( wxPG_ITERATE_CATEGORIES );
                  !it.AtEnd();
                  it++ )
            {
                arr.Add((void*)*it);
            }

            if ( arr.GetCount() )
            {
                size_t n;

                pgman->Collapse( (wxPGProperty*)arr.Item(0) );

                for ( n=arr.GetCount()-1; n>0; n-- )
                {
                    pgman->Collapse( (wxPGProperty*)arr.Item(n) );
                }
            }

        }
    }

    {
        RT_START_TEST(EnsureVisible)
        pgman->EnsureVisible(wxT("Cell Colour"));
    }

    {
        RT_START_TEST(SetPropertyBackgroundColour)
        wxCommandEvent evt;
        evt.SetInt(1); // IsChecked() will return TRUE.
        evt.SetId(ID_COLOURSCHEME4);
        OnCatColours(evt);
        OnColourScheme(evt);
    }

    {
        // Test ClearPropertyValue
        RT_START_TEST(ClearPropertyValue)

        for ( i=0; i<3; i++ )
        {
            wxPropertyGridPage* page = pgman->GetPage(i);

            // Iterate over all properties.
            wxPropertyGridIterator it;

            for ( it = page->GetIterator();
                  !it.AtEnd();
                  it++ )
            {
                wxLogDebug((*it)->GetLabel());
                pgman->ClearPropertyValue( *it );
            }
        }

    }

    {
        RT_START_TEST(ManagerClear)
        pgman->Clear();

        if ( pgman->GetPageCount() )
            RT_FAILURE();

        // Recreate the original grid
        CreateGrid( -1, -1 );
        pgman = m_pPropGridManager;
    }

    /*
    {
        // TODO: This test fails.
        RT_START_TEST(SetSplitterPosition)

        InitPanel();

        const int trySplitterPos = 50;
    
        int style = wxPG_AUTO_SORT;  // wxPG_SPLITTER_AUTO_CENTER;
        pgman = m_pPropGridManager =
            new wxPropertyGridManager(m_panel, wxID_ANY,
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      style );

        PopulateGrid();
        pgman->SetSplitterPosition(trySplitterPos);

        if ( pgman->GetGrid()->GetSplitterPosition() != trySplitterPos )
            RT_FAILURE_MSG(wxString::Format(wxT("Splitter position was %i (should have been %i)"),(int)pgman->GetGrid()->GetSplitterPosition(),trySplitterPos).c_str());

        m_topSizer->Add( m_pPropGridManager, 1, wxEXPAND );
        FinalizePanel();

        wxSize sz = GetSize();
        wxSize origSz = sz;
        sz.x += 5;
        sz.y += 5;

        if ( pgman->GetGrid()->GetSplitterPosition() != trySplitterPos )
            RT_FAILURE_MSG(wxString::Format(wxT("Splitter position was %i (should have been %i)"),(int)pgman->GetGrid()->GetSplitterPosition(),trySplitterPos).c_str());

        SetSize(origSz);

        // Recreate the original grid
        CreateGrid( -1, -1 );
        pgman = m_pPropGridManager;
    }
    */

    {
        RT_START_TEST(HideProperty)

        wxPropertyGridPage* page = pgman->GetPage(0);

        srand(0x1234);

        wxArrayPGProperty arr1;
        
        arr1 = GetPropertiesInRandomOrder(page);

        if ( !_failed_ )
        {
            for ( i=0; i<arr1.size(); i++ )
            {
                wxPGProperty* p = arr1[i];
                page->HideProperty(p, true);

                wxString s = wxString::Format(wxT("HideProperty(%i, %s)"), i, p->GetLabel().c_str());
                RT_VALIDATE_VIRTUAL_HEIGHT(page, s)
                if ( _failed_ )
                    break;
            }
        }

        if ( !_failed_ )
        {
            wxArrayPGProperty arr2 = GetPropertiesInRandomOrder(page);

            for ( i=0; i<arr2.size(); i++ )
            {
                wxPGProperty* p = arr2[i];
                page->HideProperty(p, false);

                wxString s = wxString::Format(wxT("ShowProperty(%i, %s)"), i, p->GetLabel().c_str());
                RT_VALIDATE_VIRTUAL_HEIGHT(page, s)
                if ( _failed_ )
                    break;
            }
        }

        //
        // Let's do some more, for better consistency
        arr1 = GetPropertiesInRandomOrder(page);

        if ( !_failed_ )
        {
            for ( i=0; i<arr1.size(); i++ )
            {
                wxPGProperty* p = arr1[i];
                page->HideProperty(p, true);

                wxString s = wxString::Format(wxT("HideProperty(%i, %s)"), i, p->GetLabel().c_str());
                RT_VALIDATE_VIRTUAL_HEIGHT(page, s)
                if ( _failed_ )
                    break;
            }
        }

        if ( !_failed_ )
        {
            wxArrayPGProperty arr2 = GetPropertiesInRandomOrder(page);

            for ( i=0; i<arr2.size(); i++ )
            {
                wxPGProperty* p = arr2[i];
                page->HideProperty(p, false);

                wxString s = wxString::Format(wxT("ShowProperty(%i, %s)"), i, p->GetLabel().c_str());
                RT_VALIDATE_VIRTUAL_HEIGHT(page, s)
                if ( _failed_ )
                    break;
            }
        }

        //
        // Ok, this time only hide half of them
        arr1 = GetPropertiesInRandomOrder(page);
#if wxCHECK_VERSION(2,8,0)
        arr1.resize(arr1.size()/2);
#else
        arr1.SetCount(arr1.size()/2);
#endif

        if ( !_failed_ )
        {
            for ( i=0; i<arr1.size(); i++ )
            {
                wxPGProperty* p = arr1[i];
                page->HideProperty(p, true);

                wxString s = wxString::Format(wxT("HideProperty(%i, %s)"), i, p->GetLabel().c_str());
                RT_VALIDATE_VIRTUAL_HEIGHT(page, s)
                if ( _failed_ )
                    break;
            }
        }

        if ( !_failed_ )
        {
            wxArrayPGProperty arr2 = GetPropertiesInRandomOrder(page);

            for ( i=0; i<arr2.size(); i++ )
            {
                wxPGProperty* p = arr2[i];
                page->HideProperty(p, false);

                wxString s = wxString::Format(wxT("ShowProperty(%i, %s)"), i, p->GetLabel().c_str());
                RT_VALIDATE_VIRTUAL_HEIGHT(page, s)
                if ( _failed_ )
                    break;
            }
        }

        // Recreate the original grid
        CreateGrid( -1, -1 );
        pgman = m_pPropGridManager;
    }

    if ( fullTest )
    {
        RT_START_TEST(MultipleColumns)

        // Test with multiple columns
        // FIXME: Does not display changes.
        for ( i=3; i<12; i+=2 )
        {
            RT_MSG(wxString::Format(wxT("%i columns"),i));
            CreateGrid( -1, -1 );
            pgman = m_pPropGridManager;
            pgman->SetColumnCount(i);
            Refresh();
            Update();
            wxMilliSleep(500);
        }
    }

    if ( fullTest )
    {
        RT_START_TEST(WindowStyles)

        // Recreate grid with all possible (single) flags
        wxASSERT(wxPG_AUTO_SORT == 0x000000010);

        for ( i=4; i<16; i++ )
        {
            int flag = 1<<i;
            RT_MSG(wxString::Format(wxT("Style: 0x%X"),flag));
            CreateGrid( flag, -1 );
            pgman = m_pPropGridManager;
            Update();
            wxMilliSleep(500);
        }

        wxASSERT(wxPG_EX_INIT_NOCAT == 0x00001000);

        for ( i=12; i<24; i++ )
        {
            int flag = 1<<i;
            RT_MSG(wxString::Format(wxT("ExStyle: 0x%X"),flag));
            CreateGrid( -1, flag );
            pgman = m_pPropGridManager;
            Update();
            wxMilliSleep(500);
        }

        // Recreate the original grid
        CreateGrid( -1, -1 );
        pgman = m_pPropGridManager;
    }

    RT_START_TEST(none)

    bool retVal;

    if ( failures || errorMessages.size() )
    {
        retVal = false;

        wxString s;
#ifdef __WXDEBUG__
        if ( failures )
#endif
            s = wxString::Format(wxT("%i tests failed!!!"), failures);
#ifdef __WXDEBUG__
        else
            s = wxString::Format(wxT("All tests were succesfull, but there were %i warnings!"), wxPGGlobalVars->m_warnings);
#endif
        RT_MSG(s)
        for ( i=0; i<errorMessages.size(); i++ )
            RT_MSG(errorMessages[i])
        wxMessageBox(s, wxT("Some Tests Failed"));
    }
    else
    {
        RT_MSG(wxT("All tests succesfull"))
        retVal = true;

        if ( !interactive )
            dlg->Close();
    }

    pgman->SelectPage(0);

    // Test may screw up the toolbar, so we need to refresh it.
    wxToolBar* toolBar = pgman->GetToolBar();
    if ( toolBar )
        toolBar->Refresh();

    return retVal;
}

// -----------------------------------------------------------------------

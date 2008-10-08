%module wx_rosout
%include "std_string.i"

%{
#include "wx/wxPython/wxPython.h"
#include "wx/wxPython/pyclasses.h"
#include "rosout_panel.h"
%}

%include typemaps.i
%include my_typemaps.i

%import core.i
%import windows.i

%pythonAppend RosoutPanel "self._setOORInfo(self)"

%include rosout_generated.h
%include rosout_panel.h

%init %{

%}

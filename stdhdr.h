

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// macOS replaces straight quotes/dashes with typographic ones while typing,
// but Atlantis orders must stay plain ASCII.
inline void DisableSmartSubstitutions(wxTextCtrl * pTextCtrl)
{
#ifdef __WXOSX__
    pTextCtrl->OSXDisableAllSmartSubstitutions();
#else
    wxUnusedVar(pTextCtrl);
#endif
}


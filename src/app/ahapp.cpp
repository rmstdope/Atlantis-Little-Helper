/*
 * This source file is part of the Atlantis Little Helper program.
 * Copyright (C) 2001 Maxim Shariy.
 *
 * Atlantis Little Helper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atlantis Little Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atlantis Little Helper; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "stdhdr.h"

#include "wx/splitter.h"
#include "wx/listctrl.h"
//#include "wx/resource.h"

#include "string_utils.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "objs.h"
#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>

#include "ahapp.h"
#include "ahframe.h"
#include "mapframe.h"
#include "unitframe.h"
#include "unitframefltr.h"
#include "msgframe.h"
#include "editsframe.h"
#include "editpane.h"
#include "mappane.h"
#include "listpane.h"
#include "unitpane.h"
#include "utildlgs.h"
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "listcoledit.h"
#include "optionsdlg.h"
#include "flagsdlg.h"

#ifdef __WXMAC_OSX__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#endif

CAhApp * gpApp = nullptr; // Our own the one and only pointer

IMPLEMENT_APP(CAhApp);

//=========================================================================

CAhApp::CAhApp()
{
    m_pConfigManager = std::make_unique<ConfigManager>();
    gpConfigManager  = m_pConfigManager.get();

    m_pGameRules = std::make_unique<GameRules>();
    gpGameRules  = m_pGameRules.get();

    m_pGameData = std::make_unique<GameDataManager>();
    gpGameData  = m_pGameData.get();

    m_pUIController = std::make_unique<UIController>();
    gpUIController  = m_pUIController.get();

    m_pSelectionState = std::make_unique<SelectionState>();
    gpSelectionState  = m_pSelectionState.get();

    m_pReportLoader = std::make_unique<ReportLoader>();
    gpReportLoader  = m_pReportLoader.get();

    m_pReportGenerator = std::make_unique<ReportGenerator>();
    gpReportGenerator  = m_pReportGenerator.get();
}

CAhApp::~CAhApp()
{
    gpReportGenerator = nullptr;
    gpReportLoader = nullptr;
    gpSelectionState = nullptr;
    gpUIController = nullptr;
    gpGameData = nullptr;
    gpGameRules = nullptr;
    gpConfigManager = nullptr;
    gpApp = nullptr;
}

//-------------------------------------------------------------------------

bool CAhApp::OnInit()
{
    int               i;
    std::string S, S2;

    gpApp = this;

    gpConfigManager->Init();
    CUnit::LoadCustomFlagNames(gpConfigManager->GetConfigFile(SZ_SECT_UNIT_FLAG_NAMES));

    gpUIController->Init();

    if (0==stricmp(SZ_EOL_MS, gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_EOL)))
        EOL_FILE = EOL_MS;
    else
        EOL_FILE = EOL_UNIX;

    gpGameRules->Init();
    gpGameData->Init();
    gpReportLoader->Init();

    gpUIController->OpenMapFrame();

    if ((AH_LAYOUT_3_WIN==gpUIController->m_layout || AH_LAYOUT_2_WIN==gpUIController->m_layout) &&
        atol(gpConfigManager->GetConfig(CUnitFrame::GetConfigSection(gpUIController->m_layout), SZ_KEY_OPEN)) )
    {
        gpUIController->OpenUnitFrame();
    }

    if ((AH_LAYOUT_3_WIN==gpUIController->m_layout) &&
        (atol(gpConfigManager->GetConfig(CEditsFrame::GetConfigSection(gpUIController->m_layout), SZ_KEY_OPEN))) )
    {
        gpUIController->OpenEditsFrame();
    }

    SetTopWindow(gpUIController->m_Frames[AH_FRAME_MAP]);
    gpUIController->m_Frames[AH_FRAME_MAP]->SetFocus();


    if (argc>1)
        for (i=1; i<argc; i++)
            gpReportLoader->LoadReport(wxString(argv[i]).mb_str(), i>1);
    else
        if (atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_REP)) && (((int)gpGameData->m_ReportDates.size()) > 0) )
        {
            S.clear();
            S << gpGameData->m_ReportDates[(int)gpGameData->m_ReportDates.size()-1];
            S2 = gpConfigManager->GetConfig(SZ_SECT_REPORTS, S.c_str());
            const char * p = S2.c_str();
            bool         join = false;
            while (p && *p)
            {
                p = GetToken(S, p, ',');
                gpReportLoader->LoadReport(S.c_str(), join);
                join = true;
            }
        }
    if (atol(gpConfigManager->GetConfig(CUnitFrameFltr::GetConfigSection(gpUIController->m_layout), SZ_KEY_OPEN)) )
    {
        gpUIController->OpenUnitFrameFltr(false);
    }

    return true;
}

//-------------------------------------------------------------------------

int CAhApp::OnExit()
{
    CUnit::ResetCustomFlagNames();

    gpUIController->Shutdown();

    if (!gpUIController->m_DiscardChanges)
    {
        gpConfigManager->Save();

        if (gpGameData->m_pAtlantis && ERR_OK==gpGameData->m_pAtlantis->m_ParseErr)
            gpReportLoader->SaveHistory(SZ_HISTORY_FILE);
    }

    gpGameData->m_Reports.clear();
    gpGameData->m_pAtlantis.reset();

    gpReportLoader->StdRedirectDone();

    return 0;
}

//-------------------------------------------------------------------------------

void FontToStr(const wxFont * font, std::string & s)
{
    s.clear();
    s << (long)font->GetPointSize()  << ","
      << (long)font->GetFamily   ()  << ","
      << (long)font->GetStyle    ()  << ","
      << (long)font->GetWeight   ()  << ","
      << (long)font->GetEncoding ()  << ","
      <<       font->GetFaceName ().mb_str() ;
}

//--------------------------------------------------------------------------

#if defined(_WIN32)
   #define AH_DEFAULT_FONT_SIZE 10
#else
   #define AH_DEFAULT_FONT_SIZE 12
#endif


wxFont * NewFontFromStr(const char * p)
{
    int            size;
    wxFontFamily   family;
    wxFontStyle    style;
    wxFontWeight   weight;
    int            encoding;
    wxString       facename;
    wxFont     *   font;


    std::string           S;

    if (p && *p)
    {
        p = GetToken(S, SkipSpaces(p), ',');  size     = atol(S.c_str());
        p = GetToken(S, SkipSpaces(p), ',');  family   = static_cast<wxFontFamily>(atol(S.c_str()));
        p = GetToken(S, SkipSpaces(p), ',');  style    = static_cast<wxFontStyle>(atol(S.c_str()));
        p = GetToken(S, SkipSpaces(p), ',');  weight   = static_cast<wxFontWeight>(atol(S.c_str()));
        p = GetToken(S, SkipSpaces(p), ',');  encoding = atol(S.c_str());
                                             facename = wxString::FromAscii(SkipSpaces(p));
    }
    else
    {
        size     = AH_DEFAULT_FONT_SIZE;
        family   = wxFONTFAMILY_DEFAULT;
        style    = wxFONTSTYLE_NORMAL;
        weight   = wxFONTWEIGHT_NORMAL;
        encoding = wxFONTENCODING_SYSTEM;
        facename = wxT("");
    }

    font = new wxFont(size, family, style, weight, false, facename, (wxFontEncoding)encoding);

    return font;
}

//--------------------------------------------------------------------------

void StrToColor(wxColour * cr, const char * p)
{
    std::string          S;
    int           r, g, b;

    p = GetToken(S, p, ',');
    r = atol(S.c_str());

    p = GetToken(S, p, ',');
    g = atol(S.c_str());

    p = GetToken(S, p, ',');
    b = atol(S.c_str());

    cr->Set(r,g,b);
}

//--------------------------------------------------------------------------

void ColorToStr(char * p, size_t n, wxColour * cr)
{
    snprintf(p, n, "%d, %d, %d",
            (int)(cr->Red()  ),
            (int)(cr->Green()),
            (int)(cr->Blue() )
        );
}

//--------------------------------------------------------------------------

#if defined(__WXMSW__)
  #define EQUAL_PATH_CHARS(a, b) (tolower(a) == tolower(b))
#else
  #define EQUAL_PATH_CHARS(a, b) (a == b)
#endif

#if defined(__WXMSW__)
#define SEP '\\'
#else
#define SEP '/'
#endif


void MakePathRelative(const char * cur_dir, std::string & path)
{
    const char * p          = path.c_str();
    const char * cd         = cur_dir;
    const char * lastSep_p  = nullptr;
    const char * lastSep_cd = nullptr;
    std::string  rel_path;

    while (*p && EQUAL_PATH_CHARS(*p, *cd) )
    {
        if (*p == SEP)
        {
            lastSep_p  = p;
            lastSep_cd = cd;
        }
        p++;
        cd++;
    }

    // The match only lands on a genuine shared path component if it stopped
    // exactly at a separator (or end of string) on BOTH sides - e.g. cur_dir
    // fully consumed with path continuing at a fresh '/'. Otherwise it
    // stopped mid-segment - e.g. matching just the "A" shared by
    // "Atlantis..." and "ALH2..." - which is a coincidental character
    // collision, not a shared directory/file name, so roll back to the last
    // separator both sides actually agreed on.
    bool boundary_p  = (*p  == '\0' || *p  == SEP);
    bool boundary_cd = (*cd == '\0' || *cd == SEP);
    if (!(boundary_p && boundary_cd))
    {
        if (lastSep_p)
        {
            p  = lastSep_p  + 1;
            cd = lastSep_cd + 1;
        }
        else
        {
            p  = path.c_str();
            cd = cur_dir;
        }
    }

    if (*p==SEP)
        p++;
    else
        rel_path << ".." << SEP;

    while (*cd)
    {
        if (*cd == SEP)
            rel_path << ".." << SEP;

        cd++;
    }

    rel_path << p;

    if (path.size() > rel_path.size())
        path = rel_path;
}

//-------------------------------------------------------------------------

void MakePathFull(const char * cur_dir, std::string & path)
{
    std::string full_path;
    std::string rel_path;
    
    full_path = cur_dir;
    rel_path = path;

    if (!full_path.empty() && full_path.c_str()[full_path.size()-1] != SEP)
        AddCh(full_path,  SEP);

    if (!rel_path.empty())
    {
        if (rel_path.c_str()[0]=='.' && rel_path.c_str()[1]==SEP)
            DelSubStr(rel_path, 0,2);
    }
    
    path = full_path;
    path << rel_path;
}

//-------------------------------------------------------------------------

void GetDirFromPath(const char * path, std::string & dir)
{
    int n = 0;
    const char * p;

    if (!path || !*path)
        return;

    dir = path;
    p   = dir.c_str() + (dir.size()-1);
    while (*p!='\\' && *p!='/' && n<dir.size())
    {
        p--;
        n++;
    }
    if (*p=='\\' || *p=='/')
        n++;

    if (n>0)
        DelSubStr(dir, dir.size()-n, n);
    if (dir.empty())
        dir = ".";
}

//-------------------------------------------------------------------------

void GetFileFromPath(const char * path, std::string & file)
{
    const char * p = strrchr(path, SEP);
    
    file.clear();
    if (p && *p)
    {
        p++;
        file = p;
    }
    else
        file = path;
}

//-------------------------------------------------------------------------

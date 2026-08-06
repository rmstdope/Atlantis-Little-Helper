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
    m_FirstLoad         = true;
    m_OrdersAreChanged  = false;
    m_CommentsChanged   = false;
    m_DisableErrs       = false;

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

    m_nStdoutLastPos = 0;
    m_nStderrLastPos = 0;
}

CAhApp::~CAhApp()
{
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
    const char      * p;
    const char      * szName;
    const char      * szValue;
    std::string S, S2;
    int               sectidx;
    std::set<std::pair<std::string,std::string>> CollDedup;


    gpApp = this;

    gpConfigManager->Init();
    CUnit::LoadCustomFlagNames(gpConfigManager->GetConfigFile(SZ_SECT_UNIT_FLAG_NAMES));

    gpUIController->Init();

    if (0==stricmp(SZ_EOL_MS, gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_EOL)))
        EOL_FILE = EOL_MS;
    else
        EOL_FILE = EOL_UNIX;


    // Load unit property groups
    sectidx = gpConfigManager->GetSectionFirst(SZ_SECT_UNITPROP_GROUPS, szName, szValue);
    while (sectidx >= 0)
    {
        while (szValue && *szValue)
        {
            szValue = GetToken(S, szValue, ',');
            std::string key(szName), val(S.c_str());
            if (CollDedup.insert({key, val}).second)
                m_UnitPropertyGroups.emplace(key, val);
        }
        sectidx = gpConfigManager->GetSectionNext(sectidx, SZ_SECT_UNITPROP_GROUPS, szName, szValue);
    }
    CUnit::m_PropertyGroupsColl = &m_UnitPropertyGroups;


    // Property group name must not be an alias!
    {
        std::string lastKey;
        for (auto & kv : m_UnitPropertyGroups)
        {
            if (lastKey != kv.first)
            {
                lastKey = kv.first;
                p = gpGameRules->ResolveAlias(lastKey.c_str());
                if (0!=stricmp(lastKey.c_str(), p))
                {
                    S = "Group name \"";
                    S << lastKey.c_str() << "\" can be resolved as alias for \"" << p << "\"!\r\n";
                    gpUIController->ShowError(S.c_str(), S.size(), true);
                }
            }
        }
    }


    gpGameRules->Init();
    gpGameData->Init();

    StdRedirectInit();

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
            LoadReport(wxString(argv[i]).mb_str(), i>1);
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
                LoadReport(S.c_str(), join);
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
    int  i;
    std::string S;
    std::string Name;

    CUnit::ResetCustomFlagNames();

    gpUIController->Shutdown();

    if (!gpUIController->m_DiscardChanges)
    {
        gpConfigManager->Save();

        if (gpGameData->m_pAtlantis && ERR_OK==gpGameData->m_pAtlantis->m_ParseErr)
            SaveHistory(SZ_HISTORY_FILE);
    }

    gpGameData->m_Reports.clear();
    gpGameData->m_pAtlantis.reset();

    StdRedirectDone();

    return 0;
}

//-------------------------------------------------------------------------

void CAhApp::GetShortFactName(std::string & S, int FactionId)
{
#define MAX_F_NAME 8
    int           i;
    char          ch;
    CFaction    * pFaction;
//    CBaseObject   Dummy;
//    int           idx;

    S.clear();
//    Dummy.Id = FactionId;
//    if (gpGameData->m_pAtlantis->m_Factions.Search(&Dummy, idx))
//    {
//        pFaction = (CFaction*)gpGameData->m_pAtlantis->m_Factions.At(idx);
//        S = pFaction->Name;
//    }
//    else
//        S << (long)FactionId;
    pFaction = gpGameData->m_pAtlantis->GetFaction(FactionId);
    if (pFaction)
        S = pFaction->Name;
    else
        S << (long)FactionId;

    if (0==stricmp(S.c_str(), "faction"))
    {
        S.clear();
        S << "F_" << (long)FactionId << "_";
    }

    ToLower(S);
    for (i=S.size()-1; i>=0; i--)
    {
        ch = S.c_str()[i];
        if ( (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') )
            DelCh(S, i);
    }
    if (S.size() > MAX_F_NAME)
        DelSubStr(S, MAX_F_NAME, S.size()-MAX_F_NAME);
    TrimRight(S, TRIM_ALL);

}

//-------------------------------------------------------------------------

void CAhApp::SetOrdersChanged(bool Changed)
{
    m_OrdersAreChanged = Changed;

    if (!gpUIController->m_sTitle.empty())
        gpUIController->SetMapFrameTitle();
}


//-------------------------------------------------------------------------

int CAhApp::SaveOrders(bool UsingExistingName)
{
    std::string S, FName, Section;
    int  i, id, err=ERR_OK;

    for (i=0; i<((int)gpGameData->m_pAtlantis->m_OurFactions.size()); i++)
    {
        id = gpGameData->m_pAtlantis->m_OurFactions[i];
        if (UsingExistingName)
        {
            gpConfigManager->ComposeConfigOrdersSection(Section, id);
            S.clear();
            S << (long)gpGameData->m_pAtlantis->m_YearMon;
            FName = gpConfigManager->GetConfig(Section.c_str(), S.c_str());
            TrimRight(FName, TRIM_ALL);
        }
        err = SaveOrders(FName.c_str(), id);
        if (ERR_OK!=err)
            break;
    }

    if (ERR_OK==err)
        SetOrdersChanged(false);

    return err;
}

//-------------------------------------------------------------------------

int  CAhApp::SaveOrders(const char * FNameOut, int FactionId)
{

    int         err;
    char        buf[64];
    std::string        FName;
    std::string        Dir;
    std::string        S, Section, Prompt, Key;
    CFaction  * pFaction;

    FName = FNameOut;
    TrimRight(FName, TRIM_ALL);

    gpConfigManager->ComposeConfigOrdersSection(Section, FactionId);
    if (FName.empty())
    {
        Format(S, "%d", gpGameData->m_pAtlantis->m_YearMon);
        FName = gpConfigManager->GetConfig(Section.c_str(), S.c_str());
        TrimRight(FName, TRIM_ALL);

        if (FName.empty())
        {
            GetShortFactName(S, FactionId);
            if (S.empty())
                S << (long)FactionId;
            Format(FName, "%s%04d.ord", S.c_str(), gpGameData->m_pAtlantis->m_YearMon);
        }
        pFaction = gpGameData->m_pAtlantis->GetFaction(FactionId);

        Prompt = "Save orders for ";
        if (pFaction)
            Prompt << pFaction->Name.c_str() << " ";
        else
            Prompt << "Faction ";
        Prompt << (long)FactionId;

        Dir = gpConfigManager->GetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS);
        TrimRight(Dir, TRIM_ALL);
        if (Dir.empty())
            Dir = ".";

        std::string File;
        wxString CurrentDir = wxGetCwd();
        //MakePathFull(CurrentDir.mb_str(), FName);
        GetFileFromPath(FName.c_str(), File);

        MakePathFull(CurrentDir.mb_str(), Dir);
        wxFileDialog dialog((CMapFrame*)gpUIController->m_Frames[AH_FRAME_MAP],
                            wxString::FromAscii(Prompt.c_str()),
                            wxString::FromAscii(Dir.c_str()),
                            wxString::FromAscii(File.c_str()),
                            wxT(SZ_ORD_FILES),
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
        err = dialog.ShowModal();
        wxSetWorkingDirectory(CurrentDir);

        if (wxID_OK == err)
        {
            FName = dialog.GetPath().mb_str();
            MakePathRelative(CurrentDir.mb_str(), FName);
            GetDirFromPath(FName.c_str(), Dir);
            gpConfigManager->SetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS, Dir.c_str() );
        }
        else
            return ERR_CANCEL;

        TrimRight(FName, TRIM_ALL);
    }
    if (FName.empty())
        return ERR_FNAME;

    Key.clear();
    Key << (long)FactionId;

    err = gpGameData->m_pAtlantis->SaveOrders(FName.c_str(),
                                  gpConfigManager->GetConfig(SZ_SECT_PASSWORDS, Key.c_str()),
                                  (bool)atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_DECORATE_ORDERS)),
                                  FactionId
                                 );
    if (ERR_OK==err)
    {
        snprintf(buf, sizeof(buf), "%ld", gpGameData->m_pAtlantis->m_YearMon);
        gpConfigManager->SetConfig(Section.c_str(), buf, FName.c_str());
    }

    // Save config, too
    gpConfigManager->m_Config[CONFIG_FILE_CONFIG].Save(SZ_CONFIG_FILE);
    gpConfigManager->m_Config[CONFIG_FILE_STATE ].Save(SZ_CONFIG_STATE_FILE);

    if (ERR_OK==gpGameData->m_pAtlantis->m_ParseErr)
        SaveHistory(SZ_HISTORY_FILE);

    return err;
}

//-------------------------------------------------------------------------

int  CAhApp::LoadOrders  (const char * FNameIn)
{
    int           err;
    std::string S, FName, Sect;
    int           factid;
//    CMapPane    * pMapPane = (CMapPane* )m_Panes[AH_PANE_MAP];


    FName = FNameIn;  // FNameIn can be coming from config, so do not use it directly!
    err = gpGameData->m_pAtlantis->LoadOrders(FName.c_str(), factid);
    if (ERR_OK==err)
    {
        S.clear();
        S << (long)gpGameData->m_pAtlantis->m_YearMon;
        gpConfigManager->ComposeConfigOrdersSection(Sect, factid);
        gpConfigManager->SetConfig(Sect.c_str(), S.c_str(), FName.c_str());

//        if (pMapPane)
//            pMapPane->Refresh(false, nullptr);
//            pMapPane->CleanCities(); //pMapPane->Refresh(false, nullptr); // to remove pointers to land wich could be replaced by joining orders

        gpSelectionState->OnMapSelectionChange();
        gpSelectionState->RedrawTracks();
    }
    else
       if (gpUIController->m_Frames[AH_FRAME_MSG])
           ((CAhFrame*)gpUIController->m_Frames[AH_FRAME_MSG])->Raise();

    return err;
}

//-------------------------------------------------------------------------

void EncodeConfigLine(std::string & dest, const char * src)
{
    dest.clear();
    while (src && *src)
    {
        switch (*src)
        {
        case '\r':  break;
        case '\n':  AddStr(dest, "\\n", 2);
                    break;
        default  :  AddCh(dest, *src);
        }
        src++;
    }
}

//-------------------------------------------------------------------------

void DecodeConfigLine(std::string & dest, const char * src)
{
    bool          Esc;

    dest.clear();
    Esc = false;
    while (src && *src)
    {
        if ('\\' == *src)


            Esc = true;
        else
        {
            if (Esc)
            {
                switch(*src)
                {
                case 'n':
                    dest << EOL_SCR;
                    break;
                default:
                    AddCh(dest, '\\');
                    AddCh(dest, *src);
                }
            }
            else
                AddCh(dest, *src);
            Esc = false;
        }

        src++;
    }
}

//-------------------------------------------------------------------------

void CAhApp::LoadComments()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;

    for (i=0; i<gpGameData->m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(i);
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);

        DecodeConfigLine(pUnit->DefOrders, gpConfigManager->GetConfig(SZ_SECT_DEF_ORDERS, buf));

        TrimRight(pUnit->DefOrders, TRIM_ALL);
        pUnit->ExtractCommentsFromDefOrders();
    }
    m_CommentsChanged = false;
}

//-------------------------------------------------------------------------

void CAhApp::SaveComments()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;
    const char  * p;

    for (i=0; i<gpGameData->m_pAtlantis->m_Units.Count(); i++)
    {
        S.clear();
        pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(i);
        TrimRight(pUnit->DefOrders, TRIM_ALL);
        if (pUnit->DefOrders.size() > 0)
        {
            EncodeConfigLine(S, pUnit->DefOrders.c_str());
            p = S.c_str();
        }
        else
            p = nullptr;
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);
        gpConfigManager->SetConfig(SZ_SECT_DEF_ORDERS, buf, p);
    }
    m_CommentsChanged = false;
}


//-------------------------------------------------------------------------

void CAhApp::LoadUnitFlags()
{
    int           i, x;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;

    for (i=0; i<gpGameData->m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(i);
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);

        x = atol(gpConfigManager->GetConfig(SZ_SECT_UNIT_FLAGS, buf));
        if (x & UNIT_CUSTOM_FLAG_MASK)
        {
            pUnit->Flags    |= (x & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsOrg |= (x & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsLast = ~pUnit->Flags;
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::SaveUnitFlags()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;

    for (i=0; i<gpGameData->m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(i);
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);

        S.clear();
        if (pUnit->Flags & UNIT_CUSTOM_FLAG_MASK)
            S << (long)(pUnit->Flags & UNIT_CUSTOM_FLAG_MASK);
        gpConfigManager->SetConfig(SZ_SECT_UNIT_FLAGS, buf, S.c_str());
    }
}

//-------------------------------------------------------------------------

void CAhApp::SetAllLandUnitFlags()
{
    CUnitPane  * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
    CPlane     * pPlane;
    CLand      * pLand;
    CUnit      * pUnit;
    int          i, n, f, x;
    int          rc;

    CUnitFlagsDlg dlg(gpUIController->m_Frames[AH_FRAME_MAP], eAll, 0);

    rc = dlg.ShowModal();

    if ((ID_BTN_SET_ALL_LAND==rc || ID_BTN_RMV_ALL_LAND==rc) && dlg.m_LandFlags>0)
    {
        for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
            for (i=0; i<pPlane->Lands.Count(); i++)
            {
                pLand = (CLand*)pPlane->Lands.At(i);
                x     = 1;
                for (f=0; f<LAND_FLAG_COUNT; f++)
                {
                    if (dlg.m_LandFlags & x)
                    {
                        if (ID_BTN_RMV_ALL_LAND==rc)
                        {
                            // clear flag
                            pLand->FlagText[f].clear();
                        }
                        else
                        {
                            // set flag
                            if (pLand->FlagText[f].empty())
                                pLand->FlagText[f] = LandFlagLabel[f];
                        }
                        pLand->Flags |= LAND_HAS_FLAGS;
                    }
                    x <<= 1;
                }
            }
        }

        if (gpUIController->m_Panes[AH_PANE_MAP])
            (gpUIController->m_Panes[AH_PANE_MAP])->Refresh(false);
    }

    if ( (ID_BTN_SET_ALL_UNIT==rc || ID_BTN_RMV_ALL_UNIT==rc) && dlg.m_UnitFlags>0 )
    {
        for (i=0; i<gpGameData->m_pAtlantis->m_Units.Count(); i++)
        {
            pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(i);

            if (ID_BTN_SET_ALL_UNIT==rc)
            {
                pUnit->Flags    |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
                pUnit->FlagsOrg |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            }
            else
            {
                pUnit->Flags    &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
                pUnit->FlagsOrg &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            }

            pUnit->FlagsLast = ~pUnit->Flags;
        }
        if (pUnitPane)
            pUnitPane->Update(pUnitPane->m_pCurLand);
    }
}

//-------------------------------------------------------------------------

void CAhApp::SaveLandFlags()
{
    int          i, n, f;
    CPlane     * pPlane;
    CLand      * pLand;
    std::string         sName;
    std::string         sData;
    long         ym_last;
    long         ym_first;
    const char * p;

    for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            sData.clear();
            for (f=0; f<LAND_FLAG_COUNT; f++)
            {
                TrimRight(pLand->FlagText[f], TRIM_ALL);
                if (!pLand->FlagText[f].empty())
                {
                    if (!sData.empty())
                        sData << "\\n";
                    sData << (long)f << ":" << pLand->FlagText[f];
                }
            }
            gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sName);


            if (!sData.empty() || (pLand->Flags & LAND_HAS_FLAGS)) // allow to remove flags
                gpConfigManager->SetConfig(SZ_SECT_LAND_FLAGS, sName.c_str(), sData.c_str());

            if (pLand->Flags&LAND_IS_CURRENT) //LAND_UNITS)
            {
                //ym = atol(GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()));
                p        = GetToken(sData, gpConfigManager->GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()), ',');
                ym_last  = atol(sData.c_str());
                if (sData.empty())
                    ym_first = gpGameData->m_pAtlantis->m_YearMon;
                else
                {
                    p        = GetToken(sData, SkipSpaces(p), ',');
                    ym_first = atol(sData.c_str());
                }
                if (ym_last < gpGameData->m_pAtlantis->m_YearMon)
                {
                    sData.clear();
                    sData << gpGameData->m_pAtlantis->m_YearMon << "," << ym_first;
                    gpConfigManager->SetConfig(SZ_SECT_LAND_VISITED, sName.c_str(), sData.c_str());
                }
            }
        }
    }

//    m_LandFlagsChanged = false;
}

//-------------------------------------------------------------------------

void CAhApp::LoadLandFlags()
{
    int               sectidx, n;
    const char      * szName;
    const char      * szValue;
    const char      * p;
    const char      * line;
    std::string              sData, sLine, sN;
    CLand           * pLand;

    sectidx = gpConfigManager->GetSectionFirst(SZ_SECT_LAND_FLAGS, szName, szValue);
    while (sectidx >= 0)
    {
        pLand   = gpGameData->m_pAtlantis->GetLand(szName);
        if (pLand)
        {
            DecodeConfigLine(sData, szValue);

            line = sData.c_str();
            while (line && *line)
            {
                line = GetToken(sLine, line, '\n');
                p    = sLine.c_str();
                p    = GetToken(sN, p, ':');
                if (p)
                    n = atoi(sN.c_str());
                else
                {
                    p = sN.c_str();
                    n = 0;
                }
                if (n<0 || n>=LAND_FLAG_COUNT)
                    n = 0;
                pLand->FlagText[n] = p;
                pLand->Flags |= LAND_HAS_FLAGS;
            }

        }
        sectidx = gpConfigManager->GetSectionNext(sectidx, SZ_SECT_LAND_FLAGS, szName, szValue);
    }


}


//-------------------------------------------------------------------------

void CAhApp::UpdateEdgeStructs()
{
    int          i, n, k;
    int          d, adj_dir;
    int          x, y, z;
    int          adj_index;
    CPlane     * pPlane;
    CLand      * pLand, * adj_land;
    CStruct    * pEdge;

    for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            if(!pLand) continue;
            // set the Water-Type flag
            if(gpGameRules->IsWaterTerrain(ToLower(pLand->TerrainType)))
            {
                pLand->Flags |= LAND_IS_WATER;
            }
            for(d=0; d<6; d++)
            {
                adj_dir = (d%6)-3;
                if(adj_dir < 0) adj_dir += 6;
                LandIdToCoord(pLand->Id,x,y,z);
                switch (d%6)
                {
                    case North     : y -= 2;     break;
                    case Northeast : y--; x++;   break;
                    case Southeast : y++; x++;   break;
                    case South     : y += 2;     break;
                    case Southwest : y++; x--;   break;
                    case Northwest : y--; x--;   break;
                }
                CBaseObject Dummy;
                Dummy.Id = LandCoordToId(x, y, z);
                if (pPlane->Lands.Search(&Dummy, adj_index))
                {
                    adj_land = (CLand *) pPlane->Lands.At(adj_index);
                    if(adj_land)
                    {
                        if((pLand->Flags&LAND_IS_CURRENT) && !(adj_land->Flags&LAND_IS_CURRENT))
                        {  // set the corresponding Edge Structure in adjacent region
                            adj_land->RemoveEdgeStructs(adj_dir);
                            for(k=pLand->EdgeStructs.Count(); k>=0; k--)
                            {
                                pEdge = (CStruct*) pLand->EdgeStructs.At(k);
                                if((pEdge != nullptr) && (pEdge->Location == d))                          adj_land->AddNewEdgeStruct(pEdge->Kind.c_str(), adj_dir);
                            }
                        }
                        // set CoastBits
                        if(gpGameRules->IsWaterTerrain(ToLower(adj_land->TerrainType)))
                        {
                            if(!(pLand->Flags & LAND_IS_WATER))
                                adj_land->CoastBits |= ExitFlags[adj_dir];
                        }
                        else if(pLand->Flags&LAND_IS_WATER)
                        {
                            pLand->CoastBits |= ExitFlags[d];
                        }
                    }
                }
            }
        }
    }
}


//-------------------------------------------------------------------------

void CAhApp::WriteMagesCSV()
{
    std::string FName;
    std::string S;


//    GetShortFactName(S);
//    Format(FName, "%s_%s%04d.csv", S.c_str(), "mages", gpGameData->m_pAtlantis->m_YearMon);
    Format(FName, "%s%04d.csv", "mages", gpGameData->m_pAtlantis->m_YearMon);

    CExportMagesCSVDlg Dlg(gpUIController->m_Frames[AH_FRAME_MAP], FName.c_str());
    if (wxID_OK == Dlg.ShowModal())
        gpGameData->m_pAtlantis->WriteMagesCSV(Dlg.m_pFileName->GetValue().mb_str(),
                                   0==SafeCmp(Dlg.m_pOrientation->GetValue().mb_str(), SZ_VERTICAL),
                                   Dlg.m_pSeparator->GetValue().mb_str(),
                                   Dlg.m_nFormat
                                  );
}

//-------------------------------------------------------------------------

void CAhApp::CheckTaxDetails  (CLand  * pLand, CTaxProdDetailsCollByFaction & TaxDetails)
{
    int               x;
    CUnit           * pUnit;
//    long              tax = pLand->Taxable;
    EValueType        type;
    long              men;
    std::string              sCoord;
    CTaxProdDetails * pDetail;
    CTaxProdDetails   Dummy;
    int               idx;
    CTaxProdDetailsCollByFaction Factions;
    std::string OneLine;

    for (x=0; x<pLand->Units.Count(); x++)
    {
        pUnit = (CUnit*)pLand->Units.At(x);
        if (pUnit->Flags & UNIT_FLAG_TAXING)
        {
            Dummy.FactionId = pUnit->FactionId;
            if (TaxDetails.Search(&Dummy, idx))
                pDetail = (CTaxProdDetails*)TaxDetails.At(idx);
            else
            {
                pDetail = new CTaxProdDetails;
                pDetail->FactionId = pUnit->FactionId;
                TaxDetails.Insert(pDetail);
            }
            if (Factions.Insert(pDetail))
            {
                pDetail->amount = pLand->Taxable;
                pDetail->HexCount++;
            }
            if (pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) && eLong==type)
                pDetail->amount -= men*atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));
        }
    }


    // Output
    for (x=0; x<Factions.Count(); x++)
    {
        pDetail = (CTaxProdDetails*)Factions.At(x);
        OneLine.clear();

        gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
        OneLine << pLand->TerrainType << " (" << sCoord << ") ";
        while (OneLine.size() < 24)
            AddCh(OneLine, ' ');

        if (pDetail->amount > 0)
            OneLine << "is undertaxed by " << pDetail->amount << " silv" << EOL_SCR;
        else if (pDetail->amount<0)
            OneLine << "is overtaxed  by " << (-pDetail->amount) << " silv" << EOL_SCR;
        else
            OneLine << EOL_SCR;

        pDetail->Details << OneLine;
    }

    Factions.DeleteAll();
}

//-------------------------------------------------------------------------

void CAhApp::CheckTradeDetails(CLand  * pLand, CTaxProdDetailsCollByFaction & TradeDetails)
{
    int             x, k;
    CUnit         * pUnit;
    EValueType      type;
    long            men, lvl, tool, canproduce;
    std::string            sCoord, Skill;
    CProduct      * pProd;
//    long            amount;
    TProdDetails    details;
//    bool            working;
    CTaxProdDetails * pFactionInfo;
    CTaxProdDetails   Dummy;
    int               idx;
    CTaxProdDetailsCollByFaction Factions;
    CTaxProdDetailsCollByFaction AllFactions;
    std::string OneLine;

//    gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
//    Details << pLand->TerrainType << " (" << sCoord << "). ";

    for (k=0; k<pLand->Products.Count(); k++)
    {
        pProd = (CProduct*)pLand->Products.At(k);
        if (0==pProd->Amount)
            continue;
  //      amount = pProd->Amount;
        gpGameRules->GetProdDetails(pProd->ShortName.c_str(), details);
  //      working = false;
        Skill.clear();
        Skill << details.skillname << PRP_SKILL_POSTFIX;

        for (x=0; x<pLand->Units.Count(); x++)
        {
            pUnit = (CUnit*)pLand->Units.At(x);
            if (pUnit->Flags & UNIT_FLAG_PRODUCING) 
            {
                Dummy.FactionId = pUnit->FactionId;
                if (TradeDetails.Search(&Dummy, idx))
                    pFactionInfo = (CTaxProdDetails*)TradeDetails.At(idx);
                else
                {
                    pFactionInfo = new CTaxProdDetails;
                    pFactionInfo->FactionId = pUnit->FactionId;
                    TradeDetails.Insert(pFactionInfo);
                }
                if (Factions.Insert(pFactionInfo))
                    pFactionInfo->amount = pProd->Amount;
                if (AllFactions.Insert(pFactionInfo) )
                    pFactionInfo->HexCount++;

                if ( 0==stricmp(pUnit->ProducingItem.c_str(), pProd->ShortName.c_str()))
                {
                    if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) || eLong!=type)
                        continue;

                    // check skill level
                    if (!pUnit->GetProperty(Skill.c_str(), type, (const void *&)lvl, eNormal) || (eLong!=type) )
                        continue;

                    if (!details.toolname.empty())
                        if (!pUnit->GetProperty(details.toolname.c_str(), type, (const void *&)tool, eNormal) || eLong!=type )
                            tool = 0;
                    if (tool > men)
                        tool = men;

                    canproduce = (long)((((double)men)*lvl + tool*details.toolhelp) / details.months);
                    pFactionInfo->amount -= canproduce;
    //                working = true;
                }
            }
        }



/*        if (working)
            if (amount>0)
                Details << pProd->ShortName << " is underproduced by " << amount << ". ";
            else if (amount<0)
                Details << pProd->ShortName << " is overproduced by " << (-amount) << ". ";*/

        for (x=0; x<Factions.Count(); x++)
        {
            pFactionInfo = (CTaxProdDetails*)Factions.At(x);

            gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
            OneLine.clear();
            OneLine << pLand->TerrainType << " (" << sCoord << ") ";
            while (OneLine.size() < 24)
                AddCh(OneLine, ' ');
            OneLine << pProd->ShortName;

            while (OneLine.size() < 28)
                AddCh(OneLine, ' ');
            if (pFactionInfo->amount > 0)
                OneLine << " is underproduced by " << pFactionInfo->amount << ". " << EOL_SCR;
            else if (pFactionInfo->amount<0)
                OneLine << " is overproduced  by " << (-pFactionInfo->amount) << ". " << EOL_SCR;
            else
                OneLine << " exact amount produced " << EOL_SCR;

            pFactionInfo->Details << OneLine;
        }

        Factions.DeleteAll();
    }


    AllFactions.DeleteAll();
}

//-------------------------------------------------------------------------

void CAhApp::CheckTaxTrade()
{
    std::string sTax;
    std::string sTrade;
    std::string Report, S;
    std::string Details;
//    long                tax = 0;
//    long                trade = 0;
    int                 n, i;
    CLand             * pLand;
    CPlane            * pPlane;
    CTaxProdDetailsCollByFaction  Taxes;
    CTaxProdDetailsCollByFaction  Trades;
    CTaxProdDetails              *pDetails;

    for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);

            if (pLand->Flags & LAND_TAX_NEXT)
                CheckTaxDetails(pLand, Taxes);

            if (pLand->Flags & LAND_TRADE_NEXT)
                CheckTradeDetails(pLand, Trades);
        }
    }
    Report.clear();
    for (i=0; i<Taxes.Count(); i++)
    {
        pDetails = (CTaxProdDetails*)Taxes.At(i);
        Report << "Faction " << pDetails->FactionId << " : " << pDetails->HexCount << " TAX regions"   << EOL_SCR
               << pDetails->Details << EOL_SCR  << EOL_SCR;
    }
    for (i=0; i<Trades.Count(); i++)
    {
        pDetails = (CTaxProdDetails*)Trades.At(i);
        Report << "Faction " << pDetails->FactionId << " : " << pDetails->HexCount << " TRADE regions"   << EOL_SCR
               << pDetails->Details << EOL_SCR  << EOL_SCR;
    }

    Taxes.FreeAll();
    Trades.FreeAll();

    gpUIController->ShowError(Report.c_str()      , Report.size()      , true);
}

//-------------------------------------------------------------------------

void CAhApp::CheckProduction()
{
    int    n, i, x;
    CLand  * pLand;
    CPlane * pPlane;
    CUnit  * pUnit;
    std::string Error, S;

    for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            for (x=0; x<pLand->Units.Count(); x++)
            {
                pUnit = (CUnit*)pLand->Units.At(x);
                if (!gpGameData->m_pAtlantis->CheckResourcesForProduction(pUnit, pLand, S))
                    Error << "Unit " << pUnit->Id << " " << S << EOL_SCR;
            }
        }
    }

    S.clear();
    if (Error.empty())
        wxMessageBox(wxT("No problem with resources for production detected"));
    else
    {
        S << "The following problems were detected:" << EOL_SCR << EOL_SCR << Error;
        gpUIController->ShowError(S.c_str(), S.size(), true);
    }
}

//--------------------------------------------------------------------------

void CAhApp::CheckSailing()
{
    int    n, i, x;
    CLand  * pLand;
    CPlane * pPlane;
    CStruct* pStruct;
    std::string Error, S, sCoord;

    for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
            for (x=0; x<pLand->Structs.Count(); x++)
            {
                pStruct = (CStruct*)pLand->Structs.At(x);
                if ((pStruct->Attr & SA_MOBILE) && pStruct->SailingPower > 0)
                {
                    if (pStruct->Load > pStruct->MaxLoad)
                        Error << pLand->TerrainType << " (" << sCoord << ") - Ship " << pStruct->Id << " is overloaded by " << (pStruct->Load - pStruct->MaxLoad) << "." << EOL_SCR;
                    if (pStruct->SailingPower < pStruct->MinSailingPower)
                        Error << pLand->TerrainType << " (" << sCoord << ") - Ship " << pStruct->Id << " is underpowered by " << (pStruct->MinSailingPower - pStruct->SailingPower) << "." << EOL_SCR;
                }
            }
        }
    }

    S.clear();
    if (Error.empty())
        wxMessageBox(wxT("No problems with sailing detected"));
    else
    {
        S << "The following problems were detected:" << EOL_SCR << EOL_SCR << Error;
        gpUIController->ShowError(S.c_str(), S.size(), true);
    }
}

//--------------------------------------------------------------------------

#define SET_UNIT_PROP_NAME(_name, _type)                                 \
{                                                                        \
    gpGameData->m_pAtlantis->m_UnitPropertyNames.insert(_name);                      \
    gpGameData->m_pAtlantis->m_UnitPropertyTypes.emplace(_name, (int)(_type));       \
}

void CAhApp::PreLoadReport()
{
    std::string S, FName;

    SaveLandFlags();
    SaveUnitFlags();
    if (m_CommentsChanged)
        SaveComments();
    if (GetOrdersChanged())
        SaveOrders(true);

    if (ERR_OK==gpGameData->m_pAtlantis->m_ParseErr)
        SaveHistory(SZ_HISTORY_FILE);


}

//-------------------------------------------------------------------------

void CAhApp::PostLoadReport()
{
    std::string              S;
    CMapFrame       * pMapFrame  = (CMapFrame    *)gpUIController->m_Frames[AH_FRAME_MAP];
    CMapPane        * pMapPane   = (CMapPane     *)gpUIController->m_Panes [AH_PANE_MAP];
    CUnitPaneFltr   * pUnitPaneF = (CUnitPaneFltr*)gpUIController->m_Panes [AH_PANE_UNITS_FILTER];
    CUnitPane       * pUnitPane  = (CUnitPane    *)gpUIController->m_Panes [AH_PANE_UNITS_HEX];
    long              year, mon;
    const char      * szName;
    const char      * szValue;
    CUnit           * pUnit;
    CPlane          * pPlane;
    CShortNamedObj  * pItem;
    CFaction          DummyFaction;
    CFaction        * pFaction;
    int               i, n;


    // update edge structures
    UpdateEdgeStructs();

    SaveLandFlags();

    // count number of our men in every hex

    gpGameData->m_pAtlantis->CountMenForTheFaction(gpGameData->m_pAtlantis->m_CrntFactionId);

    if (pMapFrame)
    {
        gpUIController->m_sTitle.clear();

        for (i=0; i<((int)gpGameData->m_pAtlantis->m_OurFactions.size()); i++)
        {
            pFaction = gpGameData->m_pAtlantis->GetFaction(gpGameData->m_pAtlantis->m_OurFactions[i]);
            if (pFaction)
            {
                if (!gpUIController->m_sTitle.empty())
                    gpUIController->m_sTitle << ", ";
                if (((int)gpGameData->m_pAtlantis->m_OurFactions.size())<3)
                    gpUIController->m_sTitle << pFaction->Name << " ";
                gpUIController->m_sTitle << (long)pFaction->Id;
            }
        }
        year = (long)(gpGameData->m_pAtlantis->m_YearMon/100);
        mon  = gpGameData->m_pAtlantis->m_YearMon % 100 - 1;
        if ( (mon >= 0) && (mon < 12) )
            gpUIController->m_sTitle << ". " << Monthes[mon] << " year " << year;
        gpUIController->SetMapFrameTitle();
    }

    // if loaded for the very first time, center it
    if (gpConfigManager->GetSectionFirst(SZ_SECT_REPORTS, szName, szValue) < 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, tool_centerout);

        if (gpUIController->m_Panes[AH_PANE_MAP])
            ((CMapPane*)gpUIController->m_Panes[AH_PANE_MAP])->OnToolbarCmd(event);
    }



    // stnadard unit and base properties - that is likely to be forgotten when
    // new properties are introduced :((
    SET_UNIT_PROP_NAME(PRP_COMMENTS          , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_ORDERS            , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FACTION_ID        , eLong   )
    SET_UNIT_PROP_NAME(PRP_FACTION           , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_LAND_ID           , eLong   )
    SET_UNIT_PROP_NAME(PRP_ID                , eLong   )
    SET_UNIT_PROP_NAME(PRP_NAME              , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FULL_TEXT         , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_TEACHING          , eLong   )
    SET_UNIT_PROP_NAME(PRP_SEQUENCE          , eLong   )
    SET_UNIT_PROP_NAME(PRP_FRIEND_OR_FOE     , eLong   )
    SET_UNIT_PROP_NAME(PRP_WEIGHT            , eLong   )
    SET_UNIT_PROP_NAME(PRP_DESCRIPTION       , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_COMBAT            , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_MOVEMENT          , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_STANDARD    , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_CUSTOM      , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_CUSTOM_ABBR , eCharPtr)



    // If no orders loaded, no movement will be calculated. Force it.
    if (!gpGameData->m_pAtlantis->m_OrdersLoaded)
    {
        for (i=0; i<gpGameData->m_pAtlantis->m_Units.Count(); i++)
        {
            pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(i);
            pUnit->ResetNormalProperties();
        }

        for (n=0; n<gpGameData->m_pAtlantis->m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(n);
            for (i=0; i<pPlane->Lands.Count(); i++)
            {
                ((CLand*)pPlane->Lands.At(i))->CalcStructsLoad();
                ((CLand*)pPlane->Lands.At(i))->SetFlagsFromUnits(gpGameData->m_pAtlantis.get()); // maybe not needed here...
            }
        }
    }

    // skills
    for (i=0; i<gpGameData->m_pAtlantis->m_Skills.Count(); i++)
    {
        pItem = (CShortNamedObj*)gpGameData->m_pAtlantis->m_Skills.At(i);


        EncodeConfigLine(S, pItem->Description.c_str());
        gpConfigManager->SetConfig(SZ_SECT_SKILLS, pItem->Name.c_str(), S.c_str());
    }

    // Items
    for (i=0; i<gpGameData->m_pAtlantis->m_Items.Count(); i++)
    {
        pItem = (CShortNamedObj*)gpGameData->m_pAtlantis->m_Items.At(i);

        EncodeConfigLine(S, pItem->Description.c_str());
        gpConfigManager->SetConfig(SZ_SECT_ITEMS, pItem->Name.c_str(), S.c_str());
    }

    // Objects
    for (i=0; i<gpGameData->m_pAtlantis->m_Objects.Count(); i++)
    {
        pItem = (CShortNamedObj*)gpGameData->m_pAtlantis->m_Objects.At(i);

        EncodeConfigLine(S, pItem->Description.c_str());
        gpConfigManager->SetConfig(SZ_SECT_OBJECTS, pItem->Name.c_str(), S.c_str());
    }

    if (pMapPane)
        pMapPane->Refresh(false, nullptr);


    if (pUnitPane)
        pUnitPane->m_pCurLand = nullptr; // force the unit pane to do full update

    long savedUnitId = 0;
    // Restore the last selected unit in the selected hex
    if (pMapPane && gpGameData->m_pAtlantis)
    {
        std::string   sSection;
        sSection << "PLANE_" << pMapPane->m_SelPlane;
        const char * savedUnitStr = gpConfigManager->GetConfig(sSection.c_str(), SZ_KEY_UNIT_SEL);
        if (savedUnitStr && *savedUnitStr)
            savedUnitId = atol(savedUnitStr);
        if (savedUnitId)
        {
            CLand * pLand = gpGameData->m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);
            if (pLand)
                pLand->guiUnit = savedUnitId;
        }
    }

    gpSelectionState->OnMapSelectionChange();

    // if there were Hex Events, show them
    if (!gpGameData->m_pAtlantis->m_HexEvents.Description.empty())
    {
        CBaseColl   Coll;
        Coll.Insert(&gpGameData->m_pAtlantis->m_HexEvents);
        ShowDescriptionList(Coll, "Hex Events");
    }

    // show newly discovered products (advanced resources), if any
    if (gpGameData->m_pAtlantis->m_NewProducts.Count() > 0)
        ShowDescriptionList(gpGameData->m_pAtlantis->m_NewProducts, "New products");

    if (pUnitPaneF)
        pUnitPaneF->Update(nullptr);

    CheckRedirectedOutputFiles();
    
    if (!gpGameData->m_pAtlantis->m_SecurityEvents.Description.empty())
        gpGameData->m_pAtlantis->m_SecurityEvents.Description << EOL_SCR << EOL_SCR;
}

//-------------------------------------------------------------------------

int  CAhApp::LoadReport  (const char * FNameIn, bool Join)
{
    std::string S, Sect, S2;
    std::string FName;
    int  LoadOrd;
    long n;
    int  err = ERR_FOPEN;

    wxBeginBusyCursor();

    m_DisableErrs = true;

    if (FNameIn && *FNameIn)
    {
        FName = FNameIn;
        TrimRight(FName, TRIM_ALL);

        PreLoadReport();

        if (!m_FirstLoad && !Join)
        {
            auto cachedCurrent = std::move(gpGameData->m_pAtlantis);
            if (cachedCurrent)
            {
                const long cachedYear = cachedCurrent->m_YearMon;
                auto reportIt = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), cachedYear,
                    [](const std::unique_ptr<CAtlaParser>& report, long year)
                    {
                        return report->m_YearMon < year;
                    });
                if (reportIt != gpGameData->m_Reports.end() && (*reportIt)->m_YearMon == cachedYear)
                    reportIt = gpGameData->m_Reports.erase(reportIt);
                gpGameData->m_Reports.insert(reportIt, std::move(cachedCurrent));
            }
            gpGameData->m_pAtlantis.reset(new CAtlaParser(gpGameRules));
            gpGameData->m_pAtlantis->m_pConfig = &gpConfigManager->m_Config[CONFIG_FILE_CONFIG];
        }

        if (!Join)
        {
            gpGameData->m_pAtlantis->Clear();
            gpGameData->m_pAtlantis->ParseRep(SZ_HISTORY_FILE, false, true);
        }

        // Append unit group property names here so they are available while parsing
        for (auto & upg__ : m_UnitPropertyGroups)
            SET_UNIT_PROP_NAME(upg__.first.c_str(), eLong)

        err = gpGameData->m_pAtlantis->ParseRep(FName.c_str(), Join, false);
        switch (err)
        {
            case ERR_INV_TURN:
                wxMessageBox(wxT("Wrong turn in the report"), wxT("Error"));
                break;
        }
        SetOrdersChanged(false);
        m_CommentsChanged = false;
        if ( ERR_OK==err && gpGameData->m_pAtlantis->m_YearMon != 0 && gpGameData->m_pAtlantis->m_CrntFactionId != 0 )
        {
            {
                long _ym = gpGameData->m_pAtlantis->m_YearMon;
                auto _it = std::lower_bound(gpGameData->m_ReportDates.begin(), gpGameData->m_ReportDates.end(), _ym);
                if (_it == gpGameData->m_ReportDates.end() || *_it != _ym)
                    gpGameData->m_ReportDates.insert(_it, _ym);
            }
            gpConfigManager->UpgradeConfigByFactionId();

            if (atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_READ)) && !gpGameData->m_pAtlantis->m_CrntFactionPwd.empty())
            {
                S.clear();
                S << (long)gpGameData->m_pAtlantis->m_CrntFactionId;
                gpConfigManager->SetConfig(SZ_SECT_PASSWORDS, S.c_str(), gpGameData->m_pAtlantis->m_CrntFactionPwd.c_str() );
            }

            LoadOrd = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_ORDER));
            if (LoadOrd)
            {
                S.clear();
                S << (long)gpGameData->m_pAtlantis->m_YearMon;
                gpConfigManager->ComposeConfigOrdersSection(Sect, gpGameData->m_pAtlantis->m_CrntFactionId);
                LoadOrders(gpConfigManager->GetConfig(Sect.c_str(), S.c_str()));
            }
        }

        LoadComments();
        LoadLandFlags();
        LoadUnitFlags();
        PostLoadReport();

        if ( (ERR_OK==err) && (gpGameData->m_pAtlantis->m_YearMon != 0) )
        {
            // doing it after PostLoadReport() since it will check the section
            S.clear();
            S << (long)gpGameData->m_pAtlantis->m_YearMon;
            if (!Join)
                gpConfigManager->SetConfig(SZ_SECT_REPORTS, S.c_str(), FName.c_str());
            else
            {
                S2 = gpConfigManager->GetConfig(SZ_SECT_REPORTS, S.c_str());
                if (!S2.empty())
                    S2 << ", ";
                S2 << FName;
                gpConfigManager->SetConfig(SZ_SECT_REPORTS, S.c_str(), S2.c_str());
            }
        }

        if (!m_FirstLoad && !Join)
        {
            n = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_REP_CACHE_COUNT));
            if (n<=0)
                n = 1;

            const long maxCachedReports = n - 1;
            auto currentPos = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), gpGameData->m_pAtlantis->m_YearMon,
                [](const std::unique_ptr<CAtlaParser>& report, long year)
                {
                    return report->m_YearMon < year;
                });
            long currentInsertIdx = currentPos - gpGameData->m_Reports.begin();
            while ((long)gpGameData->m_Reports.size() > maxCachedReports)
            {
                if (currentInsertIdx > maxCachedReports/2)
                {
                    gpGameData->m_Reports.erase(gpGameData->m_Reports.begin());
                    if (currentInsertIdx > 0)
                        --currentInsertIdx;
                }
                else
                    gpGameData->m_Reports.pop_back();
            }
        }
        m_FirstLoad = false;
    }

    m_DisableErrs = false;

    wxEndBusyCursor();

    return err;
}

//-------------------------------------------------------------------------

int  CAhApp::LoadReport(bool Join)
{
    int rc;
    std::string Dir;
    const char * key;

    key = Join ? SZ_KEY_FOLDER_REP_JOIN : SZ_KEY_FOLDER_REP_LOAD;
    Dir = gpConfigManager->GetConfig(SZ_SECT_FOLDERS, key);
    if (Dir.empty())
        Dir = ".";

    wxString CurrentDir = wxGetCwd();
    wxFileDialog dialog(gpUIController->m_Frames[AH_FRAME_MAP],
                        wxT("Load Report"),
                        wxString::FromAscii(Dir.c_str()),
                        wxT(""),
                        wxT(SZ_REP_FILES),
                        wxFD_OPEN);
    rc = dialog.ShowModal();
    wxSetWorkingDirectory(CurrentDir);

    if (wxID_OK == rc)
    {
        std::string S;
        S = dialog.GetPath().mb_str();
        MakePathRelative(CurrentDir.mb_str(), S);

        GetDirFromPath(S.c_str(), Dir);
        gpConfigManager->SetConfig(SZ_SECT_FOLDERS, key, Dir.c_str() );

        return LoadReport(S.c_str(), Join);
    }
    else
        return ERR_CANCEL;

}

//-------------------------------------------------------------------------

void CAhApp::SwitchToYearMon(long YearMon)
{
    std::string          S, S2;

    PreLoadReport();
    if (GetOrdersChanged())
        return;

    auto reportIt = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), YearMon,
        [](const std::unique_ptr<CAtlaParser>& report, long year)
        {
            return report->m_YearMon < year;
        });
    if (reportIt != gpGameData->m_Reports.end() && (*reportIt)->m_YearMon == YearMon)
    {
        if (gpGameData->m_pAtlantis && gpGameData->m_pAtlantis->m_YearMon != YearMon)
        {
            auto cachedCurrent = std::move(gpGameData->m_pAtlantis);
            const long cachedYear = cachedCurrent->m_YearMon;
            auto cachedIt = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), cachedYear,
                [](const std::unique_ptr<CAtlaParser>& report, long year)
                {
                    return report->m_YearMon < year;
                });
            if (cachedIt != gpGameData->m_Reports.end() && (*cachedIt)->m_YearMon == cachedYear)
                cachedIt = gpGameData->m_Reports.erase(cachedIt);
            gpGameData->m_Reports.insert(cachedIt, std::move(cachedCurrent));
        }

        reportIt = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), YearMon,
            [](const std::unique_ptr<CAtlaParser>& report, long year)
            {
            return report->m_YearMon < year;
            });
        gpGameData->m_pAtlantis = std::move(*reportIt);
        gpGameData->m_Reports.erase(reportIt);
        if (gpGameData->m_pAtlantis) gpGameData->m_pAtlantis->m_pConfig = &gpConfigManager->m_Config[CONFIG_FILE_CONFIG];
        PostLoadReport();
    }
    else
    {
        S.clear();
        S << YearMon;

        S2 = gpConfigManager->GetConfig(SZ_SECT_REPORTS, S.c_str());
        const char * p = S2.c_str();
        bool         join = false;
        while (p && *p)
        {
            p = GetToken(S, p, ',');
            LoadReport(S.c_str(), join);
            join = true;
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::SwitchToRep(eRepSeq whichrep)
{
    int  i;

    m_DisableErrs = true;

    if (CanSwitchToRep(whichrep, i))
        SwitchToYearMon(gpGameData->m_ReportDates[i]);

    m_DisableErrs = false;
}

//-------------------------------------------------------------------------

bool CAhApp::CanSwitchToRep(eRepSeq whichrep, int & RepIdx)
{
    long       ym;
    std::string       sName, sData;
    CLand    * pLand;
    CMapPane * pMapPane;

    RepIdx=-1;

    switch(whichrep)
    {
    case repFirst:
        RepIdx = 0;
        break;

    case repLast:
        if (gpGameData->m_ReportDates.empty())
            RepIdx = -1;
        else if (gpGameData->m_pAtlantis->m_YearMon == gpGameData->m_ReportDates[(int)gpGameData->m_ReportDates.size()-1] )
            RepIdx = -1;
        else
            RepIdx = ((int)gpGameData->m_ReportDates.size())-1;
        break;

    case repPrev:
        {
            auto _it = std::lower_bound(gpGameData->m_ReportDates.begin(), gpGameData->m_ReportDates.end(), (long)gpGameData->m_pAtlantis->m_YearMon);
            if (_it != gpGameData->m_ReportDates.end() && *_it == gpGameData->m_pAtlantis->m_YearMon)
            {
                RepIdx = (int)(_it - gpGameData->m_ReportDates.begin());
                RepIdx--;
            }
        }
        break;

    case repNext:
        {
            auto _it = std::lower_bound(gpGameData->m_ReportDates.begin(), gpGameData->m_ReportDates.end(), (long)gpGameData->m_pAtlantis->m_YearMon);
            if (_it != gpGameData->m_ReportDates.end() && *_it == gpGameData->m_pAtlantis->m_YearMon)
            {
                RepIdx = (int)(_it - gpGameData->m_ReportDates.begin());
                RepIdx++;
            }
        }
        break;

    case repLastVisited:
        pMapPane = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
        pLand    = gpGameData->m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);
        gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sName);
//        ym       = atol(GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()));
        GetToken(sData, gpConfigManager->GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()), ',');
        ym = atol(sData.c_str());

        {
            auto _it = std::lower_bound(gpGameData->m_ReportDates.begin(), gpGameData->m_ReportDates.end(), ym);
            bool _found = _it != gpGameData->m_ReportDates.end() && *_it == ym;
            RepIdx = _found ? (int)(_it - gpGameData->m_ReportDates.begin()) : -1;
            if (ym==gpGameData->m_pAtlantis->m_YearMon || !_found)
                RepIdx = -1;
        }
        break;
    }

    return (RepIdx>=0 && RepIdx<((int)gpGameData->m_ReportDates.size()));
}

//-------------------------------------------------------------------------

bool CAhApp::GetPrevTurnReport(CAtlaParser *& pPrevTurn)
{
    int idx;
    
    pPrevTurn = nullptr;
        
    if (CanSwitchToRep(repPrev, idx))
    {
        std::string          S, S2;
    
        long YearMon = gpGameData->m_ReportDates[idx];
    
        auto reportIt = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), YearMon,
            [](const std::unique_ptr<CAtlaParser>& report, long year)
            {
                return report->m_YearMon < year;
            });
        if (reportIt != gpGameData->m_Reports.end() && (*reportIt)->m_YearMon == YearMon)
        {
            pPrevTurn = reportIt->get();
        }
        else
        {
            S.clear();
            S << YearMon;
    
            S2 = gpConfigManager->GetConfig(SZ_SECT_REPORTS, S.c_str());
            const char * p = S2.c_str();
            bool         join = false;
            m_DisableErrs = true;
            wxBeginBusyCursor();
            std::unique_ptr<CAtlaParser> prevTurn(new CAtlaParser(gpGameRules));
            prevTurn->ParseRep(SZ_HISTORY_FILE, false, true);
            while (p && *p)
            {
                p = GetToken(S, p, ',');
                //LoadReport(S.c_str(), join);
                prevTurn->ParseRep(S.c_str(), join, false);
                join = true;
            }
            wxEndBusyCursor();
            m_DisableErrs = false;
            if (prevTurn->m_YearMon == YearMon)
            {
                pPrevTurn = prevTurn.get();
                auto insertIt = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), YearMon,
                    [](const std::unique_ptr<CAtlaParser>& report, long year)
                    {
                        return report->m_YearMon < year;
                    });
                if (insertIt != gpGameData->m_Reports.end() && (*insertIt)->m_YearMon == YearMon)
                    insertIt = gpGameData->m_Reports.erase(insertIt);
                gpGameData->m_Reports.insert(insertIt, std::move(prevTurn));
            }
            else
                pPrevTurn = nullptr;
        }
    }

    return (pPrevTurn != nullptr);
}

/*
    std::string S, Sect, S2;
    std::string FName;
    int  LoadOrd;
    int  i;
    long n;
    int  err = ERR_FOPEN;

    wxBeginBusyCursor();

    m_DisableErrs = true;

    if (FNameIn && *FNameIn)
    {
        FName = FNameIn;
        TrimRight(FName, TRIM_ALL);

        PreLoadReport();

        if (!m_FirstLoad && !Join)
            gpGameData->m_pAtlantis = new CAtlaParser(&ThisGameDataHelper);
            gpGameData->m_pAtlantis->m_pConfig = &gpConfigManager->m_Config[CONFIG_FILE_CONFIG];

        if (!Join)
        {
            gpGameData->m_pAtlantis->Clear();
            gpGameData->m_pAtlantis->ParseRep(SZ_HISTORY_FILE, false, true);
        }

        // Append unit group property names here so they are available while parsing
        for (auto & upg__ : m_UnitPropertyGroups)
            SET_UNIT_PROP_NAME(upg__.first.c_str(), eLong)


        err = gpGameData->m_pAtlantis->ParseRep(FName.c_str(), Join, false);
        switch (err)
        {
            case ERR_INV_TURN:
                wxMessageBox("Wrong turn in the report", "Error");
                break;
        }
        SetOrdersChanged(false);
        m_CommentsChanged = false;
        if ( ERR_OK==err && gpGameData->m_pAtlantis->m_YearMon != 0 && gpGameData->m_pAtlantis->m_CrntFactionId != 0 )
        {
            {
                long _ym = gpGameData->m_pAtlantis->m_YearMon;
                auto _it = std::lower_bound(gpGameData->m_ReportDates.begin(), gpGameData->m_ReportDates.end(), _ym);
                if (_it == gpGameData->m_ReportDates.end() || *_it != _ym)
                    gpGameData->m_ReportDates.insert(_it, _ym);
            }
            gpConfigManager->UpgradeConfigByFactionId();

            if (atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_READ)) && !gpGameData->m_pAtlantis->m_CrntFactionPwd.empty())
            {
                S.clear();
                S << (long)gpGameData->m_pAtlantis->m_CrntFactionId;
                gpConfigManager->SetConfig(SZ_SECT_PASSWORDS, S.c_str(), gpGameData->m_pAtlantis->m_CrntFactionPwd.c_str() );
            }

            LoadOrd = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_ORDER));
            if (LoadOrd)
            {
                S.clear();
                S << (long)gpGameData->m_pAtlantis->m_YearMon;
                gpConfigManager->ComposeConfigOrdersSection(Sect, gpGameData->m_pAtlantis->m_CrntFactionId);
                LoadOrders(gpConfigManager->GetConfig(Sect.c_str(), S.c_str()));
            }
        }

        LoadComments();
        LoadLandFlags();
        LoadUnitFlags();
        PostLoadReport();

        if ( (ERR_OK==err) && (gpGameData->m_pAtlantis->m_YearMon != 0) )
        {
            // doing it after PostLoadReport() since it will check the section
            S.clear();
            S << (long)gpGameData->m_pAtlantis->m_YearMon;
            if (!Join)
                gpConfigManager->SetConfig(SZ_SECT_REPORTS, S.c_str(), FName.c_str());
            else
            {
                S2 = gpConfigManager->GetConfig(SZ_SECT_REPORTS, S.c_str());
                if (!S2.empty())
                    S2 << ", ";
                S2 << FName;
                gpConfigManager->SetConfig(SZ_SECT_REPORTS, S.c_str(), S2.c_str());
            }
        }

        if (!m_FirstLoad && !Join)
        {
            if (([&]() -> bool { auto _ri = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), gpGameData->m_pAtlantis, [](CAtlaParser* a, CAtlaParser* b){ return a->m_YearMon < b->m_YearMon; }); if (_ri != gpGameData->m_Reports.end() && (*_ri)->m_YearMon == (gpGameData->m_pAtlantis)->m_YearMon) { i = (int)(_ri - gpGameData->m_Reports.begin()); return true; } return false; })())
                { delete gpGameData->m_Reports[i]; gpGameData->m_Reports.erase(gpGameData->m_Reports.begin() + (i)); }
            { auto _ri = std::lower_bound(gpGameData->m_Reports.begin(), gpGameData->m_Reports.end(), gpGameData->m_pAtlantis, [](CAtlaParser* a, CAtlaParser* b){ return a->m_YearMon < b->m_YearMon; }); gpGameData->m_Reports.insert(_ri, gpGameData->m_pAtlantis); }

            n = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_REP_CACHE_COUNT));
            if (n<=0)
                n = 1;
            if ((int)gpGameData->m_Reports.size()>n)
            {
                if (i > n/2)
                    n = 0;
                else
                    n = (int)gpGameData->m_Reports.size()-1;
                if (gpGameData->m_pAtlantis != gpGameData->m_Reports[n])
                    { delete gpGameData->m_Reports[n]; gpGameData->m_Reports.erase(gpGameData->m_Reports.begin() + (n)); }
            }
        }
        m_FirstLoad = false;
    }

    m_DisableErrs = false;

    wxEndBusyCursor();
*/

//-------------------------------------------------------------------------

void CAhApp::LoadOrders()
{
    int rc;
    std::string Dir;

    Dir = gpConfigManager->GetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS);
    if (Dir.empty())
        Dir = ".";

    wxString CurrentDir = wxGetCwd();
    wxFileDialog dialog(gpUIController->m_Frames[AH_FRAME_MAP],
                        wxT("Load orders"),
                        wxString::FromAscii(Dir.c_str()),
                        wxT(""),
                        wxT(SZ_ORD_FILES),
                        wxFD_OPEN );
    rc = dialog.ShowModal();
    wxSetWorkingDirectory(CurrentDir);

    if (wxID_OK==rc)
    {
        std::string S;
        S = dialog.GetPath().mb_str();
        MakePathRelative(CurrentDir.mb_str(), S);
        GetDirFromPath(S.c_str(), Dir);
        gpConfigManager->SetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS, Dir.c_str() );

        LoadOrders(S.c_str());
        SetOrdersChanged(false);
    }
}

//-------------------------------------------------------------------------


void CAhApp::ShowDescriptionList(CBaseColl & Items, const char * title) // Collection of CBaseObject
{
    CBaseObject  * pObj;

    if (Items.Count() > 0)
    {
        if (1 == Items.Count())
        {
            pObj = (CBaseObject*)Items.At(0);
            CShowOneDescriptionDlg dlg(gpUIController->m_Frames[AH_FRAME_MAP], pObj->Name.c_str(), pObj->Description.c_str());
            dlg.ShowModal();
        }
        else
        {
            CShowDescriptionListDlg dlg(gpUIController->m_Frames[AH_FRAME_MAP], title, &Items);
            dlg.ShowModal();
        }
    }

}

void CAhApp::ShowDescriptionList(CBaseCollById & Items, const char * title)
{
    // Copy into a CBaseColl for display
    CBaseColl tmp;
    for (int i = 0; i < Items.Count(); i++)
        tmp.Insert(Items.At(i));
    ShowDescriptionList(tmp, title);
    tmp.DeleteAll(); // don't free, we don't own these items
}

//--------------------------------------------------------------------------
/*
void CAhApp::ViewSkills(bool ViewAll)
{
    CBaseColl     Skills;
    CBaseObject * pSkill;
    const char  * szName;
    const char  * szValue;
    int           sectidx;

    if (ViewAll)
    {
        sectidx = gpConfigManager->GetSectionFirst(SZ_SECT_SKILLS, szName, szValue);
        while (sectidx >= 0)
        {
            pSkill              = new CBaseObject;
            pSkill->Name        = szName;
            DecodeConfigLine(pSkill->Description, szValue);
            Skills.Insert(pSkill);

            sectidx = gpConfigManager->GetSectionNext(sectidx, SZ_SECT_SKILLS, szName, szValue);
        }

        ShowDescriptionList(Skills, "Skills");
        Skills.FreeAll();
    }
    else
        ShowDescriptionList(gpGameData->m_pAtlantis->m_Skills, "Skills");
}
*/
//--------------------------------------------------------------------------

void CAhApp::ViewShortNamedObjects(bool ViewAll, const char * szSection, const char * szHeader, CBaseColl & ListNew)
{
    CBaseColl     Items;
    CBaseObject * pItem;
    const char  * szName;
    const char  * szValue;
    int           sectidx;

    if (ViewAll)
    {
        sectidx = gpConfigManager->GetSectionFirst(szSection, szName, szValue);
        while (sectidx >= 0)
        {
            pItem              = new CBaseObject;
            pItem->Name        = szName;
            DecodeConfigLine(pItem->Description, szValue);
            Items.Insert(pItem);

            sectidx = gpConfigManager->GetSectionNext(sectidx, szSection, szName, szValue);
        }

        ShowDescriptionList(Items, szHeader);
        Items.FreeAll();
    }
    else
        ShowDescriptionList(ListNew, szHeader);
}

//--------------------------------------------------------------------------

void CAhApp::ViewEvents(bool DoEvents)
{
    CBaseColl   Coll;

    if (DoEvents)
    {
        Coll.Insert(&gpGameData->m_pAtlantis->m_Events);
        ShowDescriptionList(Coll, "Events");
    }
    else
    {
//        Coll.Insert(&gpGameData->m_pAtlantis->m_Errors);
//        ShowDescriptionList(Coll, "Errors");
        gpSelectionState->m_MsgSrc.clear();
        gpUIController->ShowError(gpGameData->m_pAtlantis->m_Errors.Description.c_str(), gpGameData->m_pAtlantis->m_Errors.Description.size(), true);

    }
    Coll.DeleteAll();
}

//--------------------------------------------------------------------------

void CAhApp::ViewSecurityEvents()
{
/*    CBaseColl   Coll;

    Coll.Insert(&gpGameData->m_pAtlantis->m_SecurityEvents);
    ShowDescriptionList(Coll, "Security Events");

    Coll.DeleteAll();*/
    
        gpSelectionState->m_MsgSrc.clear();
        gpUIController->ShowError(gpGameData->m_pAtlantis->m_SecurityEvents.Description.c_str(), gpGameData->m_pAtlantis->m_SecurityEvents.Description.size(), true);
}

//--------------------------------------------------------------------------

void CAhApp::ViewNewProducts()
{
    ShowDescriptionList(gpGameData->m_pAtlantis->m_NewProducts, "New products");
}

//--------------------------------------------------------------------------

void CAhApp::ViewBattlesAll()
{
    ShowDescriptionList(gpGameData->m_pAtlantis->m_Battles, "Battles");
}

//--------------------------------------------------------------------------

void CAhApp::ViewGates()
{
    ShowDescriptionList(gpGameData->m_pAtlantis->m_Gates, "Gates");
}

//--------------------------------------------------------------------------

void CAhApp::ViewCities()
{
    CBaseCollByName    coll;
    int                np,nl;
    CPlane           * pPlane;
    CLand            * pLand;
    //int                x,y,z;
    std::string               sCoord;

    for (np=0; np<gpGameData->m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            if (!pLand->CityName.empty())
            {
                std::unique_ptr<CBaseObject> pObj(new CBaseObject);
                pObj->Name = pLand->CityName;

                //LandIdToCoord(pLand->Id, x, y, z);
                gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
                pObj->Description << pLand->TerrainType << " (" << sCoord << ") in " << pLand->Name;
                pObj->Description << ", contains " << pLand->CityName << " [" << pLand->CityType << "]";

                if (coll.Insert(pObj.get()))
                    pObj.release();
            }
        }
    }

    ShowDescriptionList(coll, "Cities");
    coll.FreeAll();
}

//--------------------------------------------------------------------------

void CAhApp::ViewProvinces()
{
    CBaseCollByName    coll;
    int                np,nl;
    CPlane           * pPlane;
    CLand            * pLand;
    std::string               sCoord;
    int                loop;

    for (loop=0; loop<2; loop++)
    {
        for (np=0; np<gpGameData->m_pAtlantis->m_Planes.Count(); np++)
        {
            pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(np);
            for (nl=0; nl<pPlane->Lands.Count(); nl++)
            {
                pLand      = (CLand*)pPlane->Lands.At(nl);
                if ((pLand->Flags&LAND_VISITED) || 1==loop) // we run it twice, so we pick visited hexes if we can
                {
                    std::unique_ptr<CBaseObject> pObj(new CBaseObject);
                    pObj->Name = pLand->Name;

                    gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
                    pObj->Description << pLand->TerrainType << " (" << sCoord << ") in " << pLand->Name;

                    if (coll.Insert(pObj.get()))
                        pObj.release();
                }
            }
        }
    }

    ShowDescriptionList(coll, "Provinces");
    coll.FreeAll();
}

//--------------------------------------------------------------------------

void CAhApp::ViewFactionInfo()
{
    std::string sMoreInfo, sInfo;
    int                np,nl;
    CPlane           * pPlane;
    CLand            * pLand;
    long               nLandsTotal = 0, nLandsVisited=0;

    sMoreInfo << EOL_SCR << "-------------------------" << EOL_SCR;
    for (np=0; np<gpGameData->m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            nLandsTotal++;
            if (pLand->Flags&LAND_VISITED)
                nLandsVisited++;
        }
    }
    sMoreInfo << "Total hexes  : " << nLandsTotal   << EOL_SCR
              << "Visited hexes: " << nLandsVisited << EOL_SCR ;


    sInfo << gpGameData->m_pAtlantis->m_FactionInfo << sMoreInfo;
    CShowOneDescriptionDlg dlg(gpUIController->m_Frames[AH_FRAME_MAP],
                               "Faction Info",
                               sInfo.c_str());
    dlg.ShowModal();
}

//--------------------------------------------------------------------------

void CAhApp::ViewFactionOverview_IncrementValue(long FactionId, const char * factionname, CBaseCollById & Factions, const char * propname, long value)
{
    CBaseObject   * pFaction;
    CBaseObject     Dummy;
    int             idx;
    EValueType      type;
    const void    * valuetot;
    
    Dummy.Id = FactionId;
    if (Factions.Search(&Dummy, idx))
        pFaction = (CBaseObject*)Factions.At(idx);
    else
    {
        pFaction       = new CBaseObject;
        pFaction->Id   = FactionId;
        if (factionname)
            pFaction->Name = factionname;
        Factions.Insert(pFaction);
    }

    if (!pFaction->GetProperty(propname, type, valuetot, eNormal))
        valuetot = (void*)0;

    if (-1==(long)valuetot || 0x7fffffff - (long)value < (long)valuetot )
        valuetot = (void*)(long)-1; // overflow protection
    else
        valuetot = (void*)((long)valuetot + (long)value);
    pFaction->SetProperty(propname, eLong, valuetot, eNormal);
}                    

//--------------------------------------------------------------------------

void CAhApp::ViewFactionOverview()
{
//m_UnitPropertyNames

    int             unitidx, propidx, nl;
    CUnit         * pUnit;
    std::string            propname;
    std::string            Skill;
    int             skilllen;
    int             maxproplen = 0;
    std::string Report;
    CMapPane      * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    bool            Selected  = false;
    EValueType      type;
    const void    * value;
    int             idx;
    CBaseObject   * pFaction;
    long            men;

    CBaseColl       Hexes(64);
    CBaseCollById   Factions(16);
    CLand         * pLand;

    if (!pMapPane->HaveSelection())
        ShowMessageBoxSwitchable("Hint", "Faction overview can be generated using only selected area on the map", "FACTION_OVERVIEW");

    if (pMapPane->HaveSelection() &&
        wxYES == wxMessageBox(wxT("Use only selected hexes?"), wxT("Confirm"), wxYES_NO, nullptr))
        Selected = true;

    skilllen    = strlen(PRP_SKILL_POSTFIX);

    // collect data
    pMapPane->GetSelectedOrAllHexes(Hexes, Selected);
    for (nl=0; nl<Hexes.Count(); nl++)
    {
        pLand = (CLand*)Hexes.At(nl);
        for (unitidx=0; unitidx<pLand->Units.Count(); unitidx++)
        {
            pUnit    = (CUnit*)pLand->Units.At(unitidx);
            men      = 0;
            if (pUnit->GetProperty(PRP_MEN, type, value, eOriginal) && (eLong==type) )
                men = (long)value;

            for (const auto& propnameStr : gpGameData->m_pAtlantis->m_UnitPropertyNames)
            {
                propname = propnameStr.c_str();

                // skip 'skill days' property
                if (IsASkillRelatedProperty(propname.c_str()) &&
                     FindSubStrR(propname, PRP_SKILL_POSTFIX) != propname.size()-skilllen)
                    continue;

                // skip some properties which can not be aggegated
                if (0==stricmp(propname.c_str(), PRP_ID        ) ||
                    0==stricmp(propname.c_str(), PRP_FACTION_ID) ||
                    0==stricmp(propname.c_str(), PRP_LAND_ID   ) ||
                    0==stricmp(propname.c_str(), PRP_STRUCT_ID ) ||
                    0==stricmp(propname.c_str(), PRP_TEACHING  ) ||
                    0==stricmp(propname.c_str(), PRP_SKILLS    ) ||
                    0==stricmp(propname.c_str(), PRP_MAG_SKILLS) ||
                    0==stricmp(propname.c_str(), PRP_SEQUENCE  ) ||
                    0==stricmp(propname.c_str(), PRP_FRIEND_OR_FOE  )


                   )
                    continue;

                if (pUnit->GetProperty(propname.c_str(), type, value, eOriginal) &&
                    (eLong==type) )
                    do
                    {
                        if (FindSubStrR(propname, PRP_SKILL_POSTFIX) == propname.size()-skilllen)
                        {
                                // it is a skill
    
                            propname << (long)value;
                            value    = (void*)men;
                        }
                        else 
                            if (IsASkillRelatedProperty(propname.c_str()))
                                break;
    
                        if (propname.size() > maxproplen)
                            maxproplen = propname.size();
    
                        ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.c_str() : nullptr, Factions, propname.c_str(), (long)value);
                        
                    } while (false);

            }

            if (pUnit->Flags & UNIT_FLAG_AVOIDING)
                ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.c_str() : nullptr, Factions, "Avoiding", men);
            else
            {
                if (pUnit->Flags & UNIT_FLAG_BEHIND)
                    ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.c_str() : nullptr, Factions, "Back Line", men);
                else
                    ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.c_str() : nullptr, Factions, "Front Line", men);
            }
            

            /*
            propidx  = 0;
            propname = pUnit->GetPropertyName(propidx);
            while (!propname.empty())
            {
                if (pUnit->GetProperty(propname.c_str(), type, value, eOriginal) &&
                    (eLong==type) )
                    do
                    {
                        if (FindSubStrR(propname, PRP_SKILL_POSTFIX) == propname.size()-skilllen)
                        {
                            // it is a skill

                            propname << (long)value;
                            if (!pUnit->GetProperty(PRP_MEN, type, value, eOriginal) &&
                                (eLong==type) )
                                break;
                        }
                        else if (IsASkillRelatedProperty(propname.c_str()) ||
                                 0==stricmp(PRP_SEQUENCE, propname.c_str()) ||
                                 0==stricmp(PRP_STRUCT_ID, propname.c_str()) )
                            break;

                        if (propname.size() > maxproplen)
                            maxproplen = propname.size();

                        Dummy.Id = pUnit->FactionId;
                        if (Factions.Search(&Dummy, idx))
                            pFaction = (CBaseObject*)Factions.At(idx);
                        else
                        {
                            pFaction       = new CBaseObject;
                            pFaction->Id   = pUnit->FactionId;
                            if (pUnit->pFaction)
                                pFaction->Name = pUnit->pFaction->Name;
                            Factions.Insert(pFaction);
                        }

                        if (!pFaction->GetProperty(propname.c_str(), type, valuetot, eNormal))
                            valuetot = (void*)0;

                        valuetot = (void*)((long)valuetot + (long)value);
                        pFaction->SetProperty(propname.c_str(), eLong, valuetot, eNormal);
                    } while (false);

                propname = pUnit->GetPropertyName(++propidx);
            }
            */
        }
    }
    Hexes.DeleteAll();

    // prepare display

    for (idx=0; idx<Factions.Count(); idx++)
    {
        pFaction = (CBaseObject*)Factions.At(idx);
        Report << "Faction " << pFaction->Id << " " << pFaction->Name << EOL_SCR << EOL_SCR;


        propidx  = 0;
        propname = pFaction->GetPropertyName(propidx);
        while (!propname.empty())
        {
            if (pFaction->GetProperty(propname.c_str(), type, value, eNormal) &&
                (eLong==type) )
            {
                while (propname.size() < maxproplen)
                    AddCh(propname, ' ');
                Report << propname << "  " << (long)value << EOL_SCR;
            }

            propname = pFaction->GetPropertyName(++propidx);
        }
        Report << EOL_SCR << "-------------------------------------------"  << EOL_SCR << EOL_SCR;
    }

    //display data

    CShowOneDescriptionDlg dlg(gpUIController->m_Frames[AH_FRAME_MAP],
                               "Factions Overview",
                               Report.c_str());
    dlg.ShowModal();
    Factions.FreeAll();
}

//--------------------------------------------------------------------------

void CAhApp::CheckMonthLongOrders()
{
    static const char dup_ord_msg[] = ";--- Duplicate month long orders";
    static const char no_ord_msg[]  = ";--- No month long orders";
    int                  x;
    CUnit              * pUnit;
    const char         * src;
    const char         * dupord;
    const char         * p;
    char                 ch;
    std::string                 Line;
    std::string                 Ord;
    const char         * order;
    bool                 IsNew;
    bool                 Found;
    std::string Errors;
    std::string S;
    std::string                 FoundOrder;
    std::set<std::string, CaseInsensitiveLess> MonthLongOrders;
    std::set<std::string, CaseInsensitiveLess> MonthLongDup;
    long                 men;
    EValueType           type;
    CUnitPaneFltr      * pUnitPaneF = nullptr;
    int                  errcount = 0;
    int                  turnlvl;
    CBaseColl            Hexes(64);
    int                  nl, unitidx;
    CLand              * pLand;
    CMapPane           * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];


    p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_MONTH_LONG));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            MonthLongOrders.insert(S.c_str());
    }

    p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_DUPLICATABLE));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            MonthLongDup.insert(S.c_str());
    }

    if (1==atol(SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_OUTPUT_LIST))))
    {
        // Output will go into the unit filter window
        gpUIController->OpenUnitFrameFltr(false);
        pUnitPaneF = (CUnitPaneFltr*)gpUIController->m_Panes [AH_PANE_UNITS_FILTER];
    }

    if (pUnitPaneF)
        pUnitPaneF->InsertUnitInit();

    pMapPane->GetSelectedOrAllHexes(Hexes, false);
    for (nl=0; nl<Hexes.Count(); nl++)
    {
        pLand = (CLand*)Hexes.At(nl);
        for (unitidx=0; unitidx<pLand->Units.Count(); unitidx++)
        {
            pUnit    = (CUnit*)pLand->Units.At(unitidx);

            if (!pUnit->IsOurs)
                continue;
            src   = pUnit->Orders.c_str();
            IsNew = false;
            Found = false;
            turnlvl = 0;
            while (src && *src)
            {
                dupord = src;
                src    = GetToken(Line, src, '\n', TRIM_ALL);
                GetToken(Ord, SkipSpaces(Line.c_str()), " \t", ch, TRIM_ALL);
                order = Ord.c_str();
                if ('@'==*order)
                    order++;
                if (0==SafeCmp("FORM", order))
                    IsNew = true;
                else if (0==SafeCmp("END", order))
                    IsNew = false;
                else if (0==SafeCmp("TURN", order))
                    turnlvl++;
                else if (0==SafeCmp("ENDTURN", order))
                    turnlvl--;
                else if (!IsNew && 0==turnlvl && MonthLongOrders.find(order) != MonthLongOrders.end() )
                {
                    if (Found)
                    {
                        if (0==stricmp(order, FoundOrder.c_str()) &&
                            MonthLongDup.find(order) != MonthLongDup.end())
                            continue; // it is an order which can be duplicated

                        errcount++;
                        if (pUnitPaneF)
                        {
                            int newpos;

                            pUnitPaneF->InsertUnit(pUnit);
                            S = dup_ord_msg;
                            S << EOL_SCR;
                            newpos = dupord - pUnit->Orders.c_str() + S.size();
                            InsBuf(pUnit->Orders, S.c_str(), dupord - pUnit->Orders.c_str(), S.size());
                            src = &pUnit->Orders.c_str()[newpos];
                        }
                        else
                        {
                            Format(S, "Unit % 5d Error : Duplicate month long orders - %s", pUnit->Id, Line.c_str());
                            Errors << S << EOL_SCR;
                        }
                        break;
                    }
                    Found      = true;
                    FoundOrder = order;
                }
            }
            if (!Found)
            {
                if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) ||
                    (eLong==type && 0==men))
                    continue; // no men - no orders is ok

                errcount++;
                if (pUnitPaneF)
                {
                    pUnitPaneF->InsertUnit(pUnit);
                    GetToken(Line, pUnit->Orders.c_str(), '\n', TRIM_ALL);
                    if (nullptr==strstr(Line.c_str(), no_ord_msg))
                    {
                        S = no_ord_msg;
                        S << EOL_SCR;
                        InsBuf(pUnit->Orders, S.c_str(), 0, S.size());
                    }
                }
                else
                {
                    Format(S, "Unit % 5d Warning : No month long orders", pUnit->Id);
                    Errors << S << EOL_SCR;
                }
            }
        }
    }

    Hexes.DeleteAll();


    if (pUnitPaneF)
        pUnitPaneF->InsertUnitDone();

    if (!pUnitPaneF && errcount>0)
        gpUIController->ShowError(Errors.c_str(), Errors.size(), true);

    if (0==errcount)
        wxMessageBox(wxT("No problems found."), wxT("Order checking"), wxOK | wxCENTRE, gpUIController->m_Frames[AH_FRAME_MAP]);


//int wxMessageBox(const wxString& message, const wxString& caption = "Message", int style = wxOK | wxCENTRE,
// wxWindow *parent = nullptr, int x = -1, int y = -1)

    MonthLongOrders.clear();
    MonthLongDup.clear();
}

//--------------------------------------------------------------------------

void CAhApp::ShowUnitsMovingIntoHex(long CurHexId, CPlane * pCurPlane)
{
    CLand          * pLand;
    CUnit          * pUnit;
    int              nl, nu, i;
    long             HexId;
    CUnitPaneFltr  * pUnitPaneF = nullptr;
    std::string UnitText, S;
    CBaseColl        FoundUnits;

    for (nl=0; nl<pCurPlane->Lands.Count(); nl++)
    {
        pLand = (CLand*)pCurPlane->Lands.At(nl);
        for (nu=0; nu<pLand->Units.Count(); nu++)
        {
            pUnit = (CUnit*)pLand->Units.At(nu);
            if (pUnit->pMovement && pUnit->pMovement->size()>0)
            {
                HexId = pUnit->pMovement->back();
                if (HexId==CurHexId)
                    FoundUnits.Insert(pUnit);
            }
        }
    }

    // now display our findings
    if (FoundUnits.Count() > 0)
    {
        if (1==atol(SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_OUTPUT_LIST))))
        {
            // Output will go into the unit filter window
            gpUIController->OpenUnitFrameFltr(false);
            pUnitPaneF = (CUnitPaneFltr*)gpUIController->m_Panes [AH_PANE_UNITS_FILTER];
            pUnitPaneF->InsertUnitInit();
        }


        for (i=0; i<FoundUnits.Count(); i++)
        {
            pUnit = (CUnit*)FoundUnits.At(i);
            if (pUnitPaneF)
                pUnitPaneF->InsertUnit(pUnit);
            else
            {
                Format(S, "Unit % 5d", pUnit->Id);
                UnitText << S << EOL_SCR;
            }
        }

        if (pUnitPaneF)
            pUnitPaneF->InsertUnitDone();
        else
            gpUIController->ShowError(UnitText.c_str(), UnitText.size(), true);
    }
    else
        wxMessageBox(wxT("Found no units moving into the current hex."), wxT("Units moving"), wxOK | wxCENTRE, gpUIController->m_Frames[AH_FRAME_MAP]);


    FoundUnits.DeleteAll();
}

//--------------------------------------------------------------------------

void CAhApp::ShowLandFinancial(CLand * pCurLand)
{
    CUnit            * pUnit;
    int                idx, factidx;
    long               CurFaction;
    long               SilvOrg = 0;
    long               SilvRes = 0;
    long               TaxOur  = 0;
    long               TaxTheir= 0;
    long               WorkOur  = 0;
    long               WorkTheir= 0;
    long               Maintain = 0;
    long               men;
    long               MovedOut = 0;
    long               Workers  = 0;
    EValueType         type;
    const void       * value;
    CBaseObject        Report;
    CBaseCollByName    coll;
    std::string               sCoord;
    std::set<long>     Factions;
    long               TaxPerTaxer;
    const char       * leadership;

    if (!pCurLand)
        return;

    TaxPerTaxer = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));

    // get faction list
    for (idx=0; idx<pCurLand->Units.Count(); idx++)
    {
        pUnit    = (CUnit*)pCurLand->Units.At(idx);
        if (pUnit->FactionId != 0)
            Factions.insert(pUnit->FactionId);
    }

    // check each faction
    for (long _factionId : Factions)
    {
        CurFaction = _factionId;
        SilvOrg  = 0;
        SilvRes  = 0;
        TaxOur   = 0;
        TaxTheir = 0;
        WorkOur  = 0;
        WorkTheir= 0;
        Maintain = 0;
        MovedOut = 0;
        Workers  = 0;

        for (idx=0; idx<pCurLand->Units.Count(); idx++)
        {
            pUnit    = (CUnit*)pCurLand->Units.At(idx);
            if (!pUnit->IsOurs)
                continue;
            if (pUnit->FactionId == CurFaction)
            {
                if (pUnit->GetProperty(PRP_SILVER, type, value, eOriginal) && eLong==type)
                    SilvOrg += (long)value;
                if (pUnit->GetProperty(PRP_SILVER, type, value, eNormal) && eLong==type)
                {
                    SilvRes += (long)value;

                    if (pUnit->pMovement && pUnit->pMovement->size()>0)
                        MovedOut += (long)value;
                }
            }

            if (pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) && eLong==type)
            {
                if (pUnit->Flags & UNIT_FLAG_TAXING)
                {
                    if (pUnit->FactionId == CurFaction)
                        TaxOur += men*TaxPerTaxer;
                    else
                        TaxTheir += men*TaxPerTaxer;
                }

                if (pUnit->IsWorking)
                {
                    if (pUnit->FactionId == CurFaction)
                    {
                        Workers += men;
                        WorkOur +=  (long)(men*pCurLand->Wages);
                    }
                    else
                        WorkTheir +=  (long)(men*pCurLand->Wages);
                }

                if (pUnit->FactionId == CurFaction && (!pUnit->pMovement || pUnit->pMovement->empty()))
                {
                    if (pUnit->GetProperty(PRP_LEADER, type, (const void *&)leadership, eNormal) && eCharPtr==type &&
                        (0==strcmp(leadership, SZ_LEADER) || 0==strcmp(leadership, SZ_HERO)))
                        Maintain += men*20;
                    else
                        Maintain += men*10;
                }
            }

        }

        long t = TaxOur + TaxTheir;
        if (t > 0 && t > pCurLand->Taxable)
            TaxOur =  (long)(((double)pCurLand->Taxable)/t*TaxOur);

        long w = WorkOur + WorkTheir;
        if (w > pCurLand->MaxWages)
            WorkOur =  (long)(((double)pCurLand->MaxWages)/w*WorkOur);

        if (Maintain>0)
        {
            Report.Description << EOL_SCR << "Faction " << (long)CurFaction << EOL_SCR;
            Report.Description << "==========" << EOL_SCR;
            Report.Description << "SILV in the beginning       "   << SilvOrg << EOL_SCR;
            Report.Description << "SILV after executing orders "   << SilvRes << EOL_SCR;
            Report.Description << "Expected Tax Income         "   << TaxOur << EOL_SCR;
            Report.Description << "Expected Work Income        "   << WorkOur << EOL_SCR;
            Report.Description << "Expected Maintenance       -"   << Maintain << EOL_SCR;
            Report.Description << "Moved out                  -"   << MovedOut << EOL_SCR;
            Report.Description << "                            -------"    << EOL_SCR;
            Report.Description << "Expected Balance            "   << (SilvRes + TaxOur + WorkOur - Maintain - MovedOut) << EOL_SCR;
            Report.Description << ""    << EOL_SCR;
            Report.Description << "Workers                     "   << Workers << EOL_SCR;
            Report.Description << "Max workers                 "   << (long)(((double)pCurLand->MaxWages)/pCurLand->Wages) << EOL_SCR;
        }
    }

    gpGameData->m_pAtlantis->ComposeLandStrCoord(pCurLand, sCoord);
    Report.Name << "Financial report for " << sCoord;
    coll.Insert(&Report);

    ShowDescriptionList(coll, "Financial report");
    coll.DeleteAll();
}

//--------------------------------------------------------------------------

void CAhApp::RerunOrders()
{
    gpGameData->m_pAtlantis->RunOrders(nullptr);
    CUnitPane * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
    if (pUnitPane)
        pUnitPane->Update(pUnitPane->m_pCurLand);
}

//--------------------------------------------------------------------------

int CAhApp::SaveHistory(const char * FNameOut)
{
    CLand            * pLand;
    CFileWriter        Dest;
    int                nl, np;
    CPlane           * pPlane;
    SAVE_HEX_OPTIONS   options;

    memset(&options, 0, sizeof(options));
    options.AlwaysSaveImmobStructs = true;
    options.SaveResources          = true;

    if ( (gpGameData->m_pAtlantis->m_Planes.Count()>0) &&
         (0==gpGameData->m_pAtlantis->m_ParseErr)      && // don't destroy if not loaded!
         Dest.Open(FNameOut)
       )
    {
        for (np=0; np<gpGameData->m_pAtlantis->m_Planes.Count(); np++)
        {
            pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(np);
            for (nl=0; nl<pPlane->Lands.Count(); nl++)
            {
                pLand    = (CLand*)pPlane->Lands.At(nl);
                gpGameData->m_pAtlantis->SaveOneHex(Dest, pLand, pPlane, &options);
            }
        }
        Dest.Close();
    }
    return ERR_OK;
}

//--------------------------------------------------------------------------




bool CAhApp::GetExportHexOptions(std::string & FName, std::string & FMode, SAVE_HEX_OPTIONS & options, eHexIncl & HexIncl,
                                 bool & InclTurnNoAcl )
{

    static std::string     stFName;
    static bool     stOverwrite     = false;
    static eHexIncl stHexIncl       = HexNew;
    static bool     stInclStructs   = true;
    static bool     stInclUnits     = true;
    static bool     stInclTurnNoAcl = false;
    static bool     stInclResources = true;

    CHexExportDlg   dlg(gpUIController->m_Frames[AH_FRAME_MAP]);

    memset(&options, 0, sizeof(options));
    options.SaveUnits = true;

    if (stFName.empty())
        Format(stFName, "map.%04d", gpGameData->m_pAtlantis->m_YearMon);

    dlg.m_tcFName         ->SetValue(wxString::FromAscii(stFName.c_str()));

    dlg.m_rbHexNew        ->SetValue(HexNew      == stHexIncl);
    dlg.m_rbHexCurrent    ->SetValue(HexCurrent  == stHexIncl);
    dlg.m_rbHexSelected   ->SetValue(HexSelected == stHexIncl);
    dlg.m_rbHexAll        ->SetValue(HexAll      == stHexIncl);

    dlg.m_rbFileOverwrite ->SetValue(false); //stOverwrite);
    dlg.m_rbFileAppend    ->SetValue(true);  //!stOverwrite);

    dlg.m_chbInclStructs  ->SetValue(stInclStructs  );
    dlg.m_chbInclUnits    ->SetValue(stInclUnits    );
    dlg.m_chbInclTurnNoAcl->SetValue(stInclTurnNoAcl);
    dlg.m_chbInclResources->SetValue(stInclResources);



    if (wxID_OK == dlg.ShowModal())
    {
        SetStr(stFName, dlg.m_tcFName->GetValue().mb_str());

        if (dlg.m_rbHexNew->GetValue())
            stHexIncl = HexNew;
        else if (dlg.m_rbHexCurrent->GetValue())
            stHexIncl = HexCurrent;
        else if (dlg.m_rbHexSelected->GetValue())
            stHexIncl = HexSelected;
        else if (dlg.m_rbHexAll->GetValue())
            stHexIncl = HexAll;

        stOverwrite = dlg.m_rbFileOverwrite->GetValue();

        stInclStructs   = dlg.m_chbInclStructs  ->GetValue();
        stInclUnits     = dlg.m_chbInclUnits    ->GetValue();
        stInclTurnNoAcl = dlg.m_chbInclTurnNoAcl->GetValue();
        stInclResources = dlg.m_chbInclResources->GetValue();

        FName = stFName;
#if defined(_MSC_VER)
        FMode = stOverwrite?"wb":"ab";
#else
        FMode = stOverwrite?"w":"a";
#endif
        options.SaveStructs  = stInclStructs;
        options.SaveUnits    = stInclUnits;
        options.SaveResources= stInclResources;
        HexIncl = stHexIncl;
        InclTurnNoAcl = stInclTurnNoAcl;

        return true;
    }


    return false;
}

//--------------------------------------------------------------------------

// will discriminate by new hex

void CAhApp::ExportOneHex(CFileWriter & Dest, CPlane * pPlane, CLand * pLand, SAVE_HEX_OPTIONS & options, bool InclTurnNoAcl, bool OnlyNew)
{
    std::string               sData, sName;
    const char       * p;
    int                ym_first = 0;
    int                ym_last  = 0;

    gpGameData->m_pAtlantis->ComposeLandStrCoord(pLand, sName);

    p  = GetToken(sData, gpConfigManager->GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()), ',');
    if (sData.empty())
    {
/*        ym_first = gpGameData->m_pAtlantis->m_YearMon;
        ym_last  = gpGameData->m_pAtlantis->m_YearMon;*/
    }
    else
    {
        ym_last = atol(sData.c_str());
        GetToken(sData, SkipSpaces(p), ',');
        ym_first = atol(sData.c_str());
    }

    if (InclTurnNoAcl)
        options.WriteTurnNo = (ym_last/100 - 1)*12 + ym_last%100;
    else
        options.WriteTurnNo = 0;

    if (ym_first==gpGameData->m_pAtlantis->m_YearMon || !OnlyNew)
    {
        gpGameData->m_pAtlantis->SaveOneHex(Dest, pLand, pPlane, &options);
    }
}

//--------------------------------------------------------------------------

void CAhApp::ExportHexes()
{
    std::string               sData, sName;
    CMapPane         * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];

    CLand            * pLand;
    CFileWriter        Dest;
    int                nl;
    CPlane           * pPlane;
    SAVE_HEX_OPTIONS   options;
    eHexIncl           HexIncl;
    bool               InclTurnNoAcl ;

    if ( GetExportHexOptions(sName, sData, options, HexIncl, InclTurnNoAcl) &&
         Dest.Open(sName.c_str(), sData.c_str()) )
    {
        if (HexCurrent==HexIncl)
        {
            pPlane   = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);
            pLand    = gpGameData->m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);
            ExportOneHex(Dest, pPlane, pLand, options, InclTurnNoAcl, false);
        }
        else
        {
            CBaseColl  Hexes(64);
            pMapPane->GetSelectedOrAllHexes(Hexes, HexSelected==HexIncl);
            for (nl=0; nl<Hexes.Count(); nl++)
            {
                pLand = (CLand*)Hexes.At(nl);
                ExportOneHex(Dest, pLand->pPlane, pLand, options, InclTurnNoAcl, HexNew==HexIncl);
            }

            Hexes.DeleteAll();
        }
    }
        Dest.Close();
}

//--------------------------------------------------------------------------

void CAhApp::FindTradeRoutes()
{
    CMapPane    * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    CBaseColl     Hexes(64);
    CLand       * pSellLand, * pBuyLand;
    int           i, j;
    std::string Report;
    int           idx;
    const char  * propnameprice;
    EValueType    type;
    const void  * value;
    std::string GoodsName, PropName, sCoord;
    long          nSaleAmount, nSalePrice, nBuyAmount, nBuyPrice;
    
    if (!pMapPane)
        return;
    wxBeginBusyCursor();
    
    pMapPane->GetSelectedOrAllHexes(Hexes, true);
    if (0==Hexes.Count())
        wxMessageBox(wxT("Please select area on the map first."));
    for (i=0; i<Hexes.Count(); i++)
    {
        pSellLand = (CLand*)Hexes.At(i);
        
        idx      = 0;
        propnameprice = pSellLand->GetPropertyName(idx);
        while (*propnameprice)
        {
            if (pSellLand->GetProperty(propnameprice, type, value, eOriginal) && 
                eLong==type && 
                0==strncmp(propnameprice, PRP_SALE_PRICE_PREFIX, sizeof(PRP_SALE_PRICE_PREFIX)-1))
            {
                nSalePrice = (long)value;
                GoodsName = &(propnameprice[sizeof(PRP_SALE_PRICE_PREFIX)-1]);
                
                PropName.clear(); 
                PropName << PRP_SALE_AMOUNT_PREFIX << GoodsName;
                if (!pSellLand->GetProperty(PropName.c_str(), type, value, eOriginal) || eLong!=type)
                    continue;
                nSaleAmount = (long)value;
                
                for (j=0; j<Hexes.Count(); j++)
                {
                    pBuyLand = (CLand*)Hexes.At(j);
                    
                    PropName.clear(); 
                    PropName << PRP_WANTED_PRICE_PREFIX << GoodsName;
                    if (!pBuyLand->GetProperty(PropName.c_str(), type, value, eOriginal) || eLong!=type)
                        continue;
                    nBuyPrice = (long)value;
                    
                    PropName.clear(); 
                    PropName << PRP_WANTED_AMOUNT_PREFIX << GoodsName;
                    if (!pBuyLand->GetProperty(PropName.c_str(), type, value, eOriginal) || eLong!=type)
                        continue;
                    nBuyAmount = (long)value;
                    
                    if (nBuyPrice > nSalePrice)
                    {
                        gpGameData->m_pAtlantis->ComposeLandStrCoord(pSellLand, sCoord);
                        Report << pSellLand->TerrainType << " (" << sCoord << ") " << EOL_SCR;
                        gpGameData->m_pAtlantis->ComposeLandStrCoord(pBuyLand, sCoord);
                        Report << "         to " << pBuyLand->TerrainType << " (" << sCoord << ")   ("
                               << nBuyPrice << "-" << nSalePrice << ")*" << std::min(nSaleAmount,nBuyAmount)
                               << " " << GoodsName
                               << " = " << (nBuyPrice - nSalePrice) * std::min(nSaleAmount,nBuyAmount) << EOL_SCR;
                    }
                }
            }
            propnameprice = pSellLand->GetPropertyName(++idx);
        }
    }
    
    if (Report.empty())
        wxMessageBox(wxT("No trade routes found."));
    else
        gpUIController->ShowError(Report.c_str()      , Report.size()      , true);

    Hexes.DeleteAll();
    wxEndBusyCursor();
}

//--------------------------------------------------------------------------

void CAhApp::StdRedirectInit()
{
#ifdef __WXMAC_OSX__
	char cwd[MAXPATHLEN];
	// Setup new working directory in case we got started from /Applications
	if((getcwd(cwd, MAXPATHLEN)) != nullptr){
		if((strncmp(cwd, "/Applications", strlen("/Applications"))) == 0){
			const char *home = getenv("HOME");
			if(home != nullptr){
				if(0 == chdir(home)){
					mkdir(".alh", 0750);
					if(0 != chdir(".alh"))
						chdir("/Applications");
				}
			}
		}
	}
#endif
    freopen("ah.stdout", "w", stdout);
    freopen("ah.stderr", "w", stderr);
    m_nStdoutLastPos = 0;
    m_nStderrLastPos = 0;
}

//--------------------------------------------------------------------------

void CAhApp::StdRedirectReadMore(bool FromStdout, std::string & sData)
{
    FILE       * f;
    int        * pCurPos;
    char         buf[1024];
    int          n;

    sData.clear();
    if (FromStdout)
    {
        fflush(stdout);
        pCurPos  =  &m_nStdoutLastPos;
        f        = fopen("ah.stdout", "rb");
    }
    else
    {
        fflush(stderr);
        pCurPos  =  &m_nStderrLastPos;
        f        = fopen("ah.stderr", "rb");
    }

    if (f)
    {
        fseek(f, *pCurPos, SEEK_SET);
        do
        {
            n = fread(buf, 1, sizeof(buf), f);
            if (n>0)
                AddBuf(sData, buf, n);
        } while (n>0);
        *pCurPos = ftell(f);
        fclose(f);
    }
}

//--------------------------------------------------------------------------

void CAhApp::CheckRedirectedOutputFiles()
{
    std::string S;

    gpApp->StdRedirectReadMore(false, S);
    if (!S.empty())
        gpUIController->ShowError(S.c_str(), S.size(), true);
    gpApp->StdRedirectReadMore(true, S);
    if (!S.empty())
        gpUIController->ShowError(S.c_str(), S.size(), true);
}

//--------------------------------------------------------------------------

void CAhApp::StdRedirectDone()
{
}

//--------------------------------------------------------------------------

void CAhApp::ViewMovedUnits()
{
}

//==========================================================================

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
    const char * p = path.c_str();
    std::string         rel_path;

    while (*p && EQUAL_PATH_CHARS(*p, *cur_dir) )
    {
        p++;
        cur_dir++;
    }

    if (*p==SEP)
        p++;
    else
        rel_path << ".." << SEP;

    while (*cur_dir)
    {
        if (*cur_dir == SEP)
            rel_path << ".." << SEP;

        cur_dir++;
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

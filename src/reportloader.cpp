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

#include "reportloader.h"

#include <set>
#include <utility>
#include "string_utils.h"
#include "consts.h"
#include "consts_ah.h"
#include "configmanager.h"
#include "gamerules.h"
#include "gamedatamanager.h"
#include "uicontroller.h"
#include "selectionstate.h"
#include "ahapp.h"
#include "ahframe.h"
#include "mapframe.h"
#include "editpane.h"
#include "mappane.h"
#include "listpane.h"
#include "unitpane.h"
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "flagsdlg.h"

ReportLoader * gpReportLoader = nullptr;

//=========================================================================

ReportLoader::ReportLoader()
{
    m_FirstLoad         = true;
    m_OrdersAreChanged  = false;
    m_CommentsChanged   = false;
    m_DisableErrs       = false;
    m_nStdoutLastPos    = 0;
    m_nStderrLastPos    = 0;
}

//-------------------------------------------------------------------------

void ReportLoader::Init()
{
    const char      * p;
    const char      * szName;
    const char      * szValue;
    std::string S;
    int               sectidx;
    std::set<std::pair<std::string,std::string>> CollDedup;

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

    StdRedirectInit();
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

void ReportLoader::GetShortFactName(std::string & S, int FactionId)
{
#define MAX_F_NAME 8
    int           i;
    char          ch;
    CFaction    * pFaction;

    S.clear();
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

void ReportLoader::SetOrdersChanged(bool Changed)
{
    m_OrdersAreChanged = Changed;

    if (!gpUIController->m_sTitle.empty())
        gpUIController->SetMapFrameTitle();
}

//-------------------------------------------------------------------------

int ReportLoader::SaveOrders(bool UsingExistingName)
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

int  ReportLoader::SaveOrders(const char * FNameOut, int FactionId)
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

int  ReportLoader::LoadOrders  (const char * FNameIn)
{
    int           err;
    std::string S, FName, Sect;
    int           factid;


    FName = FNameIn;  // FNameIn can be coming from config, so do not use it directly!
    err = gpGameData->m_pAtlantis->LoadOrders(FName.c_str(), factid);
    if (ERR_OK==err)
    {
        S.clear();
        S << (long)gpGameData->m_pAtlantis->m_YearMon;
        gpConfigManager->ComposeConfigOrdersSection(Sect, factid);
        gpConfigManager->SetConfig(Sect.c_str(), S.c_str(), FName.c_str());

        gpSelectionState->OnMapSelectionChange();
        gpSelectionState->RedrawTracks();
    }
    else
       if (gpUIController->m_Frames[AH_FRAME_MSG])
           ((CAhFrame*)gpUIController->m_Frames[AH_FRAME_MSG])->Raise();

    return err;
}

//-------------------------------------------------------------------------

void ReportLoader::LoadComments()
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

void ReportLoader::SaveComments()
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

void ReportLoader::LoadUnitFlags()
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

void ReportLoader::SaveUnitFlags()
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

void ReportLoader::SetAllLandUnitFlags()
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

void ReportLoader::SaveLandFlags()
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

void ReportLoader::LoadLandFlags()
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

void ReportLoader::UpdateEdgeStructs()
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

void ReportLoader::PreLoadReport()
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

#define SET_UNIT_PROP_NAME(_name, _type)                                 \
{                                                                        \
    gpGameData->m_pAtlantis->m_UnitPropertyNames.insert(_name);                      \
    gpGameData->m_pAtlantis->m_UnitPropertyTypes.emplace(_name, (int)(_type));       \
}

void ReportLoader::PostLoadReport()
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
        gpApp->ShowDescriptionList(Coll, "Hex Events");
    }

    // show newly discovered products (advanced resources), if any
    if (gpGameData->m_pAtlantis->m_NewProducts.Count() > 0)
        gpApp->ShowDescriptionList(gpGameData->m_pAtlantis->m_NewProducts, "New products");

    if (pUnitPaneF)
        pUnitPaneF->Update(nullptr);

    CheckRedirectedOutputFiles();

    if (!gpGameData->m_pAtlantis->m_SecurityEvents.Description.empty())
        gpGameData->m_pAtlantis->m_SecurityEvents.Description << EOL_SCR << EOL_SCR;
}

//-------------------------------------------------------------------------

int  ReportLoader::LoadReport  (const char * FNameIn, bool Join)
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

int  ReportLoader::LoadReport(bool Join)
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

void ReportLoader::SwitchToYearMon(long YearMon)
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

void ReportLoader::SwitchToRep(eRepSeq whichrep)
{
    int  i;

    m_DisableErrs = true;

    if (CanSwitchToRep(whichrep, i))
        SwitchToYearMon(gpGameData->m_ReportDates[i]);

    m_DisableErrs = false;
}

//-------------------------------------------------------------------------

bool ReportLoader::CanSwitchToRep(eRepSeq whichrep, int & RepIdx)
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

bool ReportLoader::GetPrevTurnReport(CAtlaParser *& pPrevTurn)
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

//-------------------------------------------------------------------------

void ReportLoader::LoadOrders()
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

void ReportLoader::RerunOrders()
{
    gpGameData->m_pAtlantis->RunOrders(nullptr);
    CUnitPane * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
    if (pUnitPane)
        pUnitPane->Update(pUnitPane->m_pCurLand);
}

//-------------------------------------------------------------------------

int ReportLoader::SaveHistory(const char * FNameOut)
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

//-------------------------------------------------------------------------

void ReportLoader::StdRedirectInit()
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

//-------------------------------------------------------------------------

void ReportLoader::StdRedirectReadMore(bool FromStdout, std::string & sData)
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

//-------------------------------------------------------------------------

void ReportLoader::CheckRedirectedOutputFiles()
{
    std::string S;

    StdRedirectReadMore(false, S);
    if (!S.empty())
        gpUIController->ShowError(S.c_str(), S.size(), true);
    StdRedirectReadMore(true, S);
    if (!S.empty())
        gpUIController->ShowError(S.c_str(), S.size(), true);
}

//-------------------------------------------------------------------------

void ReportLoader::StdRedirectDone()
{
}

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

#ifndef __AH_APP_INCL__
#define __AH_APP_INCL__

#include <algorithm>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <memory>
#include "stl_helpers.h"
#include "cfgfile.h"
#include "atlaparser.h"
#include "configmanager.h"
#include "gamerules.h"
#include "gamedatamanager.h"
#include "uicontroller.h"
#include "selectionstate.h"
#include "reportloader.h"

class CAhFrame;
class CEditPane;
class CHexFilterDlg;

enum eHexIncl {HexNew,
               HexCurrent,
               HexSelected, // inside a rectangle by mouse dragging
               HexAll,
               };

void FontToStr(const wxFont * font, std::string & s);
wxFont * NewFontFromStr(const char * p);

void StrToColor(wxColour * cr, const char * p);
void ColorToStr(char * p, size_t n, wxColour * cr);
void MakePathRelative(const char * cur_dir, std::string & path);
void MakePathFull(const char * cur_dir, std::string & path);
void GetDirFromPath(const char * path, std::string & dir);
void GetFileFromPath(const char * path, std::string & file);

//-------------------------------------------------------------------------------

class CAhApp : public wxApp
{
public:
    CAhApp();
    ~CAhApp();

    virtual bool         OnInit() override;
    virtual int          OnExit() override;


    void                 WriteMagesCSV();
    void                 ShowDescriptionList(CBaseColl & Items, const char * title); // Collection of CBaseObject
    void                 ShowDescriptionList(CBaseCollById & Items, const char * title);
//    void                 ViewSkills(bool ViewAll);
    void                 ViewShortNamedObjects(bool ViewAll, const char * szSection, const char * szHeader, CBaseColl & ListNew);
    void                 ViewBattlesAll();
    void                 ViewEvents(bool DoEvents);
    void                 ViewSecurityEvents();
    void                 ViewNewProducts();
    void                 ViewGates();
    void                 ViewCities();
    void                 ViewProvinces();
    void                 ViewFactionInfo();
    void                 ViewFactionOverview();
    void                 ViewFactionOverview_IncrementValue(long FactionId, const char * factionname, CBaseCollById & Factions, const char * propname, long value);
    void                 CheckMonthLongOrders();
    void                 CheckProduction();
    void                 CheckSailing();
    void                 CheckTaxDetails  (CLand  * pLand, CTaxProdDetailsCollByFaction & TaxDetails);
    void                 CheckTradeDetails(CLand  * pLand, CTaxProdDetailsCollByFaction & TradeDetails);
    void                 CheckTaxTrade();
    void                 ExportHexes();
    void                 FindTradeRoutes();
    void                 ViewMovedUnits();
    void                 ShowUnitsMovingIntoHex(long CurHexId, CPlane * pCurPlane);
    void                 ShowLandFinancial(CLand * pCurLand);

    std::unique_ptr<ConfigManager> m_pConfigManager;
    std::unique_ptr<GameRules> m_pGameRules;
    std::unique_ptr<GameDataManager> m_pGameData;
    std::unique_ptr<UIController> m_pUIController;
    std::unique_ptr<SelectionState> m_pSelectionState;
    std::unique_ptr<ReportLoader> m_pReportLoader;


private:
    bool                 GetExportHexOptions(std::string & FName, std::string & FMode, SAVE_HEX_OPTIONS & options, eHexIncl & HexIncl,
                                             bool & InclTurnNoAcl);
    void                 ExportOneHex(CFileWriter & Dest, CPlane * pPlane, CLand * pLand, SAVE_HEX_OPTIONS & options, bool InclTurnNoAcl, bool OnlyNew);

};

//-------------------------------------------------------------------------------

extern CAhApp * gpApp;


#endif

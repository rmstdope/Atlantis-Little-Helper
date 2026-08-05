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

enum
{
    AH_FRAME_MAP          =  0,
    AH_FRAME_UNITS            ,
    AH_FRAME_MSG              ,
    AH_FRAME_EDITS            ,
    AH_FRAME_UNITS_FLTR       ,

    AH_FRAME_COUNT
};

enum
{
    AH_PANE_MAP            = 0,
    AH_PANE_MAP_DESCR         ,
    AH_PANE_UNITS_HEX         ,
    AH_PANE_UNITS_FILTER      ,
    AH_PANE_UNIT_DESCR        ,
    AH_PANE_UNIT_COMMANDS     ,
    AH_PANE_UNIT_COMMENTS     ,
    AH_PANE_MSG               ,

    AH_PANE_COUNT             // whenever adding new panes, update CAhApp::ApplyFonts and CAhApp::ApplyColors!
};

enum
{
    AH_LAYOUT_2_WIN        = 0,
    AH_LAYOUT_3_WIN           ,
    AH_LAYOUT_1_WIN           ,

    AH_LAYOUT_COUNT
};

enum
{
    FONT_EDIT_DESCR        = 0,
    FONT_EDIT_ORDER           ,
    FONT_MAP_COORD            ,
    FONT_MAP_TEXT             ,
    FONT_UNIT_LIST            ,
    FONT_EDIT_HDR             ,
    FONT_VIEW_DLG             ,
    FONT_ERR_DLG              ,

    FONT_COUNT
};

class CAhFrame;
class CEditPane;
class CHexFilterDlg;

enum eRepSeq  {repFirst, repPrev, repNext, repLast, repLastVisited};
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

#define APPLY_COLOR_DELTA(x) ((unsigned char )(std::max(std::min((int)(x)-(int)gpApp->m_Brightness_Delta,255),0)))



//-------------------------------------------------------------------------------

class CAhApp : public wxApp
{
public:
    CAhApp();
    ~CAhApp();

    virtual bool         OnInit() override;
    virtual int          OnExit() override;


    void                 EditPaneChanged(CEditPane * pPane);
    void                 EditPaneDClicked(CEditPane * pPane);


    void                 FrameClosing(CAhFrame * pFrame);

    void                 ShowError (const char * msg, int msglen, bool ignore_disabled);



    int                  LoadReport(bool Join);
    int                  LoadReport(const char * FNameIn, bool Join);
    int                  SaveOrders(bool UsingExistingName);
    void                 LoadOrders();
    CUnit              * GetSelectedUnit();
    bool                 CanCloseApp();
    void                 Redraw();
    void                 ApplyFonts();
    void                 ApplyColors();
    void                 ApplyIcons();

    void                 SelectUnit(CUnit * pUnit);
    bool                 SelectLand(const char * landcoords); //  "48,52[,somewhere]"
    void                 SelectLand(CLand * pLand);

    void                 SwitchToRep(eRepSeq whichrep);
    bool                 CanSwitchToRep(eRepSeq whichrep, int & RepIdx);

    void                 OnMapSelectionChange();
    void                 OnUnitHexSelectionChange(long idx);

    void                 OpenOptionsDlg();
    void                 OpenUnitFrame();
    void                 OpenMsgFrame();
    void                 OpenEditsFrame();
    void                 OpenUnitFrameFltr(bool PopUpSettings);
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
    void                 EditListColumns(int command);
    const char         * GetListColSection(const char * sectprefix, const char * key);
    void                 ViewMovedUnits();
    bool                 GetPrevTurnReport(CAtlaParser *& pPrevTurn); 

    void                 CreateAccelerator();

    bool                 GetOrdersChanged(){return m_OrdersAreChanged;};
    void                 SetOrdersChanged(bool Changed);
    void                 StdRedirectReadMore(bool FromStdout, std::string & sData);
    void                 CheckRedirectedOutputFiles();
    void                 RerunOrders();
    void                 SetAllLandUnitFlags();
    void                 ShowUnitsMovingIntoHex(long CurHexId, CPlane * pCurPlane);
    void                 ShowLandFinancial(CLand * pCurLand);
    void                 AddTempHex(int X, int Y, int Plane);
    void                 DelTempHex(int X, int Y, int Plane);
    
    void                 SelectNextUnit();
    void                 SelectPrevUnit();
    void                 SelectUnitsPane();
    void                 SelectOrdersPane();

    std::unique_ptr<CAtlaParser> m_pAtlantis;
    std::unique_ptr<ConfigManager> m_pConfigManager;
    std::unique_ptr<GameRules> m_pGameRules;

    CAhFrame           * m_Frames[AH_FRAME_COUNT];
    wxWindow           * m_Panes [AH_PANE_COUNT ];
    wxFont             * m_Fonts [FONT_COUNT];
    const char         * m_FontDescr[FONT_COUNT];
    std::multimap<std::string, std::string> m_UnitPropertyGroups;

//    bool                 m_LandFlagsChanged;
    bool                 m_CommentsChanged;
    bool                 m_DiscardChanges;
    std::unique_ptr<wxAcceleratorTable> m_pAccel;
    long                 m_Brightness_Delta;


private:
    void                 ForgetFrame(int no, bool frameclosed);
    int                  LoadOrders  (const char * FNameIn);
    int                  SaveOrders  (const char * FNameOut, int FactionId);
    int                  SaveHistory (const char * FNameOut);
    void                 LoadComments();
    void                 SaveComments();
    void                 PostLoadReport();
    void                 PreLoadReport();
    void                 RedrawTracks();
    void                 OpenMapFrame();
    void                 GetShortFactName(std::string & S, int FactionId);
    void                 SaveLandFlags();
    void                 LoadLandFlags();
    void                 UpdateEdgeStructs();
    void                 LoadUnitFlags();
    void                 SaveUnitFlags();

    void                 UpdateHexEditPane(CLand * pLand);
    void                 UpdateHexUnitList(CLand * pLand);
    void                 SwitchToYearMon(long YearMon);

    bool                 GetExportHexOptions(std::string & FName, std::string & FMode, SAVE_HEX_OPTIONS & options, eHexIncl & HexIncl,
                                             bool & InclTurnNoAcl);
    void                 ExportOneHex(CFileWriter & Dest, CPlane * pPlane, CLand * pLand, SAVE_HEX_OPTIONS & options, bool InclTurnNoAcl, bool OnlyNew);
    void                 SetMapFrameTitle();
    void                 StdRedirectInit();
    void                 StdRedirectDone();
    void                 SelectTempUnit(CUnit * pUnit);


    std::vector<std::unique_ptr<CAtlaParser>> m_Reports;
    std::vector<long>    m_ReportDates;
    bool                 m_FirstLoad;
    std::string                 m_HexDescrSrc;
    std::string                 m_UnitDescrSrc;
    std::string                 m_MsgSrc;
    long                 m_SelUnitIdx;
    int                  m_layout;
    bool                 m_DisableErrs;
    bool                 m_OrdersAreChanged;
    std::string                 m_sTitle;
    int                  m_nStdoutLastPos;
    int                  m_nStderrLastPos;

};

//-------------------------------------------------------------------------------

extern CAhApp * gpApp;


#endif

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

#include "string_utils.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"

#include "ahapp.h"
#include "ahframe.h"
#include "mapframe.h"
#include "unitframe.h"
#include "mappane.h"
#include "editpane.h"
#include "listpane.h"
#include "unitpane.h"
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "version.h"
#include "flagsdlg.h"

#ifdef HAVE_CONFIG_H
   #include "config.h"
#endif

#ifdef HAVE_PYTHON
#include "Python.h"
#endif


// define this to use XPMs everywhere (by default, BMPs are used under Win)
#ifdef __WXMSW__
    #define USE_XPM_BITMAPS 0
#else
    #define USE_XPM_BITMAPS 1
#endif


#if USE_XPM_BITMAPS
    #include "bitmaps/zoomin.xpm"
    #include "bitmaps/zoomout.xpm"
    #include "bitmaps/centerout.xpm"
    #include "bitmaps/prevzoom.xpm"
    #include "bitmaps/showcoord.xpm"
    #include "bitmaps/shownames.xpm"
    #include "bitmaps/planeup.xpm"
    #include "bitmaps/planedwn.xpm"
    #include "bitmaps/nextturn.xpm"
    #include "bitmaps/prevturn.xpm"
    #include "bitmaps/lastturn.xpm"
    #include "bitmaps/lastvisitturn.xpm"
    #include "bitmaps/findhex.xpm"
#endif

static const int AH_BuildNumber =
#include "build_no"
;

BEGIN_EVENT_TABLE(CMapFrame, CAhFrame)

    EVT_MENU     (menu_LoadReport         , CMapFrame::OnLoadReport            )
    EVT_MENU     (menu_JoinReport         , CMapFrame::OnJoinReport            )
    EVT_MENU     (menu_LoadOrders         , CMapFrame::OnLoadOrders            )
    EVT_MENU     (menu_SaveOrders         , CMapFrame::OnSaveOrders            )
    EVT_MENU     (menu_SaveOrdersAs       , CMapFrame::OnSaveOrdersAs          )
    EVT_MENU     (menu_Options            , CMapFrame::OnOptions               )
    EVT_MENU     (menu_ViewSkillsAll      , CMapFrame::OnViewSkillsAll         )
    EVT_MENU     (menu_ViewSkillsNew      , CMapFrame::OnViewSkillsNew         )
    EVT_MENU     (menu_ViewItemsAll       , CMapFrame::OnViewItemsAll          )
    EVT_MENU     (menu_ViewItemsNew       , CMapFrame::OnViewItemsNew          )
    EVT_MENU     (menu_ViewObjectsAll     , CMapFrame::OnViewObjectsAll        )
    EVT_MENU     (menu_ViewObjectsNew     , CMapFrame::OnViewObjectsNew        )
    EVT_MENU     (menu_ViewBattlesAll     , CMapFrame::OnViewBattlesAll        )
    EVT_MENU     (menu_ViewEvents         , CMapFrame::OnViewEvents            )
    EVT_MENU     (menu_ViewSecurityEvents , CMapFrame::OnViewSecurityEvents    )
    EVT_MENU     (menu_ViewNewProducts    , CMapFrame::OnViewNewProducts       )
    EVT_MENU     (menu_ViewErrors         , CMapFrame::OnViewErrors            )
    EVT_MENU     (menu_ViewGates          , CMapFrame::OnViewGates             )
    EVT_MENU     (menu_ViewCities         , CMapFrame::OnViewCities            )
    EVT_MENU     (menu_ViewProvinces      , CMapFrame::OnViewProvinces         )

    EVT_MENU     (menu_ViewFactionInfo    , CMapFrame::OnViewFactionInfo       )
    EVT_MENU     (menu_ViewFactionOverview, CMapFrame::OnViewFactionOverview)

    EVT_MENU     (menu_WindowUnits        , CMapFrame::OnWindowUnits           )
    EVT_MENU     (menu_WindowMessages     , CMapFrame::OnWindowMessages        )
    EVT_MENU     (menu_WindowEditors      , CMapFrame::OnWindowEditors         )
    EVT_MENU     (menu_WindowUnitsFltr    , CMapFrame::OnWindowUnitsFltr       )

    EVT_MENU     (menu_ListColUnits       , CMapFrame::OnListCol               )
    EVT_MENU     (menu_ListColUnitsFltr   , CMapFrame::OnListCol               )

    EVT_MENU     (menu_ApplyDefaultOrders , CMapFrame::OnApplyDefaultOrders    )
    EVT_MENU     (menu_RerunOrders        , CMapFrame::OnRerunOrders           )

    EVT_MENU     (menu_WriteMagesCSV      , CMapFrame::OnWriteMagesCSV         )
    EVT_MENU     (menu_ExportHexes        , CMapFrame::OnExportHexes           )
    EVT_MENU     (menu_FindTradeRoutes    , CMapFrame::OnFindTradeRoutes       )
    
    EVT_MENU     (menu_CheckMonthLongOrd  , CMapFrame::OnCheckMonthLongOrd     )
    EVT_MENU     (menu_CheckTaxTrade      , CMapFrame::OnCheckTaxTrade         )
    EVT_MENU     (menu_CheckProduction    , CMapFrame::OnCheckProduction       )
    EVT_MENU     (menu_CheckSailing       , CMapFrame::OnCheckSailing          )
    EVT_MENU     (menu_FindHexes          , CMapFrame::OnFindHexes             )

    EVT_MENU     (menu_FlagNames          , CMapFrame::OnFlagNames             )
    EVT_MENU     (menu_FlagsAllSet        , CMapFrame::OnFlagsAllSet           )

    EVT_MENU     (menu_Quit               , CMapFrame::OnQuit                  )
    EVT_MENU     (menu_QuitNoSave         , CMapFrame::OnQuitNoSave            )
    EVT_MENU     (menu_About              , CMapFrame::OnAbout                 )
    
    EVT_MENU   (accel_NextUnit            , CMapFrame::OnNextUnit)
    EVT_MENU   (accel_PrevUnit            , CMapFrame::OnPrevUnit)
    EVT_MENU   (accel_UnitList            , CMapFrame::OnUnitList)
    EVT_MENU   (accel_Orders              , CMapFrame::OnOrders  )

    EVT_UPDATE_UI(menu_ViewBattlesAll     , CMapFrame::OnViewBattlesAllUpdate  )
    EVT_UPDATE_UI(menu_ViewSkillsAll      , CMapFrame::OnViewSkillsAllUpdate   )
    EVT_UPDATE_UI(menu_ViewSkillsNew      , CMapFrame::OnViewSkillsNewUpdate   )
    EVT_UPDATE_UI(menu_ViewItemsAll       , CMapFrame::OnViewItemsAllUpdate    )
    EVT_UPDATE_UI(menu_ViewItemsNew       , CMapFrame::OnViewItemsNewUpdate    )
    EVT_UPDATE_UI(menu_ViewObjectsAll     , CMapFrame::OnViewObjectsAllUpdate  )
    EVT_UPDATE_UI(menu_ViewObjectsNew     , CMapFrame::OnViewObjectsNewUpdate  )
    EVT_UPDATE_UI(menu_ViewEvents         , CMapFrame::OnViewEventsUpdate      )
    EVT_UPDATE_UI(menu_ViewSecurityEvents , CMapFrame::OnViewSecurityEventsUpdate)
    EVT_UPDATE_UI(menu_ViewNewProducts    , CMapFrame::OnViewNewProductsUpdate )
    EVT_UPDATE_UI(menu_ViewErrors         , CMapFrame::OnViewErrorsUpdate      )
    EVT_UPDATE_UI(menu_ViewGates          , CMapFrame::OnViewGatesUpdate       )

    EVT_MENU_RANGE      (tool_zoomin, tool_findhex, CMapFrame::OnToolbarCmd   )
    EVT_UPDATE_UI_RANGE (tool_zoomin, tool_findhex, CMapFrame::OnToolbarUpdate)

    EVT_CLOSE    (CMapFrame::OnCloseWindow)

END_EVENT_TABLE()




//--------------------------------------------------------------------

CMapFrame::CMapFrame(wxWindow * parent, int layout)
          :CAhFrame (parent, "", wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN )
{
#ifdef __WXMAC_OSX__
    // we need this in order to allow the about menu relocation, since ABOUT is
    // not the default id of the about menu
    wxApp::s_macAboutMenuItemId = menu_About;
#endif

    m_Splitter  = nullptr;
    m_Splitter1 = nullptr;
    m_Splitter2 = nullptr;
    m_Splitter3 = nullptr;
    m_Splitter4 = nullptr;

    MakeMenu(layout);
    MakeToolBar();
}


//--------------------------------------------------------------------

void CMapFrame::MakeMenu(int layout)
{
    wxMenuBar * menuBar;
    wxMenu    * menuItem;
//    wxMenu    * menuSubItem;

    menuBar = new wxMenuBar();

    menuItem = new wxMenu(wxT(""), wxMENU_TEAROFF);
    menuItem->Append(menu_LoadReport        , wxT("Load Report")                 , wxT("") );
    menuItem->Append(menu_JoinReport        , wxT("Join Report")                 , wxT("") );
    menuItem->Append(menu_LoadOrders        , wxT("Load Orders")                 , wxT("") );
    menuItem->AppendSeparator();
    menuItem->Append(menu_SaveOrders        , wxT("Save Orders\tCtrl-S")         , wxT("") );
    menuItem->Append(menu_SaveOrdersAs      , wxT("Save Orders As")              , wxT("") );
    menuItem->AppendSeparator();
    menuItem->Append(menu_Quit              , wxT("Exit\tAlt-X")                 , wxT("") );
    menuItem->Append(menu_QuitNoSave        , wxT("Quit and Discard Changes")    , wxT("") );
    menuBar->Append(menuItem, wxT("&File"));

    menuItem = new wxMenu;
    menuItem->Append(menu_ViewFactionInfo   , wxT("Faction Info")                , wxT("") );
    menuItem->Append(menu_ViewFactionOverview, wxT("Faction Overview")           , wxT("") );
    menuItem->AppendSeparator();
    menuItem->Append(menu_ViewSkillsAll     , wxT("All Skills")                  , wxT("") );
    menuItem->Append(menu_ViewSkillsNew     , wxT("New Skills")                  , wxT("") );
    menuItem->Append(menu_ViewItemsAll      , wxT("All Items")                   , wxT("") );
    menuItem->Append(menu_ViewItemsNew      , wxT("New Items")                   , wxT("") );
    menuItem->Append(menu_ViewObjectsAll    , wxT("All Objects")                 , wxT("") );
    menuItem->Append(menu_ViewObjectsNew    , wxT("New Objects")                 , wxT("") );
    menuItem->Append(menu_ViewBattlesAll    , wxT("All Battles")                 , wxT("") );
    menuItem->AppendSeparator();
    menuItem->Append(menu_ViewGates         , wxT("Gates")                       , wxT("") );
    menuItem->Append(menu_ViewCities        , wxT("Cities")                      , wxT("") );
    menuItem->Append(menu_ViewProvinces     , wxT("Provinces")                   , wxT("") );
    menuItem->Append(menu_ViewNewProducts   , wxT("New products")                , wxT("") );
    menuItem->Append(menu_ViewEvents        , wxT("Non-unit Events")             , wxT("") );
    menuItem->Append(menu_ViewSecurityEvents, wxT("Security Events")             , wxT("") );
    menuItem->Append(menu_ViewErrors        , wxT("All Errors")                  , wxT("") );
    menuItem->AppendSeparator();
    menuItem->Append(menu_Options           , wxT("Options")                     , wxT("") );
    menuBar->Append(menuItem, wxT("&View"));


    menuItem = new wxMenu;
    menuItem->Append(menu_ApplyDefaultOrders, wxT("Apply default orders")          , wxT(""));
    menuItem->Append(menu_RerunOrders       , wxT("Rerun orders")                  , wxT(""));
    menuItem->Append(menu_CheckMonthLongOrd , wxT("Check month long orders")       , wxT(""));
    menuItem->Append(menu_CheckTaxTrade     , wxT("Check tax, trade for next turn"), wxT(""));
    menuItem->Append(menu_CheckProduction   , wxT("Check production requirements") , wxT(""));
    menuItem->Append(menu_CheckSailing      , wxT("Check sailing")                 , wxT(""));
    //menuItem->Append(menu_FindHexes         , wxT("Find hexes")                    , wxT(""));
    menuItem->Append(menu_FindTradeRoutes   , wxT("Find Trade Routes")             , wxT(""));


    menuItem->AppendSeparator();
    menuItem->Append(menu_WriteMagesCSV     , wxT("Export mages data to CSV file") , wxT(""));
    menuItem->Append(menu_ExportHexes       , wxT("Export Hexes")                  , wxT(""));
    
    
    menuItem->AppendSeparator();
    menuItem->Append(menu_FlagNames         , wxT("Unit Flag Names")               , wxT(""));
    menuItem->Append(menu_FlagsAllSet       , wxT("Set/Clear Hex/Unit Flags")      , wxT(""));
    menuBar->Append(menuItem, wxT("&Actions"));



    menuItem = new wxMenu;
    if (AH_LAYOUT_3_WIN==layout || AH_LAYOUT_2_WIN==layout)
        menuItem->Append(menu_WindowUnits   , wxT("Units (Hex)")                 , wxT(""));
    menuItem->Append(menu_WindowUnitsFltr   , wxT("Unit Locator")                , wxT(""));
    menuItem->Append(menu_WindowMessages    , wxT("Messages")                    , wxT(""));
    if (AH_LAYOUT_3_WIN==layout)
        menuItem->Append(menu_WindowEditors , wxT("Editors")                     , wxT(""));

//    menuSubItem = new wxMenu;
//    menuSubItem->Append(menu_ListColUnits    , wxT("Units (Hex)")                 , wxT(""));
//    menuSubItem->Append(menu_ListColUnitsFltr, wxT("Unit Locator")                , wxT(""));
//    menuItem->   Append(menu_ListColumns     , wxT("List columns"), menuSubItem   , wxT(""));
    menuItem->AppendSeparator();
    menuItem->Append(menu_ListColUnits      , wxT("Columns for Units (Hex)")     , wxT(""));
    menuItem->Append(menu_ListColUnitsFltr  , wxT("Columns for Unit Locator")    , wxT(""));

    menuBar->Append(menuItem, wxT("&Windows"));


    menuItem = new wxMenu;
    menuItem->Append(menu_About             , wxT("About")                       , wxT(""));
    menuBar->Append(menuItem, wxT("&Help"));

    SetMenuBar(menuBar);
}

//--------------------------------------------------------------------

void CMapFrame::MakeToolBar()
{
#define TOOLCOUNT  13

    wxToolBar  * toolBar;
    wxBitmap     toolBarBitmap[TOOLCOUNT];
    wxString     toolTip      [TOOLCOUNT];
    int          i;


    // if changing order, change tool_xxx constants in consts_ah.h accordingly
    toolBarBitmap[ 0] = wxBITMAP(zoomin         );     toolTip[ 0] = wxT("Zoom In")            ;
    toolBarBitmap[ 1] = wxBITMAP(zoomout        );     toolTip[ 1] = wxT("Zoom Out")           ;
    toolBarBitmap[ 2] = wxBITMAP(centerout      );     toolTip[ 2] = wxT("Zoom Out and Center");
    toolBarBitmap[ 3] = wxBITMAP(prevzoom       );     toolTip[ 3] = wxT("Prev Zoom Level")    ;
    toolBarBitmap[ 4] = wxBITMAP(showcoord      );     toolTip[ 4] = wxT("Coordinates")        ;
    toolBarBitmap[ 5] = wxBITMAP(shownames      );     toolTip[ 5] = wxT("Town Names")         ;
    toolBarBitmap[ 6] = wxBITMAP(planeup        );     toolTip[ 6] = wxT("Plane Up")           ;
    toolBarBitmap[ 7] = wxBITMAP(planedwn       );     toolTip[ 7] = wxT("Plane Down")         ;
    toolBarBitmap[ 8] = wxBITMAP(lastvisitturn  );     toolTip[ 8] = wxT("Last Turn Visited")  ;
    toolBarBitmap[ 9] = wxBITMAP(prevturn       );     toolTip[ 9] = wxT("Prev Turn")          ;
    toolBarBitmap[10] = wxBITMAP(nextturn       );     toolTip[10] = wxT("Next Turn")          ;
    toolBarBitmap[11] = wxBITMAP(lastturn       );     toolTip[11] = wxT("Last Turn")          ;
    toolBarBitmap[12] = wxBITMAP(findhex        );     toolTip[12] = wxT("Find Hexes")         ;

    toolBar = CreateToolBar(wxNO_BORDER | wxTB_FLAT | wxTB_VERTICAL);

    for (i=0; i<TOOLCOUNT; i++) {
        toolBar->AddTool(tool_zoomin+i, 
                         wxEmptyString, 
                         toolBarBitmap[i], wxNullBitmap,
                         wxITEM_NORMAL,
                         toolTip[i], wxEmptyString, nullptr);
    }

    toolBar->Realize();
    toolBar->SetRows(TOOLCOUNT+1);
}


//--------------------------------------------------------------------

const char * CMapFrame::GetConfigSection(int layout)
{
    switch (layout)
    {
    case AH_LAYOUT_2_WIN:        return SZ_SECT_WND_MAP_2_WIN;
    case AH_LAYOUT_3_WIN:        return SZ_SECT_WND_MAP_3_WIN;
    case AH_LAYOUT_1_WIN:        return SZ_SECT_WND_MAP_1_WIN;
    default:                     return "";
    }
}


//--------------------------------------------------------------------

void CMapFrame::Init(int layout, const char * szConfigSection)
{
    const char        * szConfigSectionHdr;

    szConfigSection    = GetConfigSection(layout);
    szConfigSectionHdr = gpUIController->GetListColSection(SZ_SECT_LIST_COL_UNIT, SZ_KEY_LIS_COL_UNITS_HEX);
    CAhFrame::Init(layout, szConfigSection);

    switch (layout)
    {
    case AH_LAYOUT_1_WIN:
        {
            CMapPane          * p1;
            CUnitPane         * p2;
            CEditPane         * p3;
            CEditPane         * p4;
            CEditPane         * p5;
            CEditPane         * p6;
            int                 x;

            m_Splitter = new wxSplitterWindow(this       , -1, wxDefaultPosition, wxDefaultSize, wxSP_3D     | wxCLIP_CHILDREN);
            m_Splitter1= new wxSplitterWindow(m_Splitter , -1, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxCLIP_CHILDREN);
            m_Splitter2= new wxSplitterWindow(m_Splitter1, -1, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxCLIP_CHILDREN);
            m_Splitter3= new wxSplitterWindow(m_Splitter2, -1, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxCLIP_CHILDREN);
            m_Splitter4= new wxSplitterWindow(m_Splitter3, -1, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxCLIP_CHILDREN);

            p1 = new CMapPane (m_Splitter1, -1, layout  );
            p2 = new CUnitPane(m_Splitter);
            p3 = new CEditPane(m_Splitter2, "Hex description"        , false, FONT_EDIT_DESCR);
            p4 = new CEditPane(m_Splitter3, "Unit description"       , false, FONT_EDIT_DESCR);
            p5 = new CEditPane(m_Splitter4, "Orders"                 , false, FONT_EDIT_ORDER);
            p6 = new CEditPane(m_Splitter4, "Comments/Default orders", true , FONT_EDIT_ORDER);

            SetPane(AH_PANE_MAP          , p1);
            SetPane(AH_PANE_UNITS_HEX    , p2);
            SetPane(AH_PANE_MAP_DESCR    , p3);
            SetPane(AH_PANE_UNIT_DESCR   , p4);
            SetPane(AH_PANE_UNIT_COMMANDS, p5);
            SetPane(AH_PANE_UNIT_COMMENTS, p6);

            p2->Init(this, szConfigSection, szConfigSectionHdr);
            p1->Init(this);
            p3->Init();
            p4->Init();
            p5->Init();
            p6->Init();



            m_Splitter1->SetBorderSize(0);
            m_Splitter2->SetBorderSize(0);
            m_Splitter3->SetBorderSize(0);
            m_Splitter4->SetBorderSize(0);

            m_Splitter ->SetMinimumPaneSize(2);
            m_Splitter1->SetMinimumPaneSize(2);
            m_Splitter2->SetMinimumPaneSize(2);
            m_Splitter3->SetMinimumPaneSize(2);
            m_Splitter4->SetMinimumPaneSize(2);

            x = atol(gpConfigManager->GetConfig(szConfigSection, SZ_KEY_HEIGHT_0));
            m_Splitter->SplitHorizontally(m_Splitter1, p2, x);

            x  = atol(gpConfigManager->GetConfig(szConfigSection, SZ_KEY_WIDTH_0));
            m_Splitter1->SplitVertically(p1, m_Splitter2, x);

            x  = atol(gpConfigManager->GetConfig(szConfigSection, SZ_KEY_HEIGHT_1));
            m_Splitter2->SplitHorizontally(p3, m_Splitter3, x);

            x  = atol(gpConfigManager->GetConfig(szConfigSection, SZ_KEY_HEIGHT_2));
            m_Splitter3->SplitHorizontally(p4, m_Splitter4, x);


            x  = atol(gpConfigManager->GetConfig(szConfigSection, SZ_KEY_WIDTH_1));
            m_Splitter4->SplitVertically(p5, p6, x);

            break;
        }

    case AH_LAYOUT_2_WIN:
        {
            CMapPane          * p1;
            CEditPane         * p2;
            long                y0;

            y0  = atol(gpConfigManager->GetConfig(szConfigSection, SZ_KEY_HEIGHT_0));
            m_Splitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize,
                                              wxSP_3D | wxCLIP_CHILDREN);

            p1 = new CMapPane (m_Splitter, -1, layout  );
            p2 = new CEditPane(m_Splitter, nullptr, false, FONT_EDIT_DESCR);

            SetPane(AH_PANE_MAP       , p1);
            SetPane(AH_PANE_MAP_DESCR , p2);

            p1->Init(this);
            p2->Init();

            m_Splitter->SetMinimumPaneSize(2);
            m_Splitter->SplitHorizontally(p1, p2, y0);
            break;
        }

    case AH_LAYOUT_3_WIN:
        {
            CMapPane          * p1;
            p1 = new CMapPane (this, -1, layout  );
            SetPane(AH_PANE_MAP, p1);
            p1->Init(this);
            break;
        }
    }

}


//--------------------------------------------------------------------

void CMapFrame::Done(bool SetClosedFlag)
{
    CMapPane    * pMapPane;
    CUnitPane         * pUnitPane;

    switch (m_Layout)
    {
    case AH_LAYOUT_1_WIN:
        gpConfigManager->SetConfig(m_sConfigSection.c_str(), SZ_KEY_HEIGHT_0, m_Splitter ->GetSashPosition());
        gpConfigManager->SetConfig(m_sConfigSection.c_str(), SZ_KEY_WIDTH_0 , m_Splitter1->GetSashPosition());
        gpConfigManager->SetConfig(m_sConfigSection.c_str(), SZ_KEY_HEIGHT_1, m_Splitter2->GetSashPosition());
        gpConfigManager->SetConfig(m_sConfigSection.c_str(), SZ_KEY_HEIGHT_2, m_Splitter3->GetSashPosition());
        gpConfigManager->SetConfig(m_sConfigSection.c_str(), SZ_KEY_WIDTH_1 , m_Splitter4->GetSashPosition());

        pUnitPane  = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
        if (pUnitPane)
            pUnitPane->Done();

        break;

    case AH_LAYOUT_2_WIN:
        gpConfigManager->SetConfig(m_sConfigSection.c_str(), SZ_KEY_HEIGHT_0, m_Splitter->GetSashPosition());
        break;

    case AH_LAYOUT_3_WIN:
        break;
    }

    pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    if (pMapPane)
        pMapPane->Done();

    CAhFrame::Done(SetClosedFlag);
}

//--------------------------------------------------------------------

void CMapFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}


//--------------------------------------------------------------------

void CMapFrame::OnQuitNoSave(wxCommandEvent& WXUNUSED(event))
{
    gpUIController->m_DiscardChanges = true;

    // true is to force the frame to close
    Close(true);
}

//--------------------------------------------------------------------

void CMapFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    wxString sPyVersion;

#ifdef HAVE_PYTHON
    sPyVersion = wxString::FromUTF8(Py_GetVersion());
#else
    sPyVersion = wxT("is not supported");
#endif

    msg.Printf(wxT("Atlantis Little Helper version %s, build %d.\n\n%s.\n\nPython %s.\n\n"
               "Please read doc/readme.html for more information."),
               wxT(AH_VERSION),
               AH_BuildNumber,
               wxVERSION_STRING,
               sPyVersion.c_str() );


    wxMessageBox(msg, wxT("About"), wxOK | wxICON_INFORMATION, this);
}

//--------------------------------------------------------------------

void CMapFrame::OnToolbarCmd(wxCommandEvent& event)
{
    if (gpUIController->m_Panes[AH_PANE_MAP])
        ((CMapPane*)gpUIController->m_Panes[AH_PANE_MAP])->OnToolbarCmd(event);
}

//--------------------------------------------------------------------

void CMapFrame::OnToolbarUpdate(wxUpdateUIEvent& event)
{
    bool Ok = false;
    if (gpUIController->m_Panes[AH_PANE_MAP])
        Ok = ((CMapPane*)gpUIController->m_Panes[AH_PANE_MAP])->IsToolActive(event);

    event.Enable(false!=Ok); // otherwise that bitch VC++, well, bitches
}

//--------------------------------------------------------------------

void CMapFrame::OnViewBattlesAllUpdate(wxUpdateUIEvent& event)
{
    event.Enable(gpGameData->m_pAtlantis->m_Battles.Count()>0);
}


//--------------------------------------------------------------------

void CMapFrame::OnViewSkillsAllUpdate(wxUpdateUIEvent& event)
{
    const char  * szName;
    const char  * szValue;
    int           sectidx;

    sectidx = gpConfigManager->GetSectionFirst(SZ_SECT_SKILLS, szName, szValue);
    event.Enable(sectidx>=0);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewSkillsNewUpdate(wxUpdateUIEvent& event)
{
    event.Enable(gpGameData->m_pAtlantis->m_Skills.Count()>0);
}


//--------------------------------------------------------------------

void CMapFrame::OnViewItemsAllUpdate(wxUpdateUIEvent& event)
{
    const char  * szName;
    const char  * szValue;
    int           sectidx;

    sectidx = gpConfigManager->GetSectionFirst(SZ_SECT_ITEMS, szName, szValue);
    event.Enable(sectidx>=0);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewItemsNewUpdate(wxUpdateUIEvent& event)
{
    event.Enable(gpGameData->m_pAtlantis->m_Items.Count()>0);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewObjectsAllUpdate(wxUpdateUIEvent& event)
{
    const char  * szName;
    const char  * szValue;
    int           sectidx;

    sectidx = gpConfigManager->GetSectionFirst(SZ_SECT_OBJECTS, szName, szValue);
    event.Enable(sectidx>=0);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewObjectsNewUpdate(wxUpdateUIEvent& event)
{
    event.Enable(gpGameData->m_pAtlantis->m_Objects.Count()>0);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewEventsUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!gpGameData->m_pAtlantis->m_Events.Description.empty());
}

//--------------------------------------------------------------------

void CMapFrame::OnViewSecurityEventsUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!gpGameData->m_pAtlantis->m_SecurityEvents.Description.empty());
}

//--------------------------------------------------------------------

void CMapFrame::OnViewNewProductsUpdate(wxUpdateUIEvent& event)
{
    event.Enable(gpGameData->m_pAtlantis->m_NewProducts.Count() > 0);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewErrorsUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!gpGameData->m_pAtlantis->m_Errors.Description.empty());
}

//--------------------------------------------------------------------

void CMapFrame::OnViewGatesUpdate(wxUpdateUIEvent& event)
{
    event.Enable(gpGameData->m_pAtlantis->m_Gates.Count()>0);
}

//--------------------------------------------------------------------

void CMapFrame::OnCloseWindow(wxCloseEvent& event)
{
    // This is the cental shutdown point!
    if (gpUIController->CanCloseApp())
    {
        gpUIController->FrameClosing(this);
        Destroy();
    }
    else
        wxMessageBox(wxT("Can not save orders or save cancelled."));
}

//--------------------------------------------------------------------

void CMapFrame::OnLoadReport(wxCommandEvent& WXUNUSED(event))
{
    gpReportLoader->LoadReport(false);
}

//--------------------------------------------------------------------

void CMapFrame::OnJoinReport(wxCommandEvent& WXUNUSED(event))
{
    gpReportLoader->LoadReport(true);
}

//--------------------------------------------------------------------

void CMapFrame::OnLoadOrders(wxCommandEvent& WXUNUSED(event))
{
    gpReportLoader->LoadOrders();
}

//--------------------------------------------------------------------

void CMapFrame::OnSaveOrdersAs(wxCommandEvent& WXUNUSED(event))
{
    gpReportLoader->SaveOrders(false);
}

//--------------------------------------------------------------------

void CMapFrame::OnOptions(wxCommandEvent& event)
{
    gpUIController->OpenOptionsDlg();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewSkillsAll(wxCommandEvent& event)
{
    gpApp->ViewShortNamedObjects(true, SZ_SECT_SKILLS, "Skills", gpGameData->m_pAtlantis->m_Skills);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewSkillsNew(wxCommandEvent& event)
{
    gpApp->ViewShortNamedObjects(false, SZ_SECT_SKILLS, "Skills", gpGameData->m_pAtlantis->m_Skills);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewItemsAll(wxCommandEvent& event)
{
    gpApp->ViewShortNamedObjects(true, SZ_SECT_ITEMS, "Items", gpGameData->m_pAtlantis->m_Items);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewItemsNew(wxCommandEvent& event)
{
    gpApp->ViewShortNamedObjects(false, SZ_SECT_ITEMS, "Items", gpGameData->m_pAtlantis->m_Items);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewObjectsAll(wxCommandEvent& event)
{
    gpApp->ViewShortNamedObjects(true, SZ_SECT_OBJECTS, "Objects", gpGameData->m_pAtlantis->m_Objects);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewObjectsNew(wxCommandEvent& event)
{
    gpApp->ViewShortNamedObjects(false, SZ_SECT_OBJECTS, "Objects", gpGameData->m_pAtlantis->m_Objects);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewBattlesAll(wxCommandEvent& event)
{
    gpApp->ViewBattlesAll();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewEvents(wxCommandEvent& event)
{
    gpApp->ViewEvents(true);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewSecurityEvents(wxCommandEvent& event)
{
    gpApp->ViewSecurityEvents();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewNewProducts(wxCommandEvent& event)
{
    gpApp->ViewNewProducts();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewErrors(wxCommandEvent& event)
{
    gpApp->ViewEvents(false);
}

//--------------------------------------------------------------------

void CMapFrame::OnViewGates(wxCommandEvent& event)
{
    gpApp->ViewGates();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewCities(wxCommandEvent& event)
{
    gpApp->ViewCities();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewProvinces(wxCommandEvent& event)
{
    gpApp->ViewProvinces();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewFactionInfo(wxCommandEvent& event)
{
    gpApp->ViewFactionInfo();
}

//--------------------------------------------------------------------

void CMapFrame::OnViewFactionOverview(wxCommandEvent& event)
{
    gpApp->ViewFactionOverview();
}

//--------------------------------------------------------------------

void CMapFrame::OnWindowUnits(wxCommandEvent& event)
{
    gpUIController->OpenUnitFrame();
}

//--------------------------------------------------------------------

void CMapFrame::OnWindowUnitsFltr(wxCommandEvent& event)
{
    gpUIController->OpenUnitFrameFltr(true);
}

//--------------------------------------------------------------------

void CMapFrame::OnWindowMessages(wxCommandEvent& event)
{
    gpUIController->OpenMsgFrame();
}

//--------------------------------------------------------------------

void CMapFrame::OnWindowEditors(wxCommandEvent& event)
{
    gpUIController->OpenEditsFrame();
}

//--------------------------------------------------------------------

void CMapFrame::OnApplyDefaultOrders(wxCommandEvent& event)
{
    gpReportLoader->SetOrdersChanged(gpGameData->m_pAtlantis->ApplyDefaultOrders(true) //(bool)atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_DEFAULT_EMPTY_ONLY)))
                            || gpReportLoader->GetOrdersChanged());

    CUnitPane * p = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
    if (p)
        p->Update(p->m_pCurLand);

}

//--------------------------------------------------------------------

void CMapFrame::OnRerunOrders(wxCommandEvent& event)
{
    gpReportLoader->RerunOrders();
}

//--------------------------------------------------------------------

void CMapFrame::OnWriteMagesCSV(wxCommandEvent& event)
{
    gpApp->WriteMagesCSV();
}

//--------------------------------------------------------------------

void CMapFrame::OnCheckMonthLongOrd(wxCommandEvent& event)
{
    gpApp->CheckMonthLongOrders();
}

//--------------------------------------------------------------------

void CMapFrame::OnCheckTaxTrade(wxCommandEvent& event)
{
    gpApp->CheckTaxTrade();
}

//--------------------------------------------------------------------

void CMapFrame::OnCheckProduction(wxCommandEvent& event)
{
    gpApp->CheckProduction();
}

//--------------------------------------------------------------------

void CMapFrame::OnCheckSailing(wxCommandEvent& event)
{
    gpApp->CheckSailing();
}


//--------------------------------------------------------------------

void CMapFrame::OnFindHexes(wxCommandEvent& event)
{
    CMapPane * pMapPane;

    pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    if (pMapPane)
        pMapPane->FindHexes();

    //gpApp->FindHexes();
}

//--------------------------------------------------------------------

void CMapFrame::OnExportHexes(wxCommandEvent& event)
{
    gpApp->ExportHexes();
}

//--------------------------------------------------------------------

void CMapFrame::OnFindTradeRoutes(wxCommandEvent& event)
{
    gpApp->FindTradeRoutes();
    
    CMapPane * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    if (pMapPane)
    {
        pMapPane->RemoveRectangle();
        pMapPane->Refresh(false, nullptr);
    }
}

//--------------------------------------------------------------------

void CMapFrame::OnListCol(wxCommandEvent& event)
{
    gpUIController->EditListColumns(event.GetId());
}

//--------------------------------------------------------------------

void CMapFrame::OnFlagNames(wxCommandEvent& event)
{
    CUnitFlagsDlg dlg(this, eNames, 0);

    int rc = dlg.ShowModal();
    if (wxID_OK==rc)
    {
    }
}

//--------------------------------------------------------------------

void CMapFrame::OnFlagsAllSet(wxCommandEvent& event)
{
    gpReportLoader->SetAllLandUnitFlags();
}

//--------------------------------------------------------------------

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

#include "wx/listctrl.h"

#include "string_utils.h"
#include "cfgfile.h"
#include <set>
#include <vector>
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"

#include "ahapp.h"
#include "ahframe.h"
#include "listpane.h"
#include "unitpane.h"
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "editpane.h"
#include "mappane.h"
#include "extend.h"
#include "utildlgs.h"


BEGIN_EVENT_TABLE(CUnitPaneFltr, wxListCtrl)
    EVT_LIST_ITEM_SELECTED   (list_units_hex_fltr, CUnitPaneFltr::OnSelected)
    EVT_LIST_COL_CLICK       (list_units_hex_fltr, CUnitPaneFltr::OnColClicked)
    EVT_LIST_ITEM_RIGHT_CLICK(list_units_hex_fltr, CUnitPaneFltr::OnRClick)
    EVT_IDLE                 (CUnitPaneFltr::OnIdle)

    EVT_MENU             (menu_Popup_Filter ,  CUnitPaneFltr::OnPopupMenuFilter   )
    EVT_MENU             (menu_Popup_SetSort,  CUnitPaneFltr::OnPopupMenuSetSort   )

    EVT_MENU             (menu_Popup_AddToTracking , CUnitPane::OnPopupMenuAddUnitToTracking)
    EVT_MENU             (menu_Popup_UnitFlags     , CUnitPane::OnPopupMenuUnitFlags      )
    EVT_MENU             (menu_Popup_IssueOrders   , CUnitPaneFltr::OnPopupMenuIssueOrders    )


END_EVENT_TABLE()



//--------------------------------------------------------------------------

CUnitPaneFltr::CUnitPaneFltr(wxWindow *parent, wxWindowID id)
              :CUnitPane(parent, id), m_NewUnits(32)
{
    m_ColClickedFltr = -1;
    m_IsUpdating     = false;
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::Done()
{
    CMapPane       * pMapPane = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];

    CUnitPane::Done();
    m_NewUnits.FreeAll();

    if (pMapPane)
    {
        ClearLandFlags();
        pMapPane->Refresh(false);
    }
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::Update(CUnitFilterDlg * pFilter)
{
    std::string             Property[UNIT_SIMPLE_FLTR_COUNT];
    std::string             Compare [UNIT_SIMPLE_FLTR_COUNT];
    std::string             sValue  [UNIT_SIMPLE_FLTR_COUNT];
    long             lValue  [UNIT_SIMPLE_FLTR_COUNT];
    int              i,k;
    std::string             ConfigKey;
    eCompareOp       CompareOp[UNIT_SIMPLE_FLTR_COUNT];
    CUnit          * pUnit, * pPrevUnit;
    bool             ok;
    std::string             TrackingGroup;
    std::set<long>   Tracking;
    const char     * p;
    std::string             S;
    bool             bUsePython = false;
    CPythonEmbedder  Python(gpApp->m_pAtlantis.get());
    eEErr            rcPy = E_OK;
    std::string             sPythonText, sRealPythonText;
    std::string             sConfSect;
    CBaseColl        Hexes(64);
    CLand          * pLand, * pPrevLand;
    int              unitidx;
    bool             Selected = false;
    bool             ShowOnMap= false;
    CMapPane       * pMapPane = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
    int              nl;
    bool             ShowGoneUnits = false;
    CAtlaParser    * pPrevTurn = nullptr;

    sConfSect = gpApp->GetConfig(m_sConfigSection.c_str(), SZ_KEY_FLTR_SET);
    if (pFilter)
    {
        ShowOnMap     = pFilter->m_chDisplayOnMap->IsChecked();
        ShowGoneUnits = pFilter->m_bFindGoneUnits;
        Selected      = pFilter->m_chUseSelectedHexes->IsChecked();
    }

    // read boxes values
    for (i=0; i<UNIT_SIMPLE_FLTR_COUNT; i++)
    {
        if (pFilter)
        {
            Property[i] = pFilter->m_cbProperty[i]->GetValue().mb_str();
            Compare [i] = pFilter->m_cbCompare [i]->GetValue().mb_str();
            sValue  [i] = pFilter->m_tcValue   [i]->GetValue().mb_str();
        }
        else
        {
            Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_PROPERTY, i);  Property[i] = gpApp->GetConfig(sConfSect.c_str(), ConfigKey.c_str());
            Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_COMPARE , i);  Compare [i] = gpApp->GetConfig(sConfSect.c_str(), ConfigKey.c_str());
            Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_VALUE   , i);  sValue  [i] = gpApp->GetConfig(sConfSect.c_str(), ConfigKey.c_str());
        }

        TrimRight(Property[i], TRIM_ALL);    TrimLeft(Property[i], TRIM_ALL);
        TrimRight(Compare[i], TRIM_ALL);    TrimLeft(Compare[i], TRIM_ALL);
        TrimRight(sValue[i], TRIM_ALL);    TrimLeft(sValue[i], TRIM_ALL);

        CompareOp[i] = NOP;
        for (k=GT; k<NOP; k++)
            if (0==stricmp(UNIT_FILTER_OPERATION[k], Compare[i].c_str()))
            {
                CompareOp[i] = (eCompareOp)k;
                break;
            }
        lValue[i] = atol(sValue[i].c_str());
    }

    // prepare tracking group
    if (pFilter)
    {
        TrackingGroup = pFilter->m_TrackingGroup;
        gpApp->SetConfig(m_sConfigSection.c_str(), SZ_KEY_UNIT_FLTR_TRACKING, TrackingGroup.c_str());
    }
    else
        TrackingGroup = gpApp->GetConfig(m_sConfigSection.c_str(), SZ_KEY_UNIT_FLTR_TRACKING);

    TrimRight(TrackingGroup, TRIM_ALL);
    if (!TrackingGroup.empty())
    {
        p = gpApp->GetConfig(SZ_SECT_UNIT_TRACKING, TrackingGroup.c_str());
        while (p && *p)
        {
            p = GetToken(S, p, ',');
            Tracking.insert(atol(S.c_str()));
        }
    }

    // should we run Python?
    if (!ShowGoneUnits && TrackingGroup.empty())
    {
        if (pFilter)
        {
            bUsePython = TrackingGroup.empty() && pFilter->m_rbUsePython->GetValue();
            sPythonText= pFilter->m_tcFilterText->GetValue().mb_str();
        }
        else
        {
            S = gpApp->GetConfig(sConfSect.c_str(), SZ_KEY_UNIT_FLTR_SOURCE);
            bUsePython = (0==stricmp(S.c_str(), SZ_KEY_UNIT_FLTR_SOURCE_PYTHON));
            sPythonText= gpApp->GetConfig(sConfSect.c_str(), SZ_KEY_UNIT_FLTR_PYTHON_CODE);
        }
    }

    if (bUsePython)
    {
        rcPy = Python.InitUnitFilter(sPythonText.c_str(), sRealPythonText);
        if (E_OK != rcPy)
            goto Failed;
    }

    // and check all the units
    InsertUnitInit();

    if (ShowGoneUnits)
    {
        if (!gpApp->GetPrevTurnReport(pPrevTurn))
            goto Failed;
        
        CUnitPane * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
        if (!pUnitPane)
            goto Failed;
            
        if (Selected)
            pMapPane->GetSelectedOrAllHexes(Hexes, Selected);
        else
        {
            pLand = pUnitPane->m_pCurLand;
            if (!pLand)
                goto Failed;
            Hexes.Insert(pLand);
        }
            
        for (nl=0; nl<Hexes.Count(); nl++)
        {
            pLand = (CLand*)Hexes.At(nl);
            pPrevLand = pPrevTurn->GetLand(pLand->Id);
            if (!pPrevLand)
                goto Failed;
                
            for (unitidx=0; unitidx<pPrevLand->Units.Count(); unitidx++)
            {
                ok    = false;
                pUnit = nullptr;
                pPrevUnit = (CUnit*)pPrevLand->Units.At(unitidx);
                if (pLand->Units.Search(pPrevUnit, i))
                    pUnit = (CUnit*)pLand->Units.At(i);
                if (!pUnit) // that's the one!
                {
                    if (gpApp->m_pAtlantis->m_Units.Search(pPrevUnit, i))
                        pUnit = (CUnit*)gpApp->m_pAtlantis->m_Units.At(i);
                    ok = true;
                }
                if (!pUnit)
                {
                    // it is nowhere to be seen!  make new one
                    pUnit = pPrevUnit->AllocSimpleCopy();
                    InsStr(pUnit->Name, "-=DISAPPEARED=- ", 0);
                    InsStr(pUnit->Description, "-=DISAPPEARED=- ", 0);
                    pUnit->Flags |= UNIT_FLAG_TEMP;
                }
    
                if (ok)
                {
                    InsertUnit(pUnit);
                    if (ShowOnMap)
                    {
                        CLand * pUnitLand = gpApp->m_pAtlantis->GetLand(pUnit->LandId);
                        if (pUnitLand)
                            pUnitLand->Flags |=  LAND_LOCATED_UNITS;
                    }
                }
            }
        }
    }
    else
    {
        pMapPane->GetSelectedOrAllHexes(Hexes, Selected);
        for (nl=0; nl<Hexes.Count(); nl++)
        {
            pLand = (CLand*)Hexes.At(nl);
            for (unitidx=0; unitidx<pLand->Units.Count(); unitidx++)
            {
                pUnit = (CUnit*)pLand->Units.At(unitidx);
                ok    = true;
    
                if (!TrackingGroup.empty())
                    ok = (Tracking.count(pUnit->Id) > 0);
                else if (bUsePython)
                {
                    rcPy = Python.RunUnitFilter(pUnit, ok);
                    if (E_OK != rcPy)
                        break; // execution must never fail!
                }
                else
                    ok = EvaluateBaseObjectByBoxes(pUnit, Property, CompareOp, sValue, lValue, UNIT_SIMPLE_FLTR_COUNT);
    
                if (ok)
                {
                    InsertUnit(pUnit);
                    if (ShowOnMap)
                        pLand->Flags |=  LAND_LOCATED_UNITS;
                }
            }
        }
    }

    Hexes.DeleteAll();
    InsertUnitDone();
    pMapPane->RemoveRectangle();
    pMapPane->Refresh(false);


Failed:

    if (bUsePython)
    {
        std::string sOut, sErr;

        Python.DoneUnitFilter();
        gpApp->StdRedirectReadMore(false, sErr);
        gpApp->StdRedirectReadMore(true, sOut);
        if (E_OK != rcPy)
        {
            S.clear();
            S << EOL_SCR << "----------- Python code -------------" << EOL_SCR;
            gpApp->ShowError(S.c_str(), S.size(), true);
            gpApp->ShowError(sRealPythonText.c_str(), sRealPythonText.size(), true);
            S.clear();
            S << EOL_SCR << "------------------------" << EOL_SCR << EOL_SCR;
            gpApp->ShowError(S.c_str(), S.size(), true);

            gpApp->ShowError(sErr.c_str(), sErr.size(), true);
            gpApp->ShowError(sOut.c_str(), sOut.size(), true);
        }
    }
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::ClearLandFlags()
{
    int      np,nl;
    CPlane * pPlane;
    CLand  * pLand;

    for (np=0; np<gpApp->m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)gpApp->m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            pLand->Flags = pLand->Flags & (~(unsigned long)LAND_LOCATED_UNITS);
        }
    }
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::InsertUnitInit()
{
    m_IsUpdating = true;
    m_pUnits->DeleteAll();
    m_NewUnits.FreeAll();

    // clear all the land flags
    ClearLandFlags();
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::InsertUnit(CUnit * pUnit)
{
    if (IS_NEW_UNIT(pUnit))
    {
        pUnit = pUnit->AllocSimpleCopy();
        m_NewUnits.Insert(pUnit); // Store copy.
    }
    if (pUnit->Flags & UNIT_FLAG_TEMP)
        m_NewUnits.Insert(pUnit); // this is temp unit, we will need to kill it later

    m_pUnits->AtInsert(m_pUnits->Count(), pUnit);
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::InsertUnitDone()
{
    m_pUnits->SetSortMode(m_SortKey, 3);
    SetData(sel_by_no, 0, true);
    m_IsUpdating = false;
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::OnSelected(wxListEvent& event)
{
    if (m_IsUpdating)
        return;
    CUnit       * pUnit     = GetUnit(event.m_itemIndex);

    gpApp->SelectUnit(pUnit);
}


//--------------------------------------------------------------------------

void CUnitPaneFltr::OnColClicked(wxListEvent& event)
{
    wxMenu  menu;

    m_ColClickedFltr = event.m_col;
    menu.Append(menu_Popup_SetSort  , wxT("Set sort order"));
    menu.Append(menu_Popup_Filter   , wxT("Filter"        ));

    PopupMenu( &menu, event.GetPoint().x, event.GetPoint().y);
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::OnIdle(wxIdleEvent& event)
{
    CUnitPane::OnIdle(event);
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::OnPopupMenuSetSort  (wxCommandEvent& event)
{
    m_ColClicked = m_ColClickedFltr;
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::OnRClick(wxListEvent& event)
{
    wxMenu  menu;

    menu.Append(menu_Popup_Filter  , wxT("Filter")        );

    if (GetSelectedItemCount()>1)
    {
        // multiple units
        menu.Append(menu_Popup_IssueOrders     , wxT("Issue orders"));
        menu.Append(menu_Popup_UnitFlags       , wxT("Set custom flags")    );
        menu.Append(menu_Popup_AddToTracking   , wxT("Add to a tracking group"));
    }


    PopupMenu( &menu, event.GetPoint().x, event.GetPoint().y);
}

//--------------------------------------------------------------------------

void CUnitPaneFltr::OnPopupMenuFilter  (wxCommandEvent& event)
{
    CUnitFilterDlg dlg(m_pParent, m_sConfigSection.c_str());

    if (wxID_OK == dlg.ShowModal())
        Update(&dlg);
    else
    {
        CMapPane * pMapPane = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
        pMapPane->RemoveRectangle();
        pMapPane->Refresh(false);
    }
}

//--------------------------------------------------------------------------


void CUnitPaneFltr::OnPopupMenuIssueOrders(wxCommandEvent& event)
{
    long           idx;
    CUnit        * pUnit;
    CEditPane    * pOrders;
    CGetTextDlg    dlg(this, "Order", "Orders for the selected units");
    std::set<long>   LandIds;
    CLand        * pLand;
    CUnitPane    * pUnitPane;

    if (wxID_OK != dlg.ShowModal())
        return;
    TrimRight(dlg.m_Text, TRIM_ALL);
    if (dlg.m_Text.empty())
        return;


    pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
    if (pOrders)
        pOrders->SaveModifications();

    idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    while (idx>=0)
    {
        pUnit = GetUnit(idx);
        if (pUnit->IsOurs)
        {
            TrimRight(pUnit->Orders, TRIM_ALL);
            if (!pUnit->Orders.empty())
                pUnit->Orders << EOL_SCR;
            pUnit->Orders << dlg.m_Text;
            LandIds.insert(pUnit->LandId);
        }
        idx   = GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    if (!LandIds.empty())
    {
        gpApp->SetOrdersChanged(true);
/*        gpApp->m_pAtlantis->RunOrders(m_pCurLand);
        Update(m_pCurLand);*/
        for (long landId : LandIds)
        {
            pLand = gpApp->m_pAtlantis->GetLand(landId);
            gpApp->m_pAtlantis->RunOrders(pLand);
        }
        pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
        if (pUnitPane)
            pUnitPane->Update(pUnitPane->m_pCurLand);
    }
}

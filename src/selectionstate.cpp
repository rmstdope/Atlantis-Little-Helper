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

#include "selectionstate.h"

#include "string_utils.h"
#include "consts.h"
#include "consts_ah.h"
#include "gamedatamanager.h"
#include "uicontroller.h"
#include "ahapp.h"
#include "ahframe.h"
#include "mapframe.h"
#include "editpane.h"
#include "mappane.h"
#include "listpane.h"
#include "unitpane.h"

SelectionState * gpSelectionState = nullptr;

//=========================================================================

SelectionState::SelectionState()
{
    m_HexDescrSrc.reserve(128);
    m_UnitDescrSrc.reserve(128);
    m_SelUnitIdx = -1;
}

//-------------------------------------------------------------------------

CUnit * SelectionState::GetSelectedUnit()
{
    CUnit       * pUnit = nullptr;
    CUnitPane   * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];

    if (pUnitPane)
        pUnit = (CUnit*)pUnitPane->m_pUnits->At(m_SelUnitIdx);

    return pUnit;
}

//-------------------------------------------------------------------------

void SelectionState::RedrawTracks()
{
    CUnit       * pUnit = GetSelectedUnit();
    CPlane      * pPlane;
    CMapPane    * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];

    if (!pMapPane)
        return;

    pPlane   = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);
    pMapPane->RedrawTracksForUnit(pPlane, pUnit, nullptr, true);
}

//-------------------------------------------------------------------------

void SelectionState::SelectTempUnit(CUnit * pUnit)
{
    CEditPane   * pDescription = (CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_DESCR   ];
    CEditPane   * pOrders      = (CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_COMMANDS];
    CEditPane   * pComments    = (CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_COMMENTS];

    OnUnitHexSelectionChange(-1); // unselect
    m_UnitDescrSrc.clear();

    if (pUnit)
        m_UnitDescrSrc = pUnit->Description;

    if (pDescription)
        pDescription->SetSource(&m_UnitDescrSrc, nullptr);
    if (pOrders)
    {
        pOrders->SetSource(nullptr, nullptr);
        pOrders->SetReadOnly ( true );
        pOrders->ApplyFonts();
    }
    if (pComments)
    {
        pComments->SetSource(nullptr, nullptr);
//        pComments->SetReadOnly ( true );
    }
}

//-------------------------------------------------------------------------

void SelectionState::SelectUnit(CUnit * pUnit)
{
    CMapPane    * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    CUnitPane   * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
    CLand       * pLand;
    CPlane      * pPlane;
    int           nx, ny, nz;
    bool          refresh;
    bool          NeedSetUnit;

    if (!pUnit || !pMapPane)
        return;
    pLand = gpGameData->m_pAtlantis->GetLand(pUnit->LandId);
    if (!pLand)
        return;

    pLand->guiUnit = pUnit->Id;

    LandIdToCoord(pLand->Id, nx, ny, nz);
    pPlane   = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(nz);

    refresh = pMapPane->EnsureLandVisible(nx, ny, nz, false);
    if (refresh)
        pMapPane->Refresh(false);

    NeedSetUnit = (pUnitPane && (pLand==pUnitPane->m_pCurLand));

    pMapPane->SetSelection(nx, ny, pUnit, pPlane, true);

    if (pUnit->Flags & UNIT_FLAG_TEMP)
    {
        if (pUnitPane)
            pUnitPane->SelectUnit(-1);
        SelectTempUnit(pUnit);  // just redraw description
    }
    else
        if (NeedSetUnit)
            pUnitPane->SelectUnit(pUnit->Id); // otherwise will be already selected
}

//-------------------------------------------------------------------------

void SelectionState::SelectLand(CLand * pLand)
{
    CMapPane    * pMapPane  = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];
    CUnitPane   * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];
    CPlane      * pPlane;
    int           nx, ny, nz;
    bool          refresh;

    if (pLand && pMapPane)
    {
        LandIdToCoord(pLand->Id, nx, ny, nz);
        pPlane   = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(nz);

        refresh = pMapPane->EnsureLandVisible(nx, ny, nz, true);
        if (refresh)
            pMapPane->Refresh(false);


        if (!pUnitPane || pLand != pUnitPane->m_pCurLand)
            pMapPane->SetSelection(nx, ny, nullptr, pPlane, true);
    }
}

//-------------------------------------------------------------------------

bool SelectionState::SelectLand(const char * landcoords) //  "48,52[,somewhere]"
{
    CLand       * pLand     = gpGameData->m_pAtlantis->GetLand(landcoords);

    if (pLand)
    {
        SelectLand(pLand);
        return true;
    }
    else
        return false;
}

//-------------------------------------------------------------------------

void SelectionState::UpdateHexEditPane(CLand * pLand)
{
    CStruct     * pStruct;
    CEditPane   * pEditPane;
    int           i;
    bool          FlagsEmpty = true;

    m_HexDescrSrc.clear();

    pEditPane = (CEditPane*)gpUIController->m_Panes[AH_PANE_MAP_DESCR];
    if (pEditPane)
    {
        if (pLand)
        {
            m_HexDescrSrc << pLand->Description;

            m_HexDescrSrc << EOL_SCR;
            gpGameData->m_pAtlantis->ComposeProductsLine(pLand, EOL_SCR, m_HexDescrSrc);

            if (pLand->Structs.Count()>0)
            {
                TrimRight(m_HexDescrSrc, TRIM_ALL);
                m_HexDescrSrc << EOL_SCR << "-----------" << EOL_SCR;
                for (i=0; i<pLand->Structs.Count(); i++)
                {
                    pStruct = (CStruct*)pLand->Structs.At(i);
                    m_HexDescrSrc << pStruct->Description;
                    TrimRight(m_HexDescrSrc, TRIM_ALL);
                    if (pStruct->Attr & SA_MOBILE)
                        m_HexDescrSrc << " Load: " << pStruct->Load << ", Power: " << pStruct->SailingPower << ".";
                    m_HexDescrSrc << EOL_SCR;
                }
            }

            for (i=0; i<LAND_FLAG_COUNT; i++)
                if (!pLand->FlagText[i].empty())
                {
                    FlagsEmpty = false;
                    break;
                }


            if (!FlagsEmpty)
            {
                TrimRight(m_HexDescrSrc, TRIM_ALL);
                m_HexDescrSrc << EOL_SCR << "-----------";

                for (i=0; i<LAND_FLAG_COUNT; i++)
                    if (!pLand->FlagText[i].empty())
                        m_HexDescrSrc << EOL_SCR << pLand->FlagText[i];
            }

            if (!pLand->Events.empty() &&
                 0 != stricmp(SkipSpaces(pLand->Events.c_str()), "none")
               )
                m_HexDescrSrc << EOL_SCR << "Events:" << EOL_SCR << pLand->Events << EOL_SCR;
            m_HexDescrSrc << EOL_SCR << "Exits:"  << EOL_SCR << pLand->Exits;
        }
        pEditPane->SetSource(&m_HexDescrSrc, nullptr);
    }
}

//-------------------------------------------------------------------------

void SelectionState::UpdateHexUnitList(CLand * pLand)
{
    CUnitPane   * pUnitPane = (CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX];

    if (pUnitPane)
        pUnitPane->Update(pLand);
}

//-------------------------------------------------------------------------

void SelectionState::OnMapSelectionChange()
{
    CLand       * pLand    = nullptr;
    CMapPane    * pMapPane = (CMapPane* )gpUIController->m_Panes[AH_PANE_MAP];

    if (pMapPane)
        pLand   = gpGameData->m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);

    UpdateHexEditPane(pLand);  // nullptr is Ok!
    UpdateHexUnitList(pLand);
    gpUIController->SetMapFrameTitle();
}

//-------------------------------------------------------------------------

void SelectionState::OnUnitHexSelectionChange(long idx)
{
    // It can be called as a result of selecting a hex on the map!

    // It will be unit in the current hex!

    bool          ReadOnly = true;
    CEditPane   * pDescription;
    CEditPane   * pOrders;
    CEditPane   * pComments;
    CFaction    * pFaction;
    CUnit       * pUnit;


    m_SelUnitIdx = idx;
    pUnit        = GetSelectedUnit(); // depends on m_SelUnitIdx
    pFaction     = pUnit?pUnit->pFaction:nullptr;

    pDescription = (CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_DESCR   ];
    pOrders      = (CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_COMMANDS];
    pComments    = (CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_COMMENTS];

    m_UnitDescrSrc.clear();

    if (pUnit)
    {
        m_UnitDescrSrc = pUnit->Description;
        if (!pUnit->Errors.empty())
            m_UnitDescrSrc << " ***** Errors:\r\n" << pUnit->Errors;
        if (!pUnit->Events.empty())
            m_UnitDescrSrc << " ----- Events:\r\n" << pUnit->Events;

        ReadOnly = (!pUnit->IsOurs || pUnit->Id<=0) ;
    }

    if (!ReadOnly && !gpGameData->m_ReportDates.empty())
        ReadOnly = (gpGameData->m_pAtlantis->m_YearMon != gpGameData->m_ReportDates[(int)gpGameData->m_ReportDates.size()-1] );

    if (pDescription)
        pDescription->SetSource(&m_UnitDescrSrc, nullptr);
    if (pOrders)
    {
        if (pOrders->m_pEditor->IsModified())
        {
            long    Id = 0;
            CLand * pLand = nullptr;

            if (pUnit)
            {
                Id = pUnit->Id;
                pLand = gpGameData->m_pAtlantis->GetLand(pUnit->LandId);

            }

            // OnKillFocus event for the editor did not fire up
            pOrders->OnKillFocus();

            // OnKillFocus kills all new units!
            pUnit = nullptr;
            if (Id != 0 && pLand)
            {
                CBaseObject         Dummy;
                int                 idx;

                Dummy.Id = Id;
                if (pLand->Units.Search(&Dummy, idx))
                {
                    pUnit = (CUnit*)pLand->Units.At(idx);
                    SelectUnit(pUnit);
                }

            }
        }

        pOrders->SetSource(pUnit?&pUnit->Orders:nullptr,      &gpReportLoader->m_OrdersAreChanged);
        pOrders->SetReadOnly ( ReadOnly );
        pOrders->ApplyFonts();
    }
    if (pComments)
    {
        if (pComments->m_pEditor->IsModified())
        {
            // OnKillFocus event for the editor did not fire up
            pComments->OnKillFocus();
        }
        pComments->SetSource(pUnit?&pUnit->DefOrders:nullptr, &gpReportLoader->m_CommentsChanged);
    }

    RedrawTracks();
}

//-------------------------------------------------------------------------

void SelectionState::SelectNextUnit()
{
    if (gpUIController->m_Panes[AH_PANE_UNITS_HEX])
        ((CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX])->SelectNextUnit();
}

//-------------------------------------------------------------------------

void SelectionState::SelectPrevUnit()
{
    if (gpUIController->m_Panes[AH_PANE_UNITS_HEX])
        ((CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX])->SelectPrevUnit();
}

//-------------------------------------------------------------------------

void SelectionState::SelectUnitsPane()
{
    if (gpUIController->m_Panes[AH_PANE_UNITS_HEX])
        ((CUnitPane*)gpUIController->m_Panes[AH_PANE_UNITS_HEX])->SetFocus();
}

//-------------------------------------------------------------------------

void SelectionState::SelectOrdersPane()
{
    if (gpUIController->m_Panes[AH_PANE_UNIT_COMMANDS])
        ((CEditPane*)gpUIController->m_Panes[AH_PANE_UNIT_COMMANDS])->SetFocus();
}

//-------------------------------------------------------------------------

void SelectionState::AddTempHex(int X, int Y, int Plane)
{
    CLand  * pCurLand = gpGameData->m_pAtlantis->GetLand(X, Y, Plane, true);
    if (pCurLand)
        return;

    CPlane * pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(Plane);
    if (!pPlane)
        return;

    assert(Plane == pPlane->Id);

    std::string     sTerrain;
    wxString strTerrain = wxGetTextFromUser(wxT("Terrain"), wxT("Please specify terrain type"));
    sTerrain = strTerrain.mb_str();

    if (sTerrain.empty())
        return;

    CLand * pLand       = new CLand;
    pLand->ExitBits     = 0xFF;
    pLand->Id           = LandCoordToId ( X,Y, pPlane->Id );
    pLand->pPlane       = pPlane;
    pLand->Name         = SZ_MANUAL_HEX_PROVINCE;
    pLand->TerrainType  = sTerrain;
    pLand->Taxable      = 0;
    pLand->Description  << sTerrain << " (" << (long)X << "," << (long)Y << ") in " SZ_MANUAL_HEX_PROVINCE; // ", 0 peasants (unknown), $0.";
    pPlane->Lands.Insert ( pLand );
}

//-------------------------------------------------------------------------

void SelectionState::DelTempHex(int X, int Y, int Plane)
{
    int      idx;
    CLand  * pCurLand = gpGameData->m_pAtlantis->GetLand(X, Y, Plane, true);
    if (!pCurLand)
        return;

    CPlane * pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(Plane);
    if (!pPlane)
        return;

    assert(Plane == pPlane->Id);

    if (pPlane->Lands.Search(pCurLand, idx))
        pPlane->Lands.AtFree(idx);
}

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

#ifndef __SELECTIONSTATE_H_INCL__
#define __SELECTIONSTATE_H_INCL__

#include <string>

class CUnit;
class CLand;

class SelectionState
{
public:
    SelectionState();

    void                 SelectUnit(CUnit * pUnit);
    bool                 SelectLand(const char * landcoords); //  "48,52[,somewhere]"
    void                 SelectLand(CLand * pLand);
    CUnit              * GetSelectedUnit();

    void                 OnMapSelectionChange();
    void                 OnUnitHexSelectionChange(long idx);

    void                 SelectNextUnit();
    void                 SelectPrevUnit();
    void                 SelectUnitsPane();
    void                 SelectOrdersPane();

    void                 SelectTempUnit(CUnit * pUnit);
    void                 AddTempHex(int X, int Y, int Plane);
    void                 DelTempHex(int X, int Y, int Plane);

    // Public (not just SelectionState-internal) because other classes'
    // bridged calls still need to reach these directly: UIController's
    // EditPaneChanged (step 4) and CAhApp's not-yet-extracted LoadOrders
    // (ReportLoader, step 6).
    void                 UpdateHexEditPane(CLand * pLand);
    void                 UpdateHexUnitList(CLand * pLand);
    void                 RedrawTracks();

    long                 m_SelUnitIdx;
    std::string          m_HexDescrSrc;
    std::string          m_UnitDescrSrc;
    std::string          m_MsgSrc;
};

extern SelectionState * gpSelectionState;

#endif

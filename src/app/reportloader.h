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

#ifndef __REPORTLOADER_H_INCL__
#define __REPORTLOADER_H_INCL__

#include <map>
#include <string>

class CAtlaParser;

enum eRepSeq  {repFirst, repPrev, repNext, repLast, repLastVisited};

void EncodeConfigLine(std::string & dest, const char * src);
void DecodeConfigLine(std::string & dest, const char * src);

class ReportLoader
{
public:
    ReportLoader();

    void                 Init();

    int                  LoadReport(bool Join);
    int                  LoadReport(const char * FNameIn, bool Join);
    int                  SaveOrders(bool UsingExistingName);
    void                 LoadOrders();

    void                 SwitchToRep(eRepSeq whichrep);
    bool                 CanSwitchToRep(eRepSeq whichrep, int & RepIdx);
    void                 SwitchToYearMon(long YearMon);

    bool                 GetPrevTurnReport(CAtlaParser *& pPrevTurn);

    bool                 GetOrdersChanged(){return m_OrdersAreChanged;};
    void                 SetOrdersChanged(bool Changed);

    void                 StdRedirectReadMore(bool FromStdout, std::string & sData);
    void                 CheckRedirectedOutputFiles();
    void                 RerunOrders();
    void                 SetAllLandUnitFlags();

    void                 SaveComments();
    void                 SaveLandFlags();
    void                 SaveUnitFlags();
    void                 LoadComments();
    void                 LoadLandFlags();
    void                 LoadUnitFlags();
    void                 UpdateEdgeStructs();

    void                 PreLoadReport();
    void                 PostLoadReport();

    // Public (not just ReportLoader-internal) because CAhApp's OnInit/OnExit
    // still call these directly, and SaveHistory is also called from
    // CAhApp::OnExit.
    void                 StdRedirectInit();
    void                 StdRedirectDone();
    int                  SaveHistory (const char * FNameOut);

    bool                 m_DisableErrs;
    bool                 m_CommentsChanged;
    bool                 m_FirstLoad;
    std::multimap<std::string, std::string> m_UnitPropertyGroups;

    // Public (not just ReportLoader-internal) because SelectionState's
    // OnUnitHexSelectionChange (step 5) passes this field's address across
    // the class boundary into CEditPane::SetSource.
    bool                 m_OrdersAreChanged;

private:
    int                  LoadOrders  (const char * FNameIn);
    int                  SaveOrders  (const char * FNameOut, int FactionId);
    void                 GetShortFactName(std::string & S, int FactionId);

    int                  m_nStdoutLastPos;
    int                  m_nStderrLastPos;
};

extern ReportLoader * gpReportLoader;

#endif

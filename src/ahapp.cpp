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
}

CAhApp::~CAhApp()
{
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

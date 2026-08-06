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

#include "uicontroller.h"

#include <cstring>
#include "string_utils.h"
#include "consts.h"
#include "consts_ah.h"
#include "configmanager.h"
#include "gamedatamanager.h"
#include "selectionstate.h"
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
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "listcoledit.h"
#include "optionsdlg.h"

UIController * gpUIController = nullptr;

//=========================================================================

UIController::UIController()
{
    memset(m_Frames, 0, sizeof(m_Frames));
    memset(m_Panes , 0, sizeof(m_Panes ));
    memset(m_Fonts , 0, sizeof(m_Fonts ));

    m_FontDescr[FONT_EDIT_DESCR]  = "Descriptions";
    m_FontDescr[FONT_EDIT_ORDER]  = "Orders & comments";
    m_FontDescr[FONT_MAP_COORD ]  = "Map coordinates";
    m_FontDescr[FONT_MAP_TEXT  ]  = "Map text";
    m_FontDescr[FONT_UNIT_LIST ]  = "Unit list";
    m_FontDescr[FONT_EDIT_HDR  ]  = "Edit pane header";
    m_FontDescr[FONT_VIEW_DLG  ]  = "View dialogs";
    m_FontDescr[FONT_ERR_DLG   ]  = "Messages and Errors";

    m_layout            = 0;
    m_DiscardChanges    = false;
    m_Brightness_Delta  = 0;
}

//-------------------------------------------------------------------------

UIController::~UIController()
{
}

//-------------------------------------------------------------------------

void UIController::Init()
{
    m_layout = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_LAYOUT));
    if (m_layout<0)
        m_layout = 0;
    if (m_layout>=AH_LAYOUT_COUNT)
        m_layout = AH_LAYOUT_COUNT-1;

    m_Brightness_Delta = atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_BRIGHT_DELTA));

    std::string S;
    for (int i=0; i<FONT_COUNT; i++)
    {
        S.clear();
        S << (long)i;
        const char * szValue = gpConfigManager->GetConfig(SZ_SECT_FONTS_2, S.c_str());
        m_Fonts[i] = NewFontFromStr(szValue);
    }

    CreateAccelerator();
}

//-------------------------------------------------------------------------

void UIController::Shutdown()
{
    std::string S, Name;
    for (int i=0; i<FONT_COUNT; i++)
    {
        FontToStr(m_Fonts[i], S);
        Name.clear();
        Name << (long)i;
        gpConfigManager->SetConfig(SZ_SECT_FONTS_2, Name.c_str(), S.c_str());
    }

    for (int i=0; i<FONT_COUNT; i++)
        delete m_Fonts[i];

    m_pAccel.reset();
}

//-------------------------------------------------------------------------

void UIController::CreateAccelerator()
{
    static wxAcceleratorEntry entries[5];
    entries[0].Set(wxACCEL_CTRL,  (int)'S',     menu_SaveOrders);
    entries[1].Set(wxACCEL_CTRL,  (int)'N',     accel_NextUnit );
    entries[2].Set(wxACCEL_CTRL,  (int)'P',     accel_PrevUnit );
    entries[3].Set(wxACCEL_CTRL,  (int)'U',     accel_UnitList );
    entries[4].Set(wxACCEL_CTRL,  (int)'O',     accel_Orders   );

    m_pAccel.reset(new wxAcceleratorTable(5, entries));
}

//-------------------------------------------------------------------------

void UIController::Redraw()
{
    int i;

    for (i=0; i<AH_PANE_COUNT; i++)
        if (m_Panes[i])
            m_Panes[i]->Refresh(false);
}


//-------------------------------------------------------------------------

void UIController::ApplyFonts()
{

    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyFonts();
    if (m_Panes[AH_PANE_MAP_DESCR    ]) ((CEditPane*)m_Panes[AH_PANE_MAP_DESCR    ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNITS_HEX    ]) ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX    ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNITS_FILTER ]) ((CUnitPane*)m_Panes[AH_PANE_UNITS_FILTER ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNIT_DESCR   ]) ((CEditPane*)m_Panes[AH_PANE_UNIT_DESCR   ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNIT_COMMANDS]) ((CEditPane*)m_Panes[AH_PANE_UNIT_COMMANDS])->ApplyFonts();
    if (m_Panes[AH_PANE_UNIT_COMMENTS]) ((CEditPane*)m_Panes[AH_PANE_UNIT_COMMENTS])->ApplyFonts();
    if (m_Panes[AH_PANE_MSG          ]) ((CEditPane*)m_Panes[AH_PANE_MSG          ])->ApplyFonts();

}

//-------------------------------------------------------------------------

void UIController::ApplyColors()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyColors();
}

//-------------------------------------------------------------------------

void UIController::ApplyIcons()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyIcons();
}

//-------------------------------------------------------------------------

void UIController::OpenOptionsDlg()
{
    int rc;

    COptionsDialog *dialog = new COptionsDialog(m_Frames[AH_FRAME_MAP]);
    {
        dialog->Init();
        rc = dialog->ShowModal();
        if (wxID_OK==rc)
        {
        }
        dialog->Done();
    }
    dialog->Destroy();
}

//-------------------------------------------------------------------------

void UIController::OpenMapFrame()
{
    if (!m_Frames[AH_FRAME_MAP])
    {
        m_Frames[AH_FRAME_MAP] = new CMapFrame(nullptr, m_layout);
        m_Frames[AH_FRAME_MAP]->Init(m_layout, nullptr);
        m_Frames[AH_FRAME_MAP]->Show(true);
    }
    else
        m_Frames[AH_FRAME_MAP]->Raise();
}

//-------------------------------------------------------------------------

void UIController::OpenUnitFrame()
{
    if (!m_Frames[AH_FRAME_UNITS])
    {
        m_Frames[AH_FRAME_UNITS] = new CUnitFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_UNITS]->Init(m_layout, nullptr);
        m_Frames[AH_FRAME_UNITS]->Show(true);
    }
    else
        m_Frames[AH_FRAME_UNITS]->Raise();
}

//-------------------------------------------------------------------------

void UIController::OpenUnitFrameFltr(bool PopUpSettings)
{
    if (!m_Frames[AH_FRAME_UNITS_FLTR])
    {
        m_Frames[AH_FRAME_UNITS_FLTR] = new CUnitFrameFltr(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_UNITS_FLTR]->Init(m_layout, nullptr);
        m_Frames[AH_FRAME_UNITS_FLTR]->Show(true);

        CUnitPaneFltr   * pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
        wxCommandEvent    event;

        if (pUnitPaneF)
        {
            if  (PopUpSettings)
                pUnitPaneF->OnPopupMenuFilter(event);
            else
                pUnitPaneF->Update(nullptr);
        }
    }
    else
        m_Frames[AH_FRAME_UNITS_FLTR]->Raise();


}

//-------------------------------------------------------------------------

void UIController::OpenMsgFrame()
{
    if (!m_Frames[AH_FRAME_MSG])
    {
        m_Frames[AH_FRAME_MSG] = new CMsgFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_MSG]->Init(m_layout, nullptr);
        gpSelectionState->m_MsgSrc.clear();
        m_Frames[AH_FRAME_MSG]->Show(true);
    }
    else
        m_Frames[AH_FRAME_MSG]->Raise();
}

//-------------------------------------------------------------------------

void UIController::OpenEditsFrame()
{
    if (!m_Frames[AH_FRAME_EDITS])
    {
        m_Frames[AH_FRAME_EDITS] = new CEditsFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_EDITS]->Init(m_layout, nullptr);
        m_Frames[AH_FRAME_EDITS]->Show(true);
    }
    else
        m_Frames[AH_FRAME_EDITS]->Raise();
}

//-------------------------------------------------------------------------

void UIController::ForgetFrame(int no, bool frameclosed)
{
    int i;

    if (m_Frames[no])
    {
        m_Frames[no]->Done(frameclosed);

        for (i=0; i<AH_PANE_COUNT; i++)
            if (m_Frames[no]->m_Panes[i])
                m_Panes[i] = nullptr;

        m_Frames[no] = nullptr;
    }
}


//-------------------------------------------------------------------------

void UIController::FrameClosing(CAhFrame * pFrame)
{
    int  no;

    if (pFrame)
        for (no=0; no<AH_FRAME_COUNT; no++)
            if (m_Frames[no] == pFrame)
            {
                if (AH_FRAME_MAP==no)
                {
                    // shutdown in progress
                    for (no=0; no<AH_FRAME_COUNT; no++)
                        ForgetFrame(no, false);
                }
                else
                    ForgetFrame(no, true);
                break;
            }
}

//-------------------------------------------------------------------------

void UIController::ShowError(const char * msg, int msglen, bool ignore_disabled)
{
    CEditPane * p;

    if (gpReportLoader->m_DisableErrs && !ignore_disabled)
       return;

    OpenMsgFrame();
    AddStr(gpSelectionState->m_MsgSrc, msg, msglen);

    p = (CEditPane*)m_Panes[AH_PANE_MSG];
    if (p)
        p->SetSource(&gpSelectionState->m_MsgSrc, nullptr);
}

//-------------------------------------------------------------------------

void UIController::SetMapFrameTitle()
{
    CMapFrame   * pMapFrame  = (CMapFrame *)m_Frames[AH_FRAME_MAP];
    CMapPane    * pMapPane   = (CMapPane  * )m_Panes[AH_PANE_MAP];
    CPlane      * pPlane     = nullptr;

    std::string          S;

    S = m_sTitle;

    if (pMapPane)
    {
        pPlane = (CPlane*)gpGameData->m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);

        S << " (" << pMapPane->m_SelHexX << "," << pMapPane->m_SelHexY;
        if (pPlane && 0!=stricmp(DEFAULT_PLANE, pPlane->Name.c_str()))
        {
            S << "," << pPlane->Name;
        }
        S << ")";
    }

    if (gpReportLoader->GetOrdersChanged())
        S << " [modified]";
    if (pMapFrame)
        pMapFrame->SetTitle(wxString::FromAscii(S.c_str()));
}

//-------------------------------------------------------------------------

bool UIController::CanCloseApp()
{
    gpReportLoader->SaveLandFlags();
    gpReportLoader->SaveUnitFlags();
    if (gpReportLoader->m_CommentsChanged)
        gpReportLoader->SaveComments();

    return ( m_DiscardChanges || !gpReportLoader->GetOrdersChanged() || ERR_OK==gpReportLoader->SaveOrders(true));
}

//-------------------------------------------------------------------------

void UIController::EditPaneChanged(CEditPane * pPane)
{
    CMapPane  * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
    CLand     * pLand;
    CUnit     * pUnit;

    if (pPane && pMapPane)
    {
        pLand = gpGameData->m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);

        if (pPane == m_Panes[AH_PANE_UNIT_COMMANDS])
        {
            // selected unit's orders have been changed

            // TBD: is it needed? m_pCurLand->guiUnit = m_pUnitListPane->GetCurrentUnitId();
            gpGameData->m_pAtlantis->RunOrders(pLand);
            gpSelectionState->UpdateHexUnitList(pLand);
            gpSelectionState->UpdateHexEditPane(pLand);
            gpReportLoader->SetOrdersChanged(gpReportLoader->GetOrdersChanged()); // this hack is needed since EditPanes are modifying the vars directly...
        }
        else if (pPane == m_Panes[AH_PANE_UNIT_COMMENTS])
        {
            // selected unit's comments / default orders have been changed
            pUnit = gpSelectionState->GetSelectedUnit(); // depends on m_SelUnitIdx
            if (pUnit)
            {
                pUnit->ExtractCommentsFromDefOrders();
                gpSelectionState->UpdateHexUnitList(pLand);
            }
        }
    }
}

//-------------------------------------------------------------------------

void UIController::EditPaneDClicked(CEditPane * pPane)
{
    const char  * p;
    std::string          src, S;
    char          ch;
    CUnit       * pUnit;
    CBaseObject   Dummy;
    int           idx;
    long          position;


    if (pPane == m_Panes[AH_PANE_MSG])
    {
        pPane->GetValue(src);
        position = pPane->m_pEditor->GetInsertionPoint();

// There is a bug in win32 GetInsertionPoint() - returned value corresponds to "\r\n" end of lines,
// while actual returned string has "\n" end of lines
#ifdef __WXMSW__
        long x = 0;
        p = src.c_str();
        while (x<position)
        {
            if ('\n' == p[x])
                position--;
            x++;
        }
#endif
        if (position > static_cast<long>(src.size()))
            position = src.size();

        p = src.c_str();
        while (position > 0)

        {
            if (p[position-1]=='\n')
                break;
            position--;
        }

        p = &src.c_str()[position];
        p = SkipSpaces(GetToken(S, p, " \t", ch, TRIM_ALL));
        if (0==stricmp("UNIT", S.c_str()))  // that is an order problem report
        {
            GetToken(S, p, " \t", ch, TRIM_ALL);
            Dummy.Id = atol(S.c_str());
            if (gpGameData->m_pAtlantis->m_Units.Search(&Dummy, idx))
            {
                pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(idx);
                gpSelectionState->SelectUnit(pUnit);
                return;
            }
        }


        p = &src.c_str()[position];
        p = SkipSpaces(GetToken(S, p, "(\n", ch, TRIM_ALL)); // must be an error from the report file
        if ('('==ch)
        {
            GetToken(S, p, ")\n", ch, TRIM_ALL);
            Dummy.Id = atol(S.c_str());
            if (')'==ch && gpGameData->m_pAtlantis->m_Units.Search(&Dummy, idx))
            {
                pUnit = (CUnit*)gpGameData->m_pAtlantis->m_Units.At(idx);
                gpSelectionState->SelectUnit(pUnit);
                return;
            }
        }

        // land
        p = &src.c_str()[position];
        p = SkipSpaces(GetToken(S, p, "(\n", ch, TRIM_ALL));
        if ('('==ch)
        {
            p = SkipSpaces(GetToken(S, p, ")\n", ch, TRIM_ALL));
            if (')' == ch && gpSelectionState->SelectLand(S.c_str()))
                return;
        }
    }
}

//-------------------------------------------------------------------------

void UIController::EditListColumns(int command)
{
    CMapFrame   * pMapFrame  = (CMapFrame *)m_Frames[AH_FRAME_MAP];
    CUnitPane   * pUnitPane  = nullptr;
    const char  * szConfigSectionHdr;


    const char * szKey = nullptr;
    switch (command)
    {
    case menu_ListColUnits:
        szKey = SZ_KEY_LIS_COL_UNITS_HEX;
        pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];
        break;

    case menu_ListColUnitsFltr:
        szKey = SZ_KEY_LIS_COL_UNITS_FILTER;
        pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_FILTER];
        break;

    default:
        return;
    }
    if (pUnitPane)
        pUnitPane->SaveUnitListHdr();

    CListHeaderEditDlg dlg(pMapFrame, szKey);

    if (wxID_OK == dlg.ShowModal())
    {
        szConfigSectionHdr = GetListColSection(SZ_SECT_LIST_COL_UNIT, szKey);
        if (pUnitPane)
            pUnitPane->ReloadHdr(szConfigSectionHdr);
    }
}

//-------------------------------------------------------------------------

const char * UIController::GetListColSection(const char * sectprefix, const char * key)
{
    const char * sect;

    sect = gpConfigManager->GetConfig(SZ_SECT_LIST_COL_CURRENT, key);
    if (!sect || !*sect)
        sect  = gpConfigManager->GetNextSectionName(CONFIG_FILE_CONFIG, sectprefix);

    return sect;
}

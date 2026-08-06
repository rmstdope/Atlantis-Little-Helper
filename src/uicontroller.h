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

#ifndef __UICONTROLLER_H_INCL__
#define __UICONTROLLER_H_INCL__

#include <memory>
#include <string>

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

    AH_PANE_COUNT             // whenever adding new panes, update UIController::ApplyFonts and UIController::ApplyColors!
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

#define APPLY_COLOR_DELTA(x) ((unsigned char )(std::max(std::min((int)(x)-(int)gpUIController->m_Brightness_Delta,255),0)))

class CAhFrame;
class CEditPane;
class CUnit;
class CLand;

class UIController
{
public:
    UIController();
    ~UIController();

    void                 Init();
    void                 Shutdown();

    void                 ShowError (const char * msg, int msglen, bool ignore_disabled);
    bool                 CanCloseApp();
    void                 Redraw();
    void                 ApplyFonts();
    void                 ApplyColors();
    void                 ApplyIcons();

    void                 OpenOptionsDlg();
    void                 OpenMapFrame();
    void                 OpenUnitFrame();
    void                 OpenMsgFrame();
    void                 OpenEditsFrame();
    void                 OpenUnitFrameFltr(bool PopUpSettings);

    void                 FrameClosing(CAhFrame * pFrame);
    void                 CreateAccelerator();

    void                 EditPaneChanged(CEditPane * pPane);
    void                 EditPaneDClicked(CEditPane * pPane);

    void                 EditListColumns(int command);
    const char         * GetListColSection(const char * sectprefix, const char * key);

    void                 SetMapFrameTitle();

    CAhFrame           * m_Frames[AH_FRAME_COUNT];
    wxWindow           * m_Panes [AH_PANE_COUNT ];
    wxFont             * m_Fonts [FONT_COUNT];
    const char         * m_FontDescr[FONT_COUNT];
    std::unique_ptr<wxAcceleratorTable> m_pAccel;
    long                 m_Brightness_Delta;
    int                  m_layout;
    bool                 m_DiscardChanges;
    std::string          m_sTitle;

private:
    void                 ForgetFrame(int no, bool frameclosed);
};

extern UIController * gpUIController;

#endif

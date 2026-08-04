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

static CGameDataHelper ThisGameDataHelper;

//=========================================================================

CAhApp::CAhApp() : m_HexDescrSrc    (),
                   m_UnitDescrSrc   (),
                   m_OrderHash      (  3),
                   m_TradeItemsHash (  2),
                   m_MenHash        (  2),
                   m_MaxSkillHash   (  6),
                   m_MagicSkillsHash(  6)
{
    m_HexDescrSrc.reserve(128);
    m_UnitDescrSrc.reserve(128);
    m_FirstLoad         = true;
    m_OrdersAreChanged  = false;
    m_CommentsChanged   = false;
    m_UpgradeLandFlags  = false;
    m_DiscardChanges    = false;
    m_SelUnitIdx        = -1;
    m_layout            = 0;
    m_DisableErrs       = false;
    m_pAccel            = nullptr;

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

    m_pAtlantis.reset(new CAtlaParser(&ThisGameDataHelper));
    m_pAtlantis->m_pConfig = &m_Config[CONFIG_FILE_CONFIG];
    m_Brightness_Delta = 0;
    m_nStdoutLastPos = 0;
    m_nStderrLastPos = 0;
}

CAhApp::~CAhApp()
{
    gpApp = nullptr;
}

//-------------------------------------------------------------------------

bool CAhApp::OnInit()
{
    int               i;
    const char      * p;
    const char      * szName;
    const char      * szValue;
    std::string S, S2;
    int               sectidx;
    std::set<std::pair<std::string,std::string>> CollDedup;


    gpApp = this;

    m_ConfigSectionsState.insert(SZ_SECT_DEF_ORDERS       );
    m_ConfigSectionsState.insert(SZ_SECT_ORDERS           );
    m_ConfigSectionsState.insert(SZ_SECT_REPORTS          );
    m_ConfigSectionsState.insert(SZ_SECT_LAND_FLAGS       );
    m_ConfigSectionsState.insert(SZ_SECT_LAND_VISITED     );
    m_ConfigSectionsState.insert(SZ_SECT_SKILLS           );
    m_ConfigSectionsState.insert(SZ_SECT_ITEMS            );
    m_ConfigSectionsState.insert(SZ_SECT_OBJECTS          );
    m_ConfigSectionsState.insert(SZ_SECT_PASSWORDS        );
    m_ConfigSectionsState.insert(SZ_SECT_UNIT_TRACKING    );
    m_ConfigSectionsState.insert(SZ_SECT_FOLDERS          );
    m_ConfigSectionsState.insert(SZ_SECT_DO_NOT_SHOW_THESE);
    m_ConfigSectionsState.insert(SZ_SECT_TROPIC_ZONE      );
    m_ConfigSectionsState.insert(SZ_SECT_UNIT_FLAGS       );


    m_Config[CONFIG_FILE_CONFIG].Load(SZ_CONFIG_FILE);
    m_Config[CONFIG_FILE_STATE ].Load(SZ_CONFIG_STATE_FILE);

    UpgradeConfigFiles();
    CUnit::LoadCustomFlagNames(GetConfigFile(SZ_SECT_UNIT_FLAG_NAMES));

    m_layout = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LAYOUT));
    if (m_layout<0)
        m_layout = 0;
    if (m_layout>=AH_LAYOUT_COUNT)
        m_layout = AH_LAYOUT_COUNT-1;

    m_Brightness_Delta = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_BRIGHT_DELTA));


    for (i=0; i<FONT_COUNT; i++)
    {
        S.clear();
        S << (long)i;
        szValue = GetConfig(SZ_SECT_FONTS_2, S.c_str());
        m_Fonts[i] = NewFontFromStr(szValue);
    }

    if (0==stricmp(SZ_EOL_MS, GetConfig(SZ_SECT_COMMON, SZ_KEY_EOL)))
        EOL_FILE = EOL_MS;
    else
        EOL_FILE = EOL_UNIX;


    // Load unit property groups
    sectidx = GetSectionFirst(SZ_SECT_UNITPROP_GROUPS, szName, szValue);
    while (sectidx >= 0)
    {
        while (szValue && *szValue)
        {
            szValue = GetToken(S, szValue, ',');
            std::string key(szName), val(S.c_str());
            if (CollDedup.insert({key, val}).second)
                m_UnitPropertyGroups.emplace(key, val);
        }
        sectidx = GetSectionNext(sectidx, SZ_SECT_UNITPROP_GROUPS, szName, szValue);
    }
    CUnit::m_PropertyGroupsColl = &m_UnitPropertyGroups;


    // Property group name must not be an alias!
    {
        std::string lastKey;
        for (auto & kv : m_UnitPropertyGroups)
        {
            if (lastKey != kv.first)
            {
                lastKey = kv.first;
                p = ResolveAlias(lastKey.c_str());
                if (0!=stricmp(lastKey.c_str(), p))
                {
                    S = "Group name \"";
                    S << lastKey.c_str() << "\" can be resolved as alias for \"" << p << "\"!\r\n";
                    ShowError(S.c_str(), S.size(), true);
                }
            }
        }
    }


    InitMoveModes();

    //m_Attitudes.FreeAll();
    SetAttitudeForFaction(0, ATT_NEUTRAL);

    // Water terrain types
    //m_WaterTerrainNames = CStringSortColl(); no need for that
    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_WATER_TERRAINS));
    int idx;
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_WaterTerrainNames.insert(ToLower(S));
    }


    // Load order hash
    m_OrderHash["advance"    ] = O_ADVANCE    ;
    m_OrderHash["assassinate"] = O_ASSASSINATE;
    m_OrderHash["attack"     ] = O_ATTACK     ;
    m_OrderHash["autotax"    ] = O_AUTOTAX    ;
    m_OrderHash["build"      ] = O_BUILD      ;
    m_OrderHash["buy"        ] = O_BUY        ;
    m_OrderHash["claim"      ] = O_CLAIM      ;
    m_OrderHash["end"        ] = O_ENDFORM    ;
    m_OrderHash["endturn"    ] = O_ENDTURN    ;
    m_OrderHash["enter"      ] = O_ENTER      ;
    m_OrderHash["form"       ] = O_FORM       ;
    m_OrderHash["give"       ] = O_GIVE       ;
    m_OrderHash["giveif"     ] = O_GIVEIF     ;
    m_OrderHash["take"       ] = O_TAKE       ;
    m_OrderHash["send"       ] = O_SEND       ;
    m_OrderHash["withdraw"   ] = O_WITHDRAW   ;
    m_OrderHash["leave"      ] = O_LEAVE      ;
    m_OrderHash["move"       ] = O_MOVE       ;
    m_OrderHash["produce"    ] = O_PRODUCE    ;
    m_OrderHash["promote"    ] = O_PROMOTE    ;
    m_OrderHash["sail"       ] = O_SAIL       ;
    m_OrderHash["sell"       ] = O_SELL       ;
    m_OrderHash["steal"      ] = O_STEAL      ;
    m_OrderHash["study"      ] = O_STUDY      ;
    m_OrderHash["teach"      ] = O_TEACH      ;
    m_OrderHash["turn"       ] = O_TURN       ;

    m_OrderHash["tax"        ] = O_TAX        ;
    m_OrderHash["work"       ] = O_WORK       ;

    m_OrderHash["guard"      ] = O_GUARD      ;
    m_OrderHash["avoid"      ] = O_AVOID      ;
    m_OrderHash["behind"     ] = O_BEHIND     ;
    m_OrderHash["reveal"     ] = O_REVEAL     ;
    m_OrderHash["hold"       ] = O_HOLD       ;
    m_OrderHash["noaid"      ] = O_NOAID      ;
    m_OrderHash["consume"    ] = O_CONSUME    ;
    m_OrderHash["nocross"    ] = O_NOCROSS    ;
    m_OrderHash["spoils"     ] = O_SPOILS     ;

    m_OrderHash["recruit"    ] = O_RECRUIT    ;
    m_OrderHash["share"      ] = O_SHARE      ;

    m_OrderHash["template"   ] = O_TEMPLATE   ;
    m_OrderHash["endtemplate"] = O_ENDTEMPLATE;
    m_OrderHash["all"        ] = O_ALL        ;
    m_OrderHash["endall"     ] = O_ENDALL     ;

    m_OrderHash["type"       ] = O_TYPE       ;
    m_OrderHash["label"      ] = O_LABEL      ;
    m_OrderHash["name"       ] = O_NAME       ;






    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_VALID_ORDERS));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_OrderHash.emplace(S.c_str(), -1L);
    }
//    m_OrderHash.Dbg_Print();

    // Load trade items hash
    p = SkipSpaces(GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_TRADE_ITEMS));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_TradeItemsHash.emplace(S.c_str(), -1L);
    }

    // All the men hash
    p = SkipSpaces(GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_MEN));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_MenHash.emplace(S.c_str(), -1L);
    }

    // Magic skills hash
    p = SkipSpaces(GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_MAG_SKILLS));
    while (p && *p)
    {
        int x;
        p = SkipSpaces(GetToken(S, p, ','));
        x = FindSubStrR(S, PRP_SKILL_POSTFIX);
        if (x>=0)
            DelSubStr(S, x, S.size()-x+1);

        if (!S.empty())
            m_MagicSkillsHash.emplace(S.c_str(), -1L);
    }

    // Read list of year/month for report
    i = GetSectionFirst(SZ_SECT_REPORTS, szName, szValue);
    while (i>=0)
    {
        {
            long _val = atol(szName);
            auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), _val);
            if (_it == m_ReportDates.end() || *_it != _val)
                m_ReportDates.insert(_it, _val);
        }
        i = GetSectionNext (i, SZ_SECT_REPORTS, szName, szValue);
    }





    StdRedirectInit();

    CreateAccelerator();

    OpenMapFrame();

    if ((AH_LAYOUT_3_WIN==m_layout || AH_LAYOUT_2_WIN==m_layout) &&
        atol(GetConfig(CUnitFrame::GetConfigSection(m_layout), SZ_KEY_OPEN)) )
        OpenUnitFrame();

    if ((AH_LAYOUT_3_WIN==m_layout) &&
        (atol(GetConfig(CEditsFrame::GetConfigSection(m_layout), SZ_KEY_OPEN))) )
        OpenEditsFrame();

    SetTopWindow(m_Frames[AH_FRAME_MAP]);
    m_Frames[AH_FRAME_MAP]->SetFocus();


    if (argc>1)
        for (i=1; i<argc; i++)
            LoadReport(wxString(argv[i]).mb_str(), i>1);
    else
        if (atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_REP)) && (((int)m_ReportDates.size()) > 0) )
        {
            S.clear();
            S << m_ReportDates[(int)m_ReportDates.size()-1];
            S2 = GetConfig(SZ_SECT_REPORTS, S.c_str());
            const char * p = S2.c_str();
            bool         join = false;
            while (p && *p)
            {
                p = GetToken(S, p, ',');
                LoadReport(S.c_str(), join);
                join = true;
            }
        }

    if (atol(GetConfig(CUnitFrameFltr::GetConfigSection(m_layout), SZ_KEY_OPEN)) )
        OpenUnitFrameFltr(false);

    return true;
}

//-------------------------------------------------------------------------

int CAhApp::OnExit()
{
    int  i;
    std::string S;
    std::string Name;

    CUnit::ResetCustomFlagNames();

    for (i=0; i<FONT_COUNT; i++)
    {
        FontToStr(m_Fonts[i], S);
        Name.clear();
        Name << (long)i;
        SetConfig(SZ_SECT_FONTS_2, Name.c_str(), S.c_str());
    }

    if (!m_DiscardChanges)
    {
        m_Config[CONFIG_FILE_CONFIG].Save(SZ_CONFIG_FILE);
        m_Config[CONFIG_FILE_STATE ].Save(SZ_CONFIG_STATE_FILE);

        if (m_pAtlantis && ERR_OK==m_pAtlantis->m_ParseErr)
            SaveHistory(SZ_HISTORY_FILE);
    }

    m_TradeItemsHash.clear();
    m_MenHash.clear();
    m_MaxSkillHash.clear();
    m_MagicSkillsHash.clear();

    m_Reports.clear();
    m_pAtlantis.reset();
    m_pAccel.reset();

    for (i=0; i<FONT_COUNT; i++)
        delete m_Fonts[i];


    m_MoveModes.clear();
    m_MoveModesRaw.clear();
    for (auto* p : m_ItemWeights) { free(p->name); free(p->weights); delete p; }
    m_ItemWeights.clear();
    m_ConfigSectionsState.clear();
    m_OrderHash.clear();
    m_Attitudes.FreeAll();
    m_WaterTerrainNames.clear();

    StdRedirectDone();

    return 0;
}

//-------------------------------------------------------------------------

void CAhApp::CreateAccelerator()
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

void CAhApp::Redraw()
{
    int i;

    for (i=0; i<AH_PANE_COUNT; i++)
        if (m_Panes[i])
            m_Panes[i]->Refresh(false);
}


//-------------------------------------------------------------------------

void CAhApp::ApplyFonts()
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

void CAhApp::ApplyColors()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyColors();
}

//-------------------------------------------------------------------------

void CAhApp::ApplyIcons()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyIcons();
}
        
//-------------------------------------------------------------------------

void CAhApp::OpenOptionsDlg()
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
    //dialog->Close(true);
}

//-------------------------------------------------------------------------

void CAhApp::OpenMapFrame()
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

void CAhApp::OpenUnitFrame()
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

void CAhApp::OpenUnitFrameFltr(bool PopUpSettings)
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

void CAhApp::OpenMsgFrame()
{
    if (!m_Frames[AH_FRAME_MSG])
    {
        m_Frames[AH_FRAME_MSG] = new CMsgFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_MSG]->Init(m_layout, nullptr);
        m_MsgSrc.clear();
        m_Frames[AH_FRAME_MSG]->Show(true);
    }
    else
        m_Frames[AH_FRAME_MSG]->Raise();
}

//-------------------------------------------------------------------------

void CAhApp::OpenEditsFrame()
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

void CAhApp::UpgradeConfigFiles()
{
    std::string         Section;
    const char * szNextSection;
    const char * szName;
    const char * szValue;
    bool         Ok = true;
    int          fileno, idx, i;
    std::string         ConfigKey;

    // move the sections only when we still have old-style unit list headers.
    // Modern configs already store the current section layout, so scanning the
    // whole file on every startup just burns CPU and delays the UI.
    if (m_Config[CONFIG_FILE_CONFIG].GetFirstInSection(SZ_SECT_UNITLIST_HDR, szName, szValue) >= 0 ||
        m_Config[CONFIG_FILE_CONFIG].GetFirstInSection(SZ_SECT_UNITLIST_HDR_FLTR, szName, szValue) >= 0)
    {
        Ok = m_Config[CONFIG_FILE_CONFIG].GetNextSection("", szNextSection);
        while (Ok)
        {
            Section = szNextSection;
            fileno    = GetConfigFileNo(Section.c_str());

            if (CONFIG_FILE_CONFIG != fileno)
            {
                // move to the appropriate file
                idx = m_Config[CONFIG_FILE_CONFIG].GetFirstInSection(Section.c_str(), szName, szValue);
                while (idx>=0)
                {
                    m_Config[fileno].SetByName(Section.c_str(), szName, szValue);
                    idx = m_Config[CONFIG_FILE_CONFIG].GetNextInSection(idx, Section.c_str(), szName, szValue);
                }
                m_Config[CONFIG_FILE_CONFIG].RemoveSection(Section.c_str());

                // and it means land flags has to be moved, too
                m_UpgradeLandFlags = true;
            }

            Ok = m_Config[CONFIG_FILE_CONFIG].GetNextSection(Section.c_str(), szNextSection);
        }
    }

    // unit lists columns
    szValue = m_Config[CONFIG_FILE_CONFIG].GetByName(SZ_SECT_LIST_COL_CURRENT, SZ_KEY_LIS_COL_UNITS_HEX);
    if (!szValue || !*szValue)
    {
        MoveSectionEntries(CONFIG_FILE_CONFIG, SZ_SECT_UNITLIST_HDR     , SZ_SECT_LIST_COL_UNIT_DEF     );
        MoveSectionEntries(CONFIG_FILE_CONFIG, SZ_SECT_UNITLIST_HDR_FLTR, SZ_SECT_LIST_COL_UNIT_FLTR_DEF);

        SetConfig(SZ_SECT_LIST_COL_CURRENT  , SZ_KEY_LIS_COL_UNITS_HEX  ,     SZ_SECT_LIST_COL_UNIT_DEF);
        SetConfig(SZ_SECT_LIST_COL_CURRENT  , SZ_KEY_LIS_COL_UNITS_FILTER,    SZ_SECT_LIST_COL_UNIT_FLTR_DEF);
    }

    // unit filter
    Section.clear();
    Section  << SZ_SECT_UNIT_FILTER << "Default";
    Ok = false;
    for (i=0; i<UNIT_SIMPLE_FLTR_COUNT; i++)
    {
        Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_PROPERTY, i);
        szValue = SkipSpaces(gpApp->GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str()));
        if (szValue && *szValue)
        {
            gpApp->SetConfig(Section.c_str(),      ConfigKey.c_str(), szValue);
            gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str(), "");
            Ok = true;
        }

        Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_COMPARE , i);
        szValue = gpApp->GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str());
        if (szValue && *szValue)
        {
            gpApp->SetConfig(Section.c_str(),      ConfigKey.c_str(), szValue);
            gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str(), "");
            Ok = true;
        }

        Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_VALUE   , i);
        szValue = gpApp->GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str());
        if (szValue && *szValue)
        {
            gpApp->SetConfig(Section.c_str(),      ConfigKey.c_str(), szValue);
            gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str(), "");
            Ok = true;
        }
    }
    if (Ok)
        gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, SZ_KEY_FLTR_SET, Section.c_str());

    // Arcadia III roads
    szValue = m_Config[CONFIG_FILE_CONFIG].GetByName(SZ_SECT_COLORS,  SZ_KEY_MAP_ROAD_OLD);
    if (szValue && *szValue)
    {
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD    , szValue);
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD_OLD, "");
    }
    szValue = m_Config[CONFIG_FILE_CONFIG].GetByName(SZ_SECT_COLORS,  SZ_KEY_MAP_ROAD_BAD_OLD);
    if (szValue && *szValue)
    {
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD_BAD    , szValue);
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD_BAD_OLD, "");
    }
}

//-------------------------------------------------------------------------

void CAhApp::MoveSectionEntries(int fileno, const char * src, const char * dest)
{
    const char * szName;
    const char * szValue;
    std::vector<std::string> Names, Values;
    int          idx;

    idx = m_Config[fileno].GetFirstInSection(src, szName, szValue);
    while (idx>=0)
    {
        Names.push_back(szName ? szName : "");
        Values.push_back(szValue ? szValue : "");
        idx = m_Config[fileno].GetNextInSection(idx, src, szName, szValue);
    }
    m_Config[fileno].RemoveSection(src);

    for (idx=0; idx<(int)Values.size(); idx++)
    {
        m_Config[fileno].SetByName(dest, Names[idx].c_str(), Values[idx].c_str());
    }
}

//-------------------------------------------------------------------------

void CAhApp::UpgradeConfigByFactionId()
{
    int          fileno, idx;
    std::string         S, Section, Key;
    const char * szName;
    const char * szValue;

    if (m_pAtlantis->m_CrntFactionId > 0)
    {
        // Upgrade order files
        ComposeConfigOrdersSection(Section, m_pAtlantis->m_CrntFactionId);
        fileno  = GetConfigFileNo(SZ_SECT_ORDERS);
        idx     = m_Config[fileno].GetFirstInSection(SZ_SECT_ORDERS, szName, szValue);
        while (idx>=0)
        {
            m_Config[fileno].SetByName(Section.c_str(), szName, szValue);
            idx = m_Config[fileno].GetNextInSection(idx, SZ_SECT_ORDERS, szName, szValue);
        }
        m_Config[fileno].RemoveSection(SZ_SECT_ORDERS);

        // Upgrade passwords
        S = GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_OLD);
        TrimRight(S, TRIM_ALL);
        if (!S.empty())
        {
            Key.clear();
            Key << (long)m_pAtlantis->m_CrntFactionId;
            SetConfig(SZ_SECT_PASSWORDS, Key.c_str() , S.c_str() );
            SetConfig(SZ_SECT_COMMON   , SZ_KEY_PWD_OLD, (const char *)nullptr);
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::ComposeConfigOrdersSection(std::string & Sect, int FactionId)
{
    Sect = SZ_SECT_ORDERS;
    Sect << "_" << (long)FactionId;
}

//-------------------------------------------------------------------------

int CAhApp::GetConfigFileNo(const char * szSection)
{
    if (m_ConfigSectionsState.find(szSection) != m_ConfigSectionsState.end() ||
        0==strnicmp(SZ_SECT_ORDERS, szSection, sizeof(SZ_SECT_ORDERS)-1) ) // orders section is composite starting from 2.1.6
        return CONFIG_FILE_STATE;
    else
        return CONFIG_FILE_CONFIG;
}

//-------------------------------------------------------------------------

const char * CAhApp::GetConfig(const char * szSection, const char * szName)
{
    const char * p;
    int          i;
    int          fileno = GetConfigFileNo(szSection);

    p = m_Config[fileno].GetByName(szSection, szName);
    if (nullptr==p)
    {
        for (i=0; i<DefaultConfigSize; i++)
            if ( (0==stricmp(szSection, DefaultConfig[i].szSection)) &&
                 (0==stricmp(szName,    DefaultConfig[i].szName))  )
            {
                p = DefaultConfig[i].szValue;
                break;
            }
        m_Config[fileno].SetByName(szSection, szName, p?p:" ");
    }
    if (nullptr==p)
        p = "";
    return p;
}

//-------------------------------------------------------------------------

CConfigFile * CAhApp::GetConfigFile(const char * szSection)
{
    return &m_Config[GetConfigFileNo(szSection)];
}

//-------------------------------------------------------------------------

void CAhApp::SetConfig(const char * szSection, const char * szName, const char * szNewValue)
{
    int  fileno = GetConfigFileNo(szSection);
    m_Config[fileno].SetByName(szSection, szName, szNewValue);
}

//-------------------------------------------------------------------------

void CAhApp::SetConfig(const char * szSection, const char * szName, long lNewValue)
{
    char   buf[64];
    int    fileno = GetConfigFileNo(szSection);

    snprintf(buf, sizeof(buf), "%ld", lNewValue);
    m_Config[fileno].SetByName(szSection, szName, buf);
}


//-------------------------------------------------------------------------

int  CAhApp::GetSectionFirst(const char * szSection, const char *& szName, const char *& szValue)
{
    int idx;
    int i;
    int fileno = GetConfigFileNo(szSection);

    idx = m_Config[fileno].GetFirstInSection(szSection, szName, szValue);
    if (idx < 0)
    {
        for (i=0; i<DefaultConfigSize; i++)
            if (0==stricmp(szSection, DefaultConfig[i].szSection))
                m_Config[fileno].SetByName(szSection, DefaultConfig[i].szName, DefaultConfig[i].szValue);

        idx = m_Config[fileno].GetFirstInSection(szSection, szName, szValue);
    }

    return idx;
}

//-------------------------------------------------------------------------

int  CAhApp::GetSectionNext (int idx, const char * szSection, const char *& szName, const char *& szValue)
{
    int   fileno = GetConfigFileNo(szSection);
    return m_Config[fileno].GetNextInSection (idx, szSection, szName, szValue);
}

//-------------------------------------------------------------------------

void  CAhApp::RemoveSection(const char * szSection)
{
    int fileno = GetConfigFileNo(szSection);
    m_Config[fileno].RemoveSection(szSection);
}

//-------------------------------------------------------------------------

const char * CAhApp::GetNextSectionName(int fileno, const char * szStart)
{
    const char * szNextSection = nullptr;

    if (fileno!=CONFIG_FILE_STATE && fileno!=CONFIG_FILE_CONFIG)
        return nullptr;

    m_Config[fileno].GetNextSection(szStart, szNextSection);

    return szNextSection;
}

//-------------------------------------------------------------------------

void CAhApp::ForgetFrame(int no, bool frameclosed)
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

void CAhApp::FrameClosing(CAhFrame * pFrame)
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

void CAhApp::ShowError(const char * msg, int msglen, bool ignore_disabled)
{
    CEditPane * p;

    if (m_DisableErrs && !ignore_disabled)
       return;

    OpenMsgFrame();
    AddStr(m_MsgSrc, msg, msglen);

    p = (CEditPane*)m_Panes[AH_PANE_MSG];
    if (p)
        p->SetSource(&m_MsgSrc, nullptr);
}

//-------------------------------------------------------------------------

long CAhApp::GetStructAttr(const char * kind, long & MaxLoad, long & MinSailingPower)
{
    const char * attrlist;
    const char * p;
    std::string         S, Name;
    long         attr = 0;

    MaxLoad         = 0;
    MinSailingPower = 0;

    attrlist = GetConfig(SZ_SECT_STRUCTS, ResolveAlias(kind));
    while (attrlist && *attrlist)
    {
        attrlist = GetToken(S, attrlist, ',', TRIM_ALL);
        if (S.empty())
            break;

        if      (0==stricmp(SZ_ATTR_STRUCT_MOBILE , S.c_str()))      attr |= SA_MOBILE ;
        else if (0==stricmp(SZ_ATTR_STRUCT_HIDDEN , S.c_str()))      attr |= SA_HIDDEN ;
        else if (0==stricmp(SZ_ATTR_STRUCT_SHAFT  , S.c_str()))      attr |= SA_SHAFT  ;
        else if (0==stricmp(SZ_ATTR_STRUCT_GATE   , S.c_str()))      attr |= SA_GATE   ;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_N , S.c_str()))      attr |= SA_ROAD_N ;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_NE, S.c_str()))      attr |= SA_ROAD_NE;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_SE, S.c_str()))      attr |= SA_ROAD_SE;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_S , S.c_str()))      attr |= SA_ROAD_S ;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_SW, S.c_str()))      attr |= SA_ROAD_SW;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_NW, S.c_str()))      attr |= SA_ROAD_NW;
        else
        {
            // Two-token attributes, MaxLoad & MinSailingPower.
            p = SkipSpaces(GetToken(Name, S.c_str(), ' ', TRIM_ALL));
            if      (0==stricmp(SZ_ATTR_STRUCT_MAX_LOAD, Name.c_str()))   MaxLoad         = atol(p);
            else if (0==stricmp(SZ_ATTR_STRUCT_MIN_SAIL, Name.c_str()))   MinSailingPower = atol(p);
        }

    }
    if (0 == stricmp(kind, STRUCT_GATE))
        attr |= SA_GATE; // to compensate for legacy missing gate flag in the config
    return attr;
}

//-------------------------------------------------------------------------

const char * CAhApp::ResolveAlias(const char * alias)
{
    const char * p;
    const char * p1;
    int          cnt = 0;

    p1 = alias;
    do
    {
        p = SkipSpaces(GetConfig(SZ_SECT_ALIAS, p1));
        if (p && *p)
            p1 = p;
        if (cnt++ > 20)  // don't play with recursy, man!
        {
            p1 = alias;
            break;
        }

    } while (p && *p);

    return p1;
}

//-------------------------------------------------------------------------

long CAhApp::GetStudyCost(const char * skill)
{
    long        n;
    const char *p;

    p = ResolveAlias(skill);
    n = atol(GetConfig(SZ_SECT_STUDY_COST, p));

    return n;
}

//-------------------------------------------------------------------------

bool CAhApp::GetItemWeights(const char * item, int *& weights, const char **& movenames, int & movecount )
{
    ItemWeights   Dummy;
    ItemWeights * pWeights;
    int           i;
    const char  * p;
    bool          Ok = true;
    bool          Update = false;


    Dummy.name = (char *)item;

    {
        auto _it = std::lower_bound(m_ItemWeights.begin(), m_ItemWeights.end(), &Dummy,
            [](ItemWeights* a, ItemWeights* b) { return SafeCmp(a->name, b->name) < 0; });
        if (_it != m_ItemWeights.end() && SafeCmp((*_it)->name, item) == 0)
            pWeights = *_it;
        else
            pWeights = nullptr;
    }
    if (pWeights)
        ; // found above
    else
    {
        std::string S;

        p = SkipSpaces(GetConfig(SZ_SECT_WEIGHT_MOVE, item));


        Ok = (p && *p);
        pWeights          = new ItemWeights;
        pWeights->name    = strdup(item);
        pWeights->weights = (int*)malloc((int)m_MoveModes.size()*sizeof(int));


        for (i=0; i<(int)m_MoveModes.size(); i++)
        {
            p = SkipSpaces(GetToken(S, p, ','));
            if (i==4 && S.empty())
            {
                // Update swimming for 2.3.2
                int x;
                for (x=0; x<DefaultConfigSize; x++)
                    if ( (0==stricmp(SZ_SECT_WEIGHT_MOVE, DefaultConfig[x].szSection)) &&
                         (0==stricmp(item               , DefaultConfig[x].szName))  )
                    {
                        const char * q = DefaultConfig[x].szValue;
                        int          m;
                        for (m=0; m<=i; m++)
                            q = SkipSpaces(GetToken(S, q, ','));
                        Update = true;
                        break;
                    }
            }
            pWeights->weights[i] = atoi(S.c_str());
        }
        if (Update && !IsASkillRelatedProperty(item))
        {
            // Update swimming for 2.3.2
            S.clear();
            for (i=0; i<(int)m_MoveModes.size(); i++)
            {
                if (i>0)
                    S << ',';
                S << (long)pWeights->weights[i];
            }
            SetConfig(SZ_SECT_WEIGHT_MOVE, item, S.c_str());
        }
        {
            auto _it = std::lower_bound(m_ItemWeights.begin(), m_ItemWeights.end(), pWeights,
                [](ItemWeights* a, ItemWeights* b) { return SafeCmp(a->name, b->name) < 0; });
            m_ItemWeights.insert(_it, pWeights);
        }
    }

    weights   = pWeights->weights;
    m_MoveModesRaw.clear();
    for (const auto& s : m_MoveModes) m_MoveModesRaw.push_back(s.c_str());
    movenames = m_MoveModesRaw.data();
    movecount = (int)m_MoveModes.size();

    if (!Ok)
    {
        std::string S;

        if (!IsASkillRelatedProperty(item))
        {
            S.clear();
            S << "Warning! Weight and capacities for " << item <<
                 " are unknown and assumed to be zero. Movement modes can not be calculated correct. Update your " <<
                 SZ_CONFIG_FILE << " file!" <<EOL_SCR;
            ShowError(S.c_str(), S.size(), true);
        }
    }

    return Ok;
}


//-------------------------------------------------------------------------

void CAhApp::GetMoveNames(const char **& movenames)
{
    m_MoveModesRaw.clear();
    for (const auto& s : m_MoveModes) m_MoveModesRaw.push_back(s.c_str());
    movenames = m_MoveModesRaw.data();
}

//-------------------------------------------------------------------------

bool CAhApp::GetOrderId(const char * order, long & id)
{
    bool  Ok;
    auto it = m_OrderHash.find(order);
    Ok = it != m_OrderHash.end();
    id = Ok ? it->second : 0;

    return Ok;
}

//-------------------------------------------------------------------------

bool CAhApp::IsTradeItem(const char * item)
{
    return m_TradeItemsHash.find(item) != m_TradeItemsHash.end();
}

//-------------------------------------------------------------------------

bool CAhApp::IsMan(const char * item)
{
    return m_MenHash.find(item) != m_MenHash.end();
}

//-------------------------------------------------------------------------

bool CAhApp::IsMagicSkill(const char * skill)
{
    return m_MagicSkillsHash.find(skill) != m_MagicSkillsHash.end();
}

//-------------------------------------------------------------------------

const char * CAhApp::GetWeatherLine(bool IsCurrent, bool IsGood, int Zone)
{
    const char * szKey = nullptr;

    if (IsCurrent)
        if (IsGood)
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_CUR_GOOD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_CUR_GOOD_MEDIUM;
        else
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_CUR_BAD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_CUR_BAD_MEDIUM;
    else
        if (IsGood)
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_NEXT_GOOD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_NEXT_GOOD_MEDIUM;
        else
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_NEXT_BAD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_NEXT_BAD_MEDIUM;

    return GetConfig(SZ_SECT_WEATHER, szKey);
}

//-------------------------------------------------------------------------

long CAhApp::GetMaxRaceSkillLevel(const char * race, const char * skill, const char * leadership, bool IsArcadiaSkillSystem)
{
    // we will cache it a bit...
    long  level    = 0;
    long  maxlevel = 0;
    std::string  sKey;
    std::string  sVal, S;
    const char * p;

    if (!leadership)
        leadership = "";
    sKey << race << ":" << leadership << ":" << skill;

    {
        auto maxIt = m_MaxSkillHash.find(sKey.c_str());
        if (maxIt != m_MaxSkillHash.end())
        {
            level = maxIt->second;
        }
        else
        {
        sVal = GetConfig(SZ_SECT_MAX_SKILL_LVL, race);
        p = sVal.c_str();

        p = GetToken(S, p, ',', TRIM_ALL);
        maxlevel = atol(S.c_str());

        p = GetToken(S, p, ',', TRIM_ALL);
        level = atol(S.c_str());

        while (p && *p)
        {
            p = GetToken(S, p, ',', TRIM_ALL);
            if (0==stricmp(skill, S.c_str()))
            {
                level = maxlevel;
                break;
            }
        }

        if (IsArcadiaSkillSystem && *leadership)
        {
            if ( IsMagicSkill(skill))
            {
                if ( 0==stricmp(leadership, SZ_HERO))
                {
                    // reread magic skill
                    sVal = GetConfig(SZ_SECT_MAX_MAG_SKILL_LVL, race);
                    p = sVal.c_str();

                    p = GetToken(S, p, ',', TRIM_ALL);
                    maxlevel = atol(S.c_str());

                    p = GetToken(S, p, ',', TRIM_ALL);
                    level = atol(S.c_str());

                    while (p && *p)
                    {
                        p = GetToken(S, p, ',', TRIM_ALL);
                        if (0==stricmp(skill, S.c_str()))
                        {
                            level = maxlevel;
                            break;
                        }
                    }
                }
                else
                    level = 0;
            }
            else
            {
                // adjust for leadership
                int leader_bonus, hero_bonus, bonus=0;

                sVal = GetConfig(SZ_SECT_COMMON, SZ_KEY_LEAD_SKILL_BONUS);
                p = sVal.c_str();

                p = GetToken(S, p, ',', TRIM_ALL);
                leader_bonus = atol(S.c_str());

                p = GetToken(S, p, ',', TRIM_ALL);
                hero_bonus = atol(S.c_str());

                if (0==stricmp(leadership, SZ_LEADER))
                    bonus = leader_bonus;
                else
                    if (0==stricmp(leadership, SZ_HERO))
                        bonus = hero_bonus;
                level += bonus;
            }
        }

        m_MaxSkillHash[sKey.c_str()] = level;
        } // else (not found in cache)
    }

    return level;
}

//-------------------------------------------------------------------------

void CAhApp::GetProdDetails (const char * item, TProdDetails & details)
{
    std::string sVal, S;
    const char * p;
    int x;

    details.Empty();
    sVal = GetConfig(SZ_SECT_PROD_SKILL, item);
    if (!sVal.empty())
    {
        S = GetToken(details.skillname, sVal.c_str(), ' ', TRIM_ALL);
        details.skilllevel = atol(S.c_str());
    }

    sVal = GetConfig(SZ_SECT_PROD_RESOURCE, item);
    x = 0;
    p = sVal.c_str();
    while (p && *p && x<MAX_RES_NUM)
    {
        p = GetToken(details.resname[x], SkipSpaces(p), ' ', TRIM_ALL);
        p = GetToken(S, p, ',', TRIM_ALL);
        details.resamt[x] = atol(S.c_str());
        x++;
    }

    sVal = GetConfig(SZ_SECT_PROD_MONTHS, item);
    if (!sVal.empty())
        details.months = atol(sVal.c_str());

    sVal = GetConfig(SZ_SECT_PROD_TOOL, item);
    if (!sVal.empty())
    {
        S = GetToken(details.toolname, sVal.c_str(), ' ', TRIM_ALL);
        details.toolhelp = atol(S.c_str());
    }

}

//-------------------------------------------------------------------------

bool CAhApp::CanSeeAdvResources(const char * skillname, const char * terrain, std::vector<long> & Levels, std::vector<std::string> & Resources)
{
    std::string         ProdSkillLine;
    std::string         ProdLandLine;
    bool         Ok = false;
    const char * p1, * p2, *p;
    std::string         Prod1, Prod2, S1;
    long         level;

    Levels.clear();
    Resources.clear();

    ProdSkillLine = GetConfig(SZ_SECT_RESOURCE_SKILL,  skillname);
    TrimRight(ProdSkillLine, TRIM_ALL);

    ProdLandLine = GetConfig(SZ_SECT_RESOURCE_LAND,  terrain);
    TrimRight(ProdLandLine, TRIM_ALL);

    if (!ProdSkillLine.empty() && !ProdLandLine.empty())
    {
        p1 = SkipSpaces(GetToken(S1, ProdSkillLine.c_str(), ',', TRIM_ALL));
        while (!S1.empty())
        {
            p  = SkipSpaces(GetToken(Prod1, S1.c_str(), ' ', TRIM_ALL));
            level = p ? atol(p) : 0;

            p2 = SkipSpaces(GetToken(Prod2, ProdLandLine.c_str(), ',', TRIM_ALL));
            while (!Prod2.empty())
            {
                if (0==stricmp(Prod1.c_str(), Prod2.c_str()))
                {
                    Ok = true;
                    Levels.push_back(level);
                    Resources.push_back(Prod1.c_str());
                    break;
                }
                p2 = SkipSpaces(GetToken(Prod2, p2, ',', TRIM_ALL));
            }

            p1 = SkipSpaces(GetToken(S1, p1, ',', TRIM_ALL));
        }
    }

    return Ok;
}


//-------------------------------------------------------------------------

int CAhApp::GetAttitudeForFaction(int id)
{
    int player_id = atol( GetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID));
    if(id == player_id) return ATT_FRIEND2;
    int attitude = ATT_UNDECLARED;
    CAttitude * policy;
    for(int i=m_Attitudes.Count(); i>=0; i--)
    {
        policy = (CAttitude *) m_Attitudes.At(i);
        if(policy && (policy->FactionId == id)) attitude=policy->Stance;
    }
    if(attitude == ATT_UNDECLARED)
    {
        // check for default attitude
        for(int i=m_Attitudes.Count(); i>=0; i--)
        {
            policy = (CAttitude *) m_Attitudes.At(i);
            if(policy && (policy->FactionId == 0)) attitude=policy->Stance;
        }
    }
    return attitude;
}

//-------------------------------------------------------------------------
void CAhApp::SetAttitudeForFaction(int id, int attitude)
{
    int att_idx = -1;
    CAttitude * policy;
    if((attitude < ATT_FRIEND1) || (attitude >= ATT_UNDECLARED)) return;
    for(int i=m_Attitudes.Count(); i>=0; i--)
    {
        policy = (CAttitude *) m_Attitudes.At(i);
        if(policy && (policy->FactionId == id)) att_idx=i;
    }
    if(att_idx < 0)
    {   // new attitude declaration
        policy = new CAttitude;
        policy->FactionId = id;
        policy->SetStance(attitude);
        m_Attitudes.Insert(policy);
    }
    else
    {   // change existing declaration
        policy = (CAttitude *) m_Attitudes.At(att_idx);
        policy->SetStance(attitude);
    }
}

//-------------------------------------------------------------------------

void CAhApp::GetShortFactName(std::string & S, int FactionId)
{
#define MAX_F_NAME 8
    int           i;
    char          ch;
    CFaction    * pFaction;
//    CBaseObject   Dummy;
//    int           idx;

    S.clear();
//    Dummy.Id = FactionId;
//    if (m_pAtlantis->m_Factions.Search(&Dummy, idx))
//    {
//        pFaction = (CFaction*)m_pAtlantis->m_Factions.At(idx);
//        S = pFaction->Name;
//    }
//    else
//        S << (long)FactionId;
    pFaction = m_pAtlantis->GetFaction(FactionId);
    if (pFaction)
        S = pFaction->Name;
    else
        S << (long)FactionId;

    if (0==stricmp(S.c_str(), "faction"))
    {
        S.clear();
        S << "F_" << (long)FactionId << "_";
    }

    ToLower(S);
    for (i=S.size()-1; i>=0; i--)
    {
        ch = S.c_str()[i];
        if ( (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') )
            DelCh(S, i);
    }
    if (S.size() > MAX_F_NAME)
        DelSubStr(S, MAX_F_NAME, S.size()-MAX_F_NAME);
    TrimRight(S, TRIM_ALL);

}

//-------------------------------------------------------------------------

void CAhApp::SetMapFrameTitle()
{
    CMapFrame   * pMapFrame  = (CMapFrame *)m_Frames[AH_FRAME_MAP];
    CMapPane    * pMapPane   = (CMapPane  * )m_Panes[AH_PANE_MAP];
    CPlane      * pPlane     = nullptr;

    std::string          S;

    S = m_sTitle;

    if (pMapPane)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);

        S << " (" << pMapPane->m_SelHexX << "," << pMapPane->m_SelHexY;
        if (pPlane && 0!=stricmp(DEFAULT_PLANE, pPlane->Name.c_str()))
        {
            S << "," << pPlane->Name;
        }
        S << ")";
    }

    if (m_OrdersAreChanged)
        S << " [modified]";
    if (pMapFrame)
        pMapFrame->SetTitle(wxString::FromAscii(S.c_str()));
}

//-------------------------------------------------------------------------

void CAhApp::SetOrdersChanged(bool Changed)
{
    m_OrdersAreChanged = Changed;

    SetMapFrameTitle();
}


//-------------------------------------------------------------------------

int CAhApp::SaveOrders(bool UsingExistingName)
{
    std::string S, FName, Section;
    int  i, id, err=ERR_OK;

    for (i=0; i<((int)m_pAtlantis->m_OurFactions.size()); i++)
    {
        id = m_pAtlantis->m_OurFactions[i];
        if (UsingExistingName)
        {
            ComposeConfigOrdersSection(Section, id);
            S.clear();
            S << (long)m_pAtlantis->m_YearMon;
            FName = GetConfig(Section.c_str(), S.c_str());
            TrimRight(FName, TRIM_ALL);
        }
        err = SaveOrders(FName.c_str(), id);
        if (ERR_OK!=err)
            break;
    }

    if (ERR_OK==err)
        SetOrdersChanged(false);

    return err;
}

//-------------------------------------------------------------------------

int  CAhApp::SaveOrders(const char * FNameOut, int FactionId)
{

    int         err;
    char        buf[64];
    std::string        FName;
    std::string        Dir;
    std::string        S, Section, Prompt, Key;
    CFaction  * pFaction;

    FName = FNameOut;
    TrimRight(FName, TRIM_ALL);

    ComposeConfigOrdersSection(Section, FactionId);
    if (FName.empty())
    {
        Format(S, "%d", m_pAtlantis->m_YearMon);
        FName = GetConfig(Section.c_str(), S.c_str());
        TrimRight(FName, TRIM_ALL);

        if (FName.empty())
        {
            GetShortFactName(S, FactionId);
            if (S.empty())
                S << (long)FactionId;
            Format(FName, "%s%04d.ord", S.c_str(), m_pAtlantis->m_YearMon);
        }
        pFaction = m_pAtlantis->GetFaction(FactionId);

        Prompt = "Save orders for ";
        if (pFaction)
            Prompt << pFaction->Name.c_str() << " ";
        else
            Prompt << "Faction ";
        Prompt << (long)FactionId;

        Dir = GetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS);
        TrimRight(Dir, TRIM_ALL);
        if (Dir.empty())
            Dir = ".";

        std::string File;
        wxString CurrentDir = wxGetCwd();
        //MakePathFull(CurrentDir.mb_str(), FName);
        GetFileFromPath(FName.c_str(), File);

        MakePathFull(CurrentDir.mb_str(), Dir);
        wxFileDialog dialog((CMapFrame*)m_Frames[AH_FRAME_MAP],
                            wxString::FromAscii(Prompt.c_str()),
                            wxString::FromAscii(Dir.c_str()),
                            wxString::FromAscii(File.c_str()),
                            wxT(SZ_ORD_FILES),
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
        err = dialog.ShowModal();
        wxSetWorkingDirectory(CurrentDir);

        if (wxID_OK == err)
        {
            FName = dialog.GetPath().mb_str();
            MakePathRelative(CurrentDir.mb_str(), FName);
            GetDirFromPath(FName.c_str(), Dir);
            SetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS, Dir.c_str() );
        }
        else
            return ERR_CANCEL;

        TrimRight(FName, TRIM_ALL);
    }
    if (FName.empty())
        return ERR_FNAME;

    Key.clear();
    Key << (long)FactionId;

    err = m_pAtlantis->SaveOrders(FName.c_str(),
                                  GetConfig(SZ_SECT_PASSWORDS, Key.c_str()),
                                  (bool)atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_DECORATE_ORDERS)),
                                  FactionId
                                 );
    if (ERR_OK==err)
    {
        snprintf(buf, sizeof(buf), "%ld", m_pAtlantis->m_YearMon);
        SetConfig(Section.c_str(), buf, FName.c_str());
    }

    // Save config, too
    m_Config[CONFIG_FILE_CONFIG].Save(SZ_CONFIG_FILE);
    m_Config[CONFIG_FILE_STATE ].Save(SZ_CONFIG_STATE_FILE);

    if (ERR_OK==m_pAtlantis->m_ParseErr)
        SaveHistory(SZ_HISTORY_FILE);

    return err;
}

//-------------------------------------------------------------------------

void CAhApp::RedrawTracks()
{
    CUnit       * pUnit = GetSelectedUnit();
    CPlane      * pPlane;
    CMapPane    * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];

    if (!pMapPane)
        return;

    pPlane   = (CPlane*)m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);
    pMapPane->RedrawTracksForUnit(pPlane, pUnit, nullptr, true);
}

//-------------------------------------------------------------------------

CUnit * CAhApp::GetSelectedUnit()
{
    CUnit       * pUnit = nullptr;
    CUnitPane   * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];

    if (pUnitPane)
        pUnit = (CUnit*)pUnitPane->m_pUnits->At(m_SelUnitIdx);

    return pUnit;
}

//-------------------------------------------------------------------------

int  CAhApp::LoadOrders  (const char * FNameIn)
{
    int           err;
    std::string S, FName, Sect;
    int           factid;
//    CMapPane    * pMapPane = (CMapPane* )m_Panes[AH_PANE_MAP];


    FName = FNameIn;  // FNameIn can be coming from config, so do not use it directly!
    err = m_pAtlantis->LoadOrders(FName.c_str(), factid);
    if (ERR_OK==err)
    {
        S.clear();
        S << (long)m_pAtlantis->m_YearMon;
        ComposeConfigOrdersSection(Sect, factid);
        SetConfig(Sect.c_str(), S.c_str(), FName.c_str());

//        if (pMapPane)
//            pMapPane->Refresh(false, nullptr);
//            pMapPane->CleanCities(); //pMapPane->Refresh(false, nullptr); // to remove pointers to land wich could be replaced by joining orders

        OnMapSelectionChange();
        RedrawTracks();
    }
    else
       if (m_Frames[AH_FRAME_MSG])
           ((CAhFrame*)m_Frames[AH_FRAME_MSG])->Raise();

    return err;
}

//-------------------------------------------------------------------------

void EncodeConfigLine(std::string & dest, const char * src)
{
    dest.clear();
    while (src && *src)
    {
        switch (*src)
        {
        case '\r':  break;
        case '\n':  AddStr(dest, "\\n", 2);
                    break;
        default  :  AddCh(dest, *src);
        }
        src++;
    }
}

//-------------------------------------------------------------------------

void DecodeConfigLine(std::string & dest, const char * src)
{
    bool          Esc;

    dest.clear();
    Esc = false;
    while (src && *src)
    {
        if ('\\' == *src)


            Esc = true;
        else
        {
            if (Esc)
            {
                switch(*src)
                {
                case 'n':
                    dest << EOL_SCR;
                    break;
                default:
                    AddCh(dest, '\\');
                    AddCh(dest, *src);
                }
            }
            else
                AddCh(dest, *src);
            Esc = false;
        }

        src++;
    }
}

//-------------------------------------------------------------------------

void CAhApp::LoadComments()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);

        DecodeConfigLine(pUnit->DefOrders, GetConfig(SZ_SECT_DEF_ORDERS, buf));

        TrimRight(pUnit->DefOrders, TRIM_ALL);
        pUnit->ExtractCommentsFromDefOrders();
    }
    m_CommentsChanged = false;
}

//-------------------------------------------------------------------------

void CAhApp::SaveComments()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;
    const char  * p;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        S.clear();
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        TrimRight(pUnit->DefOrders, TRIM_ALL);
        if (pUnit->DefOrders.size() > 0)
        {
            EncodeConfigLine(S, pUnit->DefOrders.c_str());
            p = S.c_str();
        }
        else
            p = nullptr;
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);
        SetConfig(SZ_SECT_DEF_ORDERS, buf, p);
    }
    m_CommentsChanged = false;
}


//-------------------------------------------------------------------------

void CAhApp::LoadUnitFlags()
{
    int           i, x;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);

        x = atol(GetConfig(SZ_SECT_UNIT_FLAGS, buf));
        if (x & UNIT_CUSTOM_FLAG_MASK)
        {
            pUnit->Flags    |= (x & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsOrg |= (x & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsLast = ~pUnit->Flags;
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::SaveUnitFlags()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    std::string          S;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        snprintf(buf, sizeof(buf), "%ld", pUnit->Id);

        S.clear();
        if (pUnit->Flags & UNIT_CUSTOM_FLAG_MASK)
            S << (long)(pUnit->Flags & UNIT_CUSTOM_FLAG_MASK);
        SetConfig(SZ_SECT_UNIT_FLAGS, buf, S.c_str());
    }
}

//-------------------------------------------------------------------------

void CAhApp::SetAllLandUnitFlags()
{
    CUnitPane  * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];
    CPlane     * pPlane;
    CLand      * pLand;
    CUnit      * pUnit;
    int          i, n, f, x;
    int          rc;

    CUnitFlagsDlg dlg(m_Frames[AH_FRAME_MAP], eAll, 0);

    rc = dlg.ShowModal();

    if ((ID_BTN_SET_ALL_LAND==rc || ID_BTN_RMV_ALL_LAND==rc) && dlg.m_LandFlags>0)
    {
        for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
            for (i=0; i<pPlane->Lands.Count(); i++)
            {
                pLand = (CLand*)pPlane->Lands.At(i);
                x     = 1;
                for (f=0; f<LAND_FLAG_COUNT; f++)
                {
                    if (dlg.m_LandFlags & x)
                    {
                        if (ID_BTN_RMV_ALL_LAND==rc)
                        {
                            // clear flag
                            pLand->FlagText[f].clear();
                        }
                        else
                        {
                            // set flag
                            if (pLand->FlagText[f].empty())
                                pLand->FlagText[f] = LandFlagLabel[f];
                        }
                        pLand->Flags |= LAND_HAS_FLAGS;
                    }
                    x <<= 1;
                }
            }
        }

        if (m_Panes[AH_PANE_MAP])
            (m_Panes[AH_PANE_MAP])->Refresh(false);
    }

    if ( (ID_BTN_SET_ALL_UNIT==rc || ID_BTN_RMV_ALL_UNIT==rc) && dlg.m_UnitFlags>0 )
    {
        for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
        {
            pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);

            if (ID_BTN_SET_ALL_UNIT==rc)
            {
                pUnit->Flags    |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
                pUnit->FlagsOrg |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            }
            else
            {
                pUnit->Flags    &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
                pUnit->FlagsOrg &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            }

            pUnit->FlagsLast = ~pUnit->Flags;
        }
        if (pUnitPane)
            pUnitPane->Update(pUnitPane->m_pCurLand);
    }
}

//-------------------------------------------------------------------------

void CAhApp::SaveLandFlags()
{
    int          i, n, f;
    CPlane     * pPlane;
    CLand      * pLand;
    std::string         sName;
    std::string         sData;
    long         ym_last;
    long         ym_first;
    const char * p;

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            sData.clear();
            for (f=0; f<LAND_FLAG_COUNT; f++)
            {
                TrimRight(pLand->FlagText[f], TRIM_ALL);
                if (!pLand->FlagText[f].empty())
                {
                    if (!sData.empty())
                        sData << "\\n";
                    sData << (long)f << ":" << pLand->FlagText[f];
                }
            }
            m_pAtlantis->ComposeLandStrCoord(pLand, sName);


            if (!sData.empty() || (pLand->Flags & LAND_HAS_FLAGS)) // allow to remove flags
                SetConfig(SZ_SECT_LAND_FLAGS, sName.c_str(), sData.c_str());

            if (pLand->Flags&LAND_IS_CURRENT) //LAND_UNITS)
            {
                //ym = atol(GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()));
                p        = GetToken(sData, GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()), ',');
                ym_last  = atol(sData.c_str());
                if (sData.empty())
                    ym_first = m_pAtlantis->m_YearMon;
                else
                {
                    p        = GetToken(sData, SkipSpaces(p), ',');
                    ym_first = atol(sData.c_str());
                }
                if (ym_last < m_pAtlantis->m_YearMon)
                {
                    sData.clear();
                    sData << m_pAtlantis->m_YearMon << "," << ym_first;
                    SetConfig(SZ_SECT_LAND_VISITED, sName.c_str(), sData.c_str());
                }
            }
        }
    }

//    m_LandFlagsChanged = false;
}

//-------------------------------------------------------------------------

void CAhApp::LoadLandFlags()
{
    int               sectidx, n;
    const char      * szName;
    const char      * szValue;
    const char      * p;
    const char      * line;
    std::string              sData, sLine, sN;
    CLand           * pLand;

    sectidx = GetSectionFirst(SZ_SECT_LAND_FLAGS, szName, szValue);
    while (sectidx >= 0)
    {
        pLand   = m_pAtlantis->GetLand(szName);
        if (pLand)
        {
            DecodeConfigLine(sData, szValue);

            line = sData.c_str();
            while (line && *line)
            {
                line = GetToken(sLine, line, '\n');
                p    = sLine.c_str();
                p    = GetToken(sN, p, ':');
                if (p)
                    n = atoi(sN.c_str());
                else
                {
                    p = sN.c_str();
                    n = 0;
                }
                if (n<0 || n>=LAND_FLAG_COUNT)
                    n = 0;
                pLand->FlagText[n] = p;
                pLand->Flags |= LAND_HAS_FLAGS;
            }

        }
        sectidx = GetSectionNext(sectidx, SZ_SECT_LAND_FLAGS, szName, szValue);
    }


}


//-------------------------------------------------------------------------

void CAhApp::UpdateEdgeStructs()
{
    int          i, n, k;
    int          d, adj_dir;
    int          x, y, z;
    int          adj_index;
    CPlane     * pPlane;
    CLand      * pLand, * adj_land;
    CStruct    * pEdge;

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            if(!pLand) continue;
            // set the Water-Type flag
            if(m_WaterTerrainNames.find(ToLower(pLand->TerrainType)) != m_WaterTerrainNames.end())
            {
                pLand->Flags |= LAND_IS_WATER;
            }
            for(d=0; d<6; d++)
            {
                adj_dir = (d%6)-3;
                if(adj_dir < 0) adj_dir += 6;
                LandIdToCoord(pLand->Id,x,y,z);
                switch (d%6)
                {
                    case North     : y -= 2;     break;
                    case Northeast : y--; x++;   break;
                    case Southeast : y++; x++;   break;
                    case South     : y += 2;     break;
                    case Southwest : y++; x--;   break;
                    case Northwest : y--; x--;   break;
                }
                CBaseObject Dummy;
                Dummy.Id = LandCoordToId(x, y, z);
                if (pPlane->Lands.Search(&Dummy, adj_index))
                {
                    adj_land = (CLand *) pPlane->Lands.At(adj_index);
                    if(adj_land)
                    {
                        if((pLand->Flags&LAND_IS_CURRENT) && !(adj_land->Flags&LAND_IS_CURRENT))
                        {  // set the corresponding Edge Structure in adjacent region
                            adj_land->RemoveEdgeStructs(adj_dir);
                            for(k=pLand->EdgeStructs.Count(); k>=0; k--)
                            {
                                pEdge = (CStruct*) pLand->EdgeStructs.At(k);
                                if((pEdge != nullptr) && (pEdge->Location == d))                          adj_land->AddNewEdgeStruct(pEdge->Kind.c_str(), adj_dir);
                            }
                        }
                        // set CoastBits
                        if(m_WaterTerrainNames.find(ToLower(adj_land->TerrainType)) != m_WaterTerrainNames.end())
                        {
                            if(!(pLand->Flags & LAND_IS_WATER))
                                adj_land->CoastBits |= ExitFlags[adj_dir];
                        }
                        else if(pLand->Flags&LAND_IS_WATER)
                        {
                            pLand->CoastBits |= ExitFlags[d];
                        }
                    }
                }
            }
        }
    }
}


//-------------------------------------------------------------------------

void CAhApp::WriteMagesCSV()
{
    std::string FName;
    std::string S;


//    GetShortFactName(S);
//    Format(FName, "%s_%s%04d.csv", S.c_str(), "mages", m_pAtlantis->m_YearMon);
    Format(FName, "%s%04d.csv", "mages", m_pAtlantis->m_YearMon);

    CExportMagesCSVDlg Dlg(m_Frames[AH_FRAME_MAP], FName.c_str());
    if (wxID_OK == Dlg.ShowModal())
        m_pAtlantis->WriteMagesCSV(Dlg.m_pFileName->GetValue().mb_str(),
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
                pDetail->amount -= men*atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));
        }
    }


    // Output
    for (x=0; x<Factions.Count(); x++)
    {
        pDetail = (CTaxProdDetails*)Factions.At(x);
        OneLine.clear();

        m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
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

//    m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
//    Details << pLand->TerrainType << " (" << sCoord << "). ";

    for (k=0; k<pLand->Products.Count(); k++)
    {
        pProd = (CProduct*)pLand->Products.At(k);
        if (0==pProd->Amount)
            continue;
  //      amount = pProd->Amount;
        GetProdDetails(pProd->ShortName.c_str(), details);
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

            m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
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

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
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

    ShowError(Report.c_str()      , Report.size()      , true);
}

//-------------------------------------------------------------------------

void CAhApp::CheckProduction()
{
    int    n, i, x;
    CLand  * pLand;
    CPlane * pPlane;
    CUnit  * pUnit;
    std::string Error, S;

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            for (x=0; x<pLand->Units.Count(); x++)
            {
                pUnit = (CUnit*)pLand->Units.At(x);
                if (!m_pAtlantis->CheckResourcesForProduction(pUnit, pLand, S))
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
        ShowError(S.c_str(), S.size(), true);
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

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
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
        ShowError(S.c_str(), S.size(), true);
    }
}

//--------------------------------------------------------------------------

#define SET_UNIT_PROP_NAME(_name, _type)                                 \
{                                                                        \
    m_pAtlantis->m_UnitPropertyNames.insert(_name);                      \
    m_pAtlantis->m_UnitPropertyTypes.emplace(_name, (int)(_type));       \
}

void CAhApp::PreLoadReport()
{
    std::string S, FName;

    SaveLandFlags();
    SaveUnitFlags();
    if (m_CommentsChanged)
        SaveComments();
    if (GetOrdersChanged())
        SaveOrders(true);

    if (ERR_OK==m_pAtlantis->m_ParseErr)
        SaveHistory(SZ_HISTORY_FILE);


}

//-------------------------------------------------------------------------

void CAhApp::PostLoadReport()
{
    std::string              S;
    CMapFrame       * pMapFrame  = (CMapFrame    *)m_Frames[AH_FRAME_MAP];
    CMapPane        * pMapPane   = (CMapPane     *)m_Panes [AH_PANE_MAP];
    CUnitPaneFltr   * pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
    CUnitPane       * pUnitPane  = (CUnitPane    *)m_Panes [AH_PANE_UNITS_HEX];
    long              year, mon;
    const char      * szName;
    const char      * szValue;
    CUnit           * pUnit;
    CPlane          * pPlane;
    CShortNamedObj  * pItem;
    CFaction          DummyFaction;
    CFaction        * pFaction;
    int               i, n;


    // update edge structures
    UpdateEdgeStructs();

    SaveLandFlags();

    // count number of our men in every hex

    m_pAtlantis->CountMenForTheFaction(m_pAtlantis->m_CrntFactionId);

    if (pMapFrame)
    {
        m_sTitle.clear();

        for (i=0; i<((int)m_pAtlantis->m_OurFactions.size()); i++)
        {
            pFaction = m_pAtlantis->GetFaction(m_pAtlantis->m_OurFactions[i]);
            if (pFaction)
            {
                if (!m_sTitle.empty())
                    m_sTitle << ", ";
                if (((int)m_pAtlantis->m_OurFactions.size())<3)
                    m_sTitle << pFaction->Name << " ";
                m_sTitle << (long)pFaction->Id;
            }
        }
        year = (long)(m_pAtlantis->m_YearMon/100);
        mon  = m_pAtlantis->m_YearMon % 100 - 1;
        if ( (mon >= 0) && (mon < 12) )
            m_sTitle << ". " << Monthes[mon] << " year " << year;
        SetMapFrameTitle();
    }

    // if loaded for the very first time, center it
    if (GetSectionFirst(SZ_SECT_REPORTS, szName, szValue) < 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, tool_centerout);

        if (m_Panes[AH_PANE_MAP])
            ((CMapPane*)m_Panes[AH_PANE_MAP])->OnToolbarCmd(event);
    }



    // stnadard unit and base properties - that is likely to be forgotten when
    // new properties are introduced :((
    SET_UNIT_PROP_NAME(PRP_COMMENTS          , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_ORDERS            , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FACTION_ID        , eLong   )
    SET_UNIT_PROP_NAME(PRP_FACTION           , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_LAND_ID           , eLong   )
    SET_UNIT_PROP_NAME(PRP_ID                , eLong   )
    SET_UNIT_PROP_NAME(PRP_NAME              , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FULL_TEXT         , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_TEACHING          , eLong   )
    SET_UNIT_PROP_NAME(PRP_SEQUENCE          , eLong   )
    SET_UNIT_PROP_NAME(PRP_FRIEND_OR_FOE     , eLong   )
    SET_UNIT_PROP_NAME(PRP_WEIGHT            , eLong   )
    SET_UNIT_PROP_NAME(PRP_DESCRIPTION       , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_COMBAT            , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_MOVEMENT          , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_STANDARD    , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_CUSTOM      , eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_CUSTOM_ABBR , eCharPtr)



    // If no orders loaded, no movement will be calculated. Force it.
    if (!m_pAtlantis->m_OrdersLoaded)
    {
        for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
        {
            pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
            pUnit->ResetNormalProperties();
        }

        for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
            for (i=0; i<pPlane->Lands.Count(); i++)
            {
                ((CLand*)pPlane->Lands.At(i))->CalcStructsLoad();
                ((CLand*)pPlane->Lands.At(i))->SetFlagsFromUnits(m_pAtlantis.get()); // maybe not needed here...
            }
        }
    }

    // skills
    for (i=0; i<m_pAtlantis->m_Skills.Count(); i++)
    {
        pItem = (CShortNamedObj*)m_pAtlantis->m_Skills.At(i);


        EncodeConfigLine(S, pItem->Description.c_str());
        SetConfig(SZ_SECT_SKILLS, pItem->Name.c_str(), S.c_str());
    }

    // Items
    for (i=0; i<m_pAtlantis->m_Items.Count(); i++)
    {
        pItem = (CShortNamedObj*)m_pAtlantis->m_Items.At(i);

        EncodeConfigLine(S, pItem->Description.c_str());
        SetConfig(SZ_SECT_ITEMS, pItem->Name.c_str(), S.c_str());
    }

    // Objects
    for (i=0; i<m_pAtlantis->m_Objects.Count(); i++)
    {
        pItem = (CShortNamedObj*)m_pAtlantis->m_Objects.At(i);

        EncodeConfigLine(S, pItem->Description.c_str());
        SetConfig(SZ_SECT_OBJECTS, pItem->Name.c_str(), S.c_str());
    }

    if (pMapPane)
        pMapPane->Refresh(false, nullptr);


    if (pUnitPane)
        pUnitPane->m_pCurLand = nullptr; // force the unit pane to do full update

    // Restore the last selected unit in the selected hex
    if (pMapPane && m_pAtlantis)
    {
        std::string   sSection;
        sSection << "PLANE_" << pMapPane->m_SelPlane;
        long savedUnitId = atol(GetConfig(sSection.c_str(), SZ_KEY_UNIT_SEL));
        if (savedUnitId)
        {
            CLand * pLand = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);
            if (pLand)
                pLand->guiUnit = savedUnitId;
        }
    }

    OnMapSelectionChange();

    // if there were Hex Events, show them
    if (!m_pAtlantis->m_HexEvents.Description.empty())
    {
        CBaseColl   Coll;
        Coll.Insert(&m_pAtlantis->m_HexEvents);
        ShowDescriptionList(Coll, "Hex Events");
    }

    // show newly discovered products (advanced resources), if any
    if (m_pAtlantis->m_NewProducts.Count() > 0)
        ShowDescriptionList(m_pAtlantis->m_NewProducts, "New products");

    if (pUnitPaneF)
        pUnitPaneF->Update(nullptr);

    CheckRedirectedOutputFiles();
    
    if (!m_pAtlantis->m_SecurityEvents.Description.empty())
        m_pAtlantis->m_SecurityEvents.Description << EOL_SCR << EOL_SCR;
}

//-------------------------------------------------------------------------

int  CAhApp::LoadReport  (const char * FNameIn, bool Join)
{
    std::string S, Sect, S2;
    std::string FName;
    int  LoadOrd;
    long n;
    int  err = ERR_FOPEN;

    wxBeginBusyCursor();

    m_DisableErrs = true;

    if (FNameIn && *FNameIn)
    {
        FName = FNameIn;
        TrimRight(FName, TRIM_ALL);

        PreLoadReport();

        if (!m_FirstLoad && !Join)
        {
            auto cachedCurrent = std::move(m_pAtlantis);
            if (cachedCurrent)
            {
                const long cachedYear = cachedCurrent->m_YearMon;
                auto reportIt = std::lower_bound(m_Reports.begin(), m_Reports.end(), cachedYear,
                    [](const std::unique_ptr<CAtlaParser>& report, long year)
                    {
                        return report->m_YearMon < year;
                    });
                if (reportIt != m_Reports.end() && (*reportIt)->m_YearMon == cachedYear)
                    reportIt = m_Reports.erase(reportIt);
                m_Reports.insert(reportIt, std::move(cachedCurrent));
            }
            m_pAtlantis.reset(new CAtlaParser(&ThisGameDataHelper));
            m_pAtlantis->m_pConfig = &m_Config[CONFIG_FILE_CONFIG];
        }

        if (!Join)
        {
            m_pAtlantis->Clear();
            m_pAtlantis->ParseRep(SZ_HISTORY_FILE, false, true);
        }

        // Append unit group property names here so they are available while parsing
        for (auto & upg__ : m_UnitPropertyGroups)
            SET_UNIT_PROP_NAME(upg__.first.c_str(), eLong)


        err = m_pAtlantis->ParseRep(FName.c_str(), Join, false);
        switch (err)
        {
            case ERR_INV_TURN:
                wxMessageBox(wxT("Wrong turn in the report"), wxT("Error"));
                break;
        }
        SetOrdersChanged(false);
        m_CommentsChanged = false;
        if ( ERR_OK==err && m_pAtlantis->m_YearMon != 0 && m_pAtlantis->m_CrntFactionId != 0 )
        {
            {
                long _ym = m_pAtlantis->m_YearMon;
                auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), _ym);
                if (_it == m_ReportDates.end() || *_it != _ym)
                    m_ReportDates.insert(_it, _ym);
            }
            UpgradeConfigByFactionId();

            if (atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_READ)) && !m_pAtlantis->m_CrntFactionPwd.empty())
            {
                S.clear();
                S << (long)m_pAtlantis->m_CrntFactionId;
                SetConfig(SZ_SECT_PASSWORDS, S.c_str(), m_pAtlantis->m_CrntFactionPwd.c_str() );
            }

            LoadOrd = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_ORDER));
            if (LoadOrd)
            {
                S.clear();
                S << (long)m_pAtlantis->m_YearMon;
                ComposeConfigOrdersSection(Sect, m_pAtlantis->m_CrntFactionId);
                LoadOrders(GetConfig(Sect.c_str(), S.c_str()));
            }
        }

        LoadComments();
        LoadLandFlags();
        LoadUnitFlags();
        PostLoadReport();

        if ( (ERR_OK==err) && (m_pAtlantis->m_YearMon != 0) )
        {
            // doing it after PostLoadReport() since it will check the section
            S.clear();
            S << (long)m_pAtlantis->m_YearMon;
            if (!Join)
                SetConfig(SZ_SECT_REPORTS, S.c_str(), FName.c_str());
            else
            {
                S2 = GetConfig(SZ_SECT_REPORTS, S.c_str());
                if (!S2.empty())
                    S2 << ", ";
                S2 << FName;
                SetConfig(SZ_SECT_REPORTS, S.c_str(), S2.c_str());
            }
        }

        if (!m_FirstLoad && !Join)
        {
            n = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_REP_CACHE_COUNT));
            if (n<=0)
                n = 1;

            const long maxCachedReports = n - 1;
            auto currentPos = std::lower_bound(m_Reports.begin(), m_Reports.end(), m_pAtlantis->m_YearMon,
                [](const std::unique_ptr<CAtlaParser>& report, long year)
                {
                    return report->m_YearMon < year;
                });
            long currentInsertIdx = currentPos - m_Reports.begin();
            while ((long)m_Reports.size() > maxCachedReports)
            {
                if (currentInsertIdx > maxCachedReports/2)
                {
                    m_Reports.erase(m_Reports.begin());
                    if (currentInsertIdx > 0)
                        --currentInsertIdx;
                }
                else
                    m_Reports.pop_back();
            }
        }
        m_FirstLoad = false;
    }

    m_DisableErrs = false;

    wxEndBusyCursor();

    return err;
}

//-------------------------------------------------------------------------

int  CAhApp::LoadReport(bool Join)
{
    int rc;
    std::string Dir;
    const char * key;

    key = Join ? SZ_KEY_FOLDER_REP_JOIN : SZ_KEY_FOLDER_REP_LOAD;
    Dir = GetConfig(SZ_SECT_FOLDERS, key);
    if (Dir.empty())
        Dir = ".";

    wxString CurrentDir = wxGetCwd();
    wxFileDialog dialog(m_Frames[AH_FRAME_MAP],
                        wxT("Load Report"),
                        wxString::FromAscii(Dir.c_str()),
                        wxT(""),
                        wxT(SZ_REP_FILES),
                        wxFD_OPEN);
    rc = dialog.ShowModal();
    wxSetWorkingDirectory(CurrentDir);

    if (wxID_OK == rc)
    {
        std::string S;
        S = dialog.GetPath().mb_str();
        MakePathRelative(CurrentDir.mb_str(), S);

        GetDirFromPath(S.c_str(), Dir);
        SetConfig(SZ_SECT_FOLDERS, key, Dir.c_str() );

        return LoadReport(S.c_str(), Join);
    }
    else
        return ERR_CANCEL;

}

//-------------------------------------------------------------------------

void CAhApp::EditPaneChanged(CEditPane * pPane)
{
    CMapPane  * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
    CLand     * pLand;
    CUnit     * pUnit;

    if (pPane && pMapPane)
    {
        pLand = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);

        if (pPane == m_Panes[AH_PANE_UNIT_COMMANDS])
        {
            // selected unit's orders have been changed

            // TBD: is it needed? m_pCurLand->guiUnit = m_pUnitListPane->GetCurrentUnitId();
            m_pAtlantis->RunOrders(pLand);
            UpdateHexUnitList(pLand);
            UpdateHexEditPane(pLand);
            SetOrdersChanged(m_OrdersAreChanged); // this hack is needed since EditPanes are modifying the vars directly...
        }
        else if (pPane == m_Panes[AH_PANE_UNIT_COMMENTS])
        {
            // selected unit's comments / default orders have been changed
            pUnit = GetSelectedUnit(); // depends on m_SelUnitIdx
            if (pUnit)
            {
                pUnit->ExtractCommentsFromDefOrders();
                UpdateHexUnitList(pLand);
            }
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::SelectTempUnit(CUnit * pUnit)
{
    CEditPane   * pDescription = (CEditPane*)m_Panes[AH_PANE_UNIT_DESCR   ];
    CEditPane   * pOrders      = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMANDS];
    CEditPane   * pComments    = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMENTS];

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

void CAhApp::SelectUnit(CUnit * pUnit)
{
    CMapPane    * pMapPane  = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
    CUnitPane   * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
    CLand       * pLand;
    CPlane      * pPlane;
    int           nx, ny, nz;
    bool          refresh;
    bool          NeedSetUnit;

    if (!pUnit || !pMapPane)
        return;
    pLand = m_pAtlantis->GetLand(pUnit->LandId);
    if (!pLand)
        return;

    pLand->guiUnit = pUnit->Id;

    LandIdToCoord(pLand->Id, nx, ny, nz);
    pPlane   = (CPlane*)m_pAtlantis->m_Planes.At(nz);

    refresh = pMapPane->EnsureLandVisible(nx, ny, nz, false);
    if (refresh)
        pMapPane->Refresh(false);

    NeedSetUnit = (pUnitPane && (pLand==pUnitPane->m_pCurLand));

    pMapPane->SetSelection(nx, ny, pUnit, pPlane, true);

    if (pUnit->Flags & UNIT_FLAG_TEMP)
    {
        pUnitPane->SelectUnit(-1);
        SelectTempUnit(pUnit);  // just redraw description
    }
    else
        if (NeedSetUnit)
            pUnitPane->SelectUnit(pUnit->Id); // otherwise will be already selected
}

//-------------------------------------------------------------------------

void CAhApp::SelectLand(CLand * pLand)
{
    CMapPane    * pMapPane  = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
    CUnitPane   * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
    CPlane      * pPlane;
    int           nx, ny, nz;
    bool          refresh;

    if (pLand)
    {
        LandIdToCoord(pLand->Id, nx, ny, nz);
        pPlane   = (CPlane*)gpApp->m_pAtlantis->m_Planes.At(nz);

        refresh = pMapPane->EnsureLandVisible(nx, ny, nz, true);
        if (refresh)
            pMapPane->Refresh(false);


        if (!pUnitPane || pLand != pUnitPane->m_pCurLand)
            pMapPane->SetSelection(nx, ny, nullptr, pPlane, true);
    }
}

//-------------------------------------------------------------------------

bool CAhApp::SelectLand(const char * landcoords) //  "48,52[,somewhere]"
{
    CLand       * pLand     = m_pAtlantis->GetLand(landcoords);

    if (pLand)
    {
        SelectLand(pLand);
        return true;
    }
    else
        return false;
}

//-------------------------------------------------------------------------

void CAhApp::EditPaneDClicked(CEditPane * pPane)
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
        if (position > src.size())
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
            if (m_pAtlantis->m_Units.Search(&Dummy, idx))
            {
                pUnit = (CUnit*)m_pAtlantis->m_Units.At(idx);
                SelectUnit(pUnit);
                return;
            }
        }


        p = &src.c_str()[position];
        p = SkipSpaces(GetToken(S, p, "(\n", ch, TRIM_ALL)); // must be an error from the report file
        if ('('==ch)
        {
            GetToken(S, p, ")\n", ch, TRIM_ALL);
            Dummy.Id = atol(S.c_str());
            if (')'==ch && m_pAtlantis->m_Units.Search(&Dummy, idx))
            {
                pUnit = (CUnit*)m_pAtlantis->m_Units.At(idx);
                SelectUnit(pUnit);
                return;
            }
        }

        // land
        p = &src.c_str()[position];
        p = SkipSpaces(GetToken(S, p, "(\n", ch, TRIM_ALL));
        if ('('==ch)
        {
            p = SkipSpaces(GetToken(S, p, ")\n", ch, TRIM_ALL));
            if (')' == ch && SelectLand(S.c_str()))
                return;
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::SwitchToYearMon(long YearMon)
{
    std::string          S, S2;

    PreLoadReport();
    if (GetOrdersChanged())
        return;

    auto reportIt = std::lower_bound(m_Reports.begin(), m_Reports.end(), YearMon,
        [](const std::unique_ptr<CAtlaParser>& report, long year)
        {
            return report->m_YearMon < year;
        });
    if (reportIt != m_Reports.end() && (*reportIt)->m_YearMon == YearMon)
    {
        if (m_pAtlantis && m_pAtlantis->m_YearMon != YearMon)
        {
            auto cachedCurrent = std::move(m_pAtlantis);
            const long cachedYear = cachedCurrent->m_YearMon;
            auto cachedIt = std::lower_bound(m_Reports.begin(), m_Reports.end(), cachedYear,
                [](const std::unique_ptr<CAtlaParser>& report, long year)
                {
                    return report->m_YearMon < year;
                });
            if (cachedIt != m_Reports.end() && (*cachedIt)->m_YearMon == cachedYear)
                cachedIt = m_Reports.erase(cachedIt);
            m_Reports.insert(cachedIt, std::move(cachedCurrent));
        }

        reportIt = std::lower_bound(m_Reports.begin(), m_Reports.end(), YearMon,
            [](const std::unique_ptr<CAtlaParser>& report, long year)
            {
            return report->m_YearMon < year;
            });
        m_pAtlantis = std::move(*reportIt);
        m_Reports.erase(reportIt);
        if (m_pAtlantis) m_pAtlantis->m_pConfig = &m_Config[CONFIG_FILE_CONFIG];
        PostLoadReport();
    }
    else
    {
        S.clear();
        S << YearMon;

        S2 = GetConfig(SZ_SECT_REPORTS, S.c_str());
        const char * p = S2.c_str();
        bool         join = false;
        while (p && *p)
        {
            p = GetToken(S, p, ',');
            LoadReport(S.c_str(), join);
            join = true;
        }
    }
}

//-------------------------------------------------------------------------

void CAhApp::SwitchToRep(eRepSeq whichrep)
{
    int  i;

    m_DisableErrs = true;

    if (CanSwitchToRep(whichrep, i))
        SwitchToYearMon(m_ReportDates[i]);

    m_DisableErrs = false;
}

//-------------------------------------------------------------------------

bool CAhApp::CanSwitchToRep(eRepSeq whichrep, int & RepIdx)
{
    long       ym;
    std::string       sName, sData;
    CLand    * pLand;
    CMapPane * pMapPane;

    RepIdx=-1;

    switch(whichrep)
    {
    case repFirst:
        RepIdx = 0;
        break;

    case repLast:
        if (m_ReportDates.empty())
            RepIdx = -1;
        else if (m_pAtlantis->m_YearMon == m_ReportDates[(int)gpApp->m_ReportDates.size()-1] )
            RepIdx = -1;
        else
            RepIdx = ((int)m_ReportDates.size())-1;
        break;

    case repPrev:
        {
            auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), (long)m_pAtlantis->m_YearMon);
            if (_it != m_ReportDates.end() && *_it == m_pAtlantis->m_YearMon)
            {
                RepIdx = (int)(_it - m_ReportDates.begin());
                RepIdx--;
            }
        }
        break;

    case repNext:
        {
            auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), (long)m_pAtlantis->m_YearMon);
            if (_it != m_ReportDates.end() && *_it == m_pAtlantis->m_YearMon)
            {
                RepIdx = (int)(_it - m_ReportDates.begin());
                RepIdx++;
            }
        }
        break;

    case repLastVisited:
        pMapPane = (CMapPane* )m_Panes[AH_PANE_MAP];
        pLand    = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);
        m_pAtlantis->ComposeLandStrCoord(pLand, sName);
//        ym       = atol(GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()));
        GetToken(sData, GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()), ',');
        ym = atol(sData.c_str());

        {
            auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), ym);
            bool _found = _it != m_ReportDates.end() && *_it == ym;
            RepIdx = _found ? (int)(_it - m_ReportDates.begin()) : -1;
            if (ym==m_pAtlantis->m_YearMon || !_found)
                RepIdx = -1;
        }
        break;
    }

    return (RepIdx>=0 && RepIdx<((int)m_ReportDates.size()));
}

//-------------------------------------------------------------------------

bool CAhApp::GetPrevTurnReport(CAtlaParser *& pPrevTurn)
{
    int idx;
    
    pPrevTurn = nullptr;
        
    if (CanSwitchToRep(repPrev, idx))
    {
        std::string          S, S2;
    
        long YearMon = m_ReportDates[idx];
    
        auto reportIt = std::lower_bound(m_Reports.begin(), m_Reports.end(), YearMon,
            [](const std::unique_ptr<CAtlaParser>& report, long year)
            {
                return report->m_YearMon < year;
            });
        if (reportIt != m_Reports.end() && (*reportIt)->m_YearMon == YearMon)
        {
            pPrevTurn = reportIt->get();
        }
        else
        {
            S.clear();
            S << YearMon;
    
            S2 = GetConfig(SZ_SECT_REPORTS, S.c_str());
            const char * p = S2.c_str();
            bool         join = false;
            m_DisableErrs = true;
            wxBeginBusyCursor();
            std::unique_ptr<CAtlaParser> prevTurn(new CAtlaParser(&ThisGameDataHelper));
            prevTurn->ParseRep(SZ_HISTORY_FILE, false, true);
            while (p && *p)
            {
                p = GetToken(S, p, ',');
                //LoadReport(S.c_str(), join);
                prevTurn->ParseRep(S.c_str(), join, false);
                join = true;
            }
            wxEndBusyCursor();
            m_DisableErrs = false;
            if (prevTurn->m_YearMon == YearMon)
            {
                pPrevTurn = prevTurn.get();
                auto insertIt = std::lower_bound(m_Reports.begin(), m_Reports.end(), YearMon,
                    [](const std::unique_ptr<CAtlaParser>& report, long year)
                    {
                        return report->m_YearMon < year;
                    });
                if (insertIt != m_Reports.end() && (*insertIt)->m_YearMon == YearMon)
                    insertIt = m_Reports.erase(insertIt);
                m_Reports.insert(insertIt, std::move(prevTurn));
            }
            else
                pPrevTurn = nullptr;
        }
    }

    return (pPrevTurn != nullptr);
}

/*
    std::string S, Sect, S2;
    std::string FName;
    int  LoadOrd;
    int  i;
    long n;
    int  err = ERR_FOPEN;

    wxBeginBusyCursor();

    m_DisableErrs = true;

    if (FNameIn && *FNameIn)
    {
        FName = FNameIn;
        TrimRight(FName, TRIM_ALL);

        PreLoadReport();

        if (!m_FirstLoad && !Join)
            m_pAtlantis = new CAtlaParser(&ThisGameDataHelper);
            m_pAtlantis->m_pConfig = &m_Config[CONFIG_FILE_CONFIG];

        if (!Join)
        {
            m_pAtlantis->Clear();
            m_pAtlantis->ParseRep(SZ_HISTORY_FILE, false, true);
        }

        // Append unit group property names here so they are available while parsing
        for (auto & upg__ : m_UnitPropertyGroups)
            SET_UNIT_PROP_NAME(upg__.first.c_str(), eLong)


        err = m_pAtlantis->ParseRep(FName.c_str(), Join, false);
        switch (err)
        {
            case ERR_INV_TURN:
                wxMessageBox("Wrong turn in the report", "Error");
                break;
        }
        SetOrdersChanged(false);
        m_CommentsChanged = false;
        if ( ERR_OK==err && m_pAtlantis->m_YearMon != 0 && m_pAtlantis->m_CrntFactionId != 0 )
        {
            {
                long _ym = m_pAtlantis->m_YearMon;
                auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), _ym);
                if (_it == m_ReportDates.end() || *_it != _ym)
                    m_ReportDates.insert(_it, _ym);
            }
            UpgradeConfigByFactionId();

            if (atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_READ)) && !m_pAtlantis->m_CrntFactionPwd.empty())
            {
                S.clear();
                S << (long)m_pAtlantis->m_CrntFactionId;
                SetConfig(SZ_SECT_PASSWORDS, S.c_str(), m_pAtlantis->m_CrntFactionPwd.c_str() );
            }

            LoadOrd = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_ORDER));
            if (LoadOrd)
            {
                S.clear();
                S << (long)m_pAtlantis->m_YearMon;
                ComposeConfigOrdersSection(Sect, m_pAtlantis->m_CrntFactionId);
                LoadOrders(GetConfig(Sect.c_str(), S.c_str()));
            }
        }

        LoadComments();
        LoadLandFlags();
        LoadUnitFlags();
        PostLoadReport();

        if ( (ERR_OK==err) && (m_pAtlantis->m_YearMon != 0) )
        {
            // doing it after PostLoadReport() since it will check the section
            S.clear();
            S << (long)m_pAtlantis->m_YearMon;
            if (!Join)
                SetConfig(SZ_SECT_REPORTS, S.c_str(), FName.c_str());
            else
            {
                S2 = GetConfig(SZ_SECT_REPORTS, S.c_str());
                if (!S2.empty())
                    S2 << ", ";
                S2 << FName;
                SetConfig(SZ_SECT_REPORTS, S.c_str(), S2.c_str());
            }
        }

        if (!m_FirstLoad && !Join)
        {
            if (([&]() -> bool { auto _ri = std::lower_bound(m_Reports.begin(), m_Reports.end(), m_pAtlantis, [](CAtlaParser* a, CAtlaParser* b){ return a->m_YearMon < b->m_YearMon; }); if (_ri != m_Reports.end() && (*_ri)->m_YearMon == (m_pAtlantis)->m_YearMon) { i = (int)(_ri - m_Reports.begin()); return true; } return false; })())
                { delete m_Reports[i]; m_Reports.erase(m_Reports.begin() + (i)); }
            { auto _ri = std::lower_bound(m_Reports.begin(), m_Reports.end(), m_pAtlantis, [](CAtlaParser* a, CAtlaParser* b){ return a->m_YearMon < b->m_YearMon; }); m_Reports.insert(_ri, m_pAtlantis); }

            n = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_REP_CACHE_COUNT));
            if (n<=0)
                n = 1;
            if ((int)m_Reports.size()>n)
            {
                if (i > n/2)
                    n = 0;
                else
                    n = (int)m_Reports.size()-1;
                if (m_pAtlantis != m_Reports[n])
                    { delete m_Reports[n]; m_Reports.erase(m_Reports.begin() + (n)); }
            }
        }
        m_FirstLoad = false;
    }

    m_DisableErrs = false;

    wxEndBusyCursor();
*/


//-------------------------------------------------------------------------

void CAhApp::UpdateHexEditPane(CLand * pLand)
{
    CStruct     * pStruct;
    CEditPane   * pEditPane;
    int           i;
    bool          FlagsEmpty = true;

    m_HexDescrSrc.clear();

    pEditPane = (CEditPane*)m_Panes[AH_PANE_MAP_DESCR];
    if (pEditPane)
    {
        if (pLand)
        {
            m_HexDescrSrc << pLand->Description;

            m_HexDescrSrc << EOL_SCR;
            m_pAtlantis->ComposeProductsLine(pLand, EOL_SCR, m_HexDescrSrc);

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

void CAhApp::UpdateHexUnitList(CLand * pLand)
{
    CUnitPane   * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];

    if (pUnitPane)
        pUnitPane->Update(pLand);
}

//-------------------------------------------------------------------------

void CAhApp::OnMapSelectionChange()
{
    CLand       * pLand    = nullptr;
    CMapPane    * pMapPane = (CMapPane* )m_Panes[AH_PANE_MAP];

    if (pMapPane)
        pLand   = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);

    UpdateHexEditPane(pLand);  // nullptr is Ok!
    UpdateHexUnitList(pLand);
    SetMapFrameTitle();
}

//-------------------------------------------------------------------------


void CAhApp::OnUnitHexSelectionChange(long idx)
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

    pDescription = (CEditPane*)m_Panes[AH_PANE_UNIT_DESCR   ];
    pOrders      = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMANDS];
    pComments    = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMENTS];

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

    if (!ReadOnly && !m_ReportDates.empty())
        ReadOnly = (m_pAtlantis->m_YearMon != m_ReportDates[(int)gpApp->m_ReportDates.size()-1] );

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
                pLand = m_pAtlantis->GetLand(pUnit->LandId);

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

        pOrders->SetSource(pUnit?&pUnit->Orders:nullptr,      &m_OrdersAreChanged);
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
        pComments->SetSource(pUnit?&pUnit->DefOrders:nullptr, &m_CommentsChanged);
    }

    RedrawTracks();
}

//-------------------------------------------------------------------------

void CAhApp::LoadOrders()
{
    int rc;
    std::string Dir;

    Dir = GetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS);
    if (Dir.empty())
        Dir = ".";

    wxString CurrentDir = wxGetCwd();
    wxFileDialog dialog(m_Frames[AH_FRAME_MAP],
                        wxT("Load orders"),
                        wxString::FromAscii(Dir.c_str()),
                        wxT(""),
                        wxT(SZ_ORD_FILES),
                        wxFD_OPEN );
    rc = dialog.ShowModal();
    wxSetWorkingDirectory(CurrentDir);

    if (wxID_OK==rc)
    {
        std::string S;
        S = dialog.GetPath().mb_str();
        MakePathRelative(CurrentDir.mb_str(), S);
        GetDirFromPath(S.c_str(), Dir);
        SetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS, Dir.c_str() );

        LoadOrders(S.c_str());
        SetOrdersChanged(false);
    }
}

//-------------------------------------------------------------------------

bool CAhApp::CanCloseApp()
{
    SaveLandFlags();
    SaveUnitFlags();
    if (m_CommentsChanged)
        SaveComments();

    return ( m_DiscardChanges || !GetOrdersChanged() || ERR_OK==SaveOrders(true));
}

//--------------------------------------------------------------------------

void CAhApp::ShowDescriptionList(CBaseColl & Items, const char * title) // Collection of CBaseObject
{
    CBaseObject  * pObj;

    if (Items.Count() > 0)
    {
        if (1 == Items.Count())
        {
            pObj = (CBaseObject*)Items.At(0);
            CShowOneDescriptionDlg dlg(gpApp->m_Frames[AH_FRAME_MAP], pObj->Name.c_str(), pObj->Description.c_str());
            dlg.ShowModal();
        }
        else
        {
            CShowDescriptionListDlg dlg(gpApp->m_Frames[AH_FRAME_MAP], title, &Items);
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
        sectidx = GetSectionFirst(SZ_SECT_SKILLS, szName, szValue);
        while (sectidx >= 0)
        {
            pSkill              = new CBaseObject;
            pSkill->Name        = szName;
            DecodeConfigLine(pSkill->Description, szValue);
            Skills.Insert(pSkill);

            sectidx = GetSectionNext(sectidx, SZ_SECT_SKILLS, szName, szValue);
        }

        ShowDescriptionList(Skills, "Skills");
        Skills.FreeAll();
    }
    else
        ShowDescriptionList(m_pAtlantis->m_Skills, "Skills");
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
        sectidx = GetSectionFirst(szSection, szName, szValue);
        while (sectidx >= 0)
        {
            pItem              = new CBaseObject;
            pItem->Name        = szName;
            DecodeConfigLine(pItem->Description, szValue);
            Items.Insert(pItem);

            sectidx = GetSectionNext(sectidx, szSection, szName, szValue);
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
        Coll.Insert(&m_pAtlantis->m_Events);
        ShowDescriptionList(Coll, "Events");
    }
    else
    {
//        Coll.Insert(&m_pAtlantis->m_Errors);
//        ShowDescriptionList(Coll, "Errors");
        m_MsgSrc.clear();
        ShowError(m_pAtlantis->m_Errors.Description.c_str(), m_pAtlantis->m_Errors.Description.size(), true);

    }
    Coll.DeleteAll();
}

//--------------------------------------------------------------------------

void CAhApp::ViewSecurityEvents()
{
/*    CBaseColl   Coll;

    Coll.Insert(&m_pAtlantis->m_SecurityEvents);
    ShowDescriptionList(Coll, "Security Events");

    Coll.DeleteAll();*/
    
        m_MsgSrc.clear();
        ShowError(m_pAtlantis->m_SecurityEvents.Description.c_str(), m_pAtlantis->m_SecurityEvents.Description.size(), true);
}

//--------------------------------------------------------------------------

void CAhApp::ViewNewProducts()
{
    ShowDescriptionList(m_pAtlantis->m_NewProducts, "New products");
}

//--------------------------------------------------------------------------

void CAhApp::ViewBattlesAll()
{
    ShowDescriptionList(m_pAtlantis->m_Battles, "Battles");
}

//--------------------------------------------------------------------------

void CAhApp::ViewGates()
{
    ShowDescriptionList(m_pAtlantis->m_Gates, "Gates");
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

    for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            if (!pLand->CityName.empty())
            {
                std::unique_ptr<CBaseObject> pObj(new CBaseObject);
                pObj->Name = pLand->CityName;

                //LandIdToCoord(pLand->Id, x, y, z);
                m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
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
        for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
            for (nl=0; nl<pPlane->Lands.Count(); nl++)
            {
                pLand      = (CLand*)pPlane->Lands.At(nl);
                if ((pLand->Flags&LAND_VISITED) || 1==loop) // we run it twice, so we pick visited hexes if we can
                {
                    std::unique_ptr<CBaseObject> pObj(new CBaseObject);
                    pObj->Name = pLand->Name;

                    m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
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
    for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
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


    sInfo << m_pAtlantis->m_FactionInfo << sMoreInfo;
    CShowOneDescriptionDlg dlg(gpApp->m_Frames[AH_FRAME_MAP],
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
    CMapPane      * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
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

            for (const auto& propnameStr : m_pAtlantis->m_UnitPropertyNames)
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

    CShowOneDescriptionDlg dlg(gpApp->m_Frames[AH_FRAME_MAP],
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
    CMapPane           * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];


    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_MONTH_LONG));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            MonthLongOrders.insert(S.c_str());
    }

    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_DUPLICATABLE));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            MonthLongDup.insert(S.c_str());
    }

    if (1==atol(SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_OUTPUT_LIST))))
    {
        // Output will go into the unit filter window
        OpenUnitFrameFltr(false);
        pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
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
        ShowError(Errors.c_str(), Errors.size(), true);

    if (0==errcount)
        wxMessageBox(wxT("No problems found."), wxT("Order checking"), wxOK | wxCENTRE, m_Frames[AH_FRAME_MAP]);


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
        if (1==atol(SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_OUTPUT_LIST))))
        {
            // Output will go into the unit filter window
            OpenUnitFrameFltr(false);
            pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
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
            ShowError(UnitText.c_str(), UnitText.size(), true);
    }
    else
        wxMessageBox(wxT("Found no units moving into the current hex."), wxT("Units moving"), wxOK | wxCENTRE, m_Frames[AH_FRAME_MAP]);


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

    TaxPerTaxer = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));

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

    m_pAtlantis->ComposeLandStrCoord(pCurLand, sCoord);
    Report.Name << "Financial report for " << sCoord;
    coll.Insert(&Report);

    ShowDescriptionList(coll, "Financial report");
    coll.DeleteAll();
}

//--------------------------------------------------------------------------

void CAhApp::AddTempHex(int X, int Y, int Plane)
{
    CLand  * pCurLand = m_pAtlantis->GetLand(X, Y, Plane, true);
    if (pCurLand)
        return;
        
    CPlane * pPlane = (CPlane*)m_pAtlantis->m_Planes.At(Plane);
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

//--------------------------------------------------------------------------

void CAhApp::DelTempHex(int X, int Y, int Plane)
{
    int      idx;
    CLand  * pCurLand = m_pAtlantis->GetLand(X, Y, Plane, true);
    if (!pCurLand)
        return;
        
    CPlane * pPlane = (CPlane*)m_pAtlantis->m_Planes.At(Plane);
    if (!pPlane)
        return;
        
    assert(Plane == pPlane->Id);
    
    if (pPlane->Lands.Search(pCurLand, idx))
        pPlane->Lands.AtFree(idx);
}

//--------------------------------------------------------------------------

void CAhApp::RerunOrders()
{
    m_pAtlantis->RunOrders(nullptr);
    CUnitPane * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
    if (pUnitPane)
        pUnitPane->Update(pUnitPane->m_pCurLand);
}

//--------------------------------------------------------------------------

int CAhApp::SaveHistory(const char * FNameOut)
{
    CLand            * pLand;
    CFileWriter        Dest;
    int                nl, np;
    CPlane           * pPlane;
    SAVE_HEX_OPTIONS   options;

    memset(&options, 0, sizeof(options));
    options.AlwaysSaveImmobStructs = true;
    options.SaveResources          = true;

    if ( (m_pAtlantis->m_Planes.Count()>0) &&
         (0==m_pAtlantis->m_ParseErr)      && // don't destroy if not loaded!
         Dest.Open(FNameOut)
       )
    {
        for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
            for (nl=0; nl<pPlane->Lands.Count(); nl++)
            {
                pLand    = (CLand*)pPlane->Lands.At(nl);
                m_pAtlantis->SaveOneHex(Dest, pLand, pPlane, &options);
            }
        }
        Dest.Close();
    }
    return ERR_OK;
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

    CHexExportDlg   dlg(m_Frames[AH_FRAME_MAP]);

    memset(&options, 0, sizeof(options));
    options.SaveUnits = true;

    if (stFName.empty())
        Format(stFName, "map.%04d", m_pAtlantis->m_YearMon);

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

    m_pAtlantis->ComposeLandStrCoord(pLand, sName);

    p  = GetToken(sData, GetConfig(SZ_SECT_LAND_VISITED, sName.c_str()), ',');
    if (sData.empty())
    {
/*        ym_first = m_pAtlantis->m_YearMon;
        ym_last  = m_pAtlantis->m_YearMon;*/
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

    if (ym_first==m_pAtlantis->m_YearMon || !OnlyNew)
    {
        m_pAtlantis->SaveOneHex(Dest, pLand, pPlane, &options);
    }
}

//--------------------------------------------------------------------------

void CAhApp::ExportHexes()
{
    std::string               sData, sName;
    CMapPane         * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];

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
            pPlane   = (CPlane*)m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);
            pLand    = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, true);
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
    CMapPane    * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
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
        while (propnameprice)
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
                        m_pAtlantis->ComposeLandStrCoord(pSellLand, sCoord);
                        Report << pSellLand->TerrainType << " (" << sCoord << ") " << EOL_SCR;
                        m_pAtlantis->ComposeLandStrCoord(pBuyLand, sCoord);
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
        ShowError(Report.c_str()      , Report.size()      , true);

    Hexes.DeleteAll();
    wxEndBusyCursor();
}

//--------------------------------------------------------------------------

void CAhApp::EditListColumns(int command)
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

//--------------------------------------------------------------------------

const char * CAhApp::GetListColSection(const char * sectprefix, const char * key)
{
    const char * sect;

    sect = GetConfig(SZ_SECT_LIST_COL_CURRENT, key);
    if (!sect || !*sect)
        sect  = GetNextSectionName(CONFIG_FILE_CONFIG, sectprefix);

    return sect;
}

//--------------------------------------------------------------------------

void CAhApp::StdRedirectInit()
{
#ifdef __WXMAC_OSX__
	char cwd[MAXPATHLEN];
	// Setup new working directory in case we got started from /Applications
	if((getcwd(cwd, MAXPATHLEN)) != nullptr){
		if((strncmp(cwd, "/Applications", strlen("/Applications"))) == 0){
			const char *home = getenv("HOME");
			if(home != nullptr){
				if(0 == chdir(home)){
					mkdir(".alh", 0750);
					if(0 != chdir(".alh"))
						chdir("/Applications");
				}
			}
		}
	}
#endif
    freopen("ah.stdout", "w", stdout);
    freopen("ah.stderr", "w", stderr);
    m_nStdoutLastPos = 0;
    m_nStderrLastPos = 0;
}

//--------------------------------------------------------------------------

void CAhApp::StdRedirectReadMore(bool FromStdout, std::string & sData)
{
    FILE       * f;
    int        * pCurPos;
    char         buf[1024];
    int          n;

    sData.clear();
    if (FromStdout)
    {
        fflush(stdout);
        pCurPos  =  &m_nStdoutLastPos;
        f        = fopen("ah.stdout", "rb");
    }
    else
    {
        fflush(stderr);
        pCurPos  =  &m_nStderrLastPos;
        f        = fopen("ah.stderr", "rb");
    }

    if (f)
    {
        fseek(f, *pCurPos, SEEK_SET);
        do
        {
            n = fread(buf, 1, sizeof(buf), f);
            if (n>0)
                AddBuf(sData, buf, n);
        } while (n>0);
        *pCurPos = ftell(f);
        fclose(f);
    }
}

//--------------------------------------------------------------------------

void CAhApp::CheckRedirectedOutputFiles()
{
    std::string S;

    gpApp->StdRedirectReadMore(false, S);
    if (!S.empty())
        ShowError(S.c_str(), S.size(), true);
    gpApp->StdRedirectReadMore(true, S);
    if (!S.empty())
        ShowError(S.c_str(), S.size(), true);
}

//--------------------------------------------------------------------------

void CAhApp::StdRedirectDone()
{
}

//--------------------------------------------------------------------------

void CAhApp::InitMoveModes()
{
    const char * p;
    std::string         S;
    int          n;
    bool         Update = false;

    p     = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMENTS));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        m_MoveModes.push_back(S.c_str());
    }

    // do update here for 2.3.2
    p = SZ_DEFAULT_MOVEMENT_MODE;
    n = 0;
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        n++;

        if (n > (int)m_MoveModes.size())
        {
            m_MoveModes.push_back(S.c_str());
            Update = true;
        }
    }
    if (Update)
    {
        S.clear();
        for (n=0; n<(int)m_MoveModes.size(); n++)
        {
            if (n>0)
                S << ',';
            S << m_MoveModes[n].c_str();
        }
        SetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMENTS, S.c_str());
    }
}

//--------------------------------------------------------------------------

void CAhApp::SelectNextUnit()
{
    if (m_Panes[AH_PANE_UNITS_HEX]) 
        ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX])->SelectNextUnit();
}

//--------------------------------------------------------------------------

void CAhApp::SelectPrevUnit()
{
    if (m_Panes[AH_PANE_UNITS_HEX]) 
        ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX])->SelectPrevUnit();
}

//--------------------------------------------------------------------------

void CAhApp::SelectUnitsPane()
{
    if (m_Panes[AH_PANE_UNITS_HEX]) 
        ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX])->SetFocus();
}

//--------------------------------------------------------------------------

void CAhApp::SelectOrdersPane()
{
    if (m_Panes[AH_PANE_UNIT_COMMANDS]) 
        ((CUnitPane*)m_Panes[AH_PANE_UNIT_COMMANDS])->SetFocus();
}

//--------------------------------------------------------------------------

void CAhApp::ViewMovedUnits()
{
}

//=========================================================================

void  CGameDataHelper::ReportError(const char * msg, int msglen, bool orderrelated)
{
    gpApp->ShowError(msg, msglen, !orderrelated);
};

long  CGameDataHelper::GetStudyCost(const char * skill)
{
    return gpApp->GetStudyCost(skill);
};

long  CGameDataHelper::GetStructAttr(const char * kind, long & MaxLoad, long & MinSailingPower)
{
    return gpApp->GetStructAttr(kind, MaxLoad, MinSailingPower);
}

const char *  CGameDataHelper::ResolveAlias(const char * alias)
{
    return gpApp->ResolveAlias(alias);
}

bool CGameDataHelper::GetItemWeights(const char * item, int *& weights, const char **& movenames, int & movecount )
{
    return gpApp->GetItemWeights(ResolveAlias(item), weights, movenames, movecount );
}

void CGameDataHelper::GetMoveNames(const char **& movenames)
{
    gpApp->GetMoveNames(movenames);
}

const char * CGameDataHelper::GetConfString(const char * section, const char * param)
{
    if (!section)
        section = SZ_SECT_COMMON;
    return gpApp->GetConfig(section, param);
}

bool CGameDataHelper::GetOrderId(const char * order, long & id)
{
    return gpApp->GetOrderId(order, id);
}

bool CGameDataHelper::IsTradeItem(const char * item)
{
    return gpApp->IsTradeItem(item);
}

bool CGameDataHelper::IsMan(const char * item)
{
    return gpApp->IsMan(item);
}

const char * CGameDataHelper::GetWeatherLine(bool IsCurrent, bool IsGood, int Zone)
{
    return gpApp->GetWeatherLine(IsCurrent, IsGood, Zone);
}

bool CGameDataHelper::GetTropicZone  (const char * plane, long & y_min, long & y_max)
{
    const char * value;
    std::string         S;

    value = SkipSpaces(gpApp->GetConfig(SZ_SECT_TROPIC_ZONE, plane));
    if (!value || !*value)
        return false;

    value = GetToken(S, value, ',');
    y_min = atol(S.c_str());

    value = GetToken(S, value, ',');
    y_max = atol(S.c_str());

    return true;
}

void CGameDataHelper::SetTropicZone  (const char * plane, long y_min, long y_max)
{
    std::string S;
    S << y_min << ',' << y_max;
    gpApp->SetConfig(SZ_SECT_TROPIC_ZONE, plane, S.c_str());
}

void CGameDataHelper::GetProdDetails (const char * item, TProdDetails & details)
{
    gpApp->GetProdDetails (item, details);
}

long CGameDataHelper::MaxSkillLevel  (const char * race, const char * skill, const char * leadership, bool IsArcadiaSkillSystem)
{
    return gpApp->GetMaxRaceSkillLevel(race, skill, leadership, IsArcadiaSkillSystem);
}

bool CGameDataHelper::ImmediateProdCheck()
{
    return atol(gpApp->GetConfig(SZ_SECT_COMMON,  SZ_KEY_CHK_PROD_REQ));
}

bool CGameDataHelper::CanSeeAdvResources(const char * skillname, const char * terrain, std::vector<long> & Levels, std::vector<std::string> & Resources)
{
    return gpApp->CanSeeAdvResources(skillname, terrain, Levels, Resources);
}

int CGameDataHelper::GetAttitudeForFaction(int id)
{
    return gpApp->GetAttitudeForFaction(id);
}

void CGameDataHelper::SetAttitudeForFaction(int id, int attitude)
{
    gpApp->SetAttitudeForFaction(id, attitude);
}

void CGameDataHelper::SetPlayingFaction(long id)
{
    // set playing faction to ATT_FRIEND2
    gpApp->SetAttitudeForFaction(id, ATT_FRIEND2);
    gpApp->SetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID, id);
}

bool CGameDataHelper::ShowMoveWarnings()
{
    return atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_MOVE_MODE));
}

bool CGameDataHelper::IsRawMagicSkill(const char * skillname)
{
    static int     postlen = strlen(PRP_SKILL_POSTFIX);
    std::string           S;

    S = skillname;
    if (FindSubStrR(S, PRP_SKILL_POSTFIX) == S.size()-postlen)
    {
        DelSubStr(S, S.size()-postlen, postlen);
        return gpApp->IsMagicSkill(S.c_str());
    }

    return false;
}

bool CGameDataHelper::IsWagon(const char * item)
{
    if (!item)
        return false;
    std::string S = gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGONS);
    std::string T;
    const char * p = S.c_str();
    while (p && *p)
    {
        p = GetToken(T, p, ',', TRIM_ALL);
        if (0==stricmp(item, T.c_str()))
            return true;
    }
    return false;
}

bool CGameDataHelper::IsWagonPuller(const char * item)
{
    if (!item)
        return false;
    std::string S = gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_PULLERS);
    std::string T;
    const char * p = S.c_str();
    while (p && *p)
    {
        p = GetToken(T, p, ',', TRIM_ALL);
        if (0==stricmp(item, T.c_str()))
            return true;
    }
    return false;
}

int CGameDataHelper::WagonCapacity()
{
    return atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_CAPACITY));
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

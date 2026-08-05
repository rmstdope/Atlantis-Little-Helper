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

#include "configmanager.h"

#include "string_utils.h"
#include "files.h"
#include "consts.h"
#include "consts_ah.h"
#include "ahapp.h"
#include "ahframe.h"
#include "atlaparser.h"
#include "unitfilterdlg.h"

ConfigManager * gpConfigManager = nullptr;

//=========================================================================

ConfigManager::ConfigManager()
{
    m_UpgradeLandFlags = false;
}

//-------------------------------------------------------------------------

void ConfigManager::Init()
{
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
}

//-------------------------------------------------------------------------

void ConfigManager::Save()
{
    m_Config[CONFIG_FILE_CONFIG].Save(SZ_CONFIG_FILE);
    m_Config[CONFIG_FILE_STATE ].Save(SZ_CONFIG_STATE_FILE);
}

//-------------------------------------------------------------------------

void ConfigManager::UpgradeConfigFiles()
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
            if (Section.empty())
            {
                Ok = m_Config[CONFIG_FILE_CONFIG].GetNextSection("\1", szNextSection);
                continue;
            }
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
        szValue = SkipSpaces(GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str()));
        if (szValue && *szValue)
        {
            SetConfig(Section.c_str(),      ConfigKey.c_str(), szValue);
            SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str(), "");
            Ok = true;
        }

        Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_COMPARE , i);
        szValue = GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str());
        if (szValue && *szValue)
        {
            SetConfig(Section.c_str(),      ConfigKey.c_str(), szValue);
            SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str(), "");
            Ok = true;
        }

        Format(ConfigKey, "%s%d", SZ_KEY_UNIT_FLTR_VALUE   , i);
        szValue = GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str());
        if (szValue && *szValue)
        {
            SetConfig(Section.c_str(),      ConfigKey.c_str(), szValue);
            SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.c_str(), "");
            Ok = true;
        }
    }
    if (Ok)
        SetConfig(SZ_SECT_WND_UNITS_FLTR, SZ_KEY_FLTR_SET, Section.c_str());

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

void ConfigManager::MoveSectionEntries(int fileno, const char * src, const char * dest)
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

void ConfigManager::UpgradeConfigByFactionId()
{
    int          fileno, idx;
    std::string         S, Section, Key;
    const char * szName;
    const char * szValue;

    if (gpApp->m_pAtlantis->m_CrntFactionId > 0)
    {
        // Upgrade order files
        ComposeConfigOrdersSection(Section, gpApp->m_pAtlantis->m_CrntFactionId);
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
            Key << (long)gpApp->m_pAtlantis->m_CrntFactionId;
            SetConfig(SZ_SECT_PASSWORDS, Key.c_str() , S.c_str() );
            SetConfig(SZ_SECT_COMMON   , SZ_KEY_PWD_OLD, (const char *)nullptr);
        }
    }
}

//-------------------------------------------------------------------------

void ConfigManager::ComposeConfigOrdersSection(std::string & Sect, int FactionId)
{
    Sect = SZ_SECT_ORDERS;
    Sect << "_" << (long)FactionId;
}

//-------------------------------------------------------------------------

int ConfigManager::GetConfigFileNo(const char * szSection)
{
    if (m_ConfigSectionsState.find(szSection) != m_ConfigSectionsState.end() ||
        0==strnicmp(SZ_SECT_ORDERS, szSection, sizeof(SZ_SECT_ORDERS)-1) ) // orders section is composite starting from 2.1.6
        return CONFIG_FILE_STATE;
    else
        return CONFIG_FILE_CONFIG;
}

//-------------------------------------------------------------------------

const char * ConfigManager::GetConfig(const char * szSection, const char * szName)
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

CConfigFile * ConfigManager::GetConfigFile(const char * szSection)
{
    return &m_Config[GetConfigFileNo(szSection)];
}

//-------------------------------------------------------------------------

void ConfigManager::SetConfig(const char * szSection, const char * szName, const char * szNewValue)
{
    int  fileno = GetConfigFileNo(szSection);
    m_Config[fileno].SetByName(szSection, szName, szNewValue);
}

//-------------------------------------------------------------------------

void ConfigManager::SetConfig(const char * szSection, const char * szName, long lNewValue)
{
    char   buf[64];
    int    fileno = GetConfigFileNo(szSection);

    snprintf(buf, sizeof(buf), "%ld", lNewValue);
    m_Config[fileno].SetByName(szSection, szName, buf);
}

//-------------------------------------------------------------------------

int  ConfigManager::GetSectionFirst(const char * szSection, const char *& szName, const char *& szValue)
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

int  ConfigManager::GetSectionNext (int idx, const char * szSection, const char *& szName, const char *& szValue)
{
    int   fileno = GetConfigFileNo(szSection);
    return m_Config[fileno].GetNextInSection (idx, szSection, szName, szValue);
}

//-------------------------------------------------------------------------

void  ConfigManager::RemoveSection(const char * szSection)
{
    int fileno = GetConfigFileNo(szSection);
    m_Config[fileno].RemoveSection(szSection);
}

//-------------------------------------------------------------------------

const char * ConfigManager::GetNextSectionName(int fileno, const char * szStart)
{
    const char * szNextSection = nullptr;

    if (fileno!=CONFIG_FILE_STATE && fileno!=CONFIG_FILE_CONFIG)
        return nullptr;

    m_Config[fileno].GetNextSection(szStart, szNextSection);

    return szNextSection;
}

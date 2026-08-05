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

#ifndef __CONFIGMANAGER_H_INCL__
#define __CONFIGMANAGER_H_INCL__

#include <set>
#include <string>
#include "cfgfile.h"
#include "stl_helpers.h"

enum
{
    CONFIG_FILE_CONFIG     = 0,
    CONFIG_FILE_STATE         ,

    CONFIG_FILE_COUNT
};

class ConfigManager
{
public:
    ConfigManager();

    void                 Init();
    void                 Save();

    void                 SetConfig(const char * szSection, const char * szName, const char * szNewValue);
    void                 SetConfig(const char * szSection, const char * szName, long lNewValue);
    const char         * GetConfig(const char * szSection, const char * szName);
    CConfigFile        * GetConfigFile(const char * szSection);   // returns config file owning szSection
    int                  GetSectionFirst(const char * szSection, const char *& szName, const char *& szValue);
    int                  GetSectionNext (int idx, const char * szSection, const char *& szName, const char *& szValue);
    void                 RemoveSection(const char * szSection);
    const char         * GetNextSectionName(int fileno, const char * szStart); // sorry, but fileno is needed here
    void                 MoveSectionEntries(int fileno, const char * src, const char * dest);
    void                 UpgradeConfigByFactionId();
    void                 ComposeConfigOrdersSection(std::string & Sect, int FactionId);

    CConfigFile          m_Config[CONFIG_FILE_COUNT];
    bool                 m_UpgradeLandFlags;

private:
    int                  GetConfigFileNo(const char * szSection);
    void                 UpgradeConfigFiles();

    std::set<std::string, CaseInsensitiveLess> m_ConfigSectionsState;
};

extern ConfigManager * gpConfigManager;

#endif

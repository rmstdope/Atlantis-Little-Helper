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

#include "gamedatamanager.h"

#include <algorithm>
#include <cstdlib>
#include "configmanager.h"
#include "gamerules.h"
#include "consts_ah.h"

GameDataManager * gpGameData = nullptr;

//=========================================================================

GameDataManager::GameDataManager()
{
    m_pAtlantis.reset(new CAtlaParser(gpGameRules));
    m_pAtlantis->m_pConfig = &gpConfigManager->m_Config[CONFIG_FILE_CONFIG];
}

//-------------------------------------------------------------------------

void GameDataManager::Init()
{
    const char * szName;
    const char * szValue;

    int i = gpConfigManager->GetSectionFirst(SZ_SECT_REPORTS, szName, szValue);
    while (i>=0)
    {
        {
            long _val = atol(szName);
            auto _it = std::lower_bound(m_ReportDates.begin(), m_ReportDates.end(), _val);
            if (_it == m_ReportDates.end() || *_it != _val)
                m_ReportDates.insert(_it, _val);
        }
        i = gpConfigManager->GetSectionNext (i, SZ_SECT_REPORTS, szName, szValue);
    }
}

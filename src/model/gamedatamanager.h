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

#ifndef __GAMEDATAMANAGER_H_INCL__
#define __GAMEDATAMANAGER_H_INCL__

#include <memory>
#include <vector>
#include "files.h"
#include "atlaparser.h"

class GameDataManager
{
public:
    GameDataManager();

    void                 Init();

    std::unique_ptr<CAtlaParser> m_pAtlantis;
    std::vector<std::unique_ptr<CAtlaParser>> m_Reports;
    std::vector<long>    m_ReportDates;
};

extern GameDataManager * gpGameData;

#endif

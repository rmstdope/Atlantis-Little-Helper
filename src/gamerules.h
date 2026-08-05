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

#ifndef __GAMERULES_H_INCL__
#define __GAMERULES_H_INCL__

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "data.h"
#include "stl_helpers.h"

struct ItemWeights
{
    char * name;
    int  * weights;
};

class GameRules : public CGameDataHelper
{
public:
    GameRules();
    ~GameRules() override;

    void                 Init();

    // CGameDataHelper overrides
    void                 ReportError       (const char * msg, int msglen, bool orderrelated) override;
    long                 GetStudyCost      (const char * skill) override;
    long                 GetStructAttr     (const char * kind, long & MaxLoad, long & MinSailingPower) override;
    const char         * GetConfString     (const char * section, const char * param) override;
    bool                 GetOrderId        (const char * order, long & id) override;
    bool                 IsTradeItem       (const char * item) override;
    bool                 IsMan             (const char * item) override;
    const char         * GetWeatherLine    (bool IsCurrent, bool IsGood, int Zone) override;
    const char         * ResolveAlias      (const char * alias) override;
    bool                 GetItemWeights    (const char * item, int *& weights, const char **& movenames, int & movecount ) override;
    void                 GetMoveNames      (const char **& movenames) override;
    bool                 GetTropicZone     (const char * plane, long & y_min, long & y_max) override;
    void                 SetTropicZone     (const char * plane, long y_min, long y_max) override;
    void                 GetProdDetails    (const char * item, TProdDetails & details) override;
    long                 MaxSkillLevel     (const char * race, const char * skill, const char * leadership, bool IsArcadiaSkillSystem) override;
    bool                 ImmediateProdCheck() override;
    bool                 CanSeeAdvResources(const char * skillname, const char * terrain, std::vector<long> & Levels, std::vector<std::string> & Resources) override;
    bool                 ShowMoveWarnings  () override;
    bool                 IsRawMagicSkill   (const char * skillname) override;
    int                  GetAttitudeForFaction(int id) override;
    void                 SetAttitudeForFaction(int id, int attitude) override;
    void                 SetPlayingFaction (long id) override;
    bool                 IsWagon           (const char * item) override;
    bool                 IsWagonPuller     (const char * item) override;
    int                  WagonCapacity     () override;

    // Not part of the CGameDataHelper interface, but called directly by name
    // elsewhere (MaxSkillLevel above is the CGameDataHelper-facing alias).
    long                 GetMaxRaceSkillLevel(const char * race, const char * skill, const char * leadership, bool IsArcadiaSkillSystem);
    bool                 IsMagicSkill      (const char * skill);
    bool                 IsWaterTerrain    (const char * terrain);

private:
    void                 InitMoveModes();

    std::vector<std::string> m_MoveModes;
    std::vector<const char*> m_MoveModesRaw;
    std::vector<ItemWeights*> m_ItemWeights;
    std::unordered_map<std::string, long> m_OrderHash;
    std::unordered_map<std::string, long> m_TradeItemsHash;
    std::unordered_map<std::string, long> m_MenHash;
    std::unordered_map<std::string, long> m_MaxSkillHash;
    std::unordered_map<std::string, long> m_MagicSkillsHash;
    CBaseColl            m_Attitudes;
    std::set<std::string, CaseInsensitiveLess> m_WaterTerrainNames;
};

extern GameRules * gpGameRules;

#endif

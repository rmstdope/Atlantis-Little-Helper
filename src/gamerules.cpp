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

#include "gamerules.h"

#include <cstdlib>
#include <cstring>
#include "string_utils.h"
#include "files.h"
#include "consts.h"
#include "consts_ah.h"
#include "ahapp.h"

GameRules * gpGameRules = nullptr;

//=========================================================================

GameRules::GameRules() : m_OrderHash      (  3),
                          m_TradeItemsHash (  2),
                          m_MenHash        (  2),
                          m_MaxSkillHash   (  6),
                          m_MagicSkillsHash(  6)
{
}

//-------------------------------------------------------------------------

GameRules::~GameRules()
{
    for (auto* p : m_ItemWeights) { free(p->name); free(p->weights); delete p; }
    m_ItemWeights.clear();
}

//-------------------------------------------------------------------------

void GameRules::Init()
{
    InitMoveModes();
    SetAttitudeForFaction(0, ATT_NEUTRAL);

    // Water terrain types
    const char * p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_WATER_TERRAINS));
    std::string S;
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

    p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_VALID_ORDERS));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_OrderHash.emplace(S.c_str(), -1L);
    }

    // Load trade items hash
    p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_TRADE_ITEMS));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_TradeItemsHash.emplace(S.c_str(), -1L);
    }

    // All the men hash
    p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_MEN));
    while (p && *p)
    {
        p = SkipSpaces(GetToken(S, p, ','));
        if (!S.empty())
            m_MenHash.emplace(S.c_str(), -1L);
    }

    // Magic skills hash
    p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_MAG_SKILLS));
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
}

//-------------------------------------------------------------------------

void GameRules::InitMoveModes()
{
    const char * p;
    std::string         S;
    int          n;
    bool         Update = false;

    p     = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMENTS));
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
        gpConfigManager->SetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMENTS, S.c_str());
    }
}

//-------------------------------------------------------------------------

long GameRules::GetStructAttr(const char * kind, long & MaxLoad, long & MinSailingPower)
{
    const char * attrlist;
    const char * p;
    std::string         S, Name;
    long         attr = 0;

    MaxLoad         = 0;
    MinSailingPower = 0;

    attrlist = gpConfigManager->GetConfig(SZ_SECT_STRUCTS, ResolveAlias(kind));
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

const char * GameRules::ResolveAlias(const char * alias)
{
    const char * p;
    const char * p1;
    int          cnt = 0;

    p1 = alias;
    do
    {
        p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_ALIAS, p1));
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

long GameRules::GetStudyCost(const char * skill)
{
    long        n;
    const char *p;

    p = ResolveAlias(skill);
    n = atol(gpConfigManager->GetConfig(SZ_SECT_STUDY_COST, p));

    return n;
}

//-------------------------------------------------------------------------

bool GameRules::GetItemWeights(const char * item, int *& weights, const char **& movenames, int & movecount )
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

        p = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_WEIGHT_MOVE, item));


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
            gpConfigManager->SetConfig(SZ_SECT_WEIGHT_MOVE, item, S.c_str());
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
            ReportError(S.c_str(), S.size(), false);
        }
    }

    return Ok;
}

//-------------------------------------------------------------------------

void GameRules::GetMoveNames(const char **& movenames)
{
    m_MoveModesRaw.clear();
    for (const auto& s : m_MoveModes) m_MoveModesRaw.push_back(s.c_str());
    movenames = m_MoveModesRaw.data();
}

//-------------------------------------------------------------------------

bool GameRules::GetOrderId(const char * order, long & id)
{
    bool  Ok;
    auto it = m_OrderHash.find(order);
    Ok = it != m_OrderHash.end();
    id = Ok ? it->second : 0;

    return Ok;
}

//-------------------------------------------------------------------------

bool GameRules::IsTradeItem(const char * item)
{
    return m_TradeItemsHash.find(item) != m_TradeItemsHash.end();
}

//-------------------------------------------------------------------------

bool GameRules::IsMan(const char * item)
{
    return m_MenHash.find(item) != m_MenHash.end();
}

//-------------------------------------------------------------------------

bool GameRules::IsMagicSkill(const char * skill)
{
    return m_MagicSkillsHash.find(skill) != m_MagicSkillsHash.end();
}

//-------------------------------------------------------------------------

bool GameRules::IsWaterTerrain(const char * terrain)
{
    return m_WaterTerrainNames.find(terrain) != m_WaterTerrainNames.end();
}

//-------------------------------------------------------------------------

const char * GameRules::GetWeatherLine(bool IsCurrent, bool IsGood, int Zone)
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

    return gpConfigManager->GetConfig(SZ_SECT_WEATHER, szKey);
}

//-------------------------------------------------------------------------

long GameRules::GetMaxRaceSkillLevel(const char * race, const char * skill, const char * leadership, bool IsArcadiaSkillSystem)
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
        sVal = gpConfigManager->GetConfig(SZ_SECT_MAX_SKILL_LVL, race);
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
                    sVal = gpConfigManager->GetConfig(SZ_SECT_MAX_MAG_SKILL_LVL, race);
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

                sVal = gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_LEAD_SKILL_BONUS);
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

long GameRules::MaxSkillLevel(const char * race, const char * skill, const char * leadership, bool IsArcadiaSkillSystem)
{
    return GetMaxRaceSkillLevel(race, skill, leadership, IsArcadiaSkillSystem);
}

//-------------------------------------------------------------------------

void GameRules::GetProdDetails (const char * item, TProdDetails & details)
{
    std::string sVal, S;
    const char * p;
    int x;

    details.Empty();
    sVal = gpConfigManager->GetConfig(SZ_SECT_PROD_SKILL, item);
    if (!sVal.empty())
    {
        S = GetToken(details.skillname, sVal.c_str(), ' ', TRIM_ALL);
        details.skilllevel = atol(S.c_str());
    }

    sVal = gpConfigManager->GetConfig(SZ_SECT_PROD_RESOURCE, item);
    x = 0;
    p = sVal.c_str();
    while (p && *p && x<MAX_RES_NUM)
    {
        p = GetToken(details.resname[x], SkipSpaces(p), ' ', TRIM_ALL);
        p = GetToken(S, p, ',', TRIM_ALL);
        details.resamt[x] = atol(S.c_str());
        x++;
    }

    sVal = gpConfigManager->GetConfig(SZ_SECT_PROD_MONTHS, item);
    if (!sVal.empty())
        details.months = atol(sVal.c_str());

    sVal = gpConfigManager->GetConfig(SZ_SECT_PROD_TOOL, item);
    if (!sVal.empty())
    {
        S = GetToken(details.toolname, sVal.c_str(), ' ', TRIM_ALL);
        details.toolhelp = atol(S.c_str());
    }

}

//-------------------------------------------------------------------------

bool GameRules::CanSeeAdvResources(const char * skillname, const char * terrain, std::vector<long> & Levels, std::vector<std::string> & Resources)
{
    std::string         ProdSkillLine;
    std::string         ProdLandLine;
    bool         Ok = false;
    const char * p1, * p2, *p;
    std::string         Prod1, Prod2, S1;
    long         level;

    Levels.clear();
    Resources.clear();

    ProdSkillLine = gpConfigManager->GetConfig(SZ_SECT_RESOURCE_SKILL,  skillname);
    TrimRight(ProdSkillLine, TRIM_ALL);

    ProdLandLine = gpConfigManager->GetConfig(SZ_SECT_RESOURCE_LAND,  terrain);
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

int GameRules::GetAttitudeForFaction(int id)
{
    int player_id = atol( gpConfigManager->GetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID));
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
void GameRules::SetAttitudeForFaction(int id, int attitude)
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

//=========================================================================
// CGameDataHelper interface members with no pre-extraction CAhApp
// counterpart - these were implemented directly against gpApp->GetConfig
// in ahapp.cpp's old out-of-line CGameDataHelper:: forwarding block.

void GameRules::ReportError(const char * msg, int msglen, bool orderrelated)
{
    gpApp->ShowError(msg, msglen, !orderrelated);
}

const char * GameRules::GetConfString(const char * section, const char * param)
{
    if (!section)
        section = SZ_SECT_COMMON;
    return gpConfigManager->GetConfig(section, param);
}

bool GameRules::GetTropicZone(const char * plane, long & y_min, long & y_max)
{
    const char * value;
    std::string         S;

    value = SkipSpaces(gpConfigManager->GetConfig(SZ_SECT_TROPIC_ZONE, plane));
    if (!value || !*value)
        return false;

    value = GetToken(S, value, ',');
    y_min = atol(S.c_str());

    value = GetToken(S, value, ',');
    y_max = atol(S.c_str());

    return true;
}

void GameRules::SetTropicZone(const char * plane, long y_min, long y_max)
{
    std::string S;
    S << y_min << ',' << y_max;
    gpConfigManager->SetConfig(SZ_SECT_TROPIC_ZONE, plane, S.c_str());
}

bool GameRules::ImmediateProdCheck()
{
    return atol(gpConfigManager->GetConfig(SZ_SECT_COMMON,  SZ_KEY_CHK_PROD_REQ));
}

void GameRules::SetPlayingFaction(long id)
{
    // set playing faction to ATT_FRIEND2
    SetAttitudeForFaction(id, ATT_FRIEND2);
    gpConfigManager->SetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID, id);
}

bool GameRules::ShowMoveWarnings()
{
    return atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_MOVE_MODE));
}

bool GameRules::IsRawMagicSkill(const char * skillname)
{
    static int     postlen = strlen(PRP_SKILL_POSTFIX);
    std::string           S;

    S = skillname;
    if (FindSubStrR(S, PRP_SKILL_POSTFIX) == S.size()-postlen)
    {
        DelSubStr(S, S.size()-postlen, postlen);
        return IsMagicSkill(S.c_str());
    }

    return false;
}

bool GameRules::IsWagon(const char * item)
{
    if (!item)
        return false;
    std::string S = gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGONS);
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

bool GameRules::IsWagonPuller(const char * item)
{
    if (!item)
        return false;
    std::string S = gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_PULLERS);
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

int GameRules::WagonCapacity()
{
    return atol(gpConfigManager->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_CAPACITY));
}

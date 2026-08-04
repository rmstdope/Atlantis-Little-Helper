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

#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>
#include "files.h"
#include "consts.h"
#include "string_utils.h"
#include "cfgfile.h"

#include "objs.h"
#include "data.h"
#include "atlaparser.h"
#include "errs.h"
#include "consts_ah.h" // not very good, but will do for now
#include <time.h>
#include <math.h>

#define IN          "in "
#define CONTAINS    "contains "
#define SILVER      "silver"

#define FLAG_HDR    "$Flag"

#define YES         "Yes"
#define ORDER_CMNT  ";*** "



const char * Monthes[] = {"Jan", "Feb", "Mar",  "Apr", "May", "Jun",  "Jul", "Aug", "Sep",  "Oct", "Nov", "Dec"};

const char * EOL_MS    = "\r\n";
const char * EOL_UNIX  = "\n";
const char * EOL_SCR   = EOL_UNIX;
const char * EOL_FILE  = EOL_UNIX;

const char * STRUCT_UNIT_START = "-+*";

const char * Directions[] = {"North", "Northeast", "Southeast", "South", "Southwest", "Northwest",
                             "N",     "NE",        "SE",        "S",     "SW",        "NW"       };
//enum       eDirection     { North=0, Northeast,   Southeast,   South,   Southwest,   Northwest };
int          ExitFlags [] = { 0x01,    0x02,        0x04,        0x08,    0x10,        0x20      };
int          EntryFlags[] = { 0x08,    0x10,        0x20,        0x01,    0x02,        0x04      };

int Flags_NW_N_NE = 0x01 | 0x02 | 0x20;
int Flags_N       = 0x01;
int Flags_SW_S_SE = 0x04 | 0x08 | 0x10;
int Flags_S       = 0x08;

const char * LocationsShipsArcadia[]  = { "Northern hexside",
                                          "North Eastern hexside",
                                          "South Eastern hexside",
                                          "Southern hexside",
                                          "South Western hexside",
                                          "North Western hexside",
                                          "hex centre" };



static const char * BUG        = " - it's a bug!";
static const char * NOSETUNIT  = " - Can not set property for unit ";
static const char * NOSET      = " - Can not set unit property ";
static const char * NOTNUMERIC = " - Property is not numeric for unit ";
//static const char * NOTSTRING  = " - Property type is not string for unit ";

const char * ExitEndHeader[]  = { HDR_FACTION          ,
                                  HDR_FACTION_STATUS   ,
                                  HDR_ERRORS           ,
                                  HDR_EVENTS           ,
                                  HDR_SILVER           ,
                                  HDR_BATTLES          ,
                                  HDR_ATTACKERS        ,
                                  HDR_ATTITUDES        ,
                                  HDR_SKILLS           ,
                                  HDR_ITEMS            ,
                                  HDR_OBJECTS
                                };
int ExitEndHeaderLen[] =        { sizeof(HDR_FACTION       ) - 1,
                                  sizeof(HDR_FACTION_STATUS) - 1,
                                  sizeof(HDR_ERRORS        ) - 1,
                                  sizeof(HDR_EVENTS        ) - 1,
                                  sizeof(HDR_SILVER        ) - 1,
                                  sizeof(HDR_BATTLES       ) - 1,
                                  sizeof(HDR_ATTACKERS     ) - 1,
                                  sizeof(HDR_ATTITUDES     ) - 1,
                                  sizeof(HDR_SKILLS        ) - 1,
                                  sizeof(HDR_ITEMS         ) - 1,
                                  sizeof(HDR_OBJECTS       ) - 1
                                };


const char * BattleEndHeader[]= { HDR_ERRORS           ,
                                  HDR_EVENTS           ,
                                  HDR_SILVER           ,
                                  HDR_ATTITUDES        ,
                                  HDR_SKILLS           ,
                                  HDR_ITEMS            ,
                                  HDR_OBJECTS          ,
                                  HDR_FACTION          ,
                                  HDR_FACTION_STATUS   ,
                                  HDR_SILVER
                                };
int BattleEndHeaderLen[] =      { sizeof(HDR_ERRORS        ) - 1,
                                  sizeof(HDR_EVENTS        ) - 1,
                                  sizeof(HDR_SILVER        ) - 1,
                                  sizeof(HDR_ATTITUDES     ) - 1,
                                  sizeof(HDR_SKILLS        ) - 1,
                                  sizeof(HDR_ITEMS         ) - 1,
                                  sizeof(HDR_OBJECTS       ) - 1,
                                  sizeof(HDR_FACTION       ) - 1,
                                  sizeof(HDR_FACTION_STATUS) - 1,
                                  sizeof(HDR_SILVER        ) - 1
                                };



//----------------------------------------------------------------------

bool IsInteger(const char * s)
{
    int n = 0;

    if ((!s) || (!*s))
        return false;

    while (*s)
    {
        if ( ('-'==*s) && (n>0) )
            return false;
        else
            if ( (*s<'0') || (*s>'9') )
                return false;

        n++;
        s++;
    }
    return true;
}

//======================================================================

CAtlaParser::CAtlaParser()
{
    int x = 2;
    x = 1/(x-2);
    //assert(0);
}

//----------------------------------------------------------------------

CAtlaParser::CAtlaParser(CGameDataHelper * pHelper)
            : m_sOrderErrors()
{
    gpDataHelper      = pHelper;
    m_sOrderErrors.reserve(256);

    m_CrntFactionId   = 0;
    m_ParseErr        = ERR_NOTHING;
    m_nCurLine        = 0;
    m_GatesCount      = 0;
    m_YearMon         = 0;
    m_CurYearMon      = 0;
    m_pSource         = nullptr;
    m_pCurLand        = nullptr;
    m_pCurStruct      = nullptr;
    m_NextStructId    = 1;
    m_OrdersLoaded    = false;
//    m_MaxSkillDays    = 450;
    m_JoiningRep      = false;
    m_IsHistory       = false;
    m_Events.Name     = "Events";
    m_SecurityEvents.Name = "Security Events";
    m_HexEvents.Name  = "Hex Events";
    m_Errors.Name     = "Errors";
    m_ArcadiaSkills   = false;

    m_UnitFlagsHash["taxing"                      ] = UNIT_FLAG_TAXING            ;
    m_UnitFlagsHash["on guard"                    ] = UNIT_FLAG_GUARDING          ;
    m_UnitFlagsHash["avoiding"                    ] = UNIT_FLAG_AVOIDING          ;
    m_UnitFlagsHash["behind"                      ] = UNIT_FLAG_BEHIND            ;
    m_UnitFlagsHash["revealing unit"              ] = UNIT_FLAG_REVEALING_UNIT    ;
    m_UnitFlagsHash["revealing faction"           ] = UNIT_FLAG_REVEALING_FACTION ;
    m_UnitFlagsHash["holding"                     ] = UNIT_FLAG_HOLDING           ;
    m_UnitFlagsHash["receiving no aid"            ] = UNIT_FLAG_RECEIVING_NO_AID  ;
    m_UnitFlagsHash["consuming unit's food"       ] = UNIT_FLAG_CONSUMING_UNIT    ;
    m_UnitFlagsHash["consuming faction's food"    ] = UNIT_FLAG_CONSUMING_FACTION ;
    m_UnitFlagsHash["won't cross water"           ] = UNIT_FLAG_NO_CROSS_WATER    ;
    // MZ - Added for Arcadia
    m_UnitFlagsHash["sharing"                     ] = UNIT_FLAG_SHARING           ;

    m_UnitFlagsHash["weightless battle spoils"    ] = UNIT_FLAG_SPOILS            ;
    m_UnitFlagsHash["flying battle spoils"        ] = UNIT_FLAG_SPOILS            ;
    m_UnitFlagsHash["walking battle spoils"       ] = UNIT_FLAG_SPOILS            ;
    m_UnitFlagsHash["riding battle spoils"        ] = UNIT_FLAG_SPOILS            ;

}

//----------------------------------------------------------------------

CAtlaParser::~CAtlaParser()
{
    Clear();
    m_UnitFlagsHash.clear();
}




//----------------------------------------------------------------------

void CAtlaParser::Clear()
{
    m_Factions.FreeAll();


    m_YearMon       = 0;
    m_CurYearMon    = 0;
    m_JoiningRep    = false;
    m_IsHistory     = false;
    m_Planes.DeleteAll();
    m_PlanesNamed.FreeAll();  // this one must go before units since it checks contents of CLand::Units collection!!!
    m_Units.FreeAll();

    m_CrntFactionId = 0;
    m_CrntFactionPwd.clear();
    m_OurFactions.clear();
    m_TaxLandStrs.clear();
    m_TradeLandStrs.clear();
    m_BattleLandStrs.clear();
    m_UnitPropertyNames.clear();
    m_UnitPropertyTypes.clear();
    m_LandPropertyNames.clear();
    //m_LandPropertyTypes.FreeAll();

    m_TradeUnitIds.clear();
    m_Skills.FreeAll();
    m_Items.FreeAll();
    m_Objects.FreeAll();
    m_Battles.FreeAll();
    m_Gates.FreeAll();
    m_Events.Description.clear();
    m_SecurityEvents.Description.clear();
    m_HexEvents.Description.clear();
    m_Errors.Description.clear();
    m_NewProducts.FreeAll();
    m_TempSailingEvents.FreeAll();
}

//----------------------------------------------------------------------

bool CAtlaParser::ReadNextLine(std::string & s)
{
    bool Ok=false;

    if (m_pSource)
    {
        Ok = m_pSource->GetNextLine(s);
        if (Ok)
        {
            m_nCurLine++;
            TrimRight(s, TRIM_ALL);
            s << EOL_SCR;
        }
    }
    return Ok;
};

//----------------------------------------------------------------------

void CAtlaParser::PutLineBack (std::string & s)
{
    m_nCurLine--;
    if (m_pSource)
        m_pSource->QueueString(s.c_str(), s.size());
};

//----------------------------------------------------------------------

int CAtlaParser::ParseFactionInfo(bool GetNo, bool Join)
{
    int          err    = ERR_OK;
    std::string Line;
    std::string         FNo;
    std::string         Str;
    int          LineNo = 0;
    const char * p;
    const char * s;
    unsigned int i;
    CFaction   * pMyFaction;
    long         yearmon = 0;

    while ((ERR_OK==err) && ReadNextLine(Line))
    {
        m_FactionInfo << Line;

        TrimRight(Line, TRIM_ALL);

        if (Line.empty())
            break;  // stop at empty line


        if (GetNo)
            switch (LineNo)
            {
            case 0:  // faction number
                s = Line.c_str();
                p = strchr(s, '(');
                if (p)
                {
                    pMyFaction = new CFaction;
                    pMyFaction->Description << Line.c_str() << EOL_SCR;
                    SetStr(pMyFaction->Name, s, (p-s));
                    s = p+1;
                    p = strchr(s, ')');
                    SetStr(FNo, s, (p-s));
                    m_CrntFactionId = atol(FNo.c_str());
                    pMyFaction->Id  = m_CrntFactionId;
                    if (!m_Factions.Insert(pMyFaction))
                        delete pMyFaction;
                    m_OurFactions.push_back(m_CrntFactionId);
                    if (!Join) gpDataHelper->SetPlayingFaction((long) m_CrntFactionId);
                }
                break;
            case 1:  // date  December, Year 2
                p = GetToken(Str, Line.c_str(), ',');
                for (i=0; i<sizeof(Monthes)/sizeof(char*); i++)
                    if (0==FindSubStr(Str, Monthes[i]))
                    {
                        yearmon = i+1;
                        break;
                    }
                p = SkipSpaces(p);
                //while (p && *p<=' ')
                //    p++;
                p = GetToken(Str, p, ' '); // 'Year'
                p = GetToken(Str, p, ' ');
                yearmon += 100*atol(Str.c_str());

                if (m_JoiningRep && m_YearMon != yearmon)
                {
                    err = ERR_INV_TURN;
                    if (m_CrntFactionId > 0)
                    {
                        auto it = std::find(m_OurFactions.begin(), m_OurFactions.end(), m_CrntFactionId);
                        if (it != m_OurFactions.end())
                            m_OurFactions.erase(it);
                    }
                }
                else
                {
                    m_YearMon    = yearmon;
                    m_CurYearMon = yearmon; // file being currently loaded may not contain year/month info
                }
                break;
            }

        LineNo++;
    }


    return err;
}

//----------------------------------------------------------------------

int  CAtlaParser::SetLandFlag(const char * p, long flag)
{
    CLand * pLand = GetLand(p);

    if (pLand)
        pLand->Flags |= flag;

    return 0;
}

//----------------------------------------------------------------------

int  CAtlaParser::SetLandFlag(long LandId, long flag)
{
    CLand * pLand;

    pLand = GetLand(LandId);
    if (pLand)
        pLand->Flags |= flag;

    return 0;
}

//----------------------------------------------------------------------

int  CAtlaParser::ApplyLandFlags()
{

    int          i, idx;
    const char * s;
    CBaseObject  Dummy;
    CUnit      * pUnit;

    for (const auto& s : m_TaxLandStrs)
        SetLandFlag(s.c_str(), LAND_TAX);

    for (const auto& s : m_TradeLandStrs)
        SetLandFlag(s.c_str(), LAND_TRADE);

    for (const auto& s : m_BattleLandStrs)
        SetLandFlag(s.c_str(), LAND_BATTLE);

    for (long unitId : m_TradeUnitIds)
    {
        Dummy.Id = unitId;
        if (m_Units.Search(&Dummy, idx))
        {
            pUnit = (CUnit*)m_Units.At(idx);
            SetLandFlag(pUnit->LandId, LAND_TRADE);
        }
    }

    m_TaxLandStrs.clear();
    m_TradeLandStrs.clear();
    m_BattleLandStrs.clear();
    m_TradeUnitIds.clear();


    return 0;
}

//----------------------------------------------------------------------

void CAtlaParser::ParseOneMovementEvent(const char * params, const char * structid, const char * fullevent)
{
    std::string Buf;
    std::string Buf2;
    char         ch;
    CLand      * pLand1 = nullptr;
    CLand      * pLand2 = nullptr;
    std::string         S;
    CBaseObject* pSailEvent;

    while (params)
    {
        params = SkipSpaces(GetToken(S, params, " \n", ch, TRIM_ALL));
        if (0==stricmp(S.c_str(), "to"))
            break;
        Buf << S << ' ';
    }
    if (0==stricmp(S.c_str(), "to"))
    {
        while (params)
        {
            params = SkipSpaces(GetToken(S, params, " \n", ch, TRIM_ALL));
            Buf2 << S << ' ';
        }
    }

    ParseTerrain(nullptr, 0, Buf, false, &pLand1);

    // It is nice to parse 'to' terrain as well, since the unit
    // can be killed at the destination on sight...
    if (!Buf2.empty())
        ParseTerrain(nullptr, 0, Buf2, false, &pLand2);

    // check for links between planes
    if (pLand1 && pLand2  &&  pLand1->pPlane && pLand2->pPlane  && pLand1->pPlane != pLand2->pPlane)
    {
        pLand1->SetProperty(PRP_LAND_LINK, eLong, (void*)pLand2->Id, eBoth);
        pLand2->SetProperty(PRP_LAND_LINK, eLong, (void*)pLand1->Id, eBoth);
        m_LandsToBeLinked.Insert(pLand1);
        m_LandsToBeLinked.Insert(pLand2);
    }

    // collect info for linking sail events to captains
    if (structid && fullevent)
    {
        pSailEvent              = new CBaseObject;
        pSailEvent->Id          = atol(structid);
        pSailEvent->Description = fullevent;
        m_TempSailingEvents.Insert(pSailEvent);
    }

}

//----------------------------------------------------------------------

bool CAtlaParser::ParseOneUnitEvent(std::string & EventLine, bool IsEvent, int UnitId)
{
    CUnit      * pUnit = nullptr;
    const char * p;
    std::string         Name;
    bool         Taken = false;
    long         x;
    int          idx;
    std::string Buf;


    p = GetToken(Name, EventLine.c_str(), '(');
    if (UnitId>0)
    {
        pUnit = MakeUnit(UnitId);
        if (pUnit->Name.empty())
            pUnit->Name = Name;
    }
    if (pUnit)
    {
        if (IsEvent)
            pUnit->Events << EventLine;
        else
            pUnit->Errors << EventLine;
        Taken = true;
    }

    if (IsEvent)
    {
        // Try to get something of the unit event...
        p = strchr(p, ')');
        while (p && (*p>' ') )
            p++;
        p = SkipSpaces(p);

        p = SkipSpaces(GetToken(Buf, p, ' ', TRIM_ALL));
        if ( (0==stricmp("walks"   , Buf.c_str())) ||
             (0==stricmp("rides"   , Buf.c_str())) ||
             (0==stricmp("flies"   , Buf.c_str())) )
        {
            p = SkipSpaces(GetToken(Buf, p, ' ', TRIM_ALL));
            if (0==stricmp("from"  , Buf.c_str()))
                ParseOneMovementEvent(p, nullptr, nullptr);
        }

        else if ( p && ('$'==*p) && ( (0==stricmp("collects", Buf.c_str())) ||
                                      (0==stricmp("pillages", Buf.c_str())) )
                )
        {
            p = GetToken(Buf, p, '(', TRIM_ALL);
            p = GetToken(Buf, p, ')', TRIM_ALL);
            m_TaxLandStrs.insert(Buf.c_str());
        }
        else if (0==stricmp("produces", Buf.c_str()))
        {
            p = GetToken(Buf, p, '(', TRIM_ALL);
            p = GetToken(Buf, p, ')', TRIM_ALL);
            m_TradeLandStrs.insert(Buf.c_str());
        }

        // Performs work, is a trade activity
        // And now it is called 'construction'

        // TBD: maybe implement buying and selling trade goods later
        else if (0==stricmp("performs", Buf.c_str()))
        {
            p = SkipSpaces(GetToken(Buf, p, ' ', TRIM_ALL));
            if (0==stricmp("work"        , Buf.c_str()) ||
                0==stricmp("construction", Buf.c_str()) )
            {
                p = GetToken(Buf, EventLine.c_str(), '(', TRIM_ALL);
                p = GetToken(Buf, p                  , ')', TRIM_ALL);
                x = atol(Buf.c_str());
                m_TradeUnitIds.insert(x);

            }
        }
        
        // Mary Loo (1104): Has mithril sword [MSWO] stolen.
        // Unit (3849) is caught attempting to steal from Unit (1662) in Lotan.
        // Unit (3595) steals double bow [DBOW] from So many farmers (1766).
        // Unit (1023): Is forbidden entry to swamp (31,17) in Dorantor by 
        else if (0==stricmp("has"    , Buf.c_str()) && FindSubStr(EventLine, "stolen")>=0 ||
                 0==stricmp("is"     , Buf.c_str()) && FindSubStr(EventLine, "caught")>=0  ||
                 0==stricmp("steals" , Buf.c_str()) ||
                 0==stricmp("is"     , Buf.c_str()) && FindSubStr(EventLine, "forbidden")>=0 ||
                 0==stricmp("forbids", Buf.c_str()) && FindSubStr(EventLine, "entry")>=0
                )
        {
            bool show = true;
            if (0==stricmp("steals", Buf.c_str()) && 
                0==atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_SHOW_STEALS)))
                show = false;
            if (show)
                m_SecurityEvents.Description << EventLine;
        }

    }

    return Taken;
}

//----------------------------------------------------------------------

bool CAtlaParser::ParseOneLandEvent(std::string & EventLine, bool IsEvent)
{
    const char * p;
    std::string         Buf;
    std::string         S;
    bool         Taken = false;
    CLand      * pLand = nullptr;

    p = GetToken(Buf, EventLine.c_str(), ')');
    p = GetToken(S, p, ',');
    Buf << ") " << S;
    ParseTerrain(nullptr, 0, Buf, false, &pLand);
    if (pLand)
        m_HexEvents.Description << EventLine;

    return Taken;
}


//----------------------------------------------------------------------

//Speedy (1356): Rides from swamp (7,35) in Moffat to plain (7,37) in
//  Partry.
//Speedy (1356): Rides from plain (7,37) in Partry to plain (7,39) in
//  Partry.

//Magoga (892): Walks from plain (10,34) in Grue to plain (9,35) in
//  Grue.

//Choppers (1101): Produces 13 wood [WOOD] in swamp (7,35) in Moffat.

int CAtlaParser::ParseOneEvent(std::string & EventLine, bool IsEvent)
{
    const char * p;
    std::string Buf;
    std::string         StructId;
    std::string         Name;
    long         x;
    char         ch;
    bool         Taken = false;
    bool         Valid;

    if (EventLine.empty())
        return 0;
    EventLine << EOL_SCR;


    p = GetToken(Name, EventLine.c_str(), "([", ch, TRIM_ALL);
    switch (ch)
    {
    case '(':
        p = GetInteger(Buf, p, Valid);
        if (*p == ')')
        {
            if (0==strnicmp(Name.c_str(), "The address of ", 15))
            {
                // it will goto generic events
            }
            else
            {
                // it is  a unit!
                x = atol(Buf.c_str());
                Taken = ParseOneUnitEvent(EventLine, IsEvent, x);
            }
        }
        else
        {
            // could be a land event
            Taken = ParseOneLandEvent(EventLine, IsEvent);
        }
        break;

    case '[':   // ship, probably
        if (IsEvent)
        {
            p = SkipSpaces(GetToken(StructId, p, ']', TRIM_ALL));
            //while (p && (*p>' ') )
            //    p++;
            //p = SkipSpaces(p);
            p = SkipSpaces(GetToken(Buf, p, ' ', TRIM_ALL));
            if (0==stricmp("sails"   , Buf.c_str()))
            {
                p = SkipSpaces(GetToken(Buf, p, ' ', TRIM_ALL));
                if (0==stricmp("from"  , Buf.c_str()))
                    ParseOneMovementEvent(p, StructId.c_str(), EventLine.c_str());
//                {
//                    Buf = p;
//                    x = FindSubStr(Buf, " to ");
//                    if (x>0)
//                        DelSubStr(Buf, x, Buf.size()-x);
//                    ParseTerrain(nullptr, 0, Buf, false, nullptr);
//                }
            }
        }
        break;
    }


    if (IsEvent)
    {
        if (!Taken)
            m_Events.Description << EventLine;
    }
    else
        m_Errors.Description << EventLine;

    return 0;
}


//----------------------------------------------------------------------

int CAtlaParser::ParseEvents(bool IsEvents)
{
    int          err   = ERR_OK;
    std::string Line;
    std::string OneEvent;
    char         ch;


    while ((ERR_OK==err) && ReadNextLine(Line))
    {
        TrimRight(Line, TRIM_ALL);

        if (Line.empty())
        {
            ParseOneEvent(OneEvent, IsEvents);
            break;  // stop at empty line
        }

//        // comment/error may take more than one line.
//        // Dot at the end is not reliable!
//        if (strchr(Line.c_str(), ':') ||
//            strchr(Line.c_str(), '[') ||
//            (!OneEvent.empty() && ('.'==OneEvent.c_str()[OneEvent.size()-1]))
//           )

        // Looks like it is time to check spaces at the line start :((
        // additional event lines start with spaces
        ch = Line.c_str()[0];
        if (ch != ' ' && ch != '\t')
        {
            // That is hopefully a new event
            ParseOneEvent(OneEvent, IsEvents);
            OneEvent.clear();
        }
        if (!OneEvent.empty())
            OneEvent << EOL_SCR;
        OneEvent << Line;

    }


    return err;
}

//----------------------------------------------------------------------

int CAtlaParser::ParseOneImportantEvent(std::string & EventLine)
{
    m_HexEvents.Description << EventLine << EOL_SCR;
    m_Events.Description << EventLine << EOL_SCR;
    return ERR_OK;
}

//----------------------------------------------------------------------

int CAtlaParser::ParseImportantEvents()
{
    int          err   = ERR_OK;
    std::string Line;
    std::string OneEvent;
    char         ch;
    int          i;
    bool         DoBreak = false;



    while ((ERR_OK==err) && ReadNextLine(Line))
    {
        TrimRight(Line, TRIM_ALL);

        for (i=0; i<(int)sizeof(ExitEndHeader)/(int)sizeof(const char *); i++)
            if (0==strnicmp(Line.c_str(), ExitEndHeader[i], ExitEndHeaderLen[i] ))
        {
            Line << EOL_FILE;
            PutLineBack(Line);
            DoBreak = true;
            break;
        }
        if (DoBreak)
            break;


        // Looks like it is time to check spaces at the line start :((
        // additional event lines start with spaces
        ch = Line.c_str()[0];
        if (ch != ' ' && ch != '\t')
        {
            // That is hopefully a new event
            ParseOneImportantEvent(OneEvent);
            OneEvent.clear();
        }
        if (!OneEvent.empty())
            OneEvent << EOL_SCR;
        OneEvent << Line;

    }
    ParseOneImportantEvent(OneEvent);


    return err;

}

//----------------------------------------------------------------------

int CAtlaParser::ParseErrors()
{
    return ParseEvents(false);
}

//----------------------------------------------------------------------

int CAtlaParser::ParseUnclSilver(std::string & Line)
{
    const char * p;
    const char * s;
    std::string         N;
    CFaction   * pFaction;

    m_FactionInfo << Line;

    TrimRight(Line, TRIM_ALL);
    s = Line.c_str() + sizeof(HDR_SILVER)-1;
    p = strchr(s, '.');

    if (p)
        SetStr(N, s, p-s);
    else
        SetStr(N, s);
    TrimLeft(N);

    TrimRight(N, TRIM_ALL);

    pFaction = GetFaction(m_CrntFactionId);
    if (pFaction)
        pFaction->UnclaimedSilver = atol(N.c_str());

    return ERR_OK;
}

//----------------------------------------------------------------------

/*
Declared Attitudes (default Neutral):
Hostile : none.
Unfriendly : none.
Neutral : none.
Friendly : none.
Ally : none.
*/
int CAtlaParser::ParseAttitudes(std::string & Line, bool Join)
{
    std::string         Info;
    std::string         FNo;
    std::string         S1;
    const char * str;
    const char * p;
    const char * s;
    char         ch, c;
    int          attitude = ATT_FRIEND1;
    std::string         attitudes[4];
    bool         apply_attitudes = true;
    bool         def;

    attitudes[ATT_FRIEND1] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_FRIEND1);
    attitudes[ATT_FRIEND2] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_FRIEND2);
    attitudes[ATT_NEUTRAL] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_NEUTRAL);
    attitudes[ATT_ENEMY] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_ENEMY);

    if(Join)
    {   // check config whether to apply allied attitudes
        apply_attitudes  = (0!=SafeCmp(gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_APPLY_ON_JOIN),"0"));
    }
    else
    {
        while(attitude <= ATT_ENEMY)
        {
            if(0<=FindSubStr(attitudes[attitude], "Own")) break;
            attitude++;
        }
        gpDataHelper->SetAttitudeForFaction(-1, attitude);
        attitude = ATT_ENEMY;
    }

    while (!Line.empty()) // is this correct?
    {
        str = Line.c_str();
        m_FactionInfo << Line;

        if(apply_attitudes) // parse attitudes
        {
            while(str)
            {
                str = GetToken(Info, str, ":,.", ch, TRIM_ALL);
                p   = Info.c_str();
                switch(ch)
                {
                    case ':': // attitude type
                        // check for default line
                        def = false;
                        if(FindSubStr(Info, "(default") > 0)
                        {
                            // parse the default line
                            s = GetToken(S1, p, "(", c, TRIM_ALL);
                            GetToken(S1, s, ")" ,c , TRIM_ALL);
                            s = S1.c_str();
                            p = GetToken(Info, s, " ", c, TRIM_ALL);
                            def = true;
                            m_FactionInfo << EOL_SCR;
                        }
                        // which attitude is it?
                        while(attitude >= ATT_FRIEND1)
                        {
                            if(0<=FindSubStr(attitudes[attitude], p)) break;
                            attitude--;
                        }
                        if((!Join) && def && (attitude >= ATT_FRIEND1) && (attitude < ATT_UNDECLARED))
                        {   // set the default attitude
                            gpDataHelper->SetAttitudeForFaction(0, attitude);
                        }
                        break;
                    case '.':
                        m_FactionInfo << EOL_SCR;
                        break;
                    case ',':
                        if((attitude <= ATT_UNDECLARED) && (attitude >= ATT_FRIEND1))
                        {
                            // parse faction id
                            if(0==strcmp(p,"none")) break;
                            s = GetToken(S1, p, "(", c, TRIM_ALL);
                            GetToken(FNo, s, ")", c, TRIM_ALL);
                            if (!FNo.empty())
                            {
                                int id = atol(FNo.c_str());
                                gpDataHelper->SetAttitudeForFaction(id, attitude);
                            }
                        }
                        break;
                }
            }
        }
        ReadNextLine(Line);
        TrimRight(Line, TRIM_ALL);
    }

    return ERR_OK;
}

//----------------------------------------------------------------------

void CAtlaParser::CheckExit(CPlane * pPlane, int Direction, CLand * pLandSrc, CLand * pLandExit)
{
    int x1,y1,x2,y2, z, width;

    LandIdToCoord(pLandSrc ->Id, x1, y1, z);
    LandIdToCoord(pLandExit->Id, x2, y2, z);

    //if (0==pPlane->Width)
        switch (Direction%6)
        {
        case Northeast:

        case Southeast:
	    width = x1-x2+1;
            if (x2<x1 && width>pPlane->Width)
            {
                pPlane->WestEdge   = x2;
                pPlane->EastEdge   = x1;
                pPlane->Width      = width;

                pPlane->EdgeSrcId  = pLandSrc ->Id;
                pPlane->EdgeExitId = pLandExit->Id;
                pPlane->EdgeDir    = Direction%6;
            }
            break;

        case Northwest:
        case Southwest:
	    width = x2-x1+1;
            if (x2>x1 && width>pPlane->Width)
            {
                pPlane->WestEdge   = x1;
                pPlane->EastEdge   = x2;
                pPlane->Width      = width;

                pPlane->EdgeSrcId  = pLandSrc ->Id;
                pPlane->EdgeExitId = pLandExit->Id;
                pPlane->EdgeDir    = Direction%6;
            }
            break;
        }

		if (pPlane->Width > 0)
		{
			if (x1 > pPlane->EastEdge)
				pPlane->EastEdge = x1;
			if (x1 < pPlane->WestEdge)
				pPlane->WestEdge = x1;
			if (x2 > pPlane->EastEdge)
				pPlane->EastEdge = x2;
			if (x2 < pPlane->WestEdge)
				pPlane->WestEdge = x2;
			pPlane->Width = pPlane->EastEdge - pPlane->WestEdge + 1;
			pPlane->Width += pPlane->Width & 1;
		}
}

//----------------------------------------------------------------------

void CAtlaParser::ParseWeather(const char * src, CLand * pLand)
{
    bool         IsCurrent;
    bool         IsGood;
    int          Zone;
    unsigned int i;
    std::string         S1, S2;
    const char * p;
    int          x,y,z;
    CPlane     * pPlane = pLand->pPlane;

    if (!src || !pPlane)
        return;

    if (m_WeatherLine[0].empty())
    {
        for (i=0; i<sizeof(m_WeatherLine)/sizeof(*m_WeatherLine); i++)
        {
            // read weather lines
            // bit 0 is IsCurrent
            // bit 1 is IsGood
            // the rest is Zone
            IsCurrent = i & 1;
            IsGood    = (i & 2) >> 1;
            Zone      = i >> 2;

            m_WeatherLine[i] = gpDataHelper->GetWeatherLine(IsCurrent, IsGood, Zone);
            Normalize(m_WeatherLine[i]);
        }
    }

    src = SkipSpaces(src);
    if ('-'==src[0] && '-'==src[1] && '-'==src[2] )
    {
        while (*src > ' ')
            src++;
        src = SkipSpaces(src);
        src = GetToken(S1, src, ';', TRIM_ALL);
        src = GetToken(S2, src, '.', TRIM_ALL);
        Normalize(S1);
        Normalize(S2);

        for (i=0; i<sizeof(m_WeatherLine)/sizeof(*m_WeatherLine); i++)
        {
            // bit 0 is IsCurrent
            // bit 1 is IsGood
            // the rest is Zone
            IsCurrent = i & 1;
            IsGood    = (i & 2) >> 1;
            Zone      = i >> 2;

            /*
            if (IsGood || Zone>0)
                continue; // looks like good weather is exactly the same everywhere
                          // and we only handle Tropic zone for now
            LandIdToCoord(pLand->Id, x,y,z);
            if (y>=pPlane->TropicZoneMin && y<=pPlane->TropicZoneMax)
                continue; // known coordinate
            */

            if (IsCurrent)
                p = S1.c_str();
            else
                p = S2.c_str();
            if (0==stricmp(p, m_WeatherLine[i].c_str()))
            {
                if (IsGood )
                {
                    if (!IsCurrent)
                       pLand->WeatherWillBeGood = true;
                }
                else
                {
                    LandIdToCoord(pLand->Id, x,y,z);
                    if (Zone>0)
                        continue; // we only draw the tropic line!
                    if (y>=pPlane->TropicZoneMin && y<=pPlane->TropicZoneMax)
                        continue; // known coordinate, don't let them shrink when some hexes are not visible any more!

                    if (y >= pPlane->TropicZoneMax)
                        pPlane->TropicZoneMax = y;
                    if (y <= pPlane->TropicZoneMin)
                        pPlane->TropicZoneMin = y;
                }
            }
        }
    }
}

//----------------------------------------------------------------------

int CAtlaParser::AnalyzeTerrain(CLand * pMotherLand, CLand * pLand, bool IsExit, int ExitDir, std::string & Description)
{
/*
plain (5,39) in Partry, contains Drimnin [city], 3217 peasants (high
  elves), $22519.
------------------------------------------------------------
  It was winter last month; it will be winter next month.
  Wages: $17 (Max: $10937).
  Wanted: 144 grain [GRAI] at $20, 108 livestock [LIVE] at $21, 33
    longbows [LBOW] at $126, 28 plate armor [PARM] at $374, 10 caviar
    [CAVI] at $158, 12 cotton [COTT] at $149.
  For Sale: 35 horses [HORS] at $62, 28 wagons [WAGO] at $157, 17
    pearls [PEAR] at $71, 16 wool [WOOL] at $76, 643 high elves [HELF]
    at $68, 128 leaders [LEAD] at $136.
  Entertainment available: $1125.
  Products: 40 grain [GRAI], 33 horses [HORS].
*/
    enum         {eMain, eSale, eWanted, eProduct, eNone} SectType;
    std::string Section;
    std::string Struct;
    std::string S1;
    std::string S2;
    std::string N1;
    std::string N2;
    std::string Buf;
    long         n1;
    long         n2;
    const char * src;
    const char * str;
    const char * srcold;
    const char * p;
    char         ch;
    CProduct   * pProd;
    int          delpos = 0;
    int          dellen = 0;
    int          idx;
    bool         TerrainPassed = false;
    bool         ProductsWereEmpty;
    bool         Valid;

    SectType = eMain;

    ProductsWereEmpty = (0==pLand->Products.Count());
    // skip terrain coordinates - they are confusing for the edge thingy
    srcold   = strchr(Description.c_str(), ')');
    if (srcold)
        srcold = SkipSpaces(srcold++);
    else
        srcold = Description.c_str(); // something must be very wrong here, must never happen

    src      = GetToken(Section, srcold, '.', TRIM_ALL);
    if (!m_IsHistory && m_CurYearMon>0)
        ParseWeather(src, pLand);  // weather description should be right after the first section
    while (!Section.empty())
    {
        bool RerunSection = false;

        str = Section.c_str();
        while (str)
        {
            if (RerunSection)
                break;

            str = GetToken(Struct, str, ":,", ch, TRIM_ALL);
            p   = Struct.c_str();
            switch(ch)
            {
            case ':': // that's a section name!
                pLand->Flags|=LAND_VISITED; // Just simple presense in the report is not enough!
                if      (0==stricmp("For Sale", p))
                    SectType = eSale;
                else if (0==stricmp("Wanted"  , p))
                    SectType = eWanted;
                else if (0==stricmp("Products", p))
                {
                    SectType = eProduct;
                    delpos   = srcold - Description.c_str();
                    dellen   = src - srcold;
                }
                else if (0==stricmp("Wages"  , p))
                {
                    // Wages does not match the common pattern
                    ParseWages(pLand, str, src);
                }
                else
                    SectType = eNone;
                break;

            case ',':
            case  0 :
                switch (SectType)
                {
                case eMain:    // recognize contains, $ and peasants
                    if ('$'==*p)
                    {
                        GetInteger(N1, ++p, Valid);
                        pLand->Taxable = atol(N1.c_str());
                    }
                    else
                    {
                        p  = SkipSpaces(GetToken(S1, p, ' ', TRIM_ALL));
                        p  = GetToken(S2, p, ' ', TRIM_ALL);
                        n1 = atol(S1.c_str());
                        if (n1>0)
                        {
                            if (0==stricmp("peasants", S2.c_str()))
                            {
                                pLand->Peasants = n1;
                                p  = GetToken(S2, p, '(', TRIM_ALL);
                                p  = GetToken(pLand->PeasantRace, p, ')', TRIM_ALL);
                                Replace(pLand->PeasantRace, '\r', ' ');
                                Replace(pLand->PeasantRace, '\n', ' ');
                                Replace(pLand->PeasantRace, '\t', ' ');
                                Normalize(pLand->PeasantRace);
                                Replace(pLand->PeasantRace, ' ', '_');
                            }
                        }
                        else if (0==stricmp("contains", S1.c_str()))
                        {
                            //pLand->CityName = S2;
                            //p = GetToken(S2, p, '[');
                            //p = GetToken(pLand->CityType, p, ']');

                            //There may be a space in the city name!
                            p = GetToken(S1, Struct.c_str(), ' ', TRIM_ALL);

                            p = GetToken(pLand->CityName, p, '[', TRIM_ALL);
                            if (!p)
                            {
                                // ok, it is a dot in the city name! need to append and rerun the section!
                                RerunSection = true;
                                break;
                            }
                            p = GetToken(pLand->CityType, p, ']', TRIM_ALL);
                            // set town type LandFlags
                            if(0==SafeCmp(ToLower(pLand->CityType),"town"))
                            {
                                pLand->Flags |= LAND_TOWN;
                            }
                            else if (0==SafeCmp(ToLower(pLand->CityType),"city"))
                            {
                                pLand->Flags |= LAND_CITY;
                            }
                            // what about villages??
                        }
                        else if (!TerrainPassed)
                            TerrainPassed = true; //so we can do special parsing for Arcadia III edge objects below
                        else if (IsExit)
                        {
                            // it must be an edge object...
                            if (pMotherLand)
                            {
                                pMotherLand->AddNewEdgeStruct(S1.c_str(), ExitDir);
                                // also add to neighbouring hex
                                int adj_dir = ExitDir -3;
                                if(adj_dir < 0) adj_dir += 6;
                                pLand->AddNewEdgeStruct(S1.c_str(), adj_dir);
                            }
                        }
                    }
                    break;

                case eSale:
                case eWanted:  // 35 horses [HORS] at $62
                               // N1 S1     [S2]   at $N2

//                    p = SkipSpaces(GetToken(N1, p, ' ', TRIM_ALL));
//                    n1= atol(N1.c_str());
                    // First number is optional, if missing it is 1
                    p = GetInteger(N1, p, Valid);
                    if (N1.empty())
                    {
                        GetToken(N1, p, " [", ch);
                        if (0==stricmp(N1.c_str(), "none"))
                            N1 = "-1";
                        else if (0==stricmp(N1.c_str(), "unlimited"))
                            N1 = "10000000";
                        else
                            N1 = "1";

                    }
                    n1 = atol(N1.c_str());
                    p = GetToken(S1, p, '[', TRIM_ALL);
                    p = GetToken(S2, p, ']', TRIM_ALL);
                    p = GetToken(N2, p, '$', TRIM_ALL);
                    N2= p;
                    n2= atol(N2.c_str());

                    if ((!S2.empty()) && (n2>0) )
                    {
                        if (eSale == SectType)
                            MakeQualifiedPropertyName(PRP_SALE_AMOUNT_PREFIX, S2.c_str(), Buf);
                        else
                            MakeQualifiedPropertyName(PRP_WANTED_AMOUNT_PREFIX, S2.c_str(), Buf);
                        SetLandProperty(pLand, Buf.c_str(), eLong, (void*)n1, eBoth);

                        if (eSale == SectType)
                            MakeQualifiedPropertyName(PRP_SALE_PRICE_PREFIX, S2.c_str(), Buf);
                        else
                            MakeQualifiedPropertyName(PRP_WANTED_PRICE_PREFIX, S2.c_str(), Buf);
                        SetLandProperty(pLand, Buf.c_str(), eLong, (void*)n2, eBoth);

                    }
                    break;

                case eProduct: // Products: 40 grain [GRAI], 33 horses [HORS].
                               //           N1 S1    [S2]
                    p = SkipSpaces(GetToken(N1, p, ' ', TRIM_ALL));
                    n1= atol(N1.c_str());
                    if (0==n1)
                    {
                        if (0==stricmp(N1.c_str(), "none"))
                            n1 = -1;
                        else if  (0==stricmp(N1.c_str(), "unlimited"))
                            n1 = 10000000;
                    }
                    if (n1 >= 0)
                    {
                        pProd = new CProduct;
                        pProd->Amount = n1;
                        p = GetToken(pProd->LongName, p, '[', TRIM_ALL);
                        p = GetToken(pProd->ShortName, p, ']', TRIM_ALL);
                        if (pLand->Products.Search(pProd, idx))
                            pLand->Products.AtFree(idx);
                        else
                            if (!m_IsHistory && !ProductsWereEmpty)
                            {
                                // we have found a new product! Woo-hoo!
                                std::string          sCoord;
                                CBaseObject * pNewProd = new CBaseObject;

                                //LandIdToCoord(pLand->Id, x, y, z);
                                ComposeLandStrCoord(pLand, sCoord);
                                pNewProd->Name        << pProd->LongName << " discovered in (" << sCoord << ")";
                                pNewProd->Description << pLand->TerrainType << " (" << sCoord << ")"
                                                      << " is a new source of "  << pProd->LongName << EOL_SCR;

                                m_NewProducts.Insert(pNewProd);
                            }

                        pLand->Products.Insert(pProd);

                        // also set as a property to simplify searching
                        MakeQualifiedPropertyName(PRP_RESOURCE_PREFIX, pProd->ShortName.c_str(), Buf);
                        SetLandProperty(pLand, Buf.c_str(), eLong, (void*)n1, eBoth);
                    }

                    break;

                    /*
                case eWages: //   Wages: $17.2 (Max: $10937).
                    p = SkipSpaces(GetToken(N1, p, ' ', TRIM_ALL));
                    if (!p || !*p)
                    {
                        RerunSection = true;
                        break;
                    }

                    break;
                    */


                default:
                    break;

                }
                break; // case 0:
            }
        }
        if (RerunSection)
        {
            // a dot in the city name
            std::string S;
            src      = GetToken(S, src, '.');
            Section << '.' << S;
        }
        else
        {
            srcold   = src;
            src      = GetToken(Section, src, '.');
            SectType = eNone;
        }
    }

    if (dellen)
        DelSubStr(Description, delpos, dellen);

    return 0;
}

//----------------------------------------------------------------------

void CAtlaParser::ParseWages(CLand * pLand, const char * str1, const char * str2)
{
    //   Wages: $12.4 (Max: $350).    // str1 = "$12"  str2 = "4 (Max: $350)"
    //   Wages: $15 (Max: $10273).

    const char * src;
    std::string         N1, N2;
    std::string         sSrc;
    bool         Valid;

    str1 = SkipSpaces(str1);
    if (*str1 != '$')
        return;
    str1++;

    if (strchr(str1, '('))
    {
        src = SkipSpaces(GetInteger(N1, str1, Valid));
    }
    else
    {
        sSrc << str1 << '.' << str2;
        src = GetDouble(N1, sSrc.c_str(), Valid);
    }
    pLand->Wages = atof(N1.c_str());

    src = SkipSpaces(GetToken(N1, src, '$'));
    src = GetInteger(N1, src, Valid);

    pLand->MaxWages = atol(N1.c_str());
}

//----------------------------------------------------------------------

CPlane * CAtlaParser::MakePlane(const char * planename)
{
    static CBaseObject Dummy;  // we do not want it to be constructed/destructed all the way
                               // should be ok with multiple instances - single-threaded
    int      i;
    CPlane * pPlane;

    Dummy.Name = planename;
    if (m_PlanesNamed.Search(&Dummy, i))
        pPlane = (CPlane*)m_PlanesNamed.At(i);
    else
    {
        pPlane       = new CPlane;
        pPlane->Id   = m_Planes.Count();
        pPlane->Name = planename;
        if (!gpDataHelper->GetTropicZone(pPlane->Name.c_str(), pPlane->TropicZoneMin, pPlane->TropicZoneMax))
        {
            pPlane->TropicZoneMin  = TROPIC_ZONE_MAX;
            pPlane->TropicZoneMax  = -(TROPIC_ZONE_MAX);
        }

        m_PlanesNamed.Insert(pPlane);
        m_Planes.Insert(pPlane);
    }

    return pPlane;
}


//----------------------------------------------------------------------

int CAtlaParser::ParseTerrain(CLand * pMotherLand, int ExitDir, std::string & FirstLine, bool FullMode, CLand ** ppParsedLand)
{
// FirstLine looks somewhat like:
//    swamp (48,52[,somewhere]) in Aghleam, 118 peasants (tribesmen), $354.

    int                  err = ERR_OK;
    const char         * p;
    std::string                 Name;
    std::string S;
    long                 x, y;
    CLand              * pLand = nullptr;
    std::string CurLine;
    std::string PlaneName;
    std::string LandName;
    int                  i;
    int                  idxland;
    char                 ch;
    CPlane             * pPlane;
    CStruct            * pStruct;
    bool                 DoBreak;
    CBaseObject        * pGate;
    CBaseObject          Dummy;
    int                  no;
    int                  idx;
    std::string TempDescr; // Land description is collected in here. It can replace already existing description
    bool                 HaveEvents = false;
    std::string CompositeDescr;

    if (FirstLine.empty())
        goto Exit;

    p = SkipSpaces(GetToken(Name, FirstLine.c_str(), '(', TRIM_ALL));
    if (!p )   // it must be '('
        goto Exit;

    // ok, now goes (xxx,yyy[,somewhere]) bla-bla-bla
    p = GetToken(S, p, ',');
    if (!IsInteger(S.c_str()))
        goto Exit;
    x = atol(S.c_str());

    // yyy[,somewhere]) bla-bla-bla
    p = GetToken(S, p, ")", ch);
    if (!IsInteger(S.c_str()))
        goto Exit;
    y = atol(S.c_str());
    if (','==ch)
        // we have a plane name
        p = GetToken(PlaneName, p, ')');
    else
        PlaneName = DEFAULT_PLANE;

    if (!p)
        goto Exit;

    p = SkipSpaces(GetToken(S, SkipSpaces(p), ' ', TRIM_ALL));
    if (0!=stricmp(S.c_str(),"in"))
        goto Exit;
    GetToken(LandName, p, ",.", ch, TRIM_ALL);

    // Remove Arcadia III reference to edge location for sailing events
    if (strchr(LandName.c_str(), '('))
    {
        int x = strchr(LandName.c_str(), '(') - LandName.c_str();
        DelSubStr(LandName, x, LandName.size()-x);
        TrimRight(LandName, TRIM_ALL);
    }

    pPlane = MakePlane(PlaneName.c_str());

    Dummy.Id = LandCoordToId(x,y, pPlane->Id);
    if (pPlane->Lands.Search(&Dummy, idxland))
    {
        pLand = (CLand*)pPlane->Lands.At(idxland);
        if (0!=stricmp(pLand->TerrainType.c_str(), Name.c_str()))
        {
            Format(S, "*** Terrain changed for %s from '%s' to '%s' - clearing stored description and products! ***",
                     FirstLine.c_str(), pLand->TerrainType.c_str(), Name.c_str());
            GenericErr(1, S.c_str());
            pLand->TerrainType  = Name;
            pLand->Description.clear();
            pLand->Products.FreeAll();
        }
        if (0!=stricmp(pLand->Name.c_str(), LandName.c_str()) )
        {
            Format(S, "*** Province changed for %s from '%s' to '%s' - clearing stored description! ***",
                     FirstLine.c_str(), pLand->Name.c_str(), LandName.c_str());
            GenericErr(1, S.c_str());
            pLand->Name     = LandName;
            pLand->Description.clear();
        }
    }
    else
    {
        pLand               = new CLand;
        pLand->Id           = LandCoordToId(x,y, pPlane->Id);
        pLand->pPlane       = pPlane;
        pLand->Name         = LandName;
        pLand->TerrainType  = Name;
        pLand->Taxable      = 0;
        pPlane->Lands.Insert(pLand);
    }



    TempDescr = FirstLine;

    if (pMotherLand)  // this is an exit description
    {
        // Exit description takes more then one line!
        while (ReadNextLine(CurLine))
        {
            // the line must not start from -+* and must not contain : .
            const char * s = SkipSpaces(CurLine.c_str());
            if (!s || !*s)
                break;

            if (strchr(STRUCT_UNIT_START, *s) || strchr(s, ':'))
            {
                PutLineBack(CurLine);
                break;
            }

            TrimRight(CurLine, TRIM_ALL);
            TempDescr << CurLine << EOL_SCR;

            if (strchr(CurLine.c_str(), '.'))
                break;
        }
    }

    if (pMotherLand && (pPlane==pMotherLand->pPlane))
        CheckExit(pPlane, ExitDir, pMotherLand, pLand);


    // It is only scan for exits from the land
    if (!FullMode)
    {
        TrimRight(TempDescr, TRIM_ALL);
        TrimRight(pLand->Description, TRIM_ALL);

        //this creates problems with rivers - once you have seen a river,
        //it will be there for all following exits to the same hex

//        if (TempDescr.size() > pLand->Description.size())
//            pLand->Description = TempDescr;

        if (pLand->Description.empty())
            pLand->Description = TempDescr;

        AnalyzeTerrain(pMotherLand, pLand, pMotherLand!=nullptr, ExitDir, TempDescr);
        if (pMotherLand)
            pMotherLand->Exits << TempDescr << EOL_SCR;
        goto Exit;
    }

    // Structures can be destroyed, so remove those coming from history
    for (i=pLand->Structs.Count()-1; i>=0; i--)
    {
        pStruct = (CStruct*)pLand->Structs.At(i);
        if (0==(pStruct->Attr & SA_HIDDEN) &&   // keep the gates!
            0==(pStruct->Attr & SA_SHAFT ) )    // keep the shafts!
            pLand->Structs.AtFree(i);
    }

    // now  goes extended land description terminated by exits list
    // And check for other headers just in case!
    DoBreak = false;
    while (ReadNextLine(CurLine))
    {
        TrimRight(CurLine, TRIM_ALL);
        p = SkipSpaces(CurLine.c_str());
        if (0==stricmp("Events:", p))
        {
            HaveEvents = true;
            break;
        }
        if (0==stricmp("Exits:", p))
            break;
        for (i=0; i<(int)sizeof(ExitEndHeader)/(int)sizeof(const char *); i++)
            if (0==strnicmp(p, ExitEndHeader[i], ExitEndHeaderLen[i] ))
            {
                CurLine << EOL_FILE;
                PutLineBack(CurLine);
                DoBreak = true;
                break;
            }
        if (DoBreak)
            break;

        // remove the atlaclient's turn mark
        no=0;
        while (p && *p && '-'==*p)
        {
            no++;
            p++;
        }
        if (no>20 && ';'==*p)
        {
            no = strlen(p++);
            DelSubStr(CurLine, CurLine.size()-no, no);
            pLand->AtlaclientsLastTurnNo = atol(p);
        }

        TempDescr << CurLine << EOL_SCR;
    }

    // When should we replace old description with the new one?
    // My guess is - everytime for the full parsing!
    //pLand->Description = TempDescr;
    
    // Unfortunately, Arno in his latest game shows restricted description for hexes
    // through which your scout pass if there are no stationary units in the hex.
    
    ComposeHexDescriptionForArnoGame(pLand->Description.c_str(), TempDescr.c_str(), CompositeDescr);
    pLand->Description = CompositeDescr;
    TrimRight(pLand->Description, TRIM_ALL);
    AnalyzeTerrain(nullptr, pLand, false, ExitDir, pLand->Description);

    //Read Events and skip till Exits:
    DoBreak = false;
    if (HaveEvents)
    {
        while (ReadNextLine(CurLine))
        {
            TrimRight(CurLine, TRIM_ALL);
            p = SkipSpaces(CurLine.c_str());
            if (0==stricmp("Exits:", p))
                break;
            for (i=0; i<(int)sizeof(ExitEndHeader)/(int)sizeof(const char *); i++)
                if (0==strnicmp(p, ExitEndHeader[i], ExitEndHeaderLen[i] ))
            {
                CurLine << EOL_FILE;
                PutLineBack(CurLine);
                DoBreak = true;
                break;
            }
            if (DoBreak)
                break;
            pLand->Events << CurLine << EOL_SCR;
        }
        TrimRight(pLand->Events, TRIM_ALL);
    }


    if (!m_IsHistory && m_CurYearMon>0)
        pLand->Flags |= LAND_IS_CURRENT;

    m_pCurLand   = pLand;
    m_pCurStruct = nullptr;

    // Clear all the Edge structures which were loaded from history
    pLand->EdgeStructs.FreeAll();
    pLand->Exits.clear();

    // now is a list of exits and gates terminated by unit or structure
    pLand->ExitBits = 0;
    while (ReadNextLine(CurLine))
    {
        if (0==SafeCmp(EOL_SCR, CurLine.c_str()))
            continue;
        DoBreak = true;
        p       = GetToken(S, CurLine.c_str(), ":(", ch);
        switch (ch)
        {
        case ':':  // is it an exit?

            if (0==stricmp(S.c_str(), FLAG_HDR))

            {
                pLand->FlagText[0] = SkipSpaces(p);
                TrimRight(pLand->FlagText[0], TRIM_ALL);
            }
            for (i=0; i<(int)sizeof(Directions)/(int)sizeof(const char*); i++)
                if (0==stricmp(S.c_str(), Directions[i]))
                {
                    // yes!
                    pLand->Exits << "  " << S << ": ";
                    pPlane->ExitsCount++;
                    S = p;
                    TrimLeft(S);
                    ParseTerrain(pLand, i, S, false, nullptr);
                    pLand->ExitBits |= ExitFlags[i];
                    DoBreak = false;
                    break;
                }
            break;
        case '(': // is it a gate?
            // There is a Gate here (Gate 18 of 35).
            if (0==stricmp(S.c_str(), "There is a Gate here"))
            {
                std::string sCoord;

                p  = SkipSpaces(GetToken(S, p, ' '));
                p  = GetToken(S, p, ' ');  // S = 18
                no = atol(S.c_str());
                pStruct     = new CStruct;
                pStruct->Id = -no;  // negative, so it does not clash with structures!
                pStruct->Description = CurLine.c_str();
                pStruct->Kind        = STRUCT_GATE;
                pStruct->Attr        = gpDataHelper->GetStructAttr(pStruct->Kind.c_str(), pStruct->MaxLoad, pStruct->MinSailingPower);
                pLand->AddNewStruct(pStruct);

                p = SkipSpaces(p);
                p = SkipSpaces(GetToken(S, p, ' '));  // S = of
                p = GetToken(S, p, ' ');  // S = 35
                m_GatesCount = atol(S.c_str());
                DoBreak = false;


                pGate = new CBaseObject;
                pGate->Id = no;
                if (m_Gates.Search(pGate, idx))
                {
                    delete pGate;
                    pGate = (CBaseObject*)m_Gates.At(idx);
                }
                else
                    idx = -1;

                ComposeLandStrCoord(pLand, sCoord);
                Format(pGate->Description, "Gate % 4d. ", no);
                pGate->Description << pLand->TerrainType << " (" << sCoord << ")";
                pGate->Name        = pGate->Description;

                if (idx<0)
                    m_Gates.Insert(pGate);
            }
            break;
        }

        if (DoBreak)
        {
            PutLineBack(CurLine);
            goto Exit;

        }
    }




    // list of units....
    // will be obtained separately!

Exit:
    if (ppParsedLand)
        *ppParsedLand = pLand;

    return err;
}

//----------------------------------------------------------------------

const char * CountTokensForArno(const char * src, int & count)
{
    const char * p;
    char         ch;
    std::string Token;
    
    count = 0;
    p = GetToken(Token, src, ')');
    while (p && *p)
    {
        p = GetToken(Token, p, ",.", ch, TRIM_NONE);
        count++;
        if ('.'==ch)
            break;
    }
    return p;
}

/*
desert (67,21) in Groddland, 182 peasants (nomads), $182.
------------------------------------------------------------
  The weather was clear last month; it will be clear next month.
  Wages: $11 (Max: $667).
  Wanted: none.
  For Sale: 36 nomads [NOMA] at $44, 7 leaders [LEAD] at $88.
  Entertainment available: $9.
  Products: 16 livestock [LIVE], 12 iron [IRON], 12 stone [STON].

desert (67,21) in Groddland.
plain (55,3) in Lothmarlun, contains Rudoeton [village].
------------------------------------------------------------
  The weather was clear last month; it will be winter next month.
  Wages: $0.
  Wanted: none.
  For Sale: none.
  Entertainment available: $0.
  Products: none.
*/

void CAtlaParser::ComposeHexDescriptionForArnoGame(const char * olddescr, const char * newdescr, std::string & CompositeDescr)
{
    const char * pnew, * pold;
    std::string NewWeather; 
    int          oldcount, newcount;
    std::string Token;
    
    if (!olddescr || !*olddescr)
    {
        CompositeDescr = newdescr;
        return;
    }
    if (!newdescr || !*newdescr)
    {
        CompositeDescr = olddescr;
        return;
    }
    
    pnew = CountTokensForArno(newdescr, newcount);
    pold = CountTokensForArno(olddescr, oldcount);
    
    // Is new descr good?
    // We do not have to do full parsing here. Good descr has more pieces than bad
    if (newcount>oldcount)
    {
        CompositeDescr = newdescr;
        return;
    }
    
    // Maybe they are basically the same?
    if (newcount==oldcount)
    {
        if (strlen(olddescr) < strlen(newdescr))
            CompositeDescr = newdescr;
        return;
    }
    
    
    
    // now our new descr is worse, but it contains good weather line, we need to extract it and merge with old descr
    while (pnew && *pnew && *pnew!='-')
        pnew++;
    pnew = GetToken(Token, pnew, '\n');
    pnew = GetToken(NewWeather, pnew, '.', TRIM_NONE);
    
    CompositeDescr.clear();
    pnew = GetToken(Token, olddescr, '.');
    CompositeDescr << Token << "." << EOL_SCR;
    
    while (pnew && *pnew && *pnew!='-')
        pnew++;
    pnew = GetToken(Token, pnew, '\n');
    CompositeDescr << Token << EOL_SCR;
    
    pnew = GetToken(Token, pnew, '.');    // old weather
    CompositeDescr << NewWeather << "." << pnew;
    
}

//----------------------------------------------------------------------

CUnit * CAtlaParser::MakeUnit(long Id)
{
    CBaseObject  Dummy;
    int          idx;
    CUnit      * pUnit;

    Dummy.Id = Id;
    if (m_Units.Search(&Dummy, idx))
    {
        pUnit    = (CUnit*)m_Units.At(idx);
    }
    else
    {
        pUnit = new CUnit;
        pUnit->Id = Id;
        m_Units.Insert(pUnit);
    }

    return pUnit;
}

//----------------------------------------------------------------------

/*
void CAtlaParser::MakeUnitAndGetOld(long Id, CUnit *& pUnit, CUnit *& pUnitOld, int & idxold)
{
    CBaseObject  Dummy;

    Dummy.Id = Id;
    if (m_Units.Search(&Dummy, idxold))
        pUnitOld = (CUnit*)m_Units.At(idxold);
    else
        pUnitOld = nullptr;

    pUnit = new CUnit;
    pUnit->Id = Id;
}
*/

long CAtlaParser::SkillDaysToLevel(long days)
{
    long level = 0;

    while (days>0)
    {
        days -= (level+1)*30;
        if (days>=0)
            level++;
    }
    return level;
}

//----------------------------------------------------------------------

/*
  Taxmen (767), on guard, Yellow Pants (34), revealing faction,
  taxing, 84 high elves [HELF], vodka [VODK], spear [SPEA], 2578
  silver [SILV]. Skills: combat [COMB] 2 (90).
*/

// Only normal brackets are banned, everything else can be in names and descriptions, including .,[]


int CAtlaParser::ParseUnit(std::string & FirstLine, bool Join)
{
#define DOT          '.'
#define COMMA        ','
#define SEMICOLON    ';'
#define STRUCT_LIMIT ",.;"
#define SECT_ITEMS   "Items"
#define SECT_SKILLS  "Skills"
#define SECT_COMBAT  "Combatspell"

    CFaction    * pFaction = nullptr;
    CUnit       * pUnit    = nullptr;
    std::string CurLine;
    std::string UnitText;
    std::string          UnitPrefix;
    std::string Line;
    std::string          FactName;
    std::string Section;
    std::string Buf;
    std::string S1;
    std::string S2;
    std::string N1;
    std::string N2;
    std::string N3;
//    EValueType    type;
//    const void  * stance;
    long          n1;
    const char  * src = nullptr;
    const char  * p;
    char          Delimiter;
    char          LastDelimiter = DOT;
    char          ch;
    int           err = ERR_OK;
    int           attitude;
    bool          SkillsFound = false;
    bool          Valid;

    TrimRight(FirstLine, TRIM_ALL);
    p = FirstLine.c_str();

    // Get the prefix - leading spaces and the first word
    while (p && *p && *p<=' ')
        UnitPrefix << *p++;
    while (p && *p && *p>' ')
        UnitPrefix << *p++;
    while (p && *p && *p<=' ')
        UnitPrefix << *p++;

    UnitText = p;
    UnitText << EOL_SCR;

    while (ReadNextLine(CurLine))
    {
        p = SkipSpaces(CurLine.c_str());

        // maybe terminated by an empty string
        if (!p || !*p)
            break;

        // or a next unit may just follow...
        if ( (0==strnicmp(p, HDR_UNIT_ALIEN        , sizeof(HDR_UNIT_ALIEN)-1 )) ||
              (0==strnicmp(p, HDR_UNIT_OWN          , sizeof(HDR_UNIT_OWN  )-1 )) ||
              (0==strnicmp(p, HDR_STRUCTURE         , sizeof(HDR_STRUCTURE )-1 ))
           )
        {
            PutLineBack(CurLine);
            break;
        }

        TrimRight(CurLine, TRIM_ALL);
        AddStr(UnitText, CurLine.c_str(), CurLine.size());

        AddStr(UnitText, EOL_SCR);
    }

    // ===== Read Unit Name, which always goes first!

    p  = GetToken(S1, UnitText.c_str(), '(');
    p  = GetToken(N1, p, ')');
    n1 = atol(N1.c_str());
    if (n1<=0)
        return ERR_INV_UNIT;

    pUnit = MakeUnit(n1);
    if (m_pCurLand)
        m_pCurLand->AddUnit(pUnit);
    if (0 == strnicmp(SkipSpaces(UnitPrefix.c_str()), HDR_UNIT_OWN, sizeof(HDR_UNIT_OWN)-1) ||
        UnitPrefix.size() + UnitText.size() > pUnit->Description.size())
    {
        pUnit->Description = UnitPrefix;
        pUnit->Description << UnitText;
        pUnit->Name = S1;
    }
    if (m_pCurStruct)
    {
        SetUnitProperty(pUnit, PRP_STRUCT_ID,   eLong,    (void*)m_pCurStruct->Id,      eBoth);
        SetUnitProperty(pUnit, PRP_STRUCT_NAME, eCharPtr, m_pCurStruct->Name.c_str(), eBoth);
        if (0==m_pCurStruct->OwnerUnitId)
        {
            m_pCurStruct->OwnerUnitId = pUnit->Id;
            SetUnitProperty(pUnit, PRP_STRUCT_OWNER, eCharPtr, YES, eBoth);
        }
    }

    // ===== Read Faction Name, which may follow!

    src = p;
    p   = strchr(src, '(');
    if (p)
    {
        while (p > src && *(p-1) != ',')
            p--; // position to the start of faction name

        p  = GetToken(S1, p, '(');
        p  = GetToken(N1, p, ')');
        n1 = atol(N1.c_str());
//        if (n1<=0)
//            return ERR_INV_UNIT;
        if (n1>0)
		{
			pFaction = GetFaction(n1);
			if (!pFaction)
			{
				pFaction       = new CFaction;
				pFaction->Name = S1;
				pFaction->Id   = n1;
				m_Factions.Insert(pFaction);
			}
			pUnit->FactionId= pFaction->Id;
			pUnit->pFaction = pFaction;

			pUnit->IsOurs = pUnit->IsOurs ||
					(m_CrntFactionId > 0 && pUnit->FactionId == m_CrntFactionId);
		}
    }

    // ===== Check faction attitude and set stances prop
    attitude = ATT_UNDECLARED;
    if(pUnit->IsOurs && (!Join)) // set own units to ATT_FRIEND2
    {
        attitude = ATT_FRIEND2;
    }
    else if(pUnit->IsOurs) // set unrevealing units of reporting faction
    {
        attitude = gpDataHelper->GetAttitudeForFaction(m_CrntFactionId);
    }
    else if(pUnit->IsOurs || (pUnit->FactionId!=0)) // set PRP_FRIEND_OR_FOE according to FactionId
    {
        attitude = gpDataHelper->GetAttitudeForFaction(pUnit->FactionId);
    }
    if((attitude >= 0) && (attitude < ATT_UNDECLARED))
    {
        SetUnitProperty(pUnit,PRP_FRIEND_OR_FOE,eLong,reinterpret_cast<void*>(static_cast<intptr_t>(attitude)),eNormal);
    }

    // ===== Now analize the rest of unit text

    if (','==*src)
        src++;
    while (src && *src)
    {
        src  = SkipSpaces(GetToken(Line, src, STRUCT_LIMIT, Delimiter, TRIM_ALL));

        if (DOT == LastDelimiter)
        {
            // New section. Find out the section name,
            p = SkipSpaces(GetToken(Section, Line.c_str(), ':'));
            if (p)
            {
                // remove section name from the structure
                DelSubStr(Line, 0, p-Line.c_str());
            }
            else
            {
                // section has no name.
                if (!SkillsFound && 0!=stricmp(SECT_SKILLS, Section.c_str()))
                    Section = SECT_ITEMS;
            }
        }

/*
        Taxmen (767), on guard, Yellow Pants (34), revealing faction,
        taxing, 84 high elves [HELF], vodka [VODK], spear [SPEA], 2578
        silver [SILV]. Skills: combat [COMB] 2 (90).
*/
        if      (0==stricmp(SECT_ITEMS, Section.c_str()))
        {
            const void * data = nullptr;
            if (pUnit)
            {
                Normalize(Line);
                auto flagIt = m_UnitFlagsHash.find(Line.c_str());
                if (flagIt != m_UnitFlagsHash.end())
                {
                    pUnit->Flags    |= (unsigned long)flagIt->second;
                    pUnit->FlagsOrg |= (unsigned long)flagIt->second;

                }
            }

            // recognize patterns:
            // S1 (N1) - faction name
            // N1 S1 [S2]
            // S1 [S2]

            // '[' can be present in the unit or faction name, so give '(' precedence.
            p = GetToken(Buf, Line.c_str(), "(", ch);
            if (!p)
                p = GetToken(Buf, Line.c_str(), "[", ch);
            switch (ch)
            {
                case '(':     // it is faction name again
                    p  = GetToken(N1, p, ')');
                    break;

            case '[':
                p = GetInteger(N1, Line.c_str(), Valid);
                if (N1.empty())
                    n1 = 1;
                else
                    n1 = atol(N1.c_str());

                    p = GetToken(S1, p, '[', TRIM_ALL);
                    p = GetToken(S2, p, ']', TRIM_ALL);

                    if (!p || S2.empty() || !pUnit)
                    {
                    // there is no shortname
                        Format(Buf, "Unit description - flag/item error at line %d", m_nCurLine);
                        LOG_ERR(ERR_DESIGN, Buf.c_str());
                    }
                    else
                    {
                        SetUnitProperty(pUnit, S2.c_str(), eLong, (void*)n1, eBoth);
                    // is this a man property?
                        if (gpDataHelper->IsMan(S2.c_str()))
                        {
                        //So, is it a leader?
                            if (FindSubStr(S1, SZ_LEADER) >=0 )
                                SetUnitProperty(pUnit, PRP_LEADER, eCharPtr, SZ_LEADER, eBoth);
                            if (FindSubStr(S1, SZ_HERO) >=0 )
                                SetUnitProperty(pUnit, PRP_LEADER, eCharPtr, SZ_HERO, eBoth);
                        }

                    }

                    break;
            }
        }

        else if (0==stricmp(SECT_SKILLS, Section.c_str()))
        {
            SkillsFound = true;

            // recognize patterns:
            // S1 [S2] N1 (N2)
            // S1 [S2] N1 (N2/N3)
            p = GetToken(S1, Line.c_str(), '[', TRIM_ALL);
            p = GetToken(S2, p, ']', TRIM_ALL);
            p = GetToken(N1, p, '(', TRIM_ALL);
            p = GetToken(N2, p, ")/", ch, TRIM_ALL);
            if ('/'==ch)
            {
                p = GetToken(N3, p, ')', TRIM_ALL);
                m_ArcadiaSkills = true;
            }


            if (0!=stricmp("none", S1.c_str()))
            {
                if (!p || !pUnit)
            {
                Format(Buf, "Unit description - skills error at line %d", m_nCurLine);
                LOG_ERR(ERR_DESIGN, Buf.c_str());
            }
            else
            {
                    // classic skills
                Buf = S2;
                Buf << PRP_SKILL_POSTFIX; // That is a skill!
                SetUnitProperty(pUnit, Buf.c_str(), eLong, (void*)atol(N1.c_str()), eBoth);

                Buf = S2;
                Buf << PRP_SKILL_DAYS_POSTFIX;
                SetUnitProperty(pUnit, Buf.c_str(), eLong, (void*)atol(N2.c_str()), eBoth);

                if (m_ArcadiaSkills)
                {
                        // Arcadia III skills
                    long             n;
                    unsigned long    i;

                    n = atol(N2.c_str());
                    i = SkillDaysToLevel(n);

                    Buf = S2;
                    Buf << PRP_SKILL_STUDY_POSTFIX;
                    SetUnitProperty(pUnit, Buf.c_str(), eLong, (void*)i, eBoth);

                    n = atol(N3.c_str());
                    i = SkillDaysToLevel(n);
                    Buf = S2;
                    Buf << PRP_SKILL_EXPERIENCE_POSTFIX;
                    SetUnitProperty(pUnit, Buf.c_str(), eLong, (void*)i, eBoth);

                    Buf = S2;
                    Buf << PRP_SKILL_DAYS_EXPERIENCE_POSTFIX;
                    SetUnitProperty(pUnit, Buf.c_str(), eLong, (void*)n, eBoth);
                }
            }
            }
        }


        else if (0 == SafeCmpNoSpaces(SECT_COMBAT, Section.c_str())) // looks like combat, but need to ignore spaces....
        {
            // recognize pattern:
            // S1 [S2]
            p = GetToken(S1, Line.c_str(), '[', TRIM_ALL);
            if (p)
                p = GetToken(S2, p, ']', TRIM_ALL);
            else
                S2 = Line;
            SetUnitProperty(pUnit, PRP_COMBAT, eCharPtr, S2.c_str(), eBoth);
        }


        if (SEMICOLON == Delimiter)
        {
            // That is a description, it comes last
            SetUnitProperty(pUnit, PRP_DESCRIPTION, eCharPtr, src, eBoth);
            break;
        }
        LastDelimiter = Delimiter;
    }

    // now check if this guy can see any advanced resources
    if (pUnit->IsOurs)
        LookupAdvancedResourceVisibility(pUnit, m_pCurLand);

    return err;
}



//----------------------------------------------------------------------


int CAtlaParser::ParseStructure(std::string & FirstLine)
{
    //+ Ruin [1] : Ruin, closed to player units.
    //+ Ship [100] : Longboat, needs 10.
    //+ Forager [101] : Longboat.
    //+ Shaft [1] : Shaft, contains an inner location.

    // Description is terminated by a unit or an empty line

    std::string         S;
    std::string         Name;
    const char * p;
    long         id;
    CStruct    * pStruct;
    char         ch;
    std::string CurLine;
    std::string TmpDescr;
    std::string         Kind;
    int          Location = NO_LOCATION;
    int          i;

    if (!m_pCurLand)
        goto Exit;

    p  = FirstLine.c_str();
    p  = GetToken(Name, p, '[');
    p  = GetToken(S, p, ']');
    id = atol(S.c_str());
    if (0==id)
        goto Exit;

    TmpDescr = FirstLine;
    TrimRight(TmpDescr, TRIM_ALL);
    while (ReadNextLine(CurLine))
    {
        const char * s = SkipSpaces(CurLine.c_str());
        if (!s || !*s)
            break;

        if (strchr(STRUCT_UNIT_START, *s))
        {
            PutLineBack(CurLine);
            break;
        }

        TrimRight(CurLine, TRIM_ALL);
        TmpDescr << EOL_SCR << CurLine;
    }

    p  = GetToken(S, TmpDescr.c_str(), ':');
    p  = GetToken(Kind, p, ",.;", ch);

    Name << " [" << id << "]";

    // check for Arcadia III ships at the hex edges
    // + Ship [104] : Galley (Northern hexside).
    p = strchr(Kind.c_str(), '(');
    if (p && *p)
    {
        GetToken(S, p+1, ')');
        Normalize(S);
        for (i=0; i<(int)sizeof(LocationsShipsArcadia)/(int)sizeof(const char*); i++)
            if (0==stricmp(S.c_str(), LocationsShipsArcadia[i]))
            {
                Location = i;
                break;
            }
        i = p - Kind.c_str();
        DelSubStr(Kind, i, Kind.size()-i);
        TrimRight(Kind, TRIM_ALL);
    }


    pStruct              = new CStruct;
    pStruct->Id          = id;
    pStruct->Name        = &Name.c_str()[sizeof(HDR_STRUCTURE)-1];
    pStruct->Description = TmpDescr;
    pStruct->Kind        = Kind;
    pStruct->Attr        = gpDataHelper->GetStructAttr(pStruct->Kind.c_str(), pStruct->MaxLoad, pStruct->MinSailingPower);
    pStruct->Location    = Location;
    if (pStruct->Attr & (SA_ROAD_N | SA_ROAD_NE | SA_ROAD_SE | SA_ROAD_S | SA_ROAD_SW | SA_ROAD_NW ))
        if (FindSubStr(pStruct->Description, "needs") > 0 || FindSubStr(pStruct->Description, "decay") > 0 )
            pStruct->Attr |= SA_ROAD_BAD;

    m_pCurStruct         = m_pCurLand->AddNewStruct(pStruct);


Exit:
    return ERR_OK;
}

//----------------------------------------------------------------------

void CAtlaParser::SetShaftLinks()
{
    CLand      * pLand;
    CLand      * pLandDest;
    CStruct    * pStruct;
    int          i,j,n;
    EValueType   type;
    const void * value;
    std::string         S, T;
    const char * p;

    for (i=0; i<m_LandsToBeLinked.Count(); i++)
    {
        pLand = (CLand*)m_LandsToBeLinked.At(i);

        if (!pLand->GetProperty(PRP_LAND_LINK, type, value, eOriginal) || eLong!=type)
            continue;

        for (j=0; j<pLand->Structs.Count(); j++)
        {
            pStruct = (CStruct*)pLand->Structs.At(j);
            if (pStruct->Attr & SA_SHAFT  )
            {
                pLandDest = GetLand((long)value);
                if (pLandDest && FindSubStr(pStruct->Description, "links")<0)
                {
                    ComposeLandStrCoord(pLandDest, S);
                    if ('.' ==  pStruct->Description.c_str()[ pStruct->Description.size()-1])
                         DelCh(pStruct->Description,  pStruct->Description.size()-1);

                    p = strrchr( pStruct->Description.c_str(), '\n');
                    n = p ? ( pStruct->Description.c_str()-p) : 0;

                    T.clear();
                    T << "; links to (" << S << ").";
                    if (n+T.size() > 64)
                         pStruct->Description << EOL_SCR << "  ";
                     pStruct->Description << T;
                }
                break;
            }
        }
    }

    m_LandsToBeLinked.DeleteAll();
}

//----------------------------------------------------------------------

void CAtlaParser::ApplySailingEvents()
{
    int                ne, np, nl, nu, idx;
    CBaseObject      * pSailEvent;
    CPlane           * pPlane;
    CLand            * pLand;
    CUnit            * pUnit;
    EValueType         type, type2;
    const void       * value, * value2;

    if (0 == m_TempSailingEvents.Count())
        return;

    for (np=0; np<m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_Planes.At(np);

        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand = (CLand*)pPlane->Lands.At(nl);

            for (ne=0; ne<m_TempSailingEvents.Count(); ne++)
            {
                pSailEvent = (CBaseObject*)m_TempSailingEvents.At(ne);

                if (pLand->Structs.Search(pSailEvent, idx))
                {
                    // we do not care about the ship, we care about the captain!
                    for (nu=0; nu<pLand->Units.Count(); nu++)
                    {
                        pUnit = (CUnit*)pLand->Units.At(nu);
                        if (pUnit->GetProperty(PRP_STRUCT_ID   , type , value , eOriginal) && eLong==type &&
                            pUnit->GetProperty(PRP_STRUCT_OWNER, type2, value2, eOriginal) && eCharPtr==type2)
                            if (pSailEvent->Id == (long)value && value2 && *((char*)value2)  )
                            {
                                pUnit->Events << pSailEvent->Description;
                            }
                    }
                }
            }
        }
    }
    m_TempSailingEvents.FreeAll();
}

//----------------------------------------------------------------------

/* regular unit
* Beer Watcher (1099), Two Beers (23), behind, won't cross water,
  leader [LEAD], mithril sword [MSWO], cloth armor [CLAR], 4 rootstone
  [ROOT]. Weight: 212. Capacity: 0/0/15/0. Skills: observation [OBSE]
  5 (450), combat [COMB] 4 (442), stealth [STEA] 5 (450).

*/

/* Battle
Unit (3732) attacks City Guard (4429) in mountain (32,20) in Cromarty!

Attackers:
Unit (1769), behind, leader [LEAD], winged horse [WING], magic
  crossbow [MXBO], combat 1, crossbow 5.
Unit (2530), Two Beers (23), behind, 10 nomads [NOMA], winged horse
  [WING], horse [HORS].
Balrog (4423), Creatures, balrog [BALR] (Combat 6/6, Attacks 200,
  Hits 200, Tactics 5).

Defenders:
City Guard (4429), The Guardsmen (1), 4 leaders [LEAD], 4 swords
  [SWOR], combat 1.

Unit (3732) gets a free round of attacks.
City Guard (4429) loses 1.

Round 1:
Unit (3732) loses 0.
City Guard (4429) loses 0.

*/

const char * CAtlaParser::AnalyzeBattle_ParseUnit(const char * src, CUnit *& pUnit, bool & InFrontLine)
{
    std::string         Item, S, N1, S1, S2;
    char         ch, ch1;
    const char * p;
    long         n;

    InFrontLine = true;
    pUnit       = nullptr;

    while (src && *src)
    {
        src = GetToken(Item, src, ",.(", ch, TRIM_ALL );
        if (Item.empty())
            return src;
        if (!pUnit)
            pUnit = new CUnit;
        if ('('==ch)
        {
            // It is unit id, faction id or balrog. We do not give a shit about what exactly!
            src = GetToken(S, src, ')', TRIM_ALL);
            src = GetToken(S, src, ",.", ch, TRIM_ALL );
        }

        // now it can be one of the following which we are interested in:
        //    behind 
        //    leader [LEAD]
        //    4 leaders [LEAD]
        //    crossbow 5
        if (0==stricmp(Item.c_str(), "behind"))
            InFrontLine = false;
        else
        {
            p = Item.c_str();
            p = GetToken(N1, p, " \n", ch1, TRIM_ALL);
            p = GetToken(S1, p, '[', TRIM_ALL);
            p = GetToken(S2, p, ']', TRIM_ALL);

            if (!S2.empty())  // it is item
            {
                n = 1;
                if (IsInteger(N1))
                    n = atol(N1.c_str());
                SetUnitProperty(pUnit, S2.c_str(), eLong, (void*)n, eBoth);
            }
            else     // it is skill
            {
                S1.clear();
                N1.clear();
                p = Item.c_str();
                while (p && *p)
                {
                    p = GetToken(N1, p, " \n", ch1, TRIM_ALL);
                    if (p && *p)
                    {
                        if (!S1.empty())
                            S1 << ' ';
                        S1 << N1;
                    }
                }

                if (IsInteger(N1) && !S1.empty())
                {
                    S2 = gpDataHelper->ResolveAlias(S1.c_str()); 
                    S2 << PRP_SKILL_POSTFIX; // That is a skill!
                    n = atol(N1.c_str());
                    SetUnitProperty(pUnit, S2.c_str(), eLong, (void*)n, eBoth);
                }
            }
        }

        if ('.'==ch)
            break;
    }

    return src;
}

//----------------------------------------------------------------------

void CAtlaParser::AnalyzeBattle_SummarizeUnits(CBaseColl & Units, std::string & Details)
{
    std::string            propname;
    int             i, propidx;
    CUnit         * pUnit;
    CBaseObject     Faction;
    EValueType      type;
    const void    * value;
    const void    * valuetot;
    int             skilllen, maxproplen=0;

    skilllen    = strlen(PRP_SKILL_POSTFIX);
    for (i=0; i<Units.Count(); i++)
    {
        pUnit = (CUnit*)Units.At(i);

        for (const auto& propnameStr : m_UnitPropertyNames)
        {
            propname = propnameStr.c_str();
            if (!pUnit->GetProperty(propname.c_str(), type, value, eOriginal) || eLong!=type )
                continue;
            if (FindSubStrR(propname, PRP_SKILL_POSTFIX) == propname.size()-skilllen)
            {
                // it is a skill
                propname << (long)value;
                if (!pUnit->GetProperty(PRP_MEN, type, value, eOriginal) || eLong!=type )
                    continue;
            }
            if (!Faction.GetProperty(propname.c_str(), type, valuetot, eNormal))
                valuetot = (void*)0;

            if (-1==(long)valuetot || 0x7fffffff - (long)value < (long)valuetot )
                valuetot = (void*)(long)-1; // overflow protection
            else
                valuetot = (void*)((long)valuetot + (long)value);
            Faction.SetProperty(propname.c_str(), eLong, valuetot, eNormal);
            if (propname.size() > maxproplen)
                maxproplen = propname.size();
        }
    }

    propidx  = 0;
    propname = Faction.GetPropertyName(propidx);
    while (!propname.empty())
    {
        if (Faction.GetProperty(propname.c_str(), type, value, eNormal) &&
            (eLong==type) )
        {
            while (propname.size() < maxproplen)
                AddCh(propname, ' ');
            Details << "   " << propname << "  " << (long)value << EOL_SCR;
        }

        propname = Faction.GetPropertyName(++propidx);
    }


}


//----------------------------------------------------------------------

void CAtlaParser::AnalyzeBattle_OneSide(const char * src, std::string & Details)
{
    CBaseColl   Frontline(64), Backline;
    CUnit     * pUnit;
    bool        InFrontLine;
    std::string S1, S2;

    while (src && *src)
    {
        src = AnalyzeBattle_ParseUnit(src, pUnit, InFrontLine);
        if (!pUnit)
            continue;
        if (InFrontLine)
            Frontline.Insert(pUnit);
        else
            Backline.Insert(pUnit);
    }

    AnalyzeBattle_SummarizeUnits(Frontline, S1);
    AnalyzeBattle_SummarizeUnits(Backline, S2);

    Details << "Front line" << EOL_SCR << S1;
    Details << "Back line" << EOL_SCR << S2 ;

    Frontline.FreeAll();
    Backline.FreeAll();
}

//----------------------------------------------------------------------

void CAtlaParser::AnalyzeBattle(const char * src, std::string & Details)
{
    std::string Line, Attackers, Defenders;
    std::string         S1, N1, S2;
    const char * p;

    Details.clear();

    // skip start
    while (src && *src)
    {
        src = GetToken(Line, src, '\n', TRIM_ALL);
        if (0==strnicmp(Line.c_str(), HDR_ATTACKERS, sizeof(HDR_ATTACKERS)-1))
            break;
    }
    // read attackers
    while (src && *src)
    {
        src = GetToken(Line, src, '\n', TRIM_ALL);
        if (0==strnicmp(Line.c_str(), HDR_DEFENDERS, sizeof(HDR_ATTACKERS)-1))
            break;
        Attackers << Line << EOL_SCR;
    }

    // read defenders
    while (src && *src)
    {
        src = GetToken(Line, src, '\n', TRIM_ALL);

        // Unit (3732) gets a free round of attacks.
        p = Line.c_str();
        p = GetToken(S1, p, '(', TRIM_ALL);
        p = GetToken(N1, p, ')', TRIM_ALL);
        p = GetToken(S2, p, '.', TRIM_ALL);
        if (!S1.empty() && IsInteger(N1) && 0==stricmp(S2.c_str(), "gets a free round of attacks"))
            break;

        //Round 1:
        p = Line.c_str();
        p = GetToken(S1, p, ' ', TRIM_ALL);
        p = GetToken(N1, p, ':', TRIM_ALL);
        if (0==stricmp(S1.c_str(), "Round") && IsInteger(N1))
            break;

        Defenders << Line << EOL_SCR;
    }

    Details << EOL_SCR << EOL_SCR << "----------------------------------------------" 
            << EOL_SCR << "Statistics for the battle:" << EOL_SCR << EOL_SCR;
    Details << HDR_ATTACKERS << EOL_SCR;
    AnalyzeBattle_OneSide(Attackers.c_str(), Details);

    Details << EOL_SCR;
    Details << HDR_DEFENDERS << EOL_SCR;
    AnalyzeBattle_OneSide(Defenders.c_str(), Details);
}

//----------------------------------------------------------------------

void CAtlaParser::StoreBattle(std::string & Source)
{
    std::string          S1, S2, S3;
    std::string          N1, N2, N3;
    CBattle     * pBattle;
    const char  * p;
    int           i;
    bool          Ok = false;
    bool          RegularBattle = false;

    TrimRight(Source, TRIM_ALL);
    if (Source.empty())
        return;
    //Normalize(Source);

    // Captain (15166) is assassinated in forest (83,129) in Imyld!
    p = Source.c_str();
    p = SkipSpaces(GetToken(N1, p, '(', TRIM_ALL));
    p = SkipSpaces(GetToken(N1, p, ')', TRIM_ALL));
    p = SkipSpaces(GetToken(S1, p, ' ', TRIM_ALL));
    p = SkipSpaces(GetToken(S2, p, ' ', TRIM_ALL));

    if ( IsInteger(N1.c_str()) &&
         0 == stricmp("is"          , S1.c_str()) &&
         0 == stricmp("assassinated", S2.c_str())
       )
    {
        p  = GetToken(S3, p, '(', TRIM_ALL);
        p  = GetToken(N3, p, ',', TRIM_ALL);
        Ok = true;
    }
    else
    {
        //Xbowmen (591) attacks City Guard (24) in swamp (13,33) in Salen!
        p  = Source.c_str();
        p  = GetToken(S1, p, '(', TRIM_ALL);
        p  = GetToken(N1, p, ')', TRIM_ALL);
        p  = GetToken(S2, p, '(', TRIM_ALL);
        p  = GetToken(N2, p, ')', TRIM_ALL);
        p  = GetToken(S3, p, '(', TRIM_ALL);
        p  = GetToken(N3, p, ',', TRIM_ALL);
        Ok = (  IsInteger(N1.c_str()) &&
                IsInteger(N2.c_str()) &&
                IsInteger(N3.c_str()) &&
                (0==strnicmp(S3.c_str(), "in", 2)) );
        RegularBattle = true;
    }

    if (Ok)
    {
        p = GetToken(S3, p, ')', TRIM_ALL);
        N3 << "," << S3;
        Normalize(N3);

        m_BattleLandStrs.insert(N3.c_str());

        pBattle              = new CBattle;
        pBattle->LandStrId   = N3;
        pBattle->Description = Source;
        SetStr(pBattle->Name, Source.c_str(), p-Source.c_str());
        Normalize(pBattle->Name);

        if (RegularBattle && atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_BATTLE_STATISTICS)))
        {
            std::string Details;
            AnalyzeBattle(Source.c_str(), Details);
            pBattle->Description << EOL_SCR <<  Details;
        }

        /*
        if (!m_Battles.Insert(pBattle))
            delete pBattle;
            */
        for (i=0; i<m_Battles.Count(); i++)
        {
            CBattle * pExisting = (CBattle*)m_Battles.At(i);
            if (0==stricmp(pBattle->Name.c_str(), pExisting->Name.c_str()))
            {
                delete pBattle;
                pBattle = nullptr;
                break;
            }
        }
        if (pBattle)
            m_Battles.Insert(pBattle);
    }
}

//----------------------------------------------------------------------

// It suxx to build parsing on empty strings, but I can not see any clear sign of a battle end

int CAtlaParser::ParseBattles()
{
    std::string CurLine;
    std::string Battle;
    std::string Block1, Block2;
    const char * p;
    int          i;
    std::string         S1, S2, N1;

    while (ReadNextLine(CurLine))
    {
        TrimRight(CurLine, TRIM_ALL);

        if (CurLine.empty())
        {
            // Captain (15166) is assassinated in forest (83,129) in Imyld!
            p = SkipSpaces(Block1.c_str());
            p = SkipSpaces(GetToken(N1, p, '(', TRIM_ALL));
            p = SkipSpaces(GetToken(N1, p, ')', TRIM_ALL));
            p = SkipSpaces(GetToken(S1, p, ' ', TRIM_ALL));
            p = SkipSpaces(GetToken(S2, p, ' ', TRIM_ALL));


            if (IsInteger(N1.c_str()) &&
                0 == stricmp("is"          , S1.c_str()) &&
                0 == stricmp("assassinated", S2.c_str())
                )
            {
                // It is an assassination
                Battle << Block2 << EOL_SCR;
                StoreBattle(Battle);
                Battle.clear();
                Block2 = Block1;
                Block1.clear();
            }
            else
            {
                Battle << Block2 << EOL_SCR;
                Block2 = Block1;
                Block1.clear();
            }
        }
        else
        {
            if (0==strnicmp(CurLine.c_str(), HDR_ATTACKERS, sizeof(HDR_ATTACKERS)-1))
            {
                // Ho-ho! New battle starting!  Finish the old one first.
                StoreBattle(Battle);
                Battle.clear();
                Battle << Block2 << EOL_SCR;
                Block2 = Block1;
                Block1.clear();
            }
            else
            {
                // Is the battle list over yet?
                p = SkipSpaces(CurLine.c_str());
                for (i=0; i<(int)sizeof(BattleEndHeader)/(int)sizeof(const char *); i++)
                    if (0==strnicmp(p, BattleEndHeader[i], BattleEndHeaderLen[i] ))
                    {
                        CurLine << EOL_FILE;
                        PutLineBack(CurLine);
                        Battle << Block2 << Block1 << EOL_SCR;
                        StoreBattle(Battle);
                        Battle.clear();
                        return 0;
                    }
            }
            Block1 << CurLine << EOL_SCR;
        }
    }

    return 0;
}


//----------------------------------------------------------------------

//Skill reports:
//
//armorer [ARMO] 5: A unit with this skill may PRODUCE mithril armor
//  from one unit of mithril. Mithril armor provides a 9/10 chance of
//  surviving a successful attack in battle from a normal weapon and a
//  2/3 chance of surviving an attack from a good weapon.  Mithril armor
//  weighs one unit. Production of mithril armor can be increased by
//  using hammers.
//
//entertainment [ENTE] 1: A unit with this skill may use the ENTERTAIN
//  order to generate funds. The amount of silver gained will be 20 per
//  man, times the level of the entertainers. This amount is limited by
//  the region that the unit is in.

int CAtlaParser::ParseSkills()
{
    std::string CurLine;

    std::string OneSkill;
    std::string             S;
    const char     * p;
    CShortNamedObj * pSkill;

    while (ReadNextLine(CurLine))
    {
        TrimRight(CurLine, TRIM_ALL);
        if (CurLine.empty())
        {
            if (!OneSkill.empty())
            {
                pSkill = new CShortNamedObj;

                p = OneSkill.c_str();
                p = GetToken(S, p, '[');     pSkill->Name        = S;
                p = GetToken(S, p, ']');     pSkill->ShortName   = S;
                p = GetToken(S, p, ':');     pSkill->Level       = atol(S.c_str());

                pSkill->Description = OneSkill;

                GetToken(pSkill->Name, OneSkill.c_str(), ':');

                m_Skills.Insert(pSkill);
                OneSkill.clear();
            }
        }
        else
        {
            // check for invalid format in the first line
            if (OneSkill.empty())
            {
                p = GetToken(S, CurLine.c_str(), '[');
                if (!p)
                    break;
                p = GetToken(S, p, ']');
                if (!p)
                    break;
                p = GetToken(S, p, ':');
                if (!p)
                    break;
                if (atol(S.c_str()) <= 0)
                    break;
            }

            OneSkill << CurLine << EOL_SCR;
        }
    }
    CurLine << EOL_FILE;
    PutLineBack(CurLine);

    return 0;
}


//----------------------------------------------------------------------

//ironwood [IRWD], weight 10. Units with lumberjack [LUMB] 3 may PRODUCE
//  this item at a rate of 1 per man-month.
//
//mithril [MITH], weight 10. Units with mining [MINI] 3 may PRODUCE this
//  item at a rate of 1 per man-month.
//

int CAtlaParser::ParseItems()
{
    std::string CurLine;
    std::string OneItem;
    std::string             S;
    const char     * p;
    CShortNamedObj * pItem;

    while (ReadNextLine(CurLine))
    {
        TrimRight(CurLine, TRIM_ALL);
        if (CurLine.empty())
        {
            if (!OneItem.empty())
            {
                pItem = new CShortNamedObj;

                p = OneItem.c_str();
                p = GetToken(S, p, '[');     pItem->Name        = S;
                p = GetToken(S, p, ']');     pItem->ShortName   = S;

                pItem->Description = OneItem;
                GetToken(pItem->Name, OneItem.c_str(), ',');

                m_Items.Insert(pItem);
                OneItem.clear();
            }
        }
        else
        {
            // check for invalid format in the first line
            if (OneItem.empty())
            {
                p = GetToken(S, CurLine.c_str(), '[');
                if (!p)
                    break;
                p = GetToken(S, p, ']');
                if (!p)
                    break;
                p = GetToken(S, p, ',');
                if (!p)
                    break;
                if (!S.empty())
                    break;
            }

            OneItem << CurLine << EOL_SCR;
        }
    }
    CurLine << EOL_FILE;
    PutLineBack(CurLine);

    return 0;
}

//----------------------------------------------------------------------

//Object reports:
//
//Longboat: This is a ship. Units may enter this structure. This ship
//  requires 5 total levels of sailing skill to sail. This structure is
//  built using shipbuilding [SHIP] 1 and requires 25 wood to build.
//
//Clipper: This is a ship. Units may enter this structure. This ship
//  requires 10 total levels of sailing skill to sail. This structure is
//  built using shipbuilding [SHIP] 1 and requires 50 wood to build.


int CAtlaParser::ParseObjects()
{
    std::string CurLine;
    std::string OneItem;
    std::string             S;
    const char     * p;
    CShortNamedObj * pItem;
    char             ch;

    while (ReadNextLine(CurLine))
    {
        TrimRight(CurLine, TRIM_ALL);
        if (CurLine.empty())
        {
            if (!OneItem.empty())
            {
                pItem = new CShortNamedObj;

                p = OneItem.c_str();
                p = GetToken(S, p, ':');
                pItem->Name        = S;
                pItem->ShortName   = S;
                pItem->Description = OneItem;

                m_Objects.Insert(pItem);
                OneItem.clear();
            }
        }
        else
        {
            // check for invalid format in the first line
            if (OneItem.empty())
            {
                p = SkipSpaces(GetToken(S, CurLine.c_str(), ':'));
                if (!p)
                    break;
                p = SkipSpaces(GetToken(S, p, " \t", ch));
                if (0!=stricmp(S.c_str(), "This"))
                    break;
                p = SkipSpaces(GetToken(S, p, " \t", ch));
                if (0!=stricmp(S.c_str(), "is"))
                    break;
            }

            OneItem << CurLine << EOL_SCR;
        }
    }
    CurLine << EOL_FILE;
    PutLineBack(CurLine);

    return 0;
}


//----------------------------------------------------------------------

int CAtlaParser::ParseLines(bool Join)
{
    int        err      = ERR_OK;
    std::string CurLine;
    std::string       sErr;
    const char * p;

    while (ReadNextLine(CurLine))
    {
        // recognize header line and then call specific handler

        p = SkipSpaces(CurLine.c_str());

        if      (0==strnicmp(p, HDR_FACTION           , sizeof(HDR_FACTION)-1 ))
            err = ParseFactionInfo(true, Join);

        else if (0==strnicmp(p, HDR_FACTION_STATUS    , sizeof(HDR_FACTION_STATUS)-1 ))
            err = ParseFactionInfo(false, Join);

        else if (0==strnicmp(p, HDR_EVENTS            , sizeof(HDR_EVENTS)-1 ))
            err = ParseEvents();

        else if (0==strnicmp(p, HDR_EVENTS_2          , sizeof(HDR_EVENTS_2)-1 ))
            err = ParseImportantEvents();

        else if (0==strnicmp(p, HDR_ERRORS            , sizeof(HDR_ERRORS)-1 ))
            err = ParseErrors();

        else if (0==strnicmp(p, HDR_SILVER            , sizeof(HDR_SILVER)-1 ))
            err = ParseUnclSilver(CurLine);

        else if (0==strnicmp(p, HDR_ATTITUDES         , sizeof(HDR_ATTITUDES)-1 ))
            err = ParseAttitudes(CurLine, Join);

        else if (0==strnicmp(p, HDR_UNIT_OWN          , sizeof(HDR_UNIT_OWN)-1 ))
            err = ParseUnit(CurLine, Join);

        else if (0==strnicmp(p, HDR_UNIT_ALIEN        , sizeof(HDR_UNIT_ALIEN)-1 ))
            err = ParseUnit(CurLine, Join);

        else if (0==strnicmp(p, HDR_STRUCTURE         , sizeof(HDR_STRUCTURE)-1 ))
            err = ParseStructure(CurLine);

        else if (0==strnicmp(p, HDR_BATTLES           , sizeof(HDR_BATTLES)-1 ))
            err = ParseBattles();

        else if (0==strnicmp(p, HDR_SKILLS            , sizeof(HDR_SKILLS)-1 ))
            err = ParseSkills();

        else if (0==strnicmp(p, HDR_ITEMS             , sizeof(HDR_ITEMS)-1 ))
            err = ParseItems();

        else if (0==strnicmp(p, HDR_OBJECTS           , sizeof(HDR_OBJECTS)-1 ))
            err = ParseObjects();

        else if (0==strnicmp(p, HDR_ORDER_TEMPLATE    , sizeof(HDR_ORDER_TEMPLATE)-1 ))
            err = LoadOrders(*m_pSource, m_CrntFactionId, false); // That's a non-return point.
                                                            // Hope there is nothing else after the order template.

        else

            err = ParseTerrain(nullptr, 0, CurLine, true, nullptr);

        if (ERR_OK!=err)
        {
            sErr.clear();
            sErr << "Error parsing report related to line " << (long)m_nCurLine  << EOL_SCR 
                 << "\"" << CurLine << "\"" << "." << EOL_SCR;
                 
            if (ERR_INV_TURN==err) 
                sErr << EOL_SCR << "Joined reports must be for the same turn. This is intended for joining your ally reports.";
                 
            LOG_ERR(ERR_PARSE, sErr.c_str());
            
            if (ERR_INV_TURN==err) 
                break;
        }
    }

    return ERR_OK;
}

//----------------------------------------------------------------------

int CAtlaParser::ParseRep(const char * FNameIn, bool Join, bool IsHistory)
{
    m_pCurLand   = nullptr;
    m_pCurStruct = nullptr;
    m_ParseErr   = ERR_NOTHING;
    m_pSource    = new CFileReader;
    m_JoiningRep = Join;
    m_IsHistory  = IsHistory;
    m_CrntFactionId = 0;
    m_CrntFactionPwd.clear();
    m_IsHistory  = IsHistory;
    m_nCurLine   = 0;

    if (Join)
        m_FactionInfo << EOL_SCR << EOL_SCR << "-----------------------------------------------"
                      << EOL_SCR << EOL_SCR;

    if (!Join)
        m_YearMon = 0;
    m_CurYearMon  = 0;

    if (!m_pSource->Open(FNameIn))
    {
        m_ParseErr = ERR_FOPEN;
        goto Done;
    }

//    m_MaxSkillDays = atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_MAX_SKILL_DAYS));

    m_ParseErr = ParseLines(Join);

    ApplyLandFlags();
    SetExitFlagsAndTropicZone();
    SetShaftLinks();
    ApplySailingEvents();

Done:
    m_pSource->Close();
    delete m_pSource;
    m_pSource = nullptr;
    return m_ParseErr;
}


//----------------------------------------------------------------------

void CAtlaParser::SetExitFlagsAndTropicZone()
{

    CLand       * pLand;
    CLand       * pLandSrc;
    int           nl, np, i;
    int           x, y, z;
    int           x0, y0;
    CPlane      * pPlane;
    bool          SetAllExits; // if reading 1.0.0 history file

    for (np=0; np<m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_Planes.At(np);

        //tropic zone
        if (!m_IsHistory && m_CurYearMon>0 && pPlane->TropicZoneMin <= pPlane->TropicZoneMax)
            gpDataHelper->SetTropicZone(pPlane->Name.c_str(), pPlane->TropicZoneMin, pPlane->TropicZoneMax);

        // exit flags
        SetAllExits = ( (pPlane->ExitsCount<2) && (pPlane->Lands.Count()>2) );
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand = (CLand*)pPlane->Lands.At(nl);
            if (SetAllExits)
                pLand->ExitBits = 0xFF;
            else
                if ( (0==(pLand->Flags&LAND_VISITED)) || (pLand->Flags&LAND_SET_EXITS)  )
                {
                    pLand->ExitBits = 0;
                    LandIdToCoord(pLand->Id, x0, y0, z);
                    for (i=0; i<6; i++)
                    {
                        x = x0;
                        y = y0;

                        switch (i)
                        {
                        case North     : y -= 2;     break;
                        case Northeast : y--; x++;   break;
                        case Southeast : y++; x++;   break;
                        case South     : y += 2;     break;
                        case Southwest : y++; x--;   break;
                        case Northwest : y--; x--;   break;
                        }

                        pLandSrc = GetLand(x, y, np, true);
                        if ( pLandSrc && (pLandSrc->Flags&LAND_VISITED) )
                        {
                            if (pLandSrc->ExitBits&EntryFlags[i])
                                pLand->ExitBits |= ExitFlags[i];
                        }
                        else
                            pLand->ExitBits |= ExitFlags[i]; // assume there is an exit if nothing is known
                    }
                }
        }
    }
}

//----------------------------------------------------------------------


void AddTabbed(std::string & Dest, const char * Src, int Offs)
{
#define SPC "  "
    int  i;
    int  n;
    std::string Spc;

    for (i=0; i<Offs; i++)
        Spc << SPC;
    n = Spc.size();
    Dest << Spc;

    while (Src && *Src)
    {
        if ( (n>=64) && (' '==*Src) )
        {
            Dest << EOL_SCR << Spc << SPC;
            n = Spc.size() + sizeof(SPC) - 1;
        }
        Dest << *Src;
        Src++;
        n++;
    }
    Dest << EOL_SCR;
}

//----------------------------------------------------------------------

void CAtlaParser::GetUnitList(std::vector<CBaseObject*>* pResultColl, int x, int y, int z)
{

    CLand * pLand;
    int     i;

    pLand = GetLand(x, y, z);
    if (pLand)
        for (i=0; i<pLand->Units.Count(); i++)
            pResultColl->push_back((CBaseObject*)pLand->Units.At(i));
}

//-------------------------------------------------------------

CFaction * CAtlaParser::GetFaction(int id)
{
    CBaseObject  DummyFaction;
    int          idx;

    DummyFaction.Id = id;
    if (m_Factions.Search(&DummyFaction, idx))
        return (CFaction*)m_Factions.At(idx);
    else
        return nullptr;
}

//-------------------------------------------------------------

CLand * CAtlaParser::GetLand(int x, int y, int nPlane, bool AdjustForEdge)
{
    char     dummy[sizeof(CLand)];
    int      idx;
    CPlane * pPlane;

    pPlane = (CPlane*)m_Planes.At(nPlane);
    if (!pPlane)
        return nullptr;

    if (AdjustForEdge && (pPlane->Width > 0))
    {
        while (x>pPlane->EastEdge)
            x-=pPlane->Width;
        while (x<pPlane->WestEdge)
            x+=pPlane->Width;
    }

    ((CLand*)&dummy)->Id = LandCoordToId(x,y, pPlane->Id);

    if (pPlane->Lands.Search(&dummy, idx))
        return (CLand*)pPlane->Lands.At(idx);
    else
        return nullptr;
}

//-------------------------------------------------------------

CLand * CAtlaParser::GetLand(long LandId)
{
    int x, y, z;

    LandIdToCoord(LandId, x, y, z);
    return GetLand(x, y, z, false);
}


//-------------------------------------------------------------

void CAtlaParser::ComposeLandStrCoord(CLand * pLand, std::string & LandStr)
{
    int      x, y, z;
    CPlane * pPlane;

    LandStr.clear();
    if (!pLand)
        return;

    LandIdToCoord(pLand->Id, x, y, z);
    pPlane = (CPlane*)m_Planes.At(z);
    if (!pPlane)
        return;

    LandStr << (long)x << "," << (long)y;
    if (0!=SafeCmp(pPlane->Name.c_str(), DEFAULT_PLANE))
        LandStr << "," << pPlane->Name.c_str();
}

//-------------------------------------------------------------

bool CAtlaParser::LandStrCoordToId(const char * landcoords, long & id)
{
    std::string                 S;
    long                 x, y;
    CPlane             * pPlane;
    CBaseObject          Dummy;
    int                  i;

    // xxx,yyy[,somewhere]
    landcoords = GetToken(S, landcoords, ',');
    if (!IsInteger(S.c_str()))
        return false;
    x = atol(S.c_str());

    // yyy[,somewhere]
    landcoords = GetToken(S, landcoords, ',');
    if (!IsInteger(S.c_str()))
        return false;
    y = atol(S.c_str());


    if ( (nullptr==landcoords) || (0==*landcoords) )
        landcoords = DEFAULT_PLANE;

    Dummy.Name = landcoords;
    if (m_PlanesNamed.Search(&Dummy, i))
        pPlane = (CPlane*)m_PlanesNamed.At(i);
    else
        return false;

    id = LandCoordToId(x, y, pPlane->Id);
    return true;
}



//-------------------------------------------------------------

CLand * CAtlaParser::GetLand(const char * landcoords) //  "48,52[,somewhere]"
{
    long                 id;

    if (LandStrCoordToId(landcoords, id))
        return GetLand(id);
    else
        return nullptr;
}


//----------------------------------------------------------------------

#define CHECK_REP_LEN        \
if (S.size() - n > 65)  \
{                            \
    S << eol << "    ";      \
    n = S.size()-4;     \
}                            \

#define APPEND_SPACE_WRAP    \
if (S.size() - n > 65)  \
{                            \
    S << eol << "    ";      \
    n = S.size()-4;     \
}                            \
else                         \
    S << " ";                \


void CAtlaParser::ComposeProductsLine(CLand * pLand, const char * eol, std::string & S)
{
    std::string Line;
    int        i;

    CProduct * pProd;
    int        n = S.size();


    if (pLand->Products.Count() > 0)
    {
        S << "  Products:";
        for (i=0; i<pLand->Products.Count(); i++)
        {
            pProd = (CProduct*)pLand->Products.At(i);

            if (pProd->Amount>=0)
            {
                APPEND_SPACE_WRAP S << pProd->Amount;
                APPEND_SPACE_WRAP S << pProd->LongName;
                APPEND_SPACE_WRAP S << "["<< pProd->ShortName << "]";
            }
            else
            {
                APPEND_SPACE_WRAP S << " none";
            }

            if (pLand->Products.Count()-1 == i)
                S << ".";
            else
            {
                S << ",";
            }
        }

        S << eol;
    }
}

//-------------------------------------------------------------

bool CAtlaParser::SaveOneHex(CFileWriter & Dest, CLand * pLand, CPlane * pPlane, SAVE_HEX_OPTIONS * pOptions)
{
    CLand            * pLandExit;
    int                i;
    int                x, y, z, x0, y0;
    int                g;

    std::string sLine;
    std::string sExits;
    CStruct          * pStruct;
    const char       * p;
    bool               IsLinked;
    bool               TurnNoMarkerWritten = false;
    CUnit            * pUnit;
    EValueType         type;
    const void       * value;
    int                nstr;
    CStruct          * pEdge;

    IsLinked = false;
    sExits.clear();
    LandIdToCoord(pLand->Id, x0, y0, z);
    for (i=0; i<6; i++)
    {
        if (pLand->ExitBits&ExitFlags[i])
        {
            x = x0;
            y = y0;

            switch (i)
            {
            case North     : y -= 2;     break;
            case Northeast : y--; x++;   break;
            case Southeast : y++; x++;   break;
            case South     : y += 2;     break;
            case Southwest : y++; x--;   break;
            case Northwest : y--; x--;   break;
            }

            pLandExit = GetLand(x, y, z, true);
            if ( pLandExit && (pLandExit->ExitBits&EntryFlags[i]))
            {
                std::string sCoord;

                if (pLandExit->Flags&LAND_VISITED)
                    IsLinked = true;

//  Northeast : mountain (20,0) in Lautaro, contains Krod [city].


                //LandIdToCoord(pLandExit->Id, x, y, z);
                ComposeLandStrCoord(pLandExit, sCoord);
                sExits << "  " << Directions[i] << " : "
                      << pLandExit->TerrainType << " (" << sCoord << ") in " << pLandExit->Name;
                if (!pLandExit->CityName.empty())
                    sExits << ", contains " << pLandExit->CityName << " [" << pLandExit->CityType << "]";

                // save edge structs
                for (nstr = 0; nstr < pLand->EdgeStructs.Count(); nstr++)
                {
                    pEdge = (CStruct*)pLand->EdgeStructs.At(nstr);
                    if (pEdge->Location == i)
                        sExits << ", " << pEdge->Kind;
                }

                sExits  << "." << EOL_FILE;;
            }
        }
    }

    if ( (0==(pLand->Flags&LAND_VISITED)) && IsLinked)
        return false;


    p = pLand->Description.c_str();
    while (p)
    {
        p = GetToken(sLine, p, '\n', TRIM_NONE);
        TrimRight(sLine, TRIM_ALL);

        if (pOptions->WriteTurnNo > 0 && !TurnNoMarkerWritten && sLine.size() > 20)
        {
            bool         SkipIt = false;
            const char * p;

            p = sLine.c_str();
            while (*p)
            {
                if ('-' != *p)
                {
                    SkipIt = true;
                    break;
                }
                p++;
            }
            if (!SkipIt)
            {
                sLine << ";" << pOptions->WriteTurnNo;
                TurnNoMarkerWritten = true;
            }
        }

        sLine << EOL_FILE;
        Dest.WriteBuf(sLine.c_str(), sLine.size());
    }

    sLine.clear();

    if (pOptions->SaveResources)
        ComposeProductsLine(pLand, EOL_FILE, sLine);

    sLine << EOL_FILE << "Exits:" << EOL_FILE ;
    sLine << pLand->Exits;
    TrimRight(sLine, TRIM_ALL);
    sLine << EOL_FILE << EOL_FILE;

    Dest.WriteBuf(sLine.c_str(), sLine.size());
    sLine.clear();

    // Units out of structs, not optimized, does not matter
    if (pOptions->SaveUnits)
        for (i=0; i<pLand->UnitsSeq.Count(); i++)
        {
            pUnit = (CUnit *)pLand->UnitsSeq.At(i);
            if (!IS_NEW_UNIT(pUnit) && !pUnit->GetProperty(PRP_STRUCT_ID, type, value, eOriginal) )
            {
                sLine << pUnit->Description;
                TrimRight(sLine, TRIM_ALL);
                sLine << EOL_FILE;
            }
        }

    // Gates and structs
    for (g=0; g<pLand->Structs.Count(); g++)
    {
        pStruct = (CStruct*)pLand->Structs.At(g);
        if (0==(SA_MOBILE & pStruct->Attr) && pOptions->AlwaysSaveImmobStructs // save history
            || pOptions->SaveStructs || pOptions->SaveUnits)
        {
            sLine << EOL_FILE;
            sLine << pStruct->Description.c_str();
            TrimRight(sLine, TRIM_ALL);
            sLine << EOL_FILE;

            // Units inside structs, not optimized, does not matter
            if (pOptions->SaveUnits)
                for (i=0; i<pLand->UnitsSeq.Count(); i++)
                {
                    pUnit = (CUnit *)pLand->UnitsSeq.At(i);
                    if (!IS_NEW_UNIT(pUnit) && pUnit->GetProperty(PRP_STRUCT_ID, type, value, eOriginal) && eLong==type && (long)value==pStruct->Id)
                    {
                        sLine << pUnit->Description;
                        TrimRight(sLine, TRIM_ALL);
                        sLine << EOL_FILE;
                    }
                }
            Dest.WriteBuf(sLine.c_str(), sLine.size());
            sLine.clear();
        }
    }

    sLine << EOL_FILE << EOL_FILE;
    Dest.WriteBuf(sLine.c_str(), sLine.size());

    return true;
}

//-------------------------------------------------------------

int  CAtlaParser::SaveOrders(const char * FNameOut, const char * password, bool decorate, int factid)
{
    CUnitsByHex   Coll;
    int           i, n, idx;
    CUnit       * pUnit;
    CFileWriter   Dest;
    std::string S, S1;
    int           err = ERR_OK;
    const char  * p;
    long          OldLand = -1;
    CLand       * pLand, DummyLand;
    CPlane      * pPlane;
    char          buf[64];
    struct tm     t;
    time_t        now;
    CFaction    * pFaction = nullptr;

    if (Dest.Open(FNameOut))
    {
        pFaction =  GetFaction(factid);

        // get our units and sort them
        for (i=0; i<m_Units.Count(); i++)
        {
            pUnit = (CUnit*)m_Units.At(i);
            if (pUnit->FactionId == factid)
                Coll.Insert(pUnit);
        }

        // write orders

        S.clear();
        S << "#atlantis " << (long)factid << " \"" << password << '\"' << EOL_FILE << EOL_FILE;
        if (pFaction)
        {
            time(&now);
            t = *localtime(&now);
            i = m_YearMon%100-1;
            if ( (i < 12) && (i >=0) )
                snprintf(buf, sizeof(buf), "%s year %ld,  %s", Monthes[i], m_YearMon/100, asctime(&t));
            else
                snprintf(buf, sizeof(buf), "%02ld/%02ld,  %s", m_YearMon%100, m_YearMon/100, asctime(&t));
            S << ORDER_CMNT << pFaction->Name << " orders for " << buf  << EOL_FILE << EOL_FILE;
        }
        Dest.WriteBuf(S.c_str(), S.size());

        for (i=0; i<Coll.Count(); i++)
        {
            pUnit = (CUnit*)Coll.At(i);
            if (!pUnit->Orders.empty() && !IS_NEW_UNIT(pUnit))
            {
                // land comment
                if (decorate && (OldLand!=pUnit->LandId))
                {
                    OldLand      = pUnit->LandId;
                    DummyLand.Id = OldLand;
                    for (n=0; n<m_Planes.Count(); n++)
                    {
                        pPlane = (CPlane*)m_Planes.At(n);
                        if (pPlane->Lands.Search(&DummyLand, idx))
                        {
                            pLand = (CLand*)pPlane->Lands.At(idx);
                            GetToken(S1, pLand->Description.c_str(), '\n');
                            TrimRight(S1, TRIM_ALL);
                            S.clear();
                            S << EOL_FILE << ORDER_CMNT << S1 << EOL_FILE;
                            Dest.WriteBuf(S.c_str(), S.size());
                            break;
                        }
                    }
                }

                // unit orders
                S.clear();
                S << EOL_FILE << "unit " << pUnit->Id << EOL_FILE;
                Dest.WriteBuf(S.c_str(), S.size());

                p = pUnit->Orders.c_str();
                while (p && *p)
                {
                    p = GetToken(S, p, '\n', TRIM_NONE);
                    TrimRight(S, TRIM_ALL);
                    S << EOL_FILE;
                    Dest.WriteBuf(S.c_str(), S.size());
                }
            }
        }
        S.clear();
        S << EOL_FILE << "#end" << EOL_FILE;
        Dest.WriteBuf(S.c_str(), S.size());
        Dest.Close();

    }
    else
        err = ERR_FOPEN;

    Coll.DeleteAll();

    return err;
}

//-------------------------------------------------------------

// count number of men for the specified faction in every hex.
// store as a property

void CAtlaParser::CountMenForTheFaction(int FactionId)
{
    int           nPlane;
    int           nLand;
    int           nUnit;
    CPlane      * pPlane;
    CLand       * pLand;
    CUnit       * pUnit;
    long          nMen, x;
    EValueType    type;

    for (nPlane=0; nPlane<m_Planes.Count(); nPlane++)
    {
        pPlane = (CPlane*)m_Planes.At(nPlane);
        for (nLand=0; nLand<pPlane->Lands.Count(); nLand++)
        {
            pLand = (CLand*)pPlane->Lands.At(nLand);
            if (!pLand->GetProperty(PRP_SEL_FACT_MEN, type, (const void *&)nMen, eNormal) || (eLong!=type))
                nMen=0;

            for (nUnit=0; nUnit<pLand->Units.Count(); nUnit++)
            {
                pUnit = (CUnit*)pLand->Units.At(nUnit);
                if (FactionId == pUnit->FactionId)
                {
                    if (pUnit->GetProperty(PRP_MEN, type, (const void *&)x, eNormal) && (eLong==type))
                        nMen+=x;
                }
            }
            pLand->SetProperty(PRP_SEL_FACT_MEN, eLong, (void*)nMen, eBoth);
        }
    }

}

//-------------------------------------------------------------

int CAtlaParser::LoadOrders  (CFileReader & F, int FactionId, bool GetComments)
{
    std::string Line, S, No;
    const char  * p;
    CUnit       * pUnit;
    CUnit         Dummy;
    int           idx;
    char          ch;

    m_CrntFactionPwd.clear();

    for (idx=0; idx<m_Units.Count(); idx++)
    {
        pUnit = (CUnit*)m_Units.At(idx);
        if (pUnit->FactionId == FactionId)
            pUnit->Orders.clear();
    }
    pUnit = nullptr;

    while (F.GetNextLine(Line))
    {
        TrimRight(Line, TRIM_ALL);
        p = SkipSpaces(Line.c_str());

        if (0==SafeCmp(p, "#end"))  // that is the end of report
            break;
        if (!p || !*p || (';'==*p && ('*'==*(p+1) || !GetComments)) )
            continue;

        p = GetToken(S, p, " \t", ch, TRIM_ALL);
        if (0==stricmp(S.c_str(),"unit"))
        {
            p = GetToken(No, p, " \t", ch);
            Dummy.Id = atol(No.c_str());
            if (m_Units.Search(&Dummy, idx))
                pUnit = (CUnit*)m_Units.At(idx);
            else
                pUnit = nullptr;
        }
        else
            if (0==stricmp(S.c_str(),"#atlantis"))
            {
                // #atlantis NN "password"
                m_CrntFactionPwd = GetToken(S, p, " \t", ch, TRIM_ALL);
                if (!m_CrntFactionPwd.empty() && '\"'==m_CrntFactionPwd.c_str()[0])
                {
                    DelCh(m_CrntFactionPwd, 0);
                    if (!m_CrntFactionPwd.empty() && '\"'==m_CrntFactionPwd.c_str()[m_CrntFactionPwd.size()-1])
                        DelCh(m_CrntFactionPwd, m_CrntFactionPwd.size()-1);
                }
            }
            else
                if (pUnit)
                {
                    Line << EOL_SCR;
                    AddStr(pUnit->Orders, Line.c_str(), Line.size());
                }
    }

    RunOrders(nullptr);
    m_OrdersLoaded = true;

    return ERR_OK;
}

//-------------------------------------------------------------

int  CAtlaParser::LoadOrders  (const char * FNameIn, int & FactionId)
{
    CFileReader   F;
    std::string Line, S, No;
    CUnit       * pUnit;
    CUnit         Dummy;
    const char  * p;
    char          ch;
    int           idx;

    if (FNameIn && *FNameIn && F.Open(FNameIn))
    {
        // find out what is the faction id
        FactionId = 0;
        while (F.GetNextLine(Line))
        {
            TrimRight(Line, TRIM_ALL);
            p = SkipSpaces(Line.c_str());

            if (0==SafeCmp(p, "#end"))  // that is the end of report
                break;
            if (!p || !*p || ';'==*p || '#'==*p)
                continue;

            p = GetToken(S, p, " \t", ch, TRIM_ALL);
            if (0==stricmp(S.c_str(),"unit"))
            {
                p = GetToken(No, p, " \t", ch);
                Dummy.Id = atol(No.c_str());
                if (m_Units.Search(&Dummy, idx))
                {
                    pUnit = (CUnit*)m_Units.At(idx);
                    if (pUnit->FactionId > 0)
                    {
                        FactionId = pUnit->FactionId; // just take the first unit
                        break;
                    }
                }
            }
        }

        F.Close();
        F.Open(FNameIn);
        LoadOrders(F, FactionId, true);
        F.Close();
        return ERR_OK;
    }
    return ERR_FOPEN;
}

//-------------------------------------------------------------

void CAtlaParser::OrderErrFinalize()
{
    if (gpDataHelper && !m_sOrderErrors.empty())
        gpDataHelper->ReportError(m_sOrderErrors.c_str(), m_sOrderErrors.size(), true);

    m_sOrderErrors.clear();
}

void CAtlaParser::OrderErr(int Severity, int UnitId, const char * Msg)
{
    const char * type;
    std::string S;

    if (0==Severity)
        type = "Error  ";
    else
        type = "Warning";
    Format(S, "Unit % 5d %s : %s%s", UnitId, type, Msg, EOL_SCR);

    m_sOrderErrors << S;
}

//-------------------------------------------------------------

void CAtlaParser::GenericErr(int Severity, const char * Msg)
{
    const char * type;
    std::string S;

    if (!gpDataHelper)
        return;

    if (0==Severity)
        type = "Error  ";
    else
        type = "Warning";
    Format(S, "%s : %s%s", type, Msg, EOL_SCR);

    gpDataHelper->ReportError(S.c_str(), S.size(), false);
}

//-------------------------------------------------------------

#define GEN_ERR(pUnit, msg)                 \
{                                           \
    Line << msg;                            \
    OrderErr(0, pUnit->Id, Line.c_str()); \
    return Changed;                         \
}


bool CAtlaParser::ShareSilver(CUnit * pMainUnit)
{
    CLand             * pLand = nullptr;
    CUnit             * pUnit;
    int                 idx;
    long                unitmoney;
    long                mainmoney;
    long                n;
    EValueType          type;
    std::string Line;
    bool                Changed = false;

    do
    {
        if (!pMainUnit || IS_NEW_UNIT(pMainUnit) || !pMainUnit->IsOurs  )
            break;

        pLand = GetLand(pMainUnit->LandId);
        if (!pLand)
            break;

        RunLandOrders(pLand); // just in case...

        if (!pMainUnit->GetProperty(PRP_SILVER, type, (const void *&)mainmoney, eNormal) )
        {
            mainmoney = 0;
            type      = eLong;
            if (PE_OK!=pMainUnit->SetProperty(PRP_SILVER, type, (const void*)mainmoney, eBoth))
                GEN_ERR(pMainUnit, NOSETUNIT << pMainUnit->Id << BUG);
        }
        else if (eLong!=type)
            GEN_ERR(pMainUnit, NOTNUMERIC << pMainUnit->Id << BUG);

        mainmoney -= pMainUnit->SilvRcvd;

        if (mainmoney<=0)
            break;


        for (idx=0; idx<pLand->Units.Count(); idx++)
        {
            pUnit = (CUnit*)pLand->Units.At(idx);
            if (!pUnit->IsOurs)
                continue;

            if (!pUnit->GetProperty(PRP_SILVER, type, (const void *&)unitmoney, eNormal) )
                continue;
            else if (eLong!=type)
                GEN_ERR(pUnit, NOTNUMERIC << pUnit->Id << BUG);

            if (unitmoney>=0)
                continue;

            n = -unitmoney;
            if (n>mainmoney)
            {
                n         = mainmoney;
                mainmoney = 0;
            }
            else
                mainmoney -= n;

            TrimRight(pMainUnit->Orders, TRIM_ALL);
            if (!pMainUnit->Orders.empty())
                pMainUnit->Orders << EOL_SCR ;
            if (IS_NEW_UNIT(pUnit))
                pMainUnit->Orders << "GIVE NEW " << (long)REVERSE_NEW_UNIT_ID(pUnit->Id) << " " << n << " SILV";
            else
                pMainUnit->Orders << "GIVE " << pUnit->Id   << " " << n << " SILV";
            Changed = true;
            if (mainmoney<=0)
                break;
        }
        if (Changed)
            RunLandOrders(pLand);
    } while (false);

    OrderErrFinalize();
    return Changed;
}

//-------------------------------------------------------------

bool CAtlaParser::GenGiveEverything(CUnit * pFrom, const char * To)
{
    CLand             * pLand = nullptr;
    int                 no;
    bool                Changed = false;
    const char        * propname;
    int                 i;
    bool                skipit;
    EValueType          type;
    long                amount;
    long                amountorg;
    long                x;
    CUnit               Dummy;
    std::string                S;
    const char        * p;
    long                n1;

    do
    {
        if (!pFrom || IS_NEW_UNIT(pFrom) || !pFrom->IsOurs || !To || !*To )
            break;

        pLand = GetLand(pFrom->LandId);
        if (!pLand)
            break;

        p  = To;
        if (!GetTargetUnitId(p, pFrom->FactionId, n1))
        {
            S << "Invalid unit id " << To;
            OrderErr(0, pFrom->Id, S.c_str());
            break;
        }
        Dummy.Id = n1;
        if (n1 != 0 && !pLand->Units.Search(&Dummy, i) )
        {
            S << "Can not find unit " << To;
            OrderErr(0, pFrom->Id, S.c_str());
            break;
        }

        RunLandOrders(pLand); // just in case...

        TrimRight(pFrom->Orders, TRIM_ALL);
        if (!pFrom->Orders.empty())
            pFrom->Orders << EOL_SCR ;

        no = 0;
        do
        {
            propname = pFrom->GetPropertyName(no++);
            skipit   = false;

            if (!propname)
                break;

            // ignore standard properties
            for (i=0; i<STD_UNIT_PROPS_COUNT; i++)
                if (0==stricmp(propname, STD_UNIT_PROPS[i]))
                {
                    skipit = true;
                    break;
                }
            if (skipit)
                continue;

            // ignore skills and such
//            for (i=0; i<STD_UNIT_POSTFIXES_COUNT; i++)
//            {
//                p       = propname;
//                while (p)
//                {
//                    p = strstr(p, STD_UNIT_POSTFIXES[i]);
//                    if (p)
//                    {
//                        p += strlen(STD_UNIT_POSTFIXES[i]);
//                        if (!*p)
//                        {
//                            skipit = true;
//                            break;
//                        }
//                    }
//                }
//                if (skipit)
//                    break;
//            }
//            if (skipit)
            if (IsASkillRelatedProperty(propname))
                continue;


            // ignore strings
            if (!pFrom->GetProperty(propname, type, (const void *&)amount, eNormal) || eLong!=type)
                continue;
            // ignore strings
            if (!pFrom->GetProperty(propname, type, (const void *&)amountorg, eOriginal) || eLong!=type)
                continue;
            x = std::min(amount, amountorg);
            if (x<=0)
                continue;

            pFrom->Orders << "GIVE " << To << " " << x << " " << propname << EOL_SCR;
            Changed = true;

        } while (propname);

        if (Changed)
            RunLandOrders(pLand);
    }
    while (false);

    OrderErrFinalize();
    return Changed;
}

//-------------------------------------------------------------

bool CAtlaParser::GenOrdersTeach(CUnit * pMainUnit)
{
    CLand             * pLand = nullptr;
    CUnit             * pUnit;
    int                 idx;
    EValueType          type;
    long                n1, n2;
    std::string Line;
    std::string                Skill;
    bool                Changed  = false;
    bool                leader_checked = false;
    const void        * value;


    do
    {
        if (!pMainUnit || IS_NEW_UNIT(pMainUnit) || !pMainUnit->IsOurs)
            break;

        pLand = GetLand(pMainUnit->LandId);
        if (!pLand)
            break;

        if (!leader_checked && !pMainUnit->GetProperty(PRP_LEADER, type, value, eNormal) )
            break;
        else
            leader_checked = true;


        RunLandOrders(pLand); // just in case...

        for (idx=0; idx<pLand->UnitsSeq.Count(); idx++)
        {
            pUnit = (CUnit*)pLand->UnitsSeq.At(idx);
            if (!pUnit->StudyingSkill.empty())
            {
                Skill = pUnit->StudyingSkill;
                if (!pMainUnit->GetProperty(Skill.c_str(), type, (const void *&)n1, eNormal) )
                    n1 = 0;
                if (!pUnit->GetProperty(Skill.c_str(), type, (const void *&)n2, eNormal) )
                    n2 = 0;

                if (n1 <= n2)
                {
                    // can not teach in the normal game, but try for Arcadia III

                    int  SkillPos = FindSubStrR(Skill, PRP_SKILL_POSTFIX);
                    if (SkillPos>=0)
                        DelSubStr(Skill, SkillPos, strlen(PRP_SKILL_POSTFIX));

                    Skill << PRP_SKILL_STUDY_POSTFIX;
                    if (!pMainUnit->GetProperty(Skill.c_str(), type, (const void *&)n1, eNormal) )
                        n1 = 0;
                    if (!pUnit->GetProperty(Skill.c_str(), type, (const void *&)n2, eNormal) )
                        n2 = 0;
                }

                if ( (n1 > n2) &&            // can teach
                     (pUnit->Teaching <= 20) // student only partially taught by someone else
                   )
                {
                    // count men now

                    if (!pMainUnit->GetProperty(PRP_MEN, type, (const void *&)n1, eNormal)  || (n1<=0))
                        break;
                    if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)n2, eNormal) || (n2<=0))
                        continue;

                    if (n1*(STUDENTS_PER_TEACHER - pMainUnit->Teaching) >= n2)
                    {
                        pMainUnit->Teaching += (double)n2/n1;

                        TrimRight(pMainUnit->Orders, TRIM_ALL);
                        if (!pMainUnit->Orders.empty())
                            pMainUnit->Orders << EOL_SCR ;
                        if (IS_NEW_UNIT(pUnit))
                            pMainUnit->Orders << "TEACH " << "NEW " << (long)REVERSE_NEW_UNIT_ID(pUnit->Id);
                        else
                            pMainUnit->Orders << "TEACH " << pUnit->Id;
                        Changed = true;
                    }
                }
            }
        }

    } while (false);

    if (Changed)
        RunLandOrders(pLand); // just in case...

    return Changed;
}

//-------------------------------------------------------------

bool CAtlaParser::DiscardJunkItems(CUnit * pUnit, const char * junk)
{
    CLand             * pLand = nullptr;
    EValueType          type;
    long                value;
    std::string sJunkItem;
    bool                Changed = false;

    if (!pUnit || IS_NEW_UNIT(pUnit) || !pUnit->IsOurs )
        return false;

    pLand = GetLand(pUnit->LandId);
    if (!pLand)
        return false;

    RunLandOrders(pLand); // just in case...

    while (junk && *junk)
    {
        junk = GetToken(sJunkItem, junk, ',');
        if (!pUnit->GetProperty(sJunkItem.c_str(), type, (const void *&)value, eNormal) || (eLong!=type))
            continue;

        TrimRight(pUnit->Orders, TRIM_ALL);
        if (!pUnit->Orders.empty())
            pUnit->Orders << EOL_SCR ;
        pUnit->Orders << "GIVE 0 " << value << " " << sJunkItem;
        Changed = true;
    }
    if (Changed)
        RunLandOrders(pLand);
    return Changed;
}



//-------------------------------------------------------------

bool CAtlaParser::DetectSpies(CUnit * pUnit, long lonum, long hinum, long amount)
{

#define DETECT_ONE_SPY                                                         \
{                                                                              \
    pUnit->Orders << "GIVE " << no << " " << amount << " SILV ;ne"  << EOL_SCR;\
    Changed = true;                                                            \
    no++;                                                                      \
}

    CLand             * pLand = nullptr;
    bool                Changed = false;
    long                no;
    int                 idx = 0;
    CUnit             * pNext;

    if (!pUnit || IS_NEW_UNIT(pUnit) || !pUnit->IsOurs)
        return false;

    pLand = GetLand(pUnit->LandId);
    if (!pLand)
        return false;

    RunLandOrders(pLand); // just in case...

    TrimRight(pUnit->Orders, TRIM_ALL);
    if (!pUnit->Orders.empty())
        pUnit->Orders << EOL_SCR ;

    no = lonum;
    while (no<=hinum)
    {
        pNext = (CUnit*)m_Units.At(idx);

        if (pNext)
        {
            while (no<pNext->Id)
                DETECT_ONE_SPY
            if (no == pNext->Id)
                no++;
            idx++;
        }
        else
            DETECT_ONE_SPY
    }

    if (Changed)
        RunLandOrders(pLand);

    return Changed;
}


//-------------------------------------------------------------

const char * CAtlaParser::ReadPropertyName(const char * src, std::string & Name)
{
    char ch;
    int  i;
    std::string S;

    Name.clear();
    src = SkipSpaces(GetToken(S, src, " \t;\n", ch, TRIM_ALL)); // SH-EXCPT

    if (!S.empty())
    {
//        if ('"'==S.c_str()[0])
//        {
//            if ('"'==S.c_str()[S.size()-1])
//            {
                // remove quotes, replace spaces with underscores

//                DelCh(S, S.size()-1);
//                DelCh(S, 0);
                for (i=0; i<S.size(); i++)
                    if (' '==S.c_str()[i])
                        SetCh(S, i, '_');
//            }
//            else
//                return src;
//        }
        Name = gpDataHelper->ResolveAlias(S.c_str());
    }

    return src;
}

//-------------------------------------------------------------

bool CAtlaParser::GetTargetUnitId(const char *& p, long FactionId, long & nId)
{
    std::string N1, N, X, Y;
    char                ch;

                                                                          
    p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
    if (0==stricmp("FACTION", N1.c_str()))
    {
        // FACTION X NEW Y
        p = SkipSpaces(GetToken(X, p, " \t", ch, TRIM_ALL));  // X
        if (!IsInteger(X))
            return false;
        p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));  // NEW
        if (0!=stricmp(N1.c_str(), "NEW"))
            return false;
        p = SkipSpaces(GetToken(Y, p, " \t", ch, TRIM_ALL));  // Y
        if (!IsInteger(Y))
            return false;

        if ( atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_CHECK_NEW_UNIT_FACTION)))
            nId = NEW_UNIT_ID(atol(Y.c_str()), atol(X.c_str()));
        else
            nId = 0;
        return true;
    }
    if (0==stricmp("NEW", N1.c_str()))
    {
        p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
        if (!IsInteger(N1))
            return false;
        nId = NEW_UNIT_ID(atol(N1.c_str()), FactionId);
        return true;
    }

    if (!IsInteger(N1))
        return false;
    nId = atol(N1.c_str());
    return true;
}

//-------------------------------------------------------------

#define SHOW_WARN_CONTINUE(msg)                      \
{                                                    \
    if (!skiperror)                                  \
    {                                                \
        ErrorLine.clear();                           \
        ErrorLine << Line << msg;                    \
        OrderErr(1, pUnit->Id, ErrorLine.c_str()); \
        continue;                                    \
    }                                                \
}

#define SHOW_WARN_BREAK(msg)                         \
{                                                    \
    if (!skiperror)                                  \
    {                                                \
        ErrorLine.clear();                           \
        ErrorLine << Line << msg;                    \
        OrderErr(1, pUnit->Id, ErrorLine.c_str()); \
        break;                                       \
    }                                                \
}



enum
{      SQ_MIN    ,
       SQ_FORM   ,
       SQ_CLAIM  ,
       SQ_LEAVE  ,
       SQ_ENTER  ,
       SQ_PROMOTE,
       SQ_GIVE   ,
       SQ_SELL   , // Shar1 Extrict SELL/BUY check. Sell is executed before buy
       SQ_BUY    ,
       SQ_STUDY  ,
       SQ_TEACH  ,
       SQ_MOVE   ,
       SQ_MAX
};

void CAtlaParser::RunLandOrders(CLand * pLand, const char * sCheckTeach)
{
    CBaseObject         Dummy;
    CUnit             * pUnit;
    CUnit             * pUnitNew;
    CUnit             * pUnitMaster;
    CUnit             * pUnit2;
    CStruct           * pStruct;
    std::string Line;
    std::string Cmd;
    std::string S;
    std::string S1;
    std::string N1;
    std::string ErrorLine;
    const char        * p;
    const char        * src;
    long                n1;
    long                unitmoney;
    int                 mainidx;
    int                 sequence;
    bool                errors = false;
    int                 idx;
    EValueType          type;
    const void        * value;
    int                 X, Y, Z;    // current coordinates for moving unit
    int                 XN=0, YN=0;     // saved current coords while new unit is processed
    int                 LocA3, LocA3N=0; // support for ArcadiaIII sailing
    char                ch;
    bool                TeachCheckGlb = (bool)atoi(sCheckTeach?sCheckTeach:gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_CHECK_TEACH_LVL));
    bool                skiperror;
    int                 nNestingMode; // Shar1 Support for TURN/ENDTURN
    long                order;


    // Reset land and old units and remove new units
    pLand->ResetNormalProperties();
    pLand->ResetUnitsAndStructs();

    // Run Orders
    for (sequence=SQ_MIN+1; sequence<SQ_MAX; sequence++)
    {
        if (errors)
            break;

        for (mainidx=0; mainidx<pLand->UnitsSeq.Count(); mainidx++)
        {
            nNestingMode = 0; // Shar1 Support for TURN/ENDTURN
            pUnitMaster = nullptr;
            pUnit       = (CUnit*)pLand->UnitsSeq.At(mainidx);
            if (IS_NEW_UNIT(pUnit))
                continue;

            LandIdToCoord(pLand->Id, X, Y, Z);
            LocA3 = NO_LOCATION;

            src         = pUnit->Orders.c_str();
            while (src && *src)
            {
                src  = GetToken(Line, src, '\n', TRIM_ALL);

                // trim all the comments
                p = strchr(Line.c_str(), ';');
                if (p)
                {
                    S1 = SkipSpaces(++p);
                    TrimRight(S1, TRIM_ALL);
                    skiperror = (0==stricmp(S1.c_str(),"ne") || 0==stricmp(S1.c_str(),"$ne"));
                    RunPseudoComment(sequence, pLand, pUnit, S1.c_str());
                    DelSubStr(Line, p-Line.c_str()-1, Line.size() - (p-Line.c_str()-1) );
                }
                else
                    skiperror = false;

                p    = SkipSpaces(GetToken(Cmd, Line.c_str(), " \t", ch, TRIM_ALL));
                if ('@'==Cmd.c_str()[0])
                {
                    DelCh(Cmd, 0);
                    if (Cmd.empty()) // I do not know if putting spaces after '@' is legal... anyway
                        p    = SkipSpaces(GetToken(Cmd, p, " \t", ch, TRIM_ALL));
                }
                if ('!'==Cmd.c_str()[0])
                {
                    DelCh(Cmd, 0);
                    if (Cmd.empty()) // I do not know if putting spaces after '@' is legal... anyway
                        p    = SkipSpaces(GetToken(Cmd, p, " \t", ch, TRIM_ALL));
                    skiperror = true;
                }

                if (Cmd.empty())
                    continue;
                if (!gpDataHelper->GetOrderId(Cmd.c_str(), order))
                {
                    if (SQ_MIN+1 == sequence)
                        SHOW_WARN_CONTINUE(" - unknown order")
                    else
                        continue;
                }

                // Shar1 Support for TURN/ENDTURN - Start
                if (nNestingMode)
                {
                    // I don't know if Atlantis support nested TURN/ENDTURN tags
                    // By now, ALH won't support them. But it can be changed easily,
                    // changing the nNestingMode into a turnLevel (integer)
                    if (O_TURN == order || O_TEMPLATE == order || O_ALL == order)
                    {
                        SHOW_WARN_CONTINUE(" - Nesting TURN, TEMPLATE and such orders are not allowed");
                    }
                    else if (nNestingMode+1 == order)
                    {
                        nNestingMode = 0;
                    }
                    else
                    {
                        // here we can do something special!
                    }
                    continue;
                }

                switch (order)
                {
                    case O_TURN:
                    case O_TEMPLATE:
                    case O_ALL:
                        nNestingMode = order;
                        break;

                    case O_ENDTURN:
                    case O_ENDTEMPLATE:
                    case O_ENDALL:
                        SHOW_WARN_CONTINUE(" - ENDXXX without XXX");
                        break;                        // Shar1 Support for TURN/ENDTURN - End


                    case O_GIVE:
                        if (SQ_GIVE == sequence)
                            RunOrder_Give(Line, ErrorLine, skiperror, pUnit, pLand, p, false);
                        break;

                    case O_GIVEIF:
                        if (SQ_GIVE == sequence)
                            RunOrder_Give(Line, ErrorLine, skiperror, pUnit, pLand, p, true);
                        break;

                    case O_TAKE:
                        if (SQ_GIVE == sequence)
                            RunOrder_Take(Line, ErrorLine, skiperror, pUnit, pLand, p, true);
                        break;

                    case O_SEND:
                        if (SQ_BUY+1 == sequence) // send is after buy
                            RunOrder_Send(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;

                    case O_WITHDRAW:
                        if (SQ_BUY+1 == sequence)
                            RunOrder_Withdraw(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;

                    case O_RECRUIT:
                        if (SQ_BUY+1 == sequence)
                        {
                            // do recruiting here
                            p        = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            Dummy.Id = atol(N1.c_str());
                            if (!pLand->Units.Search(&Dummy, idx))
                                SHOW_WARN_CONTINUE(" - Can not find unit " << N1);
                        }
                        break;


                    case O_BUY:
                        if (SQ_BUY==sequence)
                            RunOrder_Buy(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;


                    case O_SELL:
                        if (SQ_SELL == sequence)
                            RunOrder_Sell(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;

                    case O_FORM:
                        p        = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                        n1       = NEW_UNIT_ID(atol(N1.c_str()), pUnit->FactionId);
                        Dummy.Id = n1;

                        if (SQ_FORM==sequence)
                        {
                            if (!IS_NEW_UNIT_ID(n1))
                                SHOW_WARN_CONTINUE(" - Invalid new unit number!");
                            if (pLand->Units.Search(&Dummy, idx))
                                SHOW_WARN_CONTINUE(" - Unit already exists!");
                            pUnitNew             = new CUnit;
                            pUnitNew->Id         = n1;
                            pUnitNew->FactionId  = pUnit->FactionId;
                            pUnitNew->pFaction   = pUnit->pFaction;
                            pUnitNew->Flags      = pUnit->Flags;
                            pUnitNew->IsOurs     = true;
                            pUnitNew->Name        << "NEW " << N1;
                            pUnitNew->Description << "Created by " << pUnit->Id;
                            
                            // set attitude:
                            int attitude = gpDataHelper->GetAttitudeForFaction(pUnit->FactionId);
                            SetUnitProperty(pUnitNew,PRP_FRIEND_OR_FOE,eLong,reinterpret_cast<void*>(static_cast<intptr_t>(attitude)),eNormal);

                            if (pUnit->GetProperty(PRP_STRUCT_ID, type, value, eOriginal) )
                                pUnitNew->SetProperty(PRP_STRUCT_ID, type, value, eBoth);
                            if (pUnit->GetProperty(PRP_STRUCT_NAME, type, value, eOriginal) )
                                pUnitNew->SetProperty(PRP_STRUCT_NAME, type, value, eBoth);

                            if (!pLand->AddUnit(pUnitNew))
                                SHOW_WARN_CONTINUE(" - Can not create new unit! " << BUG);
                        }
                        else
                        {
                            if (!IS_NEW_UNIT_ID(n1))
                                continue;
                            if (pUnitMaster)
                                SHOW_WARN_CONTINUE(" - Sorry, we do not support new units creating units :(");
                            pUnitMaster       = pUnit;
                            if (!pLand->Units.Search(&Dummy, idx))
                                SHOW_WARN_CONTINUE(" - Can not find new unit! " << BUG);
                            pUnit = (CUnit*)pLand->Units.At(idx);
                            XN     = X;
                            YN     = Y;
                            LocA3N = LocA3;
                            LandIdToCoord(pLand->Id, X, Y, Z);
                            LocA3 = NO_LOCATION;
                        }
                        break;

                    case O_ENDFORM:
                        if (SQ_FORM!=sequence)
                        {
                            if (!pUnitMaster)
                            {
                                if (SQ_FORM+1 == sequence)
                                {
                                    SHOW_WARN_CONTINUE(" - FORM was not called!");
                                }
                                else
                                    continue;
                            }

                            if  (SQ_TEACH==sequence)  // have to call it here or new units teaching will not be calculated
                                OrderProcess_Teach(skiperror, pUnit);

                            pUnit       = pUnitMaster;
                            pUnitMaster = nullptr;
                            X     = XN;
                            Y     = YN;
                            LocA3 = LocA3N;
                        }
                        break;

                    case O_ATTACK:
                    case O_ASSASSINATE:
                    case O_STEAL:
                        if (SQ_CLAIM==sequence)
                        {
                            // just check if the target is valid
                            // and SQ_CLAIM is good enough, no need to introduce a new phase
                            while (p && *p)
                            {
                                p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));

                                n1 = atol(N1.c_str());
                                Dummy.Id = n1;
                                if (pLand->Units.Search(&Dummy, idx))
                                {
                                    pUnit2 = (CUnit*)pLand->Units.At(idx);
                                    if (pUnit2->IsOurs)
                                        SHOW_WARN_CONTINUE(" - Unit " << n1 << " is our, can not " << Cmd << "!");
                                }
                                else
                                    SHOW_WARN_CONTINUE(" - Unit " << n1 << " is not here, can not " << Cmd << "!");

                                if (O_ATTACK != order) // only attack can have multiple targets
                                    break;
                            }
                        }
                        break;


                    case O_NAME:
                        if (SQ_CLAIM==sequence)
                            RunOrder_Name(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;

                    case O_STUDY:
                        if (SQ_STUDY==sequence)
                            RunOrder_Study(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;

                    case O_TEACH:
                        if (SQ_TEACH==sequence)
                            RunOrder_Teach(Line, ErrorLine, skiperror, pUnit, pLand, p, TeachCheckGlb);
                        break;

                    case O_CLAIM:
                        if (SQ_CLAIM==sequence)
                        {
                            p        = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));

                            n1       = atol(N1.c_str());

                            if (!pUnit->GetProperty(PRP_SILVER, type, (const void *&)unitmoney, eNormal) )
                            {
                                unitmoney = 0;
                                if (PE_OK!=pUnit->SetProperty(PRP_SILVER, eLong, (const void*)unitmoney, eBoth))
                                    SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                            }
                            else if (eLong!=type)
                                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

                            unitmoney += n1;

                            if (PE_OK!=pUnit->SetProperty(PRP_SILVER,   eLong, (const void *)unitmoney, eNormal))
                                SHOW_WARN_CONTINUE(NOSET << BUG);
                        }
                        break;


                    case O_LEAVE:
                        if (SQ_LEAVE==sequence)
                        {
                            if (pUnit->GetProperty(PRP_STRUCT_ID, type, (const void *&)n1) && eLong==type)
                            {
                                pStruct  = pLand->GetStructById(n1);
                                if (pStruct && pStruct->OwnerUnitId == pUnit->Id)
                                    pStruct->OwnerUnitId = 0;
                            }

                            if ( (PE_OK!=pUnit->SetProperty(PRP_STRUCT_NAME,  eCharPtr, "", eNormal)) ||
                                (PE_OK!=pUnit->SetProperty(PRP_STRUCT_OWNER, eCharPtr, "", eNormal)) ||
                                (PE_OK!=pUnit->SetProperty(PRP_STRUCT_ID,    eLong   ,  0, eNormal)) )
                                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                        }
                        break;


                    case O_ENTER:
                        if (SQ_ENTER==sequence)
                        {
                            p        = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            n1       = atol(N1.c_str());
                            pStruct  = pLand->GetStructById(n1);
                            if (pStruct)
                            {
                                if (0==pStruct->OwnerUnitId)
                                {
                                    pStruct->OwnerUnitId = pUnit->Id;
                                    S1 = YES;
                                }
                                else
                                    S1.clear();
                                if ( (PE_OK!=pUnit->SetProperty(PRP_STRUCT_ID,  eLong, 0,                  eNormal)) ||
                                    (PE_OK!=pUnit->SetProperty(PRP_STRUCT_ID,  eLong, (void*)pStruct->Id, eNormal)) ||
                                    (PE_OK!=pUnit->SetProperty(PRP_STRUCT_NAME,  eCharPtr, "",                      eNormal)) ||
                                    (PE_OK!=pUnit->SetProperty(PRP_STRUCT_NAME,  eCharPtr, pStruct->Name.c_str(), eNormal)) ||
                                    (PE_OK!=pUnit->SetProperty(PRP_STRUCT_OWNER, eCharPtr, S1.c_str(), eNormal)) )
                                    SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                            }
                            else
                                SHOW_WARN_CONTINUE(" - Invalid structure number " << n1);
                        }
                        break;


                    case O_PROMOTE:
                        if (SQ_PROMOTE==sequence)
                            RunOrder_Promote(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;


                    case O_MOVE:
                    case O_SAIL:
                    case O_ADVANCE:
                        if (SQ_MOVE==sequence)
                            RunOrder_Move(Line, ErrorLine, skiperror, pUnit, pLand, p, X, Y, LocA3, order);
                        break;

                    // autotax flag must be removed in the very first pass, or land flag will be set and never removed
                    case O_AUTOTAX:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_TAXING;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_TAXING;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_GUARD:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_GUARDING;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_GUARDING;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_AVOID:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_AVOIDING;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_AVOIDING;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_BEHIND:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_BEHIND;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_BEHIND;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_HOLD:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_HOLDING;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_HOLDING;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_NOAID:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_RECEIVING_NO_AID;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_RECEIVING_NO_AID;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_NOCROSS:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_NO_CROSS_WATER;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_NO_CROSS_WATER;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_SPOILS:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (N1.empty() || 0==stricmp(N1.c_str(), "ALL"))
                                pUnit->Flags &= ~UNIT_FLAG_SPOILS;
                            else
                                pUnit->Flags |=  UNIT_FLAG_SPOILS;
                        }
                        break;

                    // MZ - Added for Arcadia
                    case O_SHARE:
                        if (SQ_CLAIM==sequence)
                        {
                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "1"))
                                pUnit->Flags |=  UNIT_FLAG_SHARING;
                            else if (0==stricmp(N1.c_str(), "0"))
                                pUnit->Flags &= ~UNIT_FLAG_SHARING;
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_REVEAL:
                        if (SQ_CLAIM==sequence)
                        {

                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "UNIT"))
                            {
                                pUnit->Flags |=  UNIT_FLAG_REVEALING_UNIT;
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_FACTION;
                            }
                            else if (0==stricmp(N1.c_str(), "FACTION"))
                            {
                                pUnit->Flags |=  UNIT_FLAG_REVEALING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_UNIT;
                            }
                            else if (N1.empty())
                            {
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_UNIT;
                            }
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;

                    case O_CONSUME:
                        if (SQ_CLAIM==sequence)
                        {

                            p = SkipSpaces(GetToken(N1, p, " \t", ch, TRIM_ALL));
                            if (0==stricmp(N1.c_str(), "UNIT"))
                            {
                                pUnit->Flags |=  UNIT_FLAG_CONSUMING_UNIT;
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_FACTION;
                            }
                            else if (0==stricmp(N1.c_str(), "FACTION"))
                            {
                                pUnit->Flags |=  UNIT_FLAG_CONSUMING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_UNIT;
                            }
                            else if (N1.empty())
                            {
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_UNIT;
                            }
                            else
                                SHOW_WARN_CONTINUE(" - Invalid parameter");
                        }
                        break;


                    case O_TAX:
                        if (SQ_CLAIM ==sequence )
                            pUnit->Flags |=  UNIT_FLAG_TAXING;
                        break;

                    case O_WORK:
                        if (SQ_MAX-1==sequence)
                            pUnit->IsWorking = true;
                        break;

                    case O_BUILD:
                        if (SQ_MAX-1==sequence)
                            pUnit->Flags |= UNIT_FLAG_PRODUCING;
                        break;

                    case O_PRODUCE:
                        if (SQ_MAX-1==sequence)
                            RunOrder_Produce(Line, ErrorLine, skiperror, pUnit, pLand, p);
                        break;

                }//switch (order)

                // copy commands to the new unit
                if (pUnitMaster && (SQ_MAX-1==sequence) && (O_FORM != order) && !errors)
                    pUnit->Orders << Line << EOL_SCR;

            } // commands loop


            if  (SQ_TEACH==sequence)  // we have to collect all the students, then teach
                OrderProcess_Teach(skiperror, pUnit);

            //if (SQ_MAX-1==sequence)
            //{
            //    // set land flags in the last sequence, so all unit flags are already applied
            //    if ((pUnit->Flags & UNIT_FLAG_TAXING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
            //        pLand->Flags |= LAND_TAX_NEXT;
            //    if ((pUnit->Flags & UNIT_FLAG_PRODUCING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
            //        pLand->Flags |= LAND_TRADE_NEXT;
            //}

        }   // units loop
    }   // phases loop
    pLand->CalcStructsLoad();
    pLand->SetFlagsFromUnits(this);
    OrderErrFinalize();
}


//-------------------------------------------------------------

void CAtlaParser::RunOrder_Study(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    EValueType          type;
    long                n, n1, n2;
    long                unitmoney;
    std::string S;
    std::string SkillNaked;
    long                minlevel = -1;
    long                level;
    long                no, propval;
    // MZ - Added for Arcadia
    const char        * lead = "";
    const char        * racename;

    do
    {
        params = ReadPropertyName(params, SkillNaked);

        n1 = gpDataHelper->GetStudyCost(SkillNaked.c_str());
        if (n1<=0)
            SHOW_WARN_CONTINUE(" - Can not study that!");

        if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)n2, eNormal) || (n2<=0))
        {
            n2 = 0;
            SHOW_WARN_CONTINUE(" - There are no men in the unit!");
        }
        if (eLong!=type)
            SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);


        // find min max skill level for the unit
        no = 0;
        racename = pUnit->GetPropertyName(no);
        while (racename)
        {
            if (gpDataHelper->IsMan(racename))
            {
                if (pUnit->GetProperty(racename, type, (const void *&)propval, eNormal) &&
                    eLong==type && propval>0) // 0 welf does not affect anything
                {
                    pUnit->GetProperty(PRP_LEADER, type, (const void *&)lead, eNormal);
                    level = gpDataHelper->MaxSkillLevel(racename, SkillNaked.c_str(), lead, m_ArcadiaSkills);
                    if (-1==minlevel)
                        minlevel = level;
                    else
                        if (level < minlevel)
                            minlevel = level;
                }
            }
            no++;
            racename = pUnit->GetPropertyName(no);
        }

        // now, can we still study?
        S = SkillNaked;
        S << PRP_SKILL_POSTFIX;
        if (pUnit->GetProperty(S.c_str(), type, (const void *&)n, eNormal) && eLong==type && n>=minlevel)
            SHOW_WARN_CONTINUE(" - Already knows the skill!");

        if (!pUnit->GetProperty(PRP_SILVER, type, (const void *&)unitmoney, eNormal) )
        {
            unitmoney = 0;
            if (PE_OK!=pUnit->SetProperty(PRP_SILVER, eLong, (const void*)unitmoney, eBoth))
                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
        }
        else if (eLong!=type)
            SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

        unitmoney -= n1*n2;

        if (PE_OK!=pUnit->SetProperty(PRP_SILVER,   eLong, (const void *)unitmoney, eNormal))
            SHOW_WARN_CONTINUE(NOSET << BUG);

        pUnit->StudyingSkill = SkillNaked;
        pUnit->StudyingSkill << PRP_SKILL_POSTFIX;
    } while (false);
}


//-------------------------------------------------------------

void CAtlaParser::RunOrder_Name(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    std::string                What, Name, NewName;
//     long                id;
//     EValueType          type;
//     const char        * isowner;
//     CStruct           * pStruct;

    do
    {
        params = SkipSpaces(GetToken(What, params, ' ', TRIM_ALL));
        params = GetToken(Name, params, ' ', TRIM_ALL);

        if (0==stricmp(What.c_str(), "unit"))
        {
            if (IS_NEW_UNIT(pUnit))
            {
                NewName << "NEW " << (long)REVERSE_NEW_UNIT_ID(pUnit->Id) << " - " << Name;
                Name = NewName;
            }
            pUnit->SetName(Name.c_str());
        }
//         else if (0==stricmp(What.c_str(), "object"))
//         {
//             if (!pUnit->GetProperty(PRP_STRUCT_ID, type, (const void *&)id) || eLong!=type)
//                 SHOW_WARN_CONTINUE(" - Is not inside an object!");
//             if (!pUnit->GetProperty(PRP_STRUCT_OWNER, type, (const void *&)isowner) || eCharPtr!=type || 0!=stricmp(isowner, YES))
//                 SHOW_WARN_CONTINUE(" - Is not an object owner!");
//
//             pStruct  = pLand->GetStructById(id);
//             if (!pStruct)
//                 SHOW_WARN_CONTINUE(" - Can not locate structure!" << BUG);
//             pStruct->SetName(Name.c_str());
//             pUnit->SetProperty(PRP_STRUCT_NAME,  eCharPtr, pStruct->Name.c_str(), eNormal);
//         }

    } while (false);
}


//-------------------------------------------------------------

bool CAtlaParser::CheckResourcesForProduction(CUnit * pUnit, CLand * pLand, std::string & Error)
{
    bool                Ok = true;
    unsigned int        x;
    int                 i;
    TProdDetails        details;
    CUnit             * pSharer;
    const char        * propname;
    bool                SharerFound;
    EValueType          type;
    const void        * value;
    long                nlvl  = 0;
    long                ntool = 0;
    long                ncanproduce = 0;
    long                nres  = 0;
    long                nrequired = 0;
    long                nmen  = 0;
    int                 no;
    std::string                S;

    Error.clear();
    if ((pUnit->Flags & UNIT_FLAG_PRODUCING) && !pUnit->ProducingItem.empty())
    {
        gpDataHelper->GetProdDetails (pUnit->ProducingItem.c_str(), details);

        if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)nmen, eNormal)  || (nmen<=0))
        {
            Error << " - There are no men in the unit!";
            Ok = false;
        }

        if (details.skillname.empty() || details.months<=0)
        {
            Error << " - Production requirements for item '" << pUnit->ProducingItem << "' are not configured! ";
            Ok = false;
        }

        // check skill level
        S << details.skillname << PRP_SKILL_POSTFIX;
        if (pUnit->GetProperty(S.c_str(), type, value, eNormal) && (eLong==type) )
        {
            nlvl = (long)value;
            if (nlvl < details.skilllevel)
            {
                Error << " - Skill " << details.skillname << " level " << details.skilllevel << " is required for production";
                Ok = false;
            }
        }
        else
        {
            Error << " - Skill " << details.skillname << " is required for production";
            Ok = false;
        }


        if (!details.toolname.empty())
            if (!pUnit->GetProperty(details.toolname.c_str(), type, (const void *&)ntool, eNormal) || eLong!=type )
                ntool = 0;
        if (ntool > nmen)
            ntool = nmen;

        ncanproduce = (long)((((double)nmen)*nlvl + ntool*details.toolhelp) / details.months);


        for (x=0; x<sizeof(details.resname)/sizeof(*details.resname); x++)
        {
            SharerFound = false;
            if (!details.resname[x].empty())
            {
                if (!pUnit->GetProperty(details.resname[x].c_str(), type, (const void *&)nres, eNormal) || eLong!=type )
                    nres = 0;

                // how many do we need?
                nrequired = (ncanproduce * details.resamt[x] );

                if (nrequired > nres)
                {
                    // Now check if there are our units SHARING the resource.
                    // We will not give a damn if they have enough or not though :)
                    // since we do not know how does server disribute the resources
                    for (i=0; i<pLand->Units.Count(); i++)
                    {
                        pSharer = (CUnit*)pLand->Units.At(i);
                        if (pSharer->FactionId == pUnit->FactionId && (pSharer->Flags & UNIT_FLAG_SHARING))
                        {
                            no = 0;
                            propname = pSharer->GetPropertyName(no);
                            while (propname)
                            {
                                if (0==stricmp(propname, details.resname[x].c_str()))
                                {
                                    SharerFound = true;
                                    break;
                                }

                                propname = pSharer->GetPropertyName(++no);
                            }
                        }
                    }

                    if (!SharerFound)
                    {
                        if (0==nres)
                            Error << " - " << details.resname[x] << " needed for production!";
                        else
                            Error << " - " << (nrequired - nres) << " more units of " << details.resname[x] << " needed for production at full capacity (" << ncanproduce << ")!";
                        Ok = false;
                    }
                }
            }
        }
    }

    return Ok;
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Produce(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    TProdDetails        details;
    const void        * value;
    EValueType          type;
    long                nlvl  = 0;
    long                nmen  = 0;
    std::string S, Product, Error;

    do
    {
        params  = ReadPropertyName(params, Product);
        pUnit->Flags        |= UNIT_FLAG_PRODUCING;
        pUnit->ProducingItem = gpDataHelper->ResolveAlias(Product.c_str());
        Product = pUnit->ProducingItem;
        gpDataHelper->GetProdDetails (Product.c_str(), details);

        if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)nmen, eNormal)  || (nmen<=0))
            SHOW_WARN_CONTINUE(" - There are no men in the unit!");

        if (details.skillname.empty() || details.months<=0)
            SHOW_WARN_CONTINUE(" - Production requirements for item '" << Product << "' are not configured! ");

        // check skill level
        S << details.skillname << PRP_SKILL_POSTFIX;
        if (pUnit->GetProperty(S.c_str(), type, value, eNormal) && (eLong==type) )
        {
            nlvl = (long)value;
            if (nlvl < details.skilllevel)
                SHOW_WARN_CONTINUE(" - Skill " << details.skillname << " level " << details.skilllevel << " is required for production");
        }
        else
            SHOW_WARN_CONTINUE(" - Skill " << details.skillname << " is required for production");

        // check required resources
        if (gpDataHelper->ImmediateProdCheck())
            if (!CheckResourcesForProduction(pUnit, pLand, Error))
                SHOW_WARN_CONTINUE(Error.c_str());

    } while (false);
}


//-------------------------------------------------------------

bool CAtlaParser::GetItemAndAmountForGive(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, std::string & Item, int & amount, const char * command, CUnit * pUnit2)
{
    bool                Ok = false;
    std::string S1;
    char                ch;
    long                item_avail=0;
    EValueType          type;

    // UNIT
    // 15 SILV
    // ALL SILV
    // ALL SILV EXCEPT 15
    do
    {
        Item.clear();
        amount = 0;

        params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
        params = SkipSpaces(GetToken(Item, params, " \t", ch, TRIM_ALL));

        if (0 != stricmp("UNIT", S1.c_str()))
        {
            if (0 == stricmp("TAKE", command))
            {
                pUnit2->GetProperty(Item.c_str(), type, (const void*&)item_avail, eNormal);
                if (eLong!=type)
                    SHOW_WARN_CONTINUE(" - Can not take that!");
            }
            else
            {
                if ( !pUnit->GetProperty(Item.c_str(), type, (const void*&)item_avail, eNormal) || (eLong!=type))
                {
                    SHOW_WARN_CONTINUE(" - Can not " << command << " that!");
                    break;
                }
            }
        }

        if (0 == stricmp("UNIT", S1.c_str()))
            Item = S1;
        else if (0 == stricmp("ALL", S1.c_str()))
        {
            amount = item_avail;
            params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
            if (0==stricmp("EXCEPT", S1.c_str()))
            {
                params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
                if (item_avail < atol(S1.c_str()))
                {
                    SHOW_WARN_CONTINUE(" - EXCEPT is too big. Use no more than " << item_avail << ".");
                    break;
                }
                amount -= atol(S1.c_str());
            }
        }
        else
            amount = atol(S1.c_str());

        if (amount < 0)
        {
            SHOW_WARN_CONTINUE(" - Can not " << command << " negative amount " << (long)amount);
            break;
        }

        if (item_avail < amount)
        {
            SHOW_WARN_CONTINUE(" - Too many. " << command << " " << item_avail << " at most.");
            break;
        }

        Ok = true;
    } while (false);

    return Ok;
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Withdraw(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    EValueType          type;
    const void        * value;

//    int               * weights;    // calculate weight change while giving
//    const char       ** movenames;
//    int                 movecount;

    std::string                Item, N;
    int                 amount;
    char                ch;

    do
    {
        // WITHDRAW 100 SILV
        params = GetToken(N, SkipSpaces(params), " \t", ch, TRIM_ALL);
        params = GetToken(Item, params, " \t", ch, TRIM_ALL);

        amount = atol(N.c_str()); // we allow negative amounts!

//        if ( gpDataHelper->(Item.c_str(), weights, movenames, movecount) )
//        {
            if (!pUnit->GetProperty(Item.c_str(), type, value, eNormal) )
            {
                type  = eLong;
                value = (const void*)0L;
                    // set original value to 0!
                if (PE_OK!=pUnit->SetProperty(Item.c_str(), type, value, eNormal))
                    SHOW_WARN_CONTINUE(NOSETUNIT << BUG);
            }
            else if (eLong!=type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << BUG);

            if (PE_OK!=pUnit->SetProperty(Item.c_str(), type, (const void*)((long)value+amount), eNormal))
                SHOW_WARN_CONTINUE(NOSETUNIT << BUG);

            pUnit->CalcWeightsAndMovement();
//            pUnit ->AddWeight(amount, weights, movenames, movecount);
//        }

    } while (false);

}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Give(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, bool IgnoreMissingTarget)
{
    EValueType          type;
    long                n1;
    CBaseObject         Dummy;
    int                 idx;
    CUnit             * pUnit2 = nullptr;
    const void        * value;
    const void        * value2;

//    int               * weights;    // calculate weight change while giving
//    const char       ** movenames;
//    int                 movecount;

    std::string                Item;
    int                 amount;

    do
    {
        // GIVE               15 UNIT
        // GIVE [NEW]         3  15 SILV
        // GIVE FACTION X NEW Y  N  Something
        // GIVE               15 ALL SILV EXCEPT 15 - SH-EXCPT
        //                    n1 n2 S1


        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
            SHOW_WARN_CONTINUE(" - Invalid unit id");
        if (n1==pUnit->Id)
            SHOW_WARN_CONTINUE(" - Giving to yourself");
        if (0!=n1)
        {
            Dummy.Id = n1;
            if (pLand->Units.Search(&Dummy, idx))
                pUnit2 = (CUnit*)pLand->Units.At(idx);
            else
                SHOW_WARN_CONTINUE(" - Can not locate target unit");
        }

        if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, Item, amount, "give", nullptr) )
        {
            // nullptr==pUnit2 is normal, always check!

            if (0==stricmp("UNIT", Item.c_str()))
            {
                if (pUnit2 && pUnit->FactionId==pUnit2->FactionId)
                    SHOW_WARN_CONTINUE(" - Target unit belongs to the same faction");
                break;
            }

            if (!pUnit->GetProperty(Item.c_str(), type, value, eNormal) || (eLong!=type))
                SHOW_WARN_CONTINUE(" - Can not give " << Item);

            if (PE_OK!=pUnit->SetProperty(Item.c_str(), type, (const void*)((long)value-amount), eNormal))
                SHOW_WARN_CONTINUE(NOSET << BUG);

            if (pUnit2)
            {
                if (!pUnit2->GetProperty(Item.c_str(), type, value2, eNormal) )
                {
                    value2 = (const void*)0L;
                    // set original value to 0!
                    if (PE_OK!=pUnit2->SetProperty(Item.c_str(), type, value2, eNormal))
                        SHOW_WARN_CONTINUE(NOSETUNIT << n1 << BUG);
                }
                else if (eLong!=type)
                    SHOW_WARN_CONTINUE(NOTNUMERIC << n1 << BUG);

                if (PE_OK!=pUnit2->SetProperty(Item.c_str(), type, (const void*)((long)value2+amount), eNormal))
                    SHOW_WARN_CONTINUE(NOSET << BUG);
                if (0==stricmp(PRP_SILVER, Item.c_str()))
                    pUnit2->SilvRcvd += amount;

                // check how giving men affects skills
                AdjustSkillsAfterGivingMen(pUnit, pUnit2, Item, amount);
            }

//            if ( gpDataHelper->GetItemWeights(Item.c_str(), weights, movenames, movecount) )
//            {
//                pUnit ->AddWeight(-amount, weights, movenames, movecount);
//                if (pUnit2)
//                    pUnit2->AddWeight( amount, weights, movenames, movecount);
//            }
            pUnit->CalcWeightsAndMovement();
            if (pUnit2)
                pUnit2->CalcWeightsAndMovement();
        }

    } while (false);
}



//-------------------------------------------------------------

bool CAtlaParser::FindTargetsForSend(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char *& params, CUnit *& pUnit2, CLand *& pLand2)
{
    bool                Ok = false;
    CBaseObject         Dummy;
    int                 idx;
    long                target_id;
    int                 X, Y, Z, X2, Y2, Z2, ID;
    int                 i;
    std::string S1;
    char                ch;

    /*
    SEND DIRECTION [dir] [quantity] [item]
    SEND DIRECTION [dir] UNIT [unit] [quantity] [item]
    SEND UNIT [unit] [quantity] [item]
    SEND UNIT [unit] ALL [item]
    SEND UNIT [unit] ALL [item] EXCEPT [quantity]
    SEND UNIT [unit] ALL [item class]
    */
    do
    {
        params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
        if (0==stricmp("DIRECTION", S1.c_str()))
        {
            pUnit2 = nullptr;
            params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
            LandIdToCoord(pLand->Id, X, Y, Z);

            for (i=0; i<(int)sizeof(Directions)/(int)sizeof(const char*); i++)
                if (0==stricmp(S1.c_str(), Directions[i]))
                {
                    switch (i%6)
                    {
                    case North     : Y -= 2;     break;
                    case Northeast : Y--; X++;   break;
                    case Southeast : Y++; X++;   break;
                    case South     : Y += 2;     break;
                    case Southwest : Y++; X--;   break;
                    case Northwest : Y--; X--;   break;
                    }

                    if (pLand->pPlane->Width > 0)
                    {
                        if (X>pLand->pPlane->EastEdge)
                            X = pLand->pPlane->WestEdge;
                        else
                            if (X<pLand->pPlane->WestEdge)
                                X = pLand->pPlane->EastEdge;
                    }

                    ID = LandCoordToId(X,Y, pLand->pPlane->Id);
                    pLand2 = GetLand(ID);

                    break;
                }
            if (!pLand2)
                SHOW_WARN_CONTINUE(" - Can not find land in given direction");

            if (0==strnicmp("UNIT", params, 4))
            {
                params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
                if (!GetTargetUnitId(params, pUnit->FactionId, target_id))
                    SHOW_WARN_CONTINUE(" - Invalid unit Id");
                if (target_id==pUnit->Id)
                    SHOW_WARN_CONTINUE(" - Giving to yourself");
                if (0 == target_id)
                    SHOW_WARN_CONTINUE(" - Invalid target unit");
                Dummy.Id = target_id;
                if (pLand2->Units.Search(&Dummy, idx))
                    pUnit2 = (CUnit*)m_Units.At(idx);
                if (!pUnit2)
                    SHOW_WARN_CONTINUE(" - Invalid target unit");
            }
        }
        else if (0==stricmp("UNIT", S1.c_str()))
        {
            if (!GetTargetUnitId(params, pUnit->FactionId, target_id))
                SHOW_WARN_CONTINUE(" - Invalid unit Id");
            if (target_id==pUnit->Id)
                SHOW_WARN_CONTINUE(" - Giving to yourself");
            if (0 == target_id)
                SHOW_WARN_CONTINUE(" - Invalid target unit");

            Dummy.Id = target_id;
            if (m_Units.Search(&Dummy, idx))
                pUnit2 = (CUnit*)m_Units.At(idx);
            if (!pUnit2)
                SHOW_WARN_CONTINUE(" - Invalid target unit");
            if (pUnit2)
                pLand2 = GetLand(pUnit2->LandId);
            if (pLand2)
            {
                //is it a neighbouring hex?
                LandIdToCoord(pLand->Id, X, Y, Z);
                LandIdToCoord(pLand2->Id, X2, Y2, Z2);
                if (Z!=Z2)
                    SHOW_WARN_CONTINUE(" - Target land on different plane");
                if (abs(Y-Y2)>2)
                    SHOW_WARN_CONTINUE(" - Target land is too far away");
                if (abs(X-X2)>1)
                {
                    // could be overlapping, check for it
                    if (pLand->pPlane->Width > 0)
                    {
                        if (X2 < X)
                        {
                            // X will be less
                            Z  = X2;
                            X2 = X;
                            X  = Z;
                        }
                        X += pLand->pPlane->Width;
                        if (abs(X-X2)>1)
                            SHOW_WARN_CONTINUE(" - Target land is too far away");
                    }
                    else
                        SHOW_WARN_CONTINUE(" - Target land is too far away");
                }
            }
        }
        else
            SHOW_WARN_CONTINUE(" - Invalid SEND command");

        Ok = true;
    } while (false);

    return Ok;
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Take(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, bool IgnoreMissingTarget)
{
    EValueType          type;
    long                n1;
    CBaseObject         Dummy;
    int                 idx;
    CUnit             * pUnit2 = nullptr;
    const void        * value;
    const void        * value2;

//    int               * weights;    // calculate weight change while giving
//    const char       ** movenames;
//    int                 movecount;

    std::string                Item;
    int                 amount;
    char                ch;

    do
    {
        // TAKE FROM <TARGET> (<AMOUNT>|ALL) <ITEM> [EXCEPT <AMOUNT>]
        // Where:
        //   <TARGET> is
        //     <UNIT ID>                          existing unit
        //     NEW <UNIT ID>                      new own unit
        //     FACTION <FACTION ID> NEW <UNIT ID> other faction new unit

        // Remove FROM token from the params
        params = SkipSpaces(GetToken(Item, params, " \t", ch, TRIM_ALL));

        if (0!=stricmp("FROM", Item.c_str()))
            SHOW_WARN_CONTINUE(" - Invalid TARGET command");

        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
            SHOW_WARN_CONTINUE(" - Invalid unit id");
        if (n1==pUnit->Id)
            SHOW_WARN_CONTINUE(" - Taking from yourself");
        if (0!=n1)
        {
            Dummy.Id = n1;
            if (pLand->Units.Search(&Dummy, idx))
                pUnit2 = (CUnit*)pLand->Units.At(idx);
            else
                SHOW_WARN_CONTINUE(" - Can not locate target unit");
        }
        else
            SHOW_WARN_CONTINUE(" - Invalid unit id");
         
        if (pUnit2 && pUnit->FactionId!=pUnit2->FactionId)
            SHOW_WARN_CONTINUE(" - Target unit must belong to the same faction");

        if (pUnit2)
        {
            if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, Item, amount, "take", pUnit2) )
            {
                if (0==stricmp("UNIT", Item.c_str()))
                    SHOW_WARN_CONTINUE(" - Taking unit is not allowed");

                if (!pUnit2->GetProperty(Item.c_str(), type, value, eNormal) || (eLong!=type))
                    SHOW_WARN_CONTINUE(" - Can not take " << Item);

                if (PE_OK!=pUnit2->SetProperty(Item.c_str(), type, (const void*)((long)value-amount), eNormal))
                    SHOW_WARN_CONTINUE(NOSET << BUG);

                if (!pUnit->GetProperty(Item.c_str(), type, value2, eNormal) )
                {
                    value2 = (const void*)0L;
                    // set original value to 0!
                    if (PE_OK!=pUnit->SetProperty(Item.c_str(), type, value2, eNormal))
                        SHOW_WARN_CONTINUE(NOSETUNIT << n1 << BUG);
                }
                else if (eLong!=type)
                    SHOW_WARN_CONTINUE(NOTNUMERIC << n1 << BUG);

                if (PE_OK!=pUnit->SetProperty(Item.c_str(), type, (const void*)((long)value2+amount), eNormal))
                    SHOW_WARN_CONTINUE(NOSET << BUG);
                if (0==stricmp(PRP_SILVER, Item.c_str()))
                    pUnit->SilvRcvd += amount;

                // check how giving men affects skills
                AdjustSkillsAfterGivingMen(pUnit2, pUnit, Item, amount);

                pUnit->CalcWeightsAndMovement();
                pUnit2->CalcWeightsAndMovement();
            }
        }

    } while (false);
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Send(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    CUnit             * pUnit2 = nullptr;
    CLand             * pLand2 = nullptr;
    std::string                Item;
    int                 amount, effweight;
    int                 price = 0;
    int                 WeatherMultiplier;
    long                unitmoney;

    EValueType          type;
    const void        * value;

    int               * weights;    // calculate weight change while giving
    const char       ** movenames;
    int                 movecount;


    /*
    SEND DIRECTION [dir] [quantity] [item]
    SEND DIRECTION [dir] UNIT [unit] [quantity] [item]
    SEND UNIT [unit] [quantity] [item]
    SEND UNIT [unit] ALL [item]
    SEND UNIT [unit] ALL [item] EXCEPT [quantity]
    SEND UNIT [unit] ALL [item class]
    */
    do
    {

        // Find the target unit and land first
        if (!FindTargetsForSend(Line, ErrorLine, skiperror, pUnit, pLand, params, pUnit2, pLand2))
            return;

        if (!pLand2)
            SHOW_WARN_CONTINUE(" - Unable to locate target hex");

        if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, Item, amount, "send", nullptr) )
        {
            if (!pUnit->GetProperty(Item.c_str(), type, value, eNormal) || (eLong!=type))
                SHOW_WARN_CONTINUE(" - Can not send " << Item);

            if (PE_OK!=pUnit->SetProperty(Item.c_str(), type, (const void*)((long)value-amount), eNormal))
                SHOW_WARN_CONTINUE(NOSET << BUG);

            if ( gpDataHelper->GetItemWeights(Item.c_str(), weights, movenames, movecount) )
            {

                // now we must pay the terrible price!
                WeatherMultiplier = 2;
                if (pLand2 && pLand2->WeatherWillBeGood)
                    WeatherMultiplier = 1;

                effweight = weights[0] - weights[1];
                if (effweight<0)
                    effweight = 0;
                price  = (int) (amount * floor(sqrt((double)effweight)) * WeatherMultiplier);

                if (!pUnit->GetProperty(PRP_SILVER, type, (const void *&)unitmoney, eNormal) )
                {
                    unitmoney = 0;
                    if (PE_OK!=pUnit->SetProperty(PRP_SILVER, type, (const void*)unitmoney, eBoth))
                        SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                }
                else if (eLong!=type)
                    SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

                unitmoney -= price;
                if  (PE_OK!=pUnit->SetProperty(PRP_SILVER,   type, (const void *)unitmoney, eNormal))
                    SHOW_WARN_CONTINUE(NOSET << BUG);
                pUnit->CalcWeightsAndMovement();
//                pUnit ->AddWeight(-amount, weights, movenames, movecount);
            }
        }

    } while (false);
}


//-------------------------------------------------------------

void CAtlaParser::AdjustSkillsAfterGivingMen(CUnit * pUnitGive, CUnit * pUnitTake, std::string & item, long AmountGiven)
{
    int                 idx;
    const char        * propname_days;
    std::set<std::string, CaseInsensitiveLess> SkillNames;
    int                 postlen;
    std::string                S, BasePropName, Prop, PropDays, PropStudy;
    CUnit             * tmpunits[2] = {pUnitGive, pUnitTake};
    int                 i;
    char              * p;
    long                dayssrc, daystarg, newdays, newskill, mentarg, newdaysexp, newskillexp;
    EValueType          type;
    bool                bWasSetGive, bWasSetTake;
    const void        * valueGive = nullptr;
    const void        * valueTake = nullptr;

    if (!gpDataHelper->IsMan(item.c_str()))
        return;

    if ( !pUnitTake->GetProperty(PRP_MEN, type, (const void*&)mentarg, eNormal) || eLong!=type || mentarg <= 0)
    {
        LOG_ERR(ERR_UNKNOWN, "There must be men in the unit!");
        return;
    }

    // Adjust leadership here so we do not need to make another func

    if (pUnitGive->GetProperty(PRP_LEADER, type, valueGive, eNormal) && eCharPtr!=type)
    {
        S = "Wrong property type ";  S << BUG;
        OrderErr(1, pUnitGive->Id,  S.c_str());
        return;
    }
    if (!pUnitTake->GetProperty(PRP_LEADER, type, valueTake, eNormal))
    {
        pUnitTake->SetProperty(PRP_LEADER, eCharPtr, "", eBoth);
    }
    else if (eCharPtr!=type)
    {
        S = "Wrong property type ";  S << BUG;
        OrderErr(1, pUnitTake->Id,  S.c_str());
        return;
    }

    if (valueGive && !valueTake && 0==mentarg-AmountGiven)
    {
        // giving to new unit
        pUnitTake->SetProperty(PRP_LEADER, eCharPtr, valueGive, eNormal);
    }
    // else we do not give a damn!




    // make big list of skills
    postlen  = strlen(PRP_SKILL_DAYS_POSTFIX);
    for (i=0; i<2; i++)
    {
        idx      = 0;
        propname_days = tmpunits[i]->GetPropertyName(idx);
        while (propname_days)
        {
            S = propname_days;
            if (FindSubStrR(S, PRP_SKILL_DAYS_POSTFIX) == S.size()-postlen)
            {
                SkillNames.insert(propname_days);
            }
            propname_days = tmpunits[i]->GetPropertyName(++idx);
        }
    }

    // now handle each skill
    for (const auto& skillNameStr : SkillNames)
    {
        propname_days = skillNameStr.c_str();
        BasePropName = propname_days;
        DelSubStr(BasePropName, BasePropName.size()-postlen, postlen);


        // Generic skill properties

        Prop     = BasePropName;  Prop     << PRP_SKILL_POSTFIX;
        PropDays = BasePropName;  PropDays << PRP_SKILL_DAYS_POSTFIX;
        if ( !pUnitGive->GetProperty(PropDays.c_str(), type, (const void*&)dayssrc, eNormal) )
            dayssrc = 0;
        if ( !pUnitTake->GetProperty(PropDays.c_str(), type, (const void*&)daystarg, eNormal) )
        {
            pUnitTake->SetProperty(PropDays.c_str(), eLong, (const void*)0L, eNormal);
            pUnitTake->SetProperty(Prop.c_str()    , eLong, (const void*)0L, eNormal);
            daystarg = 0;
        }

        newdays  = (daystarg*(mentarg-AmountGiven) + dayssrc*AmountGiven)/mentarg;
        newskill = SkillDaysToLevel(newdays);

        pUnitTake->SetProperty(PropDays.c_str() , eLong, (const void*)newdays , eNormal);
        pUnitTake->SetProperty(Prop.c_str()     , eLong, (const void*)newskill, eNormal);

        // Arcadia skill properties

        Prop      = BasePropName;  Prop      << PRP_SKILL_EXPERIENCE_POSTFIX;
        PropDays  = BasePropName;  PropDays  << PRP_SKILL_DAYS_EXPERIENCE_POSTFIX;
        PropStudy = BasePropName;  PropStudy << PRP_SKILL_STUDY_POSTFIX;

        bWasSetGive = pUnitGive->GetProperty(PropDays.c_str(), type, (const void*&)dayssrc, eNormal);
        bWasSetTake = pUnitTake->GetProperty(PropDays.c_str(), type, (const void*&)daystarg, eNormal);

        if (bWasSetGive || bWasSetTake)
        {
            // It is Arcadia indeed!

            PropStudy = BasePropName; PropStudy << PRP_SKILL_STUDY_POSTFIX;

            if ( !bWasSetGive )
                dayssrc = 0;
            if ( !bWasSetTake )
            {
                pUnitTake->SetProperty(PropDays.c_str() , eLong, (const void*)0L, eNormal);
                pUnitTake->SetProperty(Prop.c_str()     , eLong, (const void*)0L, eNormal);
                pUnitTake->SetProperty(PropStudy.c_str(), eLong, (const void*)0L, eNormal);
                daystarg = 0;
            }
            pUnitTake->SetProperty(PropStudy.c_str(), eLong, (const void*)newskill, eNormal);  //PRP_SKILL_STUDY_POSTFIX;

            newdaysexp  = (daystarg*(mentarg-AmountGiven) + dayssrc*AmountGiven)/mentarg;
            newskillexp = SkillDaysToLevel(newdaysexp);

            pUnitTake->SetProperty(PropDays.c_str() , eLong, (const void*)newdaysexp , eNormal);  //PRP_SKILL_DAYS_EXPERIENCE_POSTFIX
            pUnitTake->SetProperty(Prop.c_str()     , eLong, (const void*)newskillexp, eNormal);  //PRP_SKILL_EXPERIENCE_POSTFIX

            newskill = newskill + newskillexp;
            Prop     = BasePropName;  Prop     << PRP_SKILL_POSTFIX;
            pUnitTake->SetProperty(Prop.c_str()     , eLong, (const void*)newskill, eNormal);  //PRP_SKILL_POSTFIX
        }
    }


    SkillNames.clear();
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Buy(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    EValueType          type;
    std::string S1;
    std::string N1;
    char                ch;
    long                n1;
    long                peritem;
    long                unitmoney;
    long                unitprop;
    std::string LandProp;
    long                landprop; // Shar1 Extrict SELL/BUY check
    CUnit               DummyGiver;

//    int               * weights;    // calculate weight change while giving
//    const char       ** movenames;
//    int                 movecount;

    do
    {
        // BUY 33 VIKI
        //     n1 S1
        params = SkipSpaces(GetToken(N1, params, " \t", ch, TRIM_ALL));
        //params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
        params = ReadPropertyName(params, S1);
        n1= atol(N1.c_str());
        if (S1.empty() || (n1<=0 && 0!=stricmp(N1.c_str(), "ALL")) )
            SHOW_WARN_CONTINUE(" - Invalid BUY command");

        // buying peasants requires substituting the alias by the real type of men.
        if (0==stricmp(S1.c_str(), "peas") || 0==stricmp(S1.c_str(), "peasant")
            || 0==stricmp(S1.c_str(), "peasants"))
            if (!pLand->PeasantRace.empty())
                ReadPropertyName(pLand->PeasantRace.c_str(), S1);

        MakeQualifiedPropertyName(PRP_SALE_PRICE_PREFIX, S1.c_str(), LandProp);
        if ( pLand->GetProperty(LandProp.c_str(), type, (const void *&)peritem, eNormal) && (eLong==type))
        {
            if (0==stricmp(N1.c_str(), "ALL"))
            {
                MakeQualifiedPropertyName(PRP_SALE_AMOUNT_PREFIX, S1.c_str(), LandProp);
                if ( !pLand->GetProperty(LandProp.c_str(), type, (const void *&)n1, eNormal) || (eLong!=type))
                    n1=0;
            }

            if (!pUnit->GetProperty(S1.c_str(), type, (const void *&)unitprop, eNormal) )
            {
                unitprop = 0;
                if (PE_OK!=pUnit->SetProperty(S1.c_str(), type, (const void *)unitprop, eBoth))
                    SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
            }
            else if (eLong!=type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

            if (!pUnit->GetProperty(PRP_SILVER, type, (const void *&)unitmoney, eNormal) )
            {
                unitmoney = 0;
                if (PE_OK!=pUnit->SetProperty(PRP_SILVER, type, (const void*)unitmoney, eBoth))
                    SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
            }
            else if (eLong!=type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

            // Shar1 Extrict SELL/BUY check. Start

            // Checking amount
            MakeQualifiedPropertyName(PRP_SALE_AMOUNT_PREFIX, S1.c_str(), LandProp);
            if (!pLand->GetProperty(LandProp.c_str(), type, (const void *&) landprop, eNormal) || (eLong!=type))
                landprop = 0;
            if (n1 > landprop)
            {
                SHOW_WARN_CONTINUE(" - That is too MANY! Buy " << (long) landprop << " at max.");
                n1 = landprop;
            }
            unitmoney -= n1*peritem; // This is the old code
            unitprop  += n1;         // This is the old code
            landprop  -= n1;

            if ( (PE_OK!=pUnit->SetProperty(S1.c_str(), type, (const void *)unitprop,  eNormal)) || // This is the old code
                 (PE_OK!=pUnit->SetProperty(PRP_SILVER,   type, (const void *)unitmoney, eNormal)) || // This is ALMOST the old code
                 (PE_OK!=pLand->SetProperty(LandProp.c_str(), type, (const void *)landprop,  eNormal)))
            // Shar1 End
                SHOW_WARN_CONTINUE(NOSET << BUG);

            if (gpDataHelper->IsTradeItem(S1.c_str()))
                pUnit->Flags |= UNIT_FLAG_PRODUCING;

            // adjust weight
//            if (gpDataHelper->GetItemWeights(S1.c_str(), weights, movenames, movecount))
//                pUnit ->AddWeight(n1, weights, movenames, movecount);
            pUnit->CalcWeightsAndMovement();

            AdjustSkillsAfterGivingMen(&DummyGiver, pUnit, S1, n1);
        }
        else
            SHOW_WARN_CONTINUE(" - Can not BUY that!");
    } while (false);

}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Sell(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    EValueType          type;
    EValueType          type1;
    std::string S1;
    std::string N1;
    char                ch;
    long                n1 = 0;
    long                peritem = 0;
    long                unitmoney = 0;
    long                unitprop = 0;
    std::string LandProp;
    long                landprop = 0; // Shar1 Extrict SELL/BUY check

//    int               * weights;    // calculate weight change while giving
//    const char       ** movenames;
//    int                 movecount;

    do
    {
         // BUY 33 VIKI
         //     n1 S1
         params = SkipSpaces(GetToken(N1, params, " \t", ch, TRIM_ALL));
         //params = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
         params = ReadPropertyName(params, S1);
         n1= atol(N1.c_str());
         if (S1.empty() || (n1<=0 && 0!=stricmp(N1.c_str(), "ALL")) )
             SHOW_WARN_CONTINUE(" - Invalid SELL command");

         MakeQualifiedPropertyName(PRP_WANTED_PRICE_PREFIX, S1.c_str(), LandProp);
         if ( !pLand->GetProperty(LandProp.c_str(), type,  (const void *&)peritem,  eNormal) ||
              !pUnit->GetProperty(S1.c_str(),       type1, (const void *&)unitprop, eNormal) ||
              (eLong!=type) || (eLong!=type1)
              )
             SHOW_WARN_CONTINUE(" - Can not SELL that!");
         if (0==stricmp(N1.c_str(), "ALL"))
             n1=unitprop;
         if (!pUnit->GetProperty(PRP_SILVER, type, (const void *&)unitmoney, eNormal) )
         {
             unitmoney = 0;
             if (PE_OK!=pUnit->SetProperty(PRP_SILVER, type, (const void*)unitmoney, eBoth))
                 SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
         }
         else if (eLong!=type)
             SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

         // Shar1 Extrict SELL/BUY check. Start
         // Check amount wanted by market
         MakeQualifiedPropertyName(PRP_WANTED_AMOUNT_PREFIX, S1.c_str(), LandProp);
         if (!pLand->GetProperty(LandProp.c_str(), type,  (const void *&) landprop,  eNormal) || (eLong!=type))
         {
             SHOW_WARN_CONTINUE(" - Can not SELL that!");
             landprop = 0;
             n1 = 0;
         }
         if (n1 > landprop)
         {
             SHOW_WARN_CONTINUE(" - Selling beyond market's capacity! Sell " << (long) landprop << " at max.");
             n1 = landprop;
         }
         // Check amount owned by unit
         if (n1 > unitprop)
         {
             SHOW_WARN_CONTINUE(" - That is too MANY! Sell " << (long) unitprop << " at max.");
             n1 = unitprop;
         }
         unitmoney += n1*peritem; // This is the old code
         unitprop  -= n1; // This is the old code
         landprop  -= n1;

         if ( (PE_OK!=pUnit->SetProperty(S1.c_str(), type, (const void *)unitprop,  eNormal)) || // This is the old code
              (PE_OK!=pUnit->SetProperty(PRP_SILVER,   type, (const void *)unitmoney, eNormal)) || // This is ALMOST the old code
              (PE_OK!=pLand->SetProperty(LandProp.c_str()  ,   type, (const void *)landprop,  eNormal)))
         // Shar1. End
             SHOW_WARN_CONTINUE(NOSET << BUG);

         pUnit->SilvRcvd += n1*peritem;

         // adjust weight
//         if (gpDataHelper->GetItemWeights(S1.c_str(), weights, movenames, movecount))
//             pUnit ->AddWeight(-n1, weights, movenames, movecount);
         pUnit->CalcWeightsAndMovement(); 
    } while (false);

}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Promote(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    long                n1;
    long                id1=0, id2=0;
    int                 idx;
    CBaseObject         Dummy;
    CUnit             * pUnit2;
    const char        * p1;
    EValueType          type;
    CStruct           * pStruct;

    do
    {
        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
            SHOW_WARN_CONTINUE(" - Invalid unit Id");

        Dummy.Id = n1;
        if (pLand->Units.Search(&Dummy, idx))
        {
            pUnit2 = (CUnit*)pLand->Units.At(idx);
            params      = nullptr;
            p1     = nullptr;

            if (!pUnit->GetProperty(PRP_STRUCT_ID, type, (const void *&)id1, eNormal) )
                SHOW_WARN_CONTINUE(" - The unit is not inside a struct");
            if (eLong!=type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG);

            if (!pUnit2->GetProperty(PRP_STRUCT_ID, type, (const void *&)id2, eNormal) )
                SHOW_WARN_CONTINUE(" - The unit is not inside a struct");
            if (eLong!=type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit2->Id << BUG);

            if (id1 != id2)
                SHOW_WARN_CONTINUE(" - Units are not in the same struct");

            if   (PE_OK!=pUnit ->SetProperty(PRP_STRUCT_OWNER, eCharPtr, "",  eNormal))
                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);

            if ( (PE_OK!=pUnit2->SetProperty(PRP_STRUCT_OWNER, eCharPtr, "",  eNormal)) ||
                 (PE_OK!=pUnit2->SetProperty(PRP_STRUCT_OWNER, eCharPtr, YES, eNormal)) )
                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit2->Id << BUG);

            pStruct = pLand->GetStructById(id1);
            if (pStruct)
                pStruct->OwnerUnitId = pUnit2->Id;
        }
        else
            SHOW_WARN_CONTINUE(" - Can not find unit " << n1)
    } while (false);
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Move(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, int & X, int & Y, int & LocA3, long order)
{
    int                 ID;
    int                 i, idx=0;
    std::string                S1;
    char                ch;
    EValueType          type;
    long                n1;
    CStruct           * pStruct;
    std::string sErr, S;
    CBaseObject         Dummy;
    long                skill, nmen, structid;

    do
    {
        // should we do Arcadia III handling?
        if (O_SAIL == order &&
            pUnit->GetProperty(PRP_STRUCT_ID, type, (const void *&)n1) && eLong==type)
        {
            pStruct  = pLand->GetStructById(n1);
            if (pStruct && NO_LOCATION != pStruct->Location)
            {
                // here we go!
                if (NO_LOCATION == LocA3)
                    LocA3 = pStruct->Location; // init it here
                RunOrder_SailAIII(Line, ErrorLine, skiperror, pUnit, pLand, params, X, Y, LocA3);
                //return;
                goto SomeChecks;
            }
        }

        while (params)
        {
            params        = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
            for (i=0; i<(int)sizeof(Directions)/(int)sizeof(const char*); i++)
                if (0==stricmp(S1.c_str(), Directions[i]))
                {
                    switch (i%6)
                    {
                    case North     : Y -= 2;     break;
                    case Northeast : Y--; X++;   break;
                    case Southeast : Y++; X++;   break;
                    case South     : Y += 2;     break;
                    case Southwest : Y++; X--;   break;
                    case Northwest : Y--; X--;   break;
                    }

                    if (pLand->pPlane->Width > 0)
                    {
                        if (X>pLand->pPlane->EastEdge)
                            X = pLand->pPlane->WestEdge;
                        else
                            if (X<pLand->pPlane->WestEdge)
                                X = pLand->pPlane->EastEdge;
                    }

                    ID = LandCoordToId(X,Y, pLand->pPlane->Id);
                    if (!pUnit->pMovement)
                        pUnit->pMovement = std::make_unique<std::vector<long>>();
                    pUnit->pMovement->push_back(ID);

                    break;
                }
        }

    SomeChecks:
        if (O_SAIL == order)
        {
            // ship's sailing power
            if (!pUnit->GetProperty(PRP_STRUCT_ID, type, (const void *&)structid, eNormal) || (eLong!=type))
                SHOW_WARN_CONTINUE(" - Must be in a ship to issue SAIL order!");

            if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)nmen, eNormal) || (eLong!=type) || (nmen<=0))
                SHOW_WARN_CONTINUE(" - There are no men in the unit!");

            S = "SAIL"; S << PRP_SKILL_POSTFIX;
            if (!pUnit->GetProperty(S.c_str(), type, (const void *&)skill, eNormal) || (eLong!=type) || (skill<=0))
                SHOW_WARN_CONTINUE(" - Needs SAIL skill!");

            Dummy.Id = structid;
            if (!pLand->Structs.Search(&Dummy, idx))
                SHOW_WARN_CONTINUE(" - Invalid Struct Id " << BUG);

            pStruct = (CStruct*)pLand->Structs.At(idx);
            if (pStruct)
                pStruct->SailingPower += (nmen*skill);
        }
        else
        {
            if (gpDataHelper->ShowMoveWarnings())
            {
                pUnit->CheckWeight(sErr);
                if (!sErr.empty())
                    SHOW_WARN_CONTINUE(sErr);
                    //OrderErr(1, pUnit->Id, sErr.c_str());
            }
        }
    } while (false);
}

//-------------------------------------------------------------


    static struct
    {
        eDirection   Location;
        eDirection   Direction;
        int          dX;
        int          dY;
        eDirection   TargetLoc;
    } NextLandLoc[] =
    {
        { North    , North     ,  0, -2, South     },
        { North    , Northeast ,  1, -1, Northwest },
        { North    , Southeast ,  0,  0, Northeast },
        { North    , Southwest ,  0,  0, Northwest },
        { North    , Northwest , -1, -1, Northeast },

        { Northeast, North     ,  0, -2, Southeast },
        { Northeast, Northeast ,  1, -1, Southwest },
        { Northeast, Southeast ,  1,  1, North     },
        { Northeast, South     ,  0,  0, Southeast },
        { Northeast, Northwest ,  0,  0, North     },

        { Southeast, North     ,  0,  0, Northeast },
        { Southeast, Northeast ,  1, -1, South     },
        { Southeast, Southeast ,  1,  1, Northwest },
        { Southeast, South     ,  0,  2, Northeast },
        { Southeast, Southwest ,  0,  0, South     },

        { South    , Northeast ,  0,  0, Southeast },
        { South    , Southeast ,  1,  1, Southwest },
        { South    , South     ,  0,  2, North     },
        { South    , Southwest , -1,  1, Southeast },
        { South    , Northwest ,  0,  0, Southwest },

        { Southwest, North     ,  0,  0, Northwest },
        { Southwest, Southeast ,  0,  0, South     },
        { Southwest, South     ,  0,  2, Northwest },
        { Southwest, Southwest , -1,  1, Northeast },
        { Southwest, Northwest , -1, -1, South     },

        { Northwest, North     ,  0, -2, Southwest },
        { Northwest, Northeast ,  0,  0, North     },
        { Northwest, South     ,  0,  0, Southwest },
        { Northwest, Southwest , -1,  1, North     },
        { Northwest, Northwest , -1, -1, Southeast },

        { Center   , North     ,  0, -2, South     },
        { Center   , Northeast ,  1, -1, Southwest },
        { Center   , Southeast ,  1,  1, Northwest },
        { Center   , South     ,  0,  2, North     },
        { Center   , Southwest , -1,  1, Northeast },
        { Center   , Northwest , -1, -1, Southeast }
    };


// Special SAILing for Arcadia III.

void CAtlaParser::RunOrder_SailAIII(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, int & X, int & Y, int & LocA3)
{
    int                 ID;
    int                 i, j;
    std::string                S1;
    char                ch;
    eDirection          Dir;
    bool                GoodSail;
    std::string                SailPassed;
    CLand             * pNewLand;


    while (params && *params)
    {
        GoodSail = false;

        params        = SkipSpaces(GetToken(S1, params, " \t", ch, TRIM_ALL));
        for (i=0; i<(int)sizeof(Directions)/(int)sizeof(const char*); i++)
            if (0==stricmp(S1.c_str(), Directions[i]))
            {
                Dir = (eDirection)(i%6);

                for (j=0; j<(int)sizeof(NextLandLoc)/(int)sizeof(*NextLandLoc); j++)
                    if ( NextLandLoc[j].Location == LocA3 && NextLandLoc[j].Direction == Dir )
                    {
                        X += NextLandLoc[j].dX;
                        Y += NextLandLoc[j].dY;
                        LocA3 = NextLandLoc[j].TargetLoc;
                        GoodSail = true;
                        break;
                    }

                if (!GoodSail)
                    break;


                if (pLand->pPlane->Width > 0)
                {
                    if (X>pLand->pPlane->EastEdge)
                        X = pLand->pPlane->WestEdge;
                    else
                        if (X<pLand->pPlane->WestEdge)
                            X = pLand->pPlane->EastEdge;
                }

                ID = LandCoordToId(X,Y, pLand->pPlane->Id);

                pNewLand = GetLand(ID);
                if (!pNewLand || (pNewLand && 0 == stricmp("Ocean", pNewLand->TerrainType.c_str()) ||
             0 == stricmp("Lake", pNewLand->TerrainType.c_str()) ) )
                    LocA3 = Center;

                if (!pUnit->pMovement)
                    pUnit->pMovement = std::make_unique<std::vector<long>>();
                pUnit->pMovement->push_back(ID);

                if (!pUnit->pMoveA3Points)
                    pUnit->pMoveA3Points = std::make_unique<std::vector<long>>();
                pUnit->pMoveA3Points->push_back(LocA3);

                break;
            }

        if (!GoodSail)
            SHOW_WARN_CONTINUE(" - Can not sail " << SailPassed << S1);
        SailPassed << S1 << ' ';
    }
}

//-------------------------------------------------------------

void CAtlaParser::RunOrder_Teach(std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, bool TeachCheckGlb)
{
    EValueType          type;
    long                n1, n2;
    int                 idx;
    CUnit             * pUnit2;
    CBaseObject         Dummy;
    bool                our_unit;
    std::string                Skill;
    const void        * value;
    bool                leader_checked = false;


    while (params && *params)
    {
        if (!leader_checked && !pUnit->GetProperty(PRP_LEADER, type, value, eNormal) )
        {
            SHOW_WARN_BREAK(" - Unit " << pUnit->Id << " is not a leader or hero");
        }
        else
            leader_checked = true;


        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
            SHOW_WARN_CONTINUE(" - Invalid unit Id");
        if (0==n1)
            continue;
        Dummy.Id = n1;
        if (pLand->Units.Search(&Dummy, idx))
        {
            pUnit2 = (CUnit*)pLand->Units.At(idx);

            our_unit = pUnit2->IsOurs;

            if (pUnit2->StudyingSkill.empty() && our_unit)
            {
                SHOW_WARN_CONTINUE(" - Unit " << pUnit2->Id << " is not studying");
            }
            else
            {
                if (our_unit  && TeachCheckGlb)
                {
                    // are the levels ok?
                    Skill = pUnit2->StudyingSkill;
                    if (!pUnit2->GetProperty(Skill.c_str(), type, (const void *&)n2, eNormal) )
                    {
                        n2 = 0;
                        type = eLong;
                    }
                    if (eLong!=type)
                        SHOW_WARN_CONTINUE(NOTNUMERIC << n2 << BUG);

                    if (!pUnit->GetProperty(Skill.c_str(), type, (const void *&)n1, eNormal))
                    {
                        n1 = 0;
                        type = eLong;
                    }
                    if (eLong!=type)
                        SHOW_WARN_CONTINUE(NOTNUMERIC << n2 << BUG);

                    if (n1<=n2)
                    {
                        // can not teach in the normal game, but try for Arcadia III
                        int  SkillPos = FindSubStrR(Skill, PRP_SKILL_POSTFIX);
                        if (SkillPos>=0)
                            DelSubStr(Skill, SkillPos, strlen(PRP_SKILL_POSTFIX));

                        Skill << PRP_SKILL_STUDY_POSTFIX;
                        if (!pUnit->GetProperty(Skill.c_str(), type, (const void *&)n1, eNormal) )
                            n1 = 0;
                        if (!pUnit2->GetProperty(Skill.c_str(), type, (const void *&)n2, eNormal) )
                            n2 = 0;

                    }
                    if (n1<=n2)
                        SHOW_WARN_CONTINUE(" - Can not teach unit " << pUnit2->Id);
                }

                // levels are ok, use n1 and n2 for men counts
                if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)n1, eNormal)  || (n1<=0))
                    SHOW_WARN_CONTINUE(" - There are no men in the unit!");

                if (!pUnit2->GetProperty(PRP_MEN, type, (const void *&)n2, eNormal) || (n2<=0))
                    SHOW_WARN_CONTINUE(" - There are no men in the student unit!");

                if (!pUnit->pStudents)
                    pUnit->pStudents = std::make_unique<CBaseCollById>();

                if (!pUnit->pStudents->Insert(pUnit2))
                    SHOW_WARN_CONTINUE(" - Unit " << pUnit2->Id << " is already in the students list");

            }


        }
        else
            SHOW_WARN_CONTINUE(" - Can not find unit " << n1)
    }

}

//-------------------------------------------------------------

void CAtlaParser::OrderProcess_Teach(bool skiperror, CUnit * pUnit)
{
    long          nstud = 0; // students count
    long          n1, n2;
    CUnit       * pUnit2;
    double        teach;
    std::string ErrorLine;
    std::string          Line;
    EValueType    type;
    int           i;
    const char  * leadership;


    do
    {
        if ( pUnit->pStudents && (pUnit->pStudents->Count() > 0)  )
        {
            if (!pUnit->StudyingSkill.empty())
            {
                bool ItsOk = false;
                // is it a freaking hero?
                if (m_ArcadiaSkills &&
                    pUnit->GetProperty(PRP_LEADER, type, (const void *&)leadership, eNormal) &&
                    eCharPtr==type &&
                    0 == stricmp(leadership, SZ_HERO))
                    ItsOk = true;

                if (!ItsOk)
                    SHOW_WARN_CONTINUE(" - can not teach and study at the same time");
            }

            for (i=0; i<pUnit->pStudents->Count(); i++)
            {

                pUnit2 = (CUnit*)pUnit->pStudents->At(i);
                if (!pUnit2->GetProperty(PRP_MEN, type, (const void *&)n2, eNormal) )
                    n2 = 0; // should not happen
                nstud += n2;
            }
            if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)n1, eNormal) )
                n1 = 0; // unlikely, too

            // n1 - number of teachers
            // nstud - number of students

            if (nstud>0 && n1>0)
            {
                pUnit ->Teaching = (double)nstud/n1;
                teach = (double)n1*STUDENTS_PER_TEACHER/nstud*30;

                for (i=0; i<pUnit->pStudents->Count(); i++)
                {
                    pUnit2           = (CUnit*)pUnit->pStudents->At(i);
                    pUnit2->Teaching += teach;
                }
            }
        }
    }
    while (false);
}

//-------------------------------------------------------------

void CAtlaParser::RunPseudoComment(int sequence, CLand * pLand, CUnit * pUnit, const char * src)
{
    std::string            Command;
    std::string            S;
    char            ch;
    const char    * p;
    do
    {
        // just one damn pseudo comment please!
        p = GetToken(Command, SkipSpaces(src), " \t", ch, TRIM_ALL);

        // it must be sequenced just like the real commands!
        if (SQ_CLAIM == sequence)
            if (0==stricmp(Command.c_str(), "$GET"))  // do it in CLAIM so it will affect new units too
            {
                // GET 100 silv

                Command = src;
                RunOrder_Withdraw(Command, S, false, pUnit, pLand, p);
            }
    }
    while (false);
}


void CAtlaParser::RunOrders(CLand * pLand, const char * sCheckTeach)
{
    int         i, n;
    CPlane    * pPlane;

    if (!sCheckTeach)
        sCheckTeach = gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_CHECK_TEACH_LVL);

    if (pLand)
    {
        RunLandOrders(pLand, sCheckTeach);
    }
    else  // run orders for all lands
    {
        for (n=0; n<m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_Planes.At(n);
            for (i=0; i<pPlane->Lands.Count(); i++)
            {
                pLand = (CLand*)pPlane->Lands.At(i);
                if (pLand)
                    RunLandOrders(pLand, sCheckTeach);
            }
        }
    }
}

//-------------------------------------------------------------

bool CAtlaParser::ApplyDefaultOrders(bool EmptyOnly)
{
    int           i;
    CUnit       * pUnit;
    std::string NewOrder;
    std::string OldOrder;
    const char  * pNew;
    const char  * pOld;
    bool          Exists;
    bool          Changed = false;

    for (i=0; i<m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_Units.At(i);
        if (pUnit->IsOurs)
        {
            if (EmptyOnly)
            {
                TrimRight(pUnit->Orders, TRIM_ALL);
                if (pUnit->Orders.empty())
                {
                    pNew  = pUnit->DefOrders.c_str();
                    while (pNew)
                    {
                        pNew = GetToken(NewOrder, pNew, '\n', TRIM_ALL);
                        if ( (!NewOrder.empty()) && (';'!=NewOrder.c_str()[0]) )
                        {
                            if (!pUnit->Orders.empty())
                                pUnit->Orders << EOL_SCR;
                            pUnit->Orders << NewOrder;
                            Changed = true;
                        }
                    }
                }
            }
            else
            {
                pNew  = pUnit->DefOrders.c_str();
                while (pNew)
                {
                    pNew = GetToken(NewOrder, pNew, '\n', TRIM_ALL);
                    if ( (!NewOrder.empty()) && (';'!=NewOrder.c_str()[0]) )
                    {
                        Exists = false;
                        pOld   = pUnit->Orders.c_str();
                        while (pOld)
                        {
                            pOld = GetToken(OldOrder, pOld, '\n', TRIM_ALL);
                            if (0==stricmp(OldOrder.c_str(), NewOrder.c_str()))
                            {
                                Exists = true;
                                break;
                            }
                        }
                        if (!Exists)
                        {
                            if (!pUnit->Orders.empty())
                                pUnit->Orders << EOL_SCR;
                            pUnit->Orders << NewOrder;
                            Changed = true;

                        }

                    }
                }
            }
        }
    }
    if (Changed)
        RunOrders(nullptr);

    return Changed;
}

//-------------------------------------------------------------

int  CAtlaParser::SetUnitProperty(CUnit * pUnit, const char * name, EValueType type, const void * value, EPropertyType proptype)
{
    if (m_UnitPropertyNames.find(name) == m_UnitPropertyNames.end())
    {
        m_UnitPropertyNames.insert(name);
        m_UnitPropertyTypes.emplace(name, (int)type);
    }
    return pUnit->SetProperty(name, type, value, proptype);
}

//-------------------------------------------------------------

int  CAtlaParser::SetLandProperty(CLand * pLand, const char * name, EValueType type, const void * value, EPropertyType proptype)
{
    m_LandPropertyNames.insert(name);
    return pLand->SetProperty(name, type, value, proptype);
}

//-------------------------------------------------------------

void WriteOneMageSkill(std::string & Line, const char * skill, CUnit * pUnit, const char * separator, int format)
{
    std::string                S;
    EValueType          type;
    const void        * value;
    long                nlvl=0, ndays=0;
    int                 n;

    Line << separator;


    S << skill << PRP_SKILL_POSTFIX;
    if (pUnit->GetProperty(S.c_str(), type, value, eNormal) && (eLong==type) )
        nlvl = (long)value;

    S.clear();
    S << skill << PRP_SKILL_DAYS_POSTFIX;
    if (pUnit->GetProperty(S.c_str(), type, value, eNormal) && (eLong==type) )
        ndays = (long)value;

    switch (format)
    {
        case 0:  // Original decorated format
            if (nlvl>0)
            {
                Line << "_" << nlvl;
                for (n=1; n<=nlvl; n++)
                    ndays -= n*30;
                if (ndays<0)
                    ndays=0;
                n = ndays/30;

                while (n-- > 0)
                    Line << "+";
            }
            else
                Line << "_";

            break;

        case 1:  // just number of days
            Line << ndays;
            break;

        case 2:  // months and days
            if (ndays>0)
                Line << nlvl << "(" << ndays << ")";
            break;
    }
}

void CAtlaParser::WriteMagesCSV(const char * FName, bool vertical, const char * separator, int format)
{
    CBaseCollById       Mages;
    CUnit             * pUnit;
    int                 idx;
//    const char        * Foundations[3] = {"FORC_", "PATT_", "SPIR_"};
    EValueType          type;
    const void        * value;
    std::set<std::string, CaseInsensitiveLess> Skills;
    const char        * propname;
    int                 i, n, postlen;
    std::string                S, Line;
    CFileWriter         Dest;
    bool                IsMage;


    postlen = strlen(PRP_SKILL_POSTFIX);
    for (idx=0; idx<m_Units.Count(); idx++)
    {
        pUnit = (CUnit*)m_Units.At(idx);

        IsMage   = false;
        i        = 0;
        propname = pUnit->GetPropertyName(i);
        while (propname)
        {
            if (gpDataHelper->IsRawMagicSkill(propname))
            {
                Mages.Insert(pUnit);
                IsMage = true;
                break;
            }
            propname = pUnit->GetPropertyName(++i);
        }

        if (IsMage)
        {
            i        = 0;
            propname = pUnit->GetPropertyName(i);
            while (propname)
            {
                if (pUnit->GetProperty(propname, type, value, eNormal) && (eLong==type) )
                {
                    S = propname;
                    if (FindSubStrR(S, PRP_SKILL_POSTFIX) == S.size()-postlen)
                    {
                        DelSubStr(S, S.size()-postlen, postlen);
                        Skills.insert(S.c_str());
                    }
                }

                propname = pUnit->GetPropertyName(++i);
            }
        }

    }


    if (Dest.Open(FName))
    {
        Line.clear();
        if (vertical)
        {
            Line << "Skill";
            for (i=0; i<Mages.Count(); i++)
            {
                pUnit = (CUnit*)Mages.At(i);
                Line << separator << pUnit->Id << " " << pUnit->Name;
            }
            Line << EOL_FILE;
            Dest.WriteBuf(Line.c_str(), Line.size());

            for (const auto& skillName : Skills)
            {
                Line.clear();
                Line << skillName.c_str();
                for (idx=0; idx<Mages.Count(); idx++)
                {
                    pUnit = (CUnit*)Mages.At(idx);
                    WriteOneMageSkill(Line, skillName.c_str(), pUnit, separator, format);
                }
                Line << EOL_FILE;
                Dest.WriteBuf(Line.c_str(), Line.size());
            }
        }
        else
        {
            Line << "Id" << separator << "Name";
            for (const auto& skillName : Skills)
                Line << separator << skillName.c_str();
            Line << EOL_FILE;
            Dest.WriteBuf(Line.c_str(), Line.size());


            for (idx=0; idx<Mages.Count(); idx++)
            {
                pUnit = (CUnit*)Mages.At(idx);
                Line.clear();
                Line << pUnit->Id << separator << pUnit->Name;
                for (const auto& skillName : Skills)
                    WriteOneMageSkill(Line, skillName.c_str(), pUnit, separator, format);

                Line << EOL_FILE;
                Dest.WriteBuf(Line.c_str(), Line.size());
            }
        }
        Dest.Close();
    }




    Mages.DeleteAll();
    Skills.clear();
}

//-------------------------------------------------------------

void CAtlaParser::LookupAdvancedResourceVisibility(CUnit * pUnit, CLand * pLand)
{
    const char        * propname;
    EValueType          type;
    const void        * value;
    std::string                S;
    int                 propidx, postlen;
    std::vector<long>        Levels;
    std::vector<std::string> Resources;
    long                level;
    int                 i, idx;
    CProduct            Dummy;
    CProduct          * pProd;

    postlen = strlen(PRP_SKILL_POSTFIX);
    propidx = 0;
    propname = pUnit->GetPropertyName(propidx);
    while (propname)
    {
        if (pUnit->GetProperty(propname, type, value, eNormal) && (eLong==type) )
        {
            S = propname;
            if (FindSubStrR(S, PRP_SKILL_POSTFIX) == S.size()-postlen)
            {
                DelSubStr(S, S.size()-postlen, postlen);
                if (gpDataHelper->CanSeeAdvResources(S.c_str(), pLand->TerrainType.c_str(), Levels, Resources))
                {
                    level = (long)value;
                    for (i=0; i<(int)Levels.size(); i++)
                        if (level >= Levels[i])
                        {
                            Dummy.ShortName = Resources[i].c_str();
                            if (!pLand->Products.Search(&Dummy, idx))
                            {
                                pProd = new CProduct;
                                pProd->Amount    = 0;
                                pProd->ShortName = Dummy.ShortName;
                                pProd->LongName  = Dummy.ShortName;
                                pLand->Products.Insert(pProd);
                            }
                        }
                }
            }
        }

        propname = pUnit->GetPropertyName(++propidx);
    }

    Levels.clear();
    Resources.clear();
}

//-------------------------------------------------------------

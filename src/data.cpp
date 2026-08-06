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
#include <stdarg.h>
#include <math.h>

#include "data.h"

#include "string_utils.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "objs.h"

CGameDataHelper * gpDataHelper = nullptr;

const char * STRUCT_GATE    = "GATE";

const char * STD_UNIT_PROPS [] =
{
    PRP_FACTION_ID           ,
    PRP_FACTION              ,
    PRP_LAND_ID              ,
    PRP_STRUCT_ID            ,
    PRP_COMMENTS             ,
    PRP_ORDERS               ,
    PRP_STRUCT_OWNER         ,
    PRP_STRUCT_NAME          ,
    PRP_TEACHING             ,
    PRP_WEIGHT               ,
    PRP_MOVEMENT             ,
    PRP_DESCRIPTION          ,
    PRP_COMBAT               ,
    PRP_MEN                  ,
    PRP_SKILLS               ,
    PRP_MAG_SKILLS           ,
    PRP_STUFF                ,
    PRP_HORS                 ,
    PRP_WEAPONS              ,
    PRP_ARMOURS              ,
    PRP_MAG_ITEMS            ,
    PRP_JUNK_ITEMS           ,
    PRP_SEL_FACT_MEN         ,
    PRP_SEQUENCE             ,
    PRP_FRIEND_OR_FOE
};
const int STD_UNIT_PROPS_COUNT = sizeof(STD_UNIT_PROPS)/sizeof(*STD_UNIT_PROPS);

const char * SKILL_UNIT_POSTFIXES [] =
{
    PRP_SKILL_POSTFIX                ,
    PRP_SKILL_STUDY_POSTFIX          ,
    PRP_SKILL_EXPERIENCE_POSTFIX     ,
    PRP_SKILL_DAYS_POSTFIX           ,
    PRP_SKILL_DAYS_EXPERIENCE_POSTFIX,
};
const int SKILL_UNIT_POSTFIXES_COUNT = sizeof(SKILL_UNIT_POSTFIXES)/sizeof(*SKILL_UNIT_POSTFIXES);


//=============================================================

bool IsASkillRelatedProperty(const char * propname)
{
    int          i;
    const char * p;

    for (i=0; i<SKILL_UNIT_POSTFIXES_COUNT; i++)
    {
        p = strrchr(propname, SKILL_UNIT_POSTFIXES[i][0]);
        if (p && 0==strcmp(p, SKILL_UNIT_POSTFIXES[i]))
            return true;
    }
    return false;
}

//=============================================================

CBaseObject::CBaseObject()
{
    Name.reserve(32);
    Description.reserve(128);
    Id = 0;
}

//-------------------------------------------------------------

void CBaseObject::Done()
{
}

//-------------------------------------------------------------

const char * CBaseObject::ResolveAlias(const char * alias)
{
    if (gpDataHelper)
        return gpDataHelper->ResolveAlias(alias);
    else
        return alias;
}

//-------------------------------------------------------------

bool CBaseObject::GetProperty(const char  *  name,
                              EValueType   & type,
                              const void  *& value,
                              EPropertyType  proptype
                              )
{
    bool          Ok = true;

    name = ResolveAlias(name);

    if (!TPropertyHolder::GetProperty(name, type, value, proptype))
    {
        if      (0==stricmp(name, PRP_ID))
        {
            type  = eLong;
            value = (void*)Id;
        }
        else if (0==stricmp(name, PRP_NAME))
        {
            type  = eCharPtr;
            value = Name.c_str();
        }
        else if (0==stricmp(name, PRP_FULL_TEXT))
        {
            type  = eCharPtr;
            value = Description.c_str();
        }
        else
            Ok = false;
    }
    return Ok;
}

//-------------------------------------------------------------

void CBaseObject::SetName(const char * newname)
{
    EValueType    type;
    const void  * value;

    // save org value if not saved
    if (!GetProperty(PRP_ORG_NAME, type, value, eNormal))
        SetProperty(PRP_ORG_NAME, eCharPtr, (void*)Name.c_str(), eBoth);
    Name = newname;
}

//-------------------------------------------------------------

void CBaseObject::SetDescription(const char * newdescr)
{
    EValueType    type;
    const void  * value;

    // save org value if not saved
    if (!GetProperty(PRP_ORG_DESCR, type, value, eNormal))
        SetProperty(PRP_ORG_DESCR, eCharPtr, (void*)Description.c_str(), eBoth);
    Description = newdescr;
}

//-------------------------------------------------------------

void CBaseObject::ResetName()
{
    EValueType    type;
    const void  * value;

    if (GetProperty(PRP_ORG_NAME, type, value, eNormal) && eCharPtr==type)
        Name = (const char *)value;
}

//-------------------------------------------------------------

void CBaseObject::ResetDescription()
{
    EValueType    type;
    const void  * value;

    if (GetProperty(PRP_ORG_DESCR, type, value, eNormal) && eCharPtr==type)
        Description = (const char *)value;
}

//-------------------------------------------------------------

void CBaseObject::ResetNormalProperties()
{
    TPropertyHolder::ResetNormalProperties();
    ResetName();
    ResetDescription();
}

//-------------------------------------------------------------

void CBaseObject::DebugPrint(std::string & sDest)
{
    sDest << "\n"
          << "Id          = " << Id << "\n"
          << "Name        = " << Name.c_str() << "\n"
          << "Description = " << Description.c_str() << "\n";
}

//=============================================================

void CFaction::DebugPrint(std::string & sDest)
{
    CBaseObject::DebugPrint(sDest);

    sDest << "UnclaimedSilver = " << UnclaimedSilver << "\n";
}

//=============================================================

void CAttitude::SetStance(int newstance)
{
    Stance = newstance; // include some logic for precedence JR
}

//-------------------------------------------------------------

bool CAttitude::IsDeclaredAs(int attitude)
{
    return (Stance==attitude);
}

//-------------------------------------------------------------

bool CAttitude::IsEqual(CAttitude * attitude)
{
    return (attitude->IsDeclaredAs(Stance));
}

//=============================================================

CLand::CLand() : CBaseObject(), Units(32), UnitsSeq(32)
{
    Flags=0;
    AlarmFlags=0;
    EventFlags=0;
    guiUnit=0;
    pPlane=nullptr;
    ExitBits=0;
    CoastBits=0;
    AtlaclientsLastTurnNo=0;
    guiColor=-1;
    WeatherWillBeGood=false;
    Wages = 0.0;
    MaxWages = 0;
    for(int i=0; i<=ATT_UNDECLARED; i++) Troops[i]=0;
}

//-------------------------------------------------------------

CLand::~CLand()
{
    ResetUnitsAndStructs();
    Structs.FreeAll();
    EdgeStructs.FreeAll();
    Units.DeleteAll();
    UnitsSeq.DeleteAll();
    Products.FreeAll();
}

void CLand::DebugPrint(std::string & sDest)
{
    CBaseObject::DebugPrint(sDest);

    sDest << "Taxable   = " << Taxable << "\n";
}

//-------------------------------------------------------------

bool CLand::AddUnit(CUnit * pUnit)
{
    if (Units.Insert(pUnit))
    {
        pUnit->LandId = Id;
        Flags |= LAND_UNITS;
        UnitsSeq.Insert(pUnit);
        pUnit->SetProperty(PRP_SEQUENCE, eLong, reinterpret_cast<void*>(static_cast<intptr_t>(UnitsSeq.Count())), eNormal);
        return true;
    }
    else
        return false;

}

//-------------------------------------------------------------

void CLand::RemoveUnit(CUnit * pUnit)
{
    int i;
    CUnit * p;

    if (Units.Search(pUnit, i))
    {
        Units.AtDelete(i);

        for (i=UnitsSeq.Count()-1; i>=0; i--)
        {
            p = (CUnit*)UnitsSeq.At(i);
            if (p->Id == pUnit->Id)
            {
                UnitsSeq.AtDelete(i);
                break;
            }
        }
    }
}

//-------------------------------------------------------------

void CLand::ResetUnitsAndStructs()
{
    int       i, k;
    CUnit   * pUnit;
    CStruct * pStruct;

    for (i=UnitsSeq.Count()-1; i>=0; i--)
    {
        pUnit = (CUnit*)UnitsSeq.At(i);
        if (IS_NEW_UNIT(pUnit))
            UnitsSeq.AtDelete(i);
//        else                   this creates problems when joining reports, first has FORM orders,
//            break;             second has units invisible in the first one, so new units are left in the middle - pointer to released memory
    }

    for (i=Units.Count()-1; i>=0; i--)
    {
        pUnit = (CUnit*)Units.At(i);
        if (IS_NEW_UNIT(pUnit))
            Units.AtFree(i);
        else
        {
            pUnit->ResetNormalProperties();
            if (pUnit->pMovement)
            {
                pUnit->pMovement.reset();
            }
            if (pUnit->pMoveA3Points)
            {
                pUnit->pMoveA3Points.reset();
            }
            if (pUnit->pStudents)
                pUnit->pStudents->DeleteAll(); // probably deleting it would not be very usefull
        }
    }

    for (k=0; k<Structs.Count(); k++)
    {
        pStruct = (CStruct*)Structs.At(k);
        pStruct->ResetNormalProperties();
    }
}

//-------------------------------------------------------------

int CLand::GetNextNewUnitNo()
{
    int     no = 1;
    int     i,x;
    CUnit * pUnit;

    for (i=Units.Count()-1; i>=0; i--)
    {
        pUnit = (CUnit*)Units.At(i);
        if (IS_NEW_UNIT(pUnit))
        {
            x = REVERSE_NEW_UNIT_ID(pUnit->Id);
            if (x>=no)
                no=x+1;
        }
    }

    return no;
}

//-------------------------------------------------------------

CStruct * CLand::GetStructById(long id)
{
    CStruct     * pStruct = nullptr;
    CBaseObject   Dummy;
    int       i;

    Dummy.Id = id;
    if (Structs.Search(&Dummy, i))
        pStruct = (CStruct*)Structs.At(i);

    return pStruct;
}

//-------------------------------------------------------------

CStruct * CLand::AddNewStruct(CStruct * pNewStruct)
{
    int       idx;
    CStruct * pStruct;

    if (Structs.Search(pNewStruct, idx))
    {
        pStruct = (CStruct*)Structs.At(idx);

//        if (0==stricmp(pStruct->Kind.c_str(), "Shaft") )
        if (pStruct->Attr & SA_SHAFT  )
        {
            // process links for shafts

            int  x1, x2, x3;
            bool Link;

            x1   = FindSubStr(pNewStruct->Description, ";");
            x2   = FindSubStr(pNewStruct->Description, "links");
            x3   = FindSubStr(pNewStruct->Description, "to");
            Link = ( x1>=0 && x1<x2 && x2<x3 );

            if (Link || pNewStruct->Description.size() > pStruct->Description.size())
                pStruct->Description= pNewStruct->Description;
        }
        else
            pStruct->Description= pNewStruct->Description;
        pStruct->Name           = pNewStruct->Name       ;
        pStruct->LandId         = pNewStruct->LandId     ;
        pStruct->OwnerUnitId    = pNewStruct->OwnerUnitId;
        pStruct->Load           = pNewStruct->Load       ;
        pStruct->Attr           = pNewStruct->Attr       ;
        pStruct->Kind           = pNewStruct->Kind       ;
        pStruct->Location       = pNewStruct->Location   ;

        delete pNewStruct;
    }
    else
    {
        Structs.Insert(pNewStruct);
        if (0 == pNewStruct->Attr)                 Flags |= LAND_STR_GENERIC;
        else
        {
            if (pNewStruct->Attr & SA_HIDDEN )     Flags |= LAND_STR_HIDDEN ;
            if (pNewStruct->Attr & SA_MOBILE )     Flags |= LAND_STR_MOBILE ;
            if (pNewStruct->Attr & SA_SHAFT  )     Flags |= LAND_STR_SHAFT  ;
            if (pNewStruct->Attr & SA_GATE   )     Flags |= LAND_STR_GATE   ;
            if (pNewStruct->Attr & SA_ROAD_N )     Flags |= LAND_STR_ROAD_N ;
            if (pNewStruct->Attr & SA_ROAD_NE)     Flags |= LAND_STR_ROAD_NE;
            if (pNewStruct->Attr & SA_ROAD_SE)     Flags |= LAND_STR_ROAD_SE;
            if (pNewStruct->Attr & SA_ROAD_S )     Flags |= LAND_STR_ROAD_S ;
            if (pNewStruct->Attr & SA_ROAD_SW)     Flags |= LAND_STR_ROAD_SW;
            if (pNewStruct->Attr & SA_ROAD_NW)     Flags |= LAND_STR_ROAD_NW;
        }


        pStruct = pNewStruct;
    }

    return pStruct;
}

//-------------------------------------------------------------


void CLand::SetFlagsFromUnits(CAtlaParser* p)
{
    int                 i;
    int                 alarm=ATT_FRIEND1;
    int                 guard_stance=-1;
    int                 claim=-1;
    long                 player_id;
    long                men, weapons, armours;
    const void        * stance;
    const void        * troops;
    const void        * gear_weapon;
    const void        * gear_armour;
    EValueType          type;
    CUnit             * pUnit;

    Flags &= ~(LAND_TAX_NEXT | LAND_TRADE_NEXT);
    AlarmFlags = 0;
    {
        const char* pid = (p && p->m_pConfig) ? p->m_pConfig->GetByName(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID) : nullptr;
        player_id = atol(pid ? pid : "0");
    }

    for(i=ATT_UNDECLARED; i>=0; i--) Troops[i]=0;

    for (i=Units.Count()-1; i>=0; i--)
    {
        men=0;
        weapons=0;
        armours=0;
        pUnit = (CUnit*)UnitsSeq.At(i);
        if (pUnit->FactionId==player_id) AlarmFlags |= PRESENCE_OWN;
        if ((pUnit->Flags & UNIT_FLAG_TAXING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
            Flags |= LAND_TAX_NEXT;
        if ((pUnit->Flags & UNIT_FLAG_PRODUCING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
            Flags |= LAND_TRADE_NEXT;
        if (!((pUnit->GetProperty(PRP_FRIEND_OR_FOE,type,stance,eNormal)) && (type==eLong)))
        {
            // set the default stance if none is defined
            stance = reinterpret_cast<void*>(static_cast<intptr_t>(gpDataHelper->GetAttitudeForFaction(0)));
        }
        if ((pUnit->Flags & UNIT_FLAG_GUARDING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
        {
            if(((long) stance > guard_stance) && ((long) stance < ATT_UNDECLARED))
                guard_stance = (long) stance; // stance of guards, if any
        }
        if (((pUnit->Flags & UNIT_FLAG_TAXING) || (pUnit->Flags & UNIT_FLAG_PRODUCING))
              && !(pUnit->Flags & UNIT_FLAG_GIVEN))
        {
            if(((long) stance > claim) && ((long) stance < ATT_UNDECLARED))
                claim = (long) stance; // stance of taxers/producers
        }
        if(((long) stance <= ATT_UNDECLARED) && ((long) stance >= 0))
        {
            // stance due to presence
            if((long) stance > alarm) alarm = (long) stance;
            if ((pUnit->GetProperty(PRP_MEN,type,troops,eNormal)) && (type==eLong))
                men = (long) troops;
            if ((pUnit->GetProperty(PRP_WEAPONS,type,gear_weapon,eNormal)) && (type==eLong))
               	weapons = (long) gear_weapon;
            if ((pUnit->GetProperty(PRP_ARMOURS,type,gear_armour,eNormal)) && (type==eLong))
                armours = (long) gear_armour;
            if(weapons > (long) men) weapons = men;
            if(armours > weapons) armours = weapons;
            Troops[(long) stance] += 2*men + 2* weapons + armours + 1;
        }
    }

    if(claim<0) // pick the strongest stance group
    {
        long max = 0;
        for(i=ATT_UNDECLARED-1; i>=0; i--)
        {
            if(Troops[i] > max)
            {
                claim = i;
                max = Troops[i];
            }
        }
    }
    switch(claim)
    {
        case ATT_FRIEND1:
            AlarmFlags |= CLAIMED_BY_FRIEND;
            break;
        case ATT_FRIEND2:
            AlarmFlags |= CLAIMED_BY_OWN;
            break;
        case ATT_NEUTRAL:
            AlarmFlags |= CLAIMED_BY_NEUTRAL;
            break;
        case ATT_ENEMY:
            AlarmFlags |= CLAIMED_BY_ENEMY;
    }

    if((guard_stance >=0) && (guard_stance < ATT_UNDECLARED)) AlarmFlags |= GUARDED;
    switch(guard_stance)
    {
        case ATT_FRIEND1:
            AlarmFlags |= GUARDED_BY_FRIEND;
            break;
        case ATT_FRIEND2:
            AlarmFlags |= GUARDED_BY_OWN;
            break;
        case ATT_NEUTRAL:
            AlarmFlags |= GUARDED_BY_NEUTRAL;
            break;
        case ATT_ENEMY:
            AlarmFlags |= GUARDED_BY_ENEMY;
    }

    switch(alarm)
    {
        case ATT_FRIEND1:
            AlarmFlags |= PRESENCE_FRIEND;
            break;
        case ATT_NEUTRAL:
            AlarmFlags |= PRESENCE_NEUTRAL;
            break;
        case ATT_ENEMY:
            AlarmFlags |= PRESENCE_ENEMY;
    }
    if((alarm > guard_stance) && (alarm > claim) && (alarm > ATT_FRIEND2)) AlarmFlags |= ALARM;

    // normalise troops
    {
        const char* mm = (p && p->m_pConfig) ? p->m_pConfig->GetByName(SZ_SECT_COMMON, SZ_KEY_MIN_SEL_MEN) : nullptr;
        int minimen = 2 * atol(mm ? mm : "0");
        for(i=ATT_UNDECLARED; i>=0; i--)
        {
            long limit = 2000;
            men = Troops[i];
            Troops[i]=0;
            for(int f=10; f>=3; f-=1)
            {
                long c = (long) ((float) f * f / 100 * limit);
                if(men >= c) Troops[i]++;
            }
            if(men >= minimen) Troops[i]++;
        }
    }
}

//-------------------------------------------------------------

void CLand::CalcStructsLoad()
{
    int                 i, k;
    CUnit             * pUnit;
    CBaseObject         Dummy;
    CStruct           * pStruct;
    EValueType          type;
    const void        * value;

    for (i=0; i<Structs.Count(); i++)
        ((CStruct*)Structs.At(i))->Load = 0;

    for (i=Units.Count()-1; i>=0; i--)
    {
        pUnit = (CUnit*)UnitsSeq.At(i);
        if (pUnit->GetProperty(PRP_STRUCT_ID, type, value, eNormal) && (eLong==type) && ((long)value > 0))
        {
            Dummy.Id = (long)value;
            if (Structs.Search(&Dummy, k))
            {
                pStruct = (CStruct*)Structs.At(k);
                pStruct->Load += pUnit->Weight[0];
            }
        }
    }
}

//-------------------------------------------------------------

void CLand::RemoveEdgeStructs(int direction)
{
    CStruct * pEdge;
    int normalizedDir = ((direction % 6) + 6) % 6;
    for (int i=EdgeStructs.Count()-1; i>=0; i--)
    {
        pEdge = (CStruct*) EdgeStructs.At(i);
        if((pEdge != nullptr) && (pEdge->Location == normalizedDir))
        {
            EdgeStructs.AtFree(i);
        }
    }
}
//-------------------------------------------------------------

void CLand::AddNewEdgeStruct(const char * name, int direction)
{
    CStruct * pEdge = new CStruct;

    pEdge->Kind     = name;
    pEdge->Location = ((direction % 6) + 6) % 6;
    EdgeStructs.Insert(pEdge);
}

//=============================================================

std::multimap<std::string,std::string> * CUnit::m_PropertyGroupsColl = nullptr;
std::string          CUnit::m_CustomFlagNames[UNIT_CUSTOM_FLAG_COUNT];
bool          CUnit::m_CustomFlagNamesLoaded = false;



std::multimap<std::string,std::string> * CUnit::GetPropertyGroups()
{
    return m_PropertyGroupsColl;
}

//-------------------------------------------------------------

CUnit::CUnit() : CBaseObject()
{
    Comments.reserve(16);
    DefOrders.reserve(32);
    Orders.reserve(32);
    Errors.reserve(32);
    Events.reserve(32);
    IsOurs        = false;
    FactionId     = 0;
    pFaction      = nullptr;
    LandId        = 0;
    Teaching      = 0.0;
    pMovement     = nullptr;
    pMoveA3Points = nullptr;
    pStudents     = nullptr;
    SilvRcvd      = 0;
    Flags         = 0;
    FlagsOrg      = 0;
    FlagsLast     = ~Flags;
    IsWorking     = false;
    memset(Weight, 0, sizeof(Weight));
};

//-------------------------------------------------------------

CUnit::~CUnit()
{
    if (pStudents)
        pStudents->DeleteAll();
}

//-------------------------------------------------------------

CUnit * CUnit::AllocSimpleCopy()
{
    CUnit * pUnit = new CUnit;
    int     idx;

    pUnit->Id                     = Id                   ;
    pUnit->Name                   = Name                 ;
    pUnit->Description            = Description          ;

    pUnit->IsOurs                 = IsOurs               ;
    pUnit->FactionId              = FactionId            ;
    pUnit->pFaction               = pFaction             ;
    pUnit->LandId                 = LandId               ;
    pUnit->SilvRcvd               = SilvRcvd             ;
    pUnit->Teaching               = Teaching             ;
    pUnit->Comments               = Comments             ;
    pUnit->DefOrders              = DefOrders            ;
    pUnit->Orders                 = Orders               ;
    pUnit->Errors                 = Errors               ;
    pUnit->Events                 = Events               ;
    pUnit->StudyingSkill          = StudyingSkill        ;
    pUnit->Flags                  = Flags                ;
    pUnit->FlagsOrg               = FlagsOrg             ;
    pUnit->FlagsLast              = FlagsLast            ;
    pUnit->IsWorking              = IsWorking            ;

    memcpy(pUnit->Weight, Weight, sizeof(Weight))        ;

    const char  * propname;
    EValueType    type;
    const void  * value;

    idx = 0;
    propname = GetPropertyName(idx);
    while (*propname)
    {
        if (GetProperty(propname, type, value, eNormal))
            pUnit->SetProperty(propname, type, value, eNormal);

        propname = GetPropertyName(++idx);
    }

    return pUnit;
}

//-------------------------------------------------------------

void CUnit::ExtractCommentsFromDefOrders()
{
    const char * p;
    std::string         S;

    Comments.clear();
    p = DefOrders.c_str();
    while (p)
    {
        p = GetToken(S, p, '\n', TRIM_ALL);
        if ( (S.size() > 1) && (';' == S.c_str()[0]) )
        {
            SetStr(Comments, S.c_str() + 1, (int)S.size() - 1);
            TrimLeft(Comments, TRIM_ALL);
            break;
        }
    }

}

//-------------------------------------------------------------

void CUnit::ResetNormalProperties()
{


    CBaseObject::ResetNormalProperties();
    Teaching = 0;
    StudyingSkill.clear();
    ProducingItem.clear();
    SilvRcvd = 0;

    Flags     = FlagsOrg;
    FlagsLast = ~Flags;

    IsWorking = false;

    // calc weight;
    CalcWeightsAndMovement();
}

//-------------------------------------------------------------

void CUnit::AddWeight(int nitems, int * weights, const char ** movenames, int nweights)
{
    int  i;
    int  NW = std::min(nweights, MOVE_MODE_MAX);

    for (i=0; i<NW; i++)
        Weight[i] += nitems*weights[i];
}

//-------------------------------------------------------------

void CUnit::CalcWeightsAndMovement() 
{
    int           idx;
    const char  * propname;
    int         * weights;
    const char ** movenames = nullptr;
    int           movecount;
    EValueType    type;
    const void  * n;
    std::string          sValue;
    int           i;
    
    memset(Weight, 0, sizeof(Weight));
    SetProperty(PRP_MOVEMENT, eCharPtr, "", eNormal);

    if (!gpDataHelper)
        return;

    // preliminary calculation, no wagons
    idx      = 0;
    propname = GetPropertyName(idx);
    while (*propname)
    {
        if (GetProperty(propname, type, n, eNormal) &&
            (eLong==type) &&
            gpDataHelper->GetItemWeights(propname, weights, movenames, movecount))
        {
            AddWeight((long)n, weights, movenames, movecount);
        }

        propname = GetPropertyName(++idx);
    }
    
    if (!movenames)
        return;
    
    // adjust for wagons if needed
    i = 3;
    if (Weight[0])
        while ((i>0) && (Weight[i] < Weight[0]))
            i--;
    if (0==i)
    {
        // cannot move at all, maybe wagons will help?
        // First, find out how many horses and wagons do we have
        // Then, redestribute weights based on std::min(horses, wagons).
        // We do not do it in the preliminary calc because it is expensive
        int nHorses=0, nWagons=0;
        idx      = 0;
        propname = GetPropertyName(idx);
        while (*propname)
        {
            if (GetProperty(propname, type, n, eNormal) &&
                (eLong==type) )
            {
                if (gpDataHelper->IsWagonPuller(propname))
                    nHorses += (long)n;
                else if (gpDataHelper->IsWagon(propname))
                    nWagons += (long)n;
            }
            propname = GetPropertyName(++idx);
        }
        int nAdjust = std::min(nHorses, nWagons);
        if (nAdjust>0)
            Weight[1] += nAdjust*gpDataHelper->WagonCapacity();
    }
    
    // Set movement name
    sValue.clear();
    if (Weight[0])
    {
        i = 3;
        while ((i>0) && (Weight[i] < Weight[0]))
            i--;
    }
    else
        i = 0;
    sValue = movenames[i];

    if (Weight[4] >= Weight[0]) // can swim
        sValue << ',' << movenames[4];
    
    SetProperty(PRP_MOVEMENT, eCharPtr, sValue.c_str(), eNormal);
}

//-------------------------------------------------------------

void CUnit::CheckWeight(std::string & sErr)
{
    int           i;
    const char ** movenames;
    int           broken = 0;

    sErr.clear();
    gpDataHelper->GetMoveNames(movenames);

    for (i=1; i<MOVE_MODE_MAX; i++)
        if (Weight[0] > Weight[i] && Weight[i] > 0)
        {
            broken = i;
            break;
        }
        
    if (broken && broken<MOVE_MODE_MAX-2) // if flying broken, that is final. MOVE_MODE_MAX-2 is flying
    {
        // maybe there is good higher level movement mode?
        // can happen when unit has gliders - cannot walk, but can fly!
        for (i=MOVE_MODE_MAX-2; i>broken; i--)
            if (Weight[0] <= Weight[i] && Weight[i] > 0)
            {
                broken = 0;
                break;
            }
    }
    
    if (broken)
        sErr << " - Could " << movenames[broken] << " but is overloaded.";
        
        
}

//-------------------------------------------------------------

bool CUnit::GetProperty(const char  *  name,
                        EValueType   & type,
                        const void  *& value,
                        EPropertyType  proptype
                        )
{
    bool         Ok = true;

    name = ResolveAlias(name);

    if (0==stricmp(name, PRP_ID))
    {
        type  = eLong;
        value = (void*)Id;
        return true;
    }

    // Custom and Standard flags
    if ( FlagsLast != Flags &&
         (0==stricmp(name, PRP_FLAGS_STANDARD   ) ||
          0==stricmp(name, PRP_FLAGS_CUSTOM     ) ||
          0==stricmp(name, PRP_FLAGS_CUSTOM_ABBR)
         )
       )
    {
        std::string sValue, sValueAbbr, sKey;
        int  i, x;

        if (Flags & UNIT_FLAG_TAXING           )  sValue << '$';
        if (Flags & UNIT_FLAG_PRODUCING        )  sValue << 'P';
        if (Flags & UNIT_FLAG_GUARDING         )  sValue << 'g';
        if (Flags & UNIT_FLAG_AVOIDING         )  sValue << 'a';
        if (Flags & UNIT_FLAG_BEHIND           )  sValue << 'b';
        if (Flags & UNIT_FLAG_REVEALING_UNIT   )  sValue << 'r';
        else if (Flags & UNIT_FLAG_REVEALING_FACTION)  sValue << 'r';
        if (Flags & UNIT_FLAG_HOLDING          )  sValue << 'h';
        if (Flags & UNIT_FLAG_RECEIVING_NO_AID )  sValue << 'i';
        if (Flags & UNIT_FLAG_CONSUMING_UNIT   )  sValue << 'c';
        else if (Flags & UNIT_FLAG_CONSUMING_FACTION)  sValue << 'c';
        if (Flags & UNIT_FLAG_NO_CROSS_WATER   )  sValue << 'x';
        if (Flags & UNIT_FLAG_SPOILS           )  sValue << 's';
        // MZ - Added for Arcadia
        if (Flags & UNIT_FLAG_SHARING          )  sValue << 'z';

        type  = eCharPtr;
        SetProperty(PRP_FLAGS_STANDARD, type, sValue.c_str(), eNormal);

        sValue.clear();
        x = 1;
        for (i=0; i<UNIT_CUSTOM_FLAG_COUNT; i++)
        {
            if (Flags & x)
            {
                if (!sValue.empty())
                    sValue << ',';
                sValue     << GetCustomFlagName(i);
                sValueAbbr << (long)(i+1);
            }
            x <<= 1;
        }
        SetProperty(PRP_FLAGS_CUSTOM     , type, sValue.c_str()    , eNormal);
        SetProperty(PRP_FLAGS_CUSTOM_ABBR, type, sValueAbbr.c_str(), eNormal);

        FlagsLast = Flags;
    }

    // now the usual stuff...
    if (!CBaseObject::GetProperty(name, type, value, proptype))
    {
        if (0==stricmp(name, PRP_COMMENTS  ))
        {
            type  = eCharPtr;
            value = Comments.c_str();
        }
        else if (0==stricmp(name, PRP_ORDERS    ))
        {
            // decorate for stupid wxw 2.8.0 list control
            const char * src = Orders.c_str();
            char       * dest;
            int          destlen = 0;

            OrdersDecorated.clear();
            OrdersDecorated.resize(Orders.size() * 3 + 1);
            dest = &OrdersDecorated[0];

            while (src && *src)
            {
                if (*src != '\r' && *src != '\n')
                {
                    destlen++;
                    *dest = *src;
                    dest++;
                }
                else if ('\n' == *src)
                {
                    destlen += 3;
                    *dest = ' ';     dest++;
                    *dest = '|';     dest++;
                    *dest = ' ';     dest++;
                }
                src++;
            }
            OrdersDecorated.resize(destlen);

            type  = eCharPtr;
            value = OrdersDecorated.c_str();


            /*
            type  = eCharPtr;
            value = Orders.c_str();
            */
        }
        else if (0==stricmp(name, PRP_FACTION_ID))
        {
            type  = eLong;
            value = (void*)FactionId;
        }
        else if (0==stricmp(name, PRP_FACTION))
        {
            type  = eCharPtr;
            if (pFaction)
                value = pFaction->Name.c_str();
            else
                value = "";
        }
        else if (0==stricmp(name, PRP_LAND_ID   ))
        {
            type  = eLong;
            value = (void*)LandId;
        }
        else if (0==stricmp(name, PRP_WEIGHT ))
        {
            type  = eLong;
            value = (void*)Weight[0];
        }
        else if (0==stricmp(name, PRP_TEACHING ))
        {
            type  = eLong;
            if (StudyingSkill.empty())
                value = (void*)(long)ceil(Teaching);
            else
                value = (void*)(long)floor(Teaching);

            if (Teaching <= 0)
                Ok = false;
        }




        else
            Ok = false;
    }

    return Ok;
}


//-------------------------------------------------------------

void CUnit::LoadCustomFlagNames(CConfigFile* cfg)
{
    if (!cfg)
        return;

    int  i;
    std::string sKey;

    for (i=0; i<UNIT_CUSTOM_FLAG_COUNT; i++)
    {
        sKey.clear();
        sKey << (long)i;
        const char* val = cfg->GetByName(SZ_SECT_UNIT_FLAG_NAMES, sKey.c_str());
        m_CustomFlagNames[i] = val ? val : "";
    }
    m_CustomFlagNamesLoaded = true;
}

//-------------------------------------------------------------

void CUnit::ResetCustomFlagNames()
{
    int i;

    m_CustomFlagNamesLoaded = false;
    for (i=0; i<UNIT_CUSTOM_FLAG_COUNT; i++)
        m_CustomFlagNames[i].clear();
}

//-------------------------------------------------------------

const char * CUnit::GetCustomFlagName(int no)
{
    if (m_CustomFlagNamesLoaded && no>=0 && no<UNIT_CUSTOM_FLAG_COUNT)
        return m_CustomFlagNames[no].c_str();
    else
        return nullptr;
}

//-------------------------------------------------------------

void CUnit::DebugPrint(std::string & sDest)
{
    CBaseObject::DebugPrint(sDest);

    sDest << "FactionId = " << FactionId << "\n"
          << "LandId    = " << LandId << "\n"
        ;
}

//-------------------------------------------------------------

void CStruct::ResetNormalProperties()
{
    TPropertyHolder::ResetNormalProperties();
    Load         = 0;
    SailingPower = 0;
}

//=============================================================

CBaseColl::CBaseColl() {}
CBaseColl::CBaseColl(int /*nDelta*/) {}

void CBaseColl::FreeItem(void * pItem)
{
    if (pItem)
    {
        CBaseObject * pBase = (CBaseObject*)pItem;
        pBase->Done();
        delete pBase;
    }
}

//-------------------------------------------------------------
// CBaseCollById: sorted by Id, owns items (FreeItem deletes)

CBaseCollById::CBaseCollById() {}
CBaseCollById::CBaseCollById(int /*nDelta*/) {}

void CBaseCollById::FreeItem(void * pItem)
{
    if (pItem)
    {
        CBaseObject * pBase = (CBaseObject*)pItem;
        pBase->Done();
        delete pBase;
    }
}

int CBaseCollById::Compare(void * pItem1, void * pItem2) const
{
    long id1 = ((CBaseObject*)pItem1)->Id;
    long id2 = ((CBaseObject*)pItem2)->Id;
    if (id1 > id2) return  1;
    if (id1 < id2) return -1;
    return 0;
}

bool CBaseCollById::Insert(void * pItem)
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if      (cmp < 0) lo = mid + 1;
        else if (cmp > 0) hi = mid;
        else              return false; // duplicate
    }
    m_items.insert(m_items.begin() + lo, (CBaseObject*)pItem);
    return true;
}

bool CBaseCollById::Search(void * pItem, int & nIndex) const
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if (cmp < 0) lo = mid + 1;
        else         hi = mid;
    }
    nIndex = lo;
    return (lo < (int)m_items.size() && Compare(m_items[lo], pItem) == 0);
}

//-------------------------------------------------------------

int  CBaseCollByName::Compare(void * pItem1, void * pItem2) const
{
    CBaseObject * pBase1 = (CBaseObject*)pItem1;
    CBaseObject * pBase2 = (CBaseObject*)pItem2;
    return stricmp(pBase1->Name.c_str(), pBase2->Name.c_str());
}

//-------------------------------------------------------------
// CProductColl: sorted by ShortName, owns items

bool CProductColl::Insert(void * pItem)
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if      (cmp < 0) lo = mid + 1;
        else if (cmp > 0) hi = mid;
        else              return false;
    }
    m_items.insert(m_items.begin() + lo, (CProduct*)pItem);
    return true;
}

bool CProductColl::Search(void * pItem, int & nIndex) const
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if (cmp < 0) lo = mid + 1;
        else         hi = mid;
    }
    nIndex = lo;
    return (lo < (int)m_items.size() && Compare(m_items[lo], pItem) == 0);
}

//-------------------------------------------------------------
// CUnitsByHex: sorted by LandId+Id, does NOT own

bool CUnitsByHex::Insert(void * pItem)
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if      (cmp < 0) lo = mid + 1;
        else if (cmp > 0) hi = mid;
        else              return false;
    }
    m_items.insert(m_items.begin() + lo, (CUnit*)pItem);
    return true;
}

bool CUnitsByHex::Search(void * pItem, int & nIndex) const
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if (cmp < 0) lo = mid + 1;
        else         hi = mid;
    }
    nIndex = lo;
    return (lo < (int)m_items.size() && Compare(m_items[lo], pItem) == 0);
}

//-------------------------------------------------------------

CPlane::~CPlane()
{
    Lands.FreeAll();
}

//-------------------------------------------------------------
// CTaxProdDetailsCollByFaction: sorted by FactionId, owns items

bool CTaxProdDetailsCollByFaction::Insert(void * pItem)
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if      (cmp < 0) lo = mid + 1;
        else if (cmp > 0) hi = mid;
        else              return false;
    }
    m_items.insert(m_items.begin() + lo, (CTaxProdDetails*)pItem);
    return true;
}

bool CTaxProdDetailsCollByFaction::Search(void * pItem, int & nIndex) const
{
    int lo = 0, hi = (int)m_items.size();
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = Compare(m_items[mid], pItem);
        if (cmp < 0) lo = mid + 1;
        else         hi = mid;
    }
    nIndex = lo;
    return (lo < (int)m_items.size() && Compare(m_items[lo], pItem) == 0);
}

//-------------------------------------------------------------

void TProdDetails::Empty()
{
    int i;

    skillname.clear();
    skilllevel=0;
    months=0;
    toolname.clear();
    toolhelp=0;
    for (i=0; i<MAX_RES_NUM; i++)
    {
        resname[i].clear();
        resamt[i]=0;
    }
}

//=============================================================

void MakeQualifiedPropertyName(const char * prefix, const char * shortname, std::string & FullName)
{
    FullName.clear();
    FullName << prefix;

    if (!FullName.empty() && '.' != FullName.c_str()[FullName.size()-1])
        FullName << ".";
    FullName << shortname;
}

//-------------------------------------------------------------

void SplitQualifiedPropertyName(const char * fullname, std::string & Prefix, std::string & ShortName)
{
    const char * p;

    Prefix.clear();
    ShortName.clear();

    if (fullname && *fullname)
    {
        p = strrchr(fullname, '.');
        if (p)
        {
            ShortName = p+1;
            AddBuf(Prefix, fullname, (int)(p-fullname));
        }
    }
}

//--------------------------------------------------------------------------

bool EvaluateBaseObjectByBoxes(CBaseObject * pObj, std::string * Property, eCompareOp * CompareOp, std::string * sValue, long * lValue, int count, CAtlaParser* pParser)
{
    int i;
    EValueType       type;
    const void     * value;
    bool             ok = true;

    for (i=0; i<count; i++)
    {
        if (!Property[i].empty() && (NOP!=CompareOp[i]))
        {
            if ( !pObj->GetProperty(Property[i].c_str(), type, value, eNormal))
            {
                // make an empty sValue based on the property type registry
                type  = eLong;
                value = 0;
                if (pParser)
                {
                    auto it__ = pParser->m_UnitPropertyTypes.find(Property[i].c_str());
                    if (it__ != pParser->m_UnitPropertyTypes.end())
                    {
                        type = (EValueType)it__->second;
                        value = (eLong == type) ? 0 : reinterpret_cast<const void*>("");
                    }
                }
            }
            switch (type)
            {
            case eLong:
                switch (CompareOp[i])
                {
                case GT: ok = ((long)value >  lValue[i]); break;
                case GE: ok = ((long)value >= lValue[i]); break;
                case EQ: ok = ((long)value == lValue[i]); break;
                case LE: ok = ((long)value <= lValue[i]); break;
                case LT: ok = ((long)value <  lValue[i]); break;
                case NE: ok = ((long)value != lValue[i]); break;
                default: break;
                }
                break;

            case eCharPtr:
                switch (CompareOp[i])
                {
                case GT: ok = (stricmp((const char *)value, sValue[i].c_str()) >  0); break;
                case GE: ok = (stricmp((const char *)value, sValue[i].c_str()) >= 0); break;
                case EQ: ok = (stricmp((const char *)value, sValue[i].c_str()) == 0); break;
                case LE: ok = (stricmp((const char *)value, sValue[i].c_str()) <= 0); break;
                case LT: ok = (stricmp((const char *)value, sValue[i].c_str()) <  0); break;
                case NE: ok = (stricmp((const char *)value, sValue[i].c_str()) != 0); break;
                default: break;
                }
                break;

            default:
                ok = false;
            }
            if (!ok)
                break;
        }
    }

    return ok;
}

//=============================================================

/* creates problems with NEW_UNIT_ID!!!
   but that macro was changed to be land id independent */

#define XY_DELTA   0x00000800
#define X_MASK     0x00000FFF
#define Y_MASK     0x00FFF000
#define Z_MASK     0xFF000000
#define Y_SHIFT    12
#define Z_SHIFT    24


long LandCoordToId(int x, int y, int z)
{
    return ( (z           << Z_SHIFT) & Z_MASK) |
           (((y+XY_DELTA) << Y_SHIFT) & Y_MASK) |
           ( (x+XY_DELTA)             & X_MASK);
}

//-------------------------------------------------------------

void LandIdToCoord(long id, int & x, int & y, int & z)
{
    x = ( id & X_MASK)             - XY_DELTA;
    y = ((id & Y_MASK) >> Y_SHIFT) - XY_DELTA;
    z = ( id & Z_MASK) >> Z_SHIFT;
}
/**/



//-------------------------------------------------------------

/* slow

long LandCoordToId(int x, int y, int z)
{
    return (z*1000 + (y+500))*1000 + (x+500);  // range is -499 - +499
}

//-------------------------------------------------------------

void LandIdToCoord(long id, int & x, int & y, int & z)
{
    x  = id % 1000;
    x -= 500;

    id = id/1000;
    y  = id % 1000;
    y -=500;
    z  = id/1000;
}
*/

//-------------------------------------------------------------

void TestLandId()
{
    int  x0, y0, z0;
    int  x1, y1, z1;
    int  x = 0;
    long id;

#define _x_min_  -48
#define _x_max_  500

#define _y_min_  -48
#define _y_max_  500

#define _z_min_  0
#define _z_max_  255

    for (z0=_z_min_; z0<=_z_max_; z0++)
        for (y0=_y_min_; y0<=_y_max_; y0++)
            for (x0=_x_min_; x0<=_x_max_; x0++)
            {
                id = LandCoordToId(x0, y0, z0);
                LandIdToCoord(id, x1, y1, z1);
                if ( (x0!=x1) || (y0!=y1) || (z0!=z1) )
                {
                    x = 10/x;
                }
            }
    x = 0;
    x = 1;
};

//-------------------------------------------------------------
// Default (no-op) CGameDataHelper bodies. CGameDataHelper's virtuals have
// no inline bodies and its destructor is defaulted inline, so its "key
// function" (first out-of-line virtual, per the Itanium C++ ABI) needs a
// definition somewhere or any binary that odr-uses its vtable fails to
// link. This is that single canonical home - real consumers (GameRules)
// and test doubles (TestGameDataHelper, TestModelDataHelper) override
// what they need and never call down into these.

void  CGameDataHelper::ReportError(const char *, int, bool)
{
}

long  CGameDataHelper::GetStudyCost(const char *)
{
    return 0;
}

long  CGameDataHelper::GetStructAttr(const char *, long & MaxLoad, long & MinSailingPower)
{
    MaxLoad = 0;
    MinSailingPower = 0;
    return 0;
}

const char * CGameDataHelper::GetConfString(const char *, const char *)
{
    return "";
}

bool CGameDataHelper::GetOrderId(const char *, long & id)
{
    id = 0;
    return false;
}

bool CGameDataHelper::IsTradeItem(const char *)
{
    return false;
}

bool CGameDataHelper::IsMan(const char *)
{
    return false;
}

const char * CGameDataHelper::GetWeatherLine(bool, bool, int)
{
    return "";
}

const char * CGameDataHelper::ResolveAlias(const char * alias)
{
    return alias ? alias : "";
}

bool CGameDataHelper::GetItemWeights(const char *, int *&, const char **&, int & movecount)
{
    movecount = 0;
    return false;
}

void CGameDataHelper::GetMoveNames(const char **& movenames)
{
    movenames = nullptr;
}

bool CGameDataHelper::GetTropicZone(const char *, long & y_min, long & y_max)
{
    y_min = 0;
    y_max = 0;
    return false;
}

void CGameDataHelper::SetTropicZone(const char *, long, long)
{
}

void CGameDataHelper::GetProdDetails(const char *, TProdDetails & details)
{
    details = TProdDetails();
}

long CGameDataHelper::MaxSkillLevel(const char *, const char *, const char *, bool)
{
    return 0;
}

bool CGameDataHelper::ImmediateProdCheck()
{
    return false;
}

bool CGameDataHelper::CanSeeAdvResources(const char *, const char *, std::vector<long> &, std::vector<std::string> &)
{
    return false;
}

bool CGameDataHelper::ShowMoveWarnings()
{
    return false;
}

bool CGameDataHelper::IsRawMagicSkill(const char *)
{
    return false;
}

int CGameDataHelper::GetAttitudeForFaction(int)
{
    return 0;
}

void CGameDataHelper::SetAttitudeForFaction(int, int)
{
}

void CGameDataHelper::SetPlayingFaction(long)
{
}

bool CGameDataHelper::IsWagon(const char *)
{
    return false;
}

bool CGameDataHelper::IsWagonPuller(const char *)
{
    return false;
}

int CGameDataHelper::WagonCapacity()
{
    return 0;
}

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

#ifndef __AH_REP_PARSER_H__
#define __AH_REP_PARSER_H__

#include "data.h"
#include "stl_helpers.h"
#include "cfgfile.h"
#include <vector>
#include <set>
#include <unordered_map>
#include <string>

extern const char * Monthes[];

extern const char * EOL_MS   ;
extern const char * EOL_UNIX ;
extern const char * EOL_SCR  ;
extern const char * EOL_FILE ;

#define DEFAULT_PLANE "Overworld"

typedef enum { North=0, Northeast,   Southeast,   South,   Southwest,   Northwest, Center }   eDirection;
extern int   ExitFlags [];
extern int   EntryFlags[];

extern int Flags_NW_N_NE;
extern int Flags_N      ;
extern int Flags_SW_S_SE;
extern int Flags_S      ;

// MZ - O_SHARE added for Arcadia
enum {
    O_ADDRESS = 1,
    O_ADVANCE,
    O_ARMOR,
    O_ASSASSINATE,
    O_ATTACK,
    O_AUTOTAX,
    O_AVOID,
    O_BEHIND,
    O_BUILD,
    O_BUY,
    O_CAST,
    O_CLAIM,
    O_COMBAT,
    O_CONSUME,
    O_DECLARE,
    O_DESCRIBE,
    O_DESTROY,
    O_ENDFORM,
    O_ENTER,
    O_ENTERTAIN,
    O_EVICT,
    O_EXCHANGE,
    O_FACTION,
    O_FIND,
    O_FORGET,
    O_FORM,
    O_GIVE,
    O_GIVEIF,
    O_TAKE,
    O_SEND,
    O_GUARD,
    O_HOLD,
    O_LEAVE,
    O_MOVE,
    O_NAME,
    O_NOAID,
    O_NOCROSS,
    O_NOSPOILS,
    O_OPTION,
    O_PASSWORD,
    O_PILLAGE,
    O_PREPARE,
    O_PRODUCE,
    O_PROMOTE,
    O_QUIT,
    O_RESTART,
    O_REVEAL,
    O_SAIL,
    O_SELL,
    O_SHARE,
    O_SHOW,
    O_SPOILS,
    O_STEAL,
    O_STUDY,
    O_TAX,
    O_TEACH,
    O_WEAPON,
    O_WITHDRAW,
    O_WORK,
    O_RECRUIT,


    O_TYPE,
    O_LABEL,

    // must be in this sequence! O_ENDXXX == O_XXX+1
    O_TURN,
    O_ENDTURN,
    O_TEMPLATE,
    O_ENDTEMPLATE,
    O_ALL,
    O_ENDALL,


    NORDERS
};


typedef struct SAVE_HEX_OPTIONS_STRUCT
{
    bool   SaveStructs;
    bool   AlwaysSaveImmobStructs;
    bool   SaveUnits;
    bool   SaveResources;
    long   WriteTurnNo; // Add turn number atlaclient style
} SAVE_HEX_OPTIONS;



//======================================================================

class CAtlaParser
{
public:
    CAtlaParser();
    CAtlaParser(CGameDataHelper * pHelper);
    ~CAtlaParser();
    void       Clear();
    int        ParseRep(const char * FNameIn, bool Join, bool IsHistory);     // History is a rep!
    int        SaveOrders  (const char * FNameOut, const char * password, bool decorate, int factid);
    int        LoadOrders  (const char * FNameIn, int & FactionId);  // return an id of the order's faction
    void       RunOrders(CLand * pLand, const char * sCheckTeach = nullptr);
    bool       ShareSilver(CUnit * pMainUnit);
    bool       GenOrdersTeach(CUnit * pMainUnit);
    bool       GenGiveEverything(CUnit * pFrom, const char * To);
    bool       DiscardJunkItems(CUnit * pUnit, const char * junk);
    bool       DetectSpies(CUnit * pUnit, long lonum, long hinum, long amount);
    bool       ApplyDefaultOrders(bool EmptyOnly);
    int        ParseCBDataFile(const char * FNameIn);
    void       WriteMagesCSV(const char * FName, bool vertical, const char * separator, int format);

    CLand    * GetLand(int x, int y, int nPlane, bool AdjustForEdge=false);
    CLand    * GetLand(long LandId);
    CLand    * GetLand(const char * landcoords); //  "48,52[,somewhere]"
    void       GetUnitList(std::vector<CBaseObject*>* pResultColl, int x, int y, int z);
    void       CountMenForTheFaction(int FactionId);
    void       ComposeProductsLine(CLand * pLand, const char * eol, std::string & S);
    bool       LandStrCoordToId(const char * landcoords, long & id);
    void       ComposeLandStrCoord(CLand * pLand, std::string & LandStr);
    CFaction * GetFaction(int id);
    bool       SaveOneHex(CFileWriter & Dest, CLand * pLand, CPlane * pPlane, SAVE_HEX_OPTIONS * pOptions);
    long       SkillDaysToLevel(long days);
    bool       CheckResourcesForProduction(CUnit * pUnit, CLand * pLand, std::string & Error);


    int               m_CrntFactionId;
    std::string              m_CrntFactionPwd;
    std::vector<long>     m_OurFactions;
    CConfigFile     * m_pConfig = nullptr;   // set by CAhApp after construction
    CBaseObject       m_Events;
    CBaseObject       m_SecurityEvents;
    CBaseObject       m_HexEvents;
    CBaseObject       m_Errors;
    CBaseColl         m_NewProducts;

    CBaseCollById     m_Factions;
    CBaseCollById     m_Units;
    CBaseColl         m_Planes;
    long              m_YearMon;    // Current year/month accumulated for all loaded files
    std::set<std::string, CaseInsensitiveLess>   m_UnitPropertyNames;
    std::map<std::string, int> m_UnitPropertyTypes;
    std::set<std::string, CaseInsensitiveLess>   m_LandPropertyNames;
    //CStringSortColl   m_LandPropertyTypes;
    CBaseColl         m_Skills;
    CBaseColl         m_Items;
    CBaseColl         m_Objects;
//    CBaseCollByName   m_Battles;
    CBaseColl         m_Battles;
    CBaseCollById     m_Gates;

    long              m_nCurLine;
    long              m_GatesCount;
    int               m_ParseErr;
    bool              m_OrdersLoaded;
    std::string              m_FactionInfo;
    bool              m_ArcadiaSkills;

protected:
    int          ParseFactionInfo(bool GetNo, bool Join);
    int          ParseEvents(bool IsEvents=true);
    int          ParseUnclSilver(std::string & Line);
    int          ParseAttitudes(std::string & Line, bool Join);
    int          ParseTerrain (CLand * pMotherLand, int ExitDir, std::string & FirstLine, bool FullMode, CLand ** ppParsedLand);
    int          AnalyzeTerrain(CLand * pMotherLand, CLand * pLand, bool IsExit, int ExitDir, std::string & Description);
    void         ComposeHexDescriptionForArnoGame(const char * olddescr, const char * newdescr, std::string & CompositeDescr);

    void         ParseWages(CLand * pLand, const char * str1, const char * str2);
    void         CheckExit(CPlane * pPlane, int Direction, CLand * pLandSrc, CLand * pLandExit);
    int          ParseUnit(std::string & FirstLine, bool Join);
    int          ParseStructure (std::string & FirstLine);
    int          ParseErrors();
    int          ParseLines(bool Join);
    bool         ParseOneUnitEvent(std::string & EventLine, bool IsEvent, int UnitId);
    bool         ParseOneLandEvent(std::string & EventLine, bool IsEvent);
    void         ParseOneMovementEvent(const char * params, const char * structid, const char * fullevent);
    int          ParseOneEvent(std::string & EventLine, bool IsEvent);
    void         ParseWeather(const char * src, CLand * pLand);
    int          ApplyLandFlags();
    int          SetLandFlag(const char * p, long flag);
    int          SetLandFlag(long LandId, long flag);
    int          ParseBattles();
    int          ParseSkills();
    int          ParseItems();
    int          ParseObjects();
    void         SetExitFlagsAndTropicZone();
    int          SetUnitProperty(CUnit * pUnit, const char * name, EValueType type, const void * value, EPropertyType proptype);
    int          SetLandProperty(CLand * pLand, const char * name, EValueType type, const void * value, EPropertyType proptype);
    int          LoadOrders  (CFileReader & F, int FactionId, bool GetComments);
    const char * ReadPropertyName(const char * src, std::string & Name);
    void         StoreBattle(std::string & Source);
    void         AnalyzeBattle(const char * src, std::string & Details);
    void         AnalyzeBattle_OneSide(const char * src, std::string & Details);
    const char * AnalyzeBattle_ParseUnit(const char * src, CUnit *& pUnit, bool & InFrontLine);
    void         AnalyzeBattle_SummarizeUnits(CBaseColl & Units, std::string & Details);
    void         SetShaftLinks();
    void         ApplySailingEvents();
    bool         GetTargetUnitId(const char *& p, long FactionId, long & nId);
    int          ParseOneImportantEvent(std::string & EventLine);
    int          ParseImportantEvents();

    CUnit      * MakeUnit(long Id);
    CPlane     * MakePlane(const char * planename);

    bool         ReadNextLine(std::string & s);
    void         PutLineBack (std::string & s);

    void         GenericErr(int Severity, const char * Msg);
    void         OrderErr(int Severity, int UnitId, const char * Msg);
    void         OrderErrFinalize();
    void         RunLandOrders(CLand * pLand, const char * sCheckTeach = nullptr);
    void         OrderProcess_Teach(bool skiperror, CUnit * pUnit);

    // Order handlers and helpers
    void         RunOrder_Teach            (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, bool TeachCheckGlb);
    void         RunOrder_Move             (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, int & X, int & Y, int & LocA3, long order);
    void         RunOrder_Promote          (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Sell             (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Buy              (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Give             (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, bool IgnoreMissingTarget);
    void         RunOrder_Take             (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, bool IgnoreMissingTarget);
    void         RunOrder_Send             (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Produce          (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Study            (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Name             (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_SailAIII         (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, int & X, int & Y, int & LocA3);
    bool         FindTargetsForSend        (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char *& params, CUnit *& pUnit2, CLand *& pLand2);
    bool         GetItemAndAmountForGive   (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params, std::string & Item, int & amount, const char * command, CUnit * pUnit2);
    void         RunOrder_Withdraw         (std::string & Line, std::string & ErrorLine, bool skiperror, CUnit * pUnit, CLand * pLand, const char * params);

    void         AdjustSkillsAfterGivingMen(CUnit * pUnitGive, CUnit * pUnitTake, std::string & item, long AmountGiven);
    void         LookupAdvancedResourceVisibility(CUnit * pUnit, CLand * pLand);
    void         RunPseudoComment(int sequence, CLand * pLand, CUnit * pUnit, const char * src);

    int          ParseCBHex   (const char * FirstLine);
    int          ParseCBStruct(const char * FirstLine);

    CFileReader    * m_pSource;

    std::set<std::string, CaseInsensitiveLess>  m_TaxLandStrs;
    std::set<std::string, CaseInsensitiveLess>  m_TradeLandStrs;
    std::set<std::string, CaseInsensitiveLess>  m_BattleLandStrs;
    std::set<long>                              m_TradeUnitIds;
    CBaseCollByName  m_PlanesNamed;
    CBaseColl        m_LandsToBeLinked;
    std::unordered_map<std::string, long>       m_UnitFlagsHash;
    CBaseColl        m_TempSailingEvents;

    CLand          * m_pCurLand   ;
    CStruct        * m_pCurStruct ;
    int              m_NextStructId;
    std::string             m_sOrderErrors;
    bool             m_JoiningRep;  // joining an allies' report
    bool             m_IsHistory;   // parsing history file
    long             m_CurYearMon; // Year/month for the file being loaded
    std::string             m_WeatherLine[8];
};



#endif


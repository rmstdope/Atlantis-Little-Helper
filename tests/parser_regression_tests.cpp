#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "files.h"
#include "consts.h"
#include "consts_ah.h"
#include "atlaparser.h"
#include "test_helpers.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace
{
class TestGameDataHelper : public CGameDataHelper
{
public:
    TestGameDataHelper()
    {
        config[{SZ_SECT_ATTITUDES, SZ_ATT_FRIEND1}] = "Ally,Friendly";
        config[{SZ_SECT_ATTITUDES, SZ_ATT_FRIEND2}] = "Own";
        config[{SZ_SECT_ATTITUDES, SZ_ATT_NEUTRAL}] = "Neutral";
        config[{SZ_SECT_ATTITUDES, SZ_ATT_ENEMY}] = "Unfriendly,Hostile";
        config[{SZ_SECT_ATTITUDES, SZ_ATT_APPLY_ON_JOIN}] = "1";

        order_ids["endturn"] = O_ENDTURN;
        order_ids["hold"] = O_HOLD;
        order_ids["move"] = O_MOVE;
        order_ids["name"] = O_NAME;
    }

    void ReportError(const char * msg, int msglen, bool orderrelated) override
    {
        reported_errors.emplace_back(orderrelated, std::string(msg, msglen));
    }

    long GetStudyCost(const char *) override
    {
        return 0;
    }

    long GetStructAttr(const char *, long & MaxLoad, long & MinSailingPower) override
    {
        MaxLoad = 0;
        MinSailingPower = 0;
        return 0;
    }

    const char * GetConfString(const char * section, const char * param) override
    {
        const auto key = std::make_pair(std::string(section ? section : ""), std::string(param ? param : ""));
        auto it = config.find(key);
        if (it == config.end())
            return "";
        return it->second.c_str();
    }

    bool GetOrderId(const char * order, long & id) override
    {
        if (!order)
            return false;
        std::string normalized(order);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        auto it = order_ids.find(normalized);
        if (it == order_ids.end())
            return false;
        id = it->second;
        return true;
    }

    bool IsTradeItem(const char *) override
    {
        return false;
    }

    bool IsMan(const char * item) override
    {
        if (!item)
            return false;
        return std::string(item) == "men" || std::string(item) == "humans";
    }

    const char * GetWeatherLine(bool IsCurrent, bool IsGood, int Zone) override
    {
        const int index = (Zone << 2) | (IsGood ? 2 : 0) | (IsCurrent ? 1 : 0);
        return weather_lines[index].c_str();
    }

    const char * ResolveAlias(const char * alias) override
    {
        return alias ? alias : "";
    }

    bool GetItemWeights(const char *, int *& weights, const char **& movenames, int & movecount) override
    {
        weights = nullptr;
        movenames = nullptr;
        movecount = 0;
        return false;
    }

    void GetMoveNames(const char **& movenames) override
    {
        movenames = nullptr;
    }

    bool GetTropicZone(const char *, long & y_min, long & y_max) override
    {
        y_min = 0;
        y_max = 0;
        return false;
    }

    void SetTropicZone(const char *, long y_min, long y_max) override
    {
        tropic_zone = {y_min, y_max};
    }

    void GetProdDetails(const char *, TProdDetails & details) override
    {
        details = TProdDetails();
    }

    long MaxSkillLevel(const char *, const char *, const char *, bool) override
    {
        return 5;
    }

    bool ImmediateProdCheck() override
    {
        return false;
    }

    bool CanSeeAdvResources(const char *, const char *, std::vector<long> &, std::vector<std::string> &) override
    {
        return false;
    }

    bool ShowMoveWarnings() override
    {
        return false;
    }

    bool IsRawMagicSkill(const char *) override
    {
        return false;
    }

    int GetAttitudeForFaction(int) override
    {
        return ATT_FRIEND2;
    }

    void SetAttitudeForFaction(int id, int attitude) override
    {
        attitude_calls.emplace_back(id, attitude);
    }

    void SetPlayingFaction(long id) override
    {
        playing_faction = id;
    }

    bool IsWagon(const char *) override
    {
        return false;
    }

    bool IsWagonPuller(const char *) override
    {
        return false;
    }

    int WagonCapacity() override
    {
        return 0;
    }

    std::map<std::pair<std::string, std::string>, std::string> config;
    std::map<std::string, long> order_ids;
    std::vector<std::pair<int, int>> attitude_calls;
    std::vector<std::pair<bool, std::string>> reported_errors;
    long playing_faction = 0;
    std::pair<long, long> tropic_zone{0, 0};

private:
    std::array<std::string, 8> weather_lines = {
        "clear current bad z0",
        "clear future bad z0",
        "clear current good z0",
        "clear future good z0",
        "clear current bad z1",
        "clear future bad z1",
        "clear current good z1",
        "clear future good z1"
    };
};

// Saves/restores the process-global gpDataHelper around a test. CAtlaParser's
// constructor points gpDataHelper at this harness's own `helper` member; without
// restoring it here, gpDataHelper is left dangling at a destroyed stack address
// once this harness goes out of scope, corrupting whichever test runs next in
// this binary (see the identical guard/comment in model_regression_tests.cpp).
class ScopedDataHelper
{
public:
    explicit ScopedDataHelper(CGameDataHelper * helper) : previous(gpDataHelper)
    {
        gpDataHelper = helper;
    }
    ~ScopedDataHelper()
    {
        gpDataHelper = previous;
    }

private:
    CGameDataHelper * previous;
};

struct ParserHarness
{
    ParserHarness() : dataHelperGuard(&helper), parser(&helper) {}

    // Declared before `helper`/`parser` so it is destroyed *after* them,
    // restoring gpDataHelper only once both are gone.
    ScopedDataHelper dataHelperGuard;
    TestGameDataHelper helper;
    CAtlaParser parser;
};

static TempFile makeReport(const std::string & body)
{
    return TempFile(body);
}

static bool hasAttitudeCall(const std::vector<std::pair<int, int>> & calls, int id, int attitude)
{
    return std::find(calls.begin(), calls.end(), std::make_pair(id, attitude)) != calls.end();
}

}

TEST_CASE("ParseFactionInfo captures faction id and year", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    REQUIRE(harness.parser.m_CrntFactionId == 123);
    REQUIRE(harness.parser.m_YearMon == 201);
    REQUIRE(harness.helper.playing_faction == 123);
    REQUIRE(harness.parser.m_Factions.Count() == 1);

    auto * faction = static_cast<CFaction *>(harness.parser.m_Factions.At(0));
    REQUIRE(faction != nullptr);
    CHECK(faction->Id == 123);
    CHECK(faction->Name.find("Test Faction") == 0);
}

TEST_CASE("ParseUnclSilver stores faction silver", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n"
        "Unclaimed silver: 1500.\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    auto * faction = harness.parser.GetFaction(123);
    REQUIRE(faction != nullptr);
    CHECK(faction->UnclaimedSilver == 1500);
}

TEST_CASE("ParseAttitudes applies default and faction attitudes", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Declared Attitudes (default Friendly): 12(Alpha), 13(Beta).\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    REQUIRE(hasAttitudeCall(harness.helper.attitude_calls, -1, ATT_FRIEND2));
    CHECK(harness.parser.m_FactionInfo.find("Friendly") != std::string::npos);
}

TEST_CASE("ParseEvents stores normal event text", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Events during turn:\n"
        "A comet passes over the region.\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    CHECK(harness.parser.m_Events.Description.find("A comet passes over the region.") != std::string::npos);
    CHECK(harness.parser.m_Errors.Description.empty());
}

TEST_CASE("ParseErrors stores error text", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Errors during turn:\n"
        "Unit is out of range.\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    CHECK(harness.parser.m_Errors.Description.find("Unit is out of range.") != std::string::npos);
}

TEST_CASE("ParseSkills stores skill entries", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Skill reports:\n"
        "Alchemy [ALCH] 3: A test skill.\n"
        "\n"
        "Blacksmithing [BLSM] 2: Another test skill.\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);
    REQUIRE(harness.parser.m_Skills.Count() == 2);

    auto * skill = static_cast<CShortNamedObj *>(harness.parser.m_Skills.At(0));
    REQUIRE(skill != nullptr);
    CHECK(skill->Name.find("Alchemy") == 0);
    CHECK(skill->ShortName == "ALCH");
    CHECK(skill->Level == 3);
}

TEST_CASE("ParseItems stores item entries", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Item reports:\n"
        "ironwood [IRWD], weight 10. Units with lumberjack [LUMB] 3 may PRODUCE this item at a rate of 1 per man-month.\n"
        "\n"
        "mithril [MITH], weight 10. Units with mining [MINI] 3 may PRODUCE this item at a rate of 1 per man-month.\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);
    REQUIRE(harness.parser.m_Items.Count() == 2);

    auto * item = static_cast<CShortNamedObj *>(harness.parser.m_Items.At(0));
    REQUIRE(item != nullptr);
    CHECK(item->Name.find("ironwood") == 0);
    CHECK(item->ShortName == "IRWD");
}

TEST_CASE("ParseObjects stores object entries", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Object reports:\n"
        "Longboat: This is a ship. Units may enter this structure. This ship requires 5 total levels of sailing skill to sail.\n"
        "\n"
        "Shaft: This is a shaft. Units may enter this structure.\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);
    REQUIRE(harness.parser.m_Objects.Count() == 2);

    auto * object = static_cast<CShortNamedObj *>(harness.parser.m_Objects.At(0));
    REQUIRE(object != nullptr);
    CHECK(object->Name == "Longboat");
    CHECK(object->ShortName == "Longboat");
}

TEST_CASE("ParseRep creates a land and an own unit", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n"
        "plain (0,0) in Start, 10 peasants (humans), $0.\n"
        "Exits:\n"
        "* Hero (1), Test Faction (123)\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    REQUIRE(harness.parser.m_Planes.Count() == 1);
    auto * plane = static_cast<CPlane *>(harness.parser.m_Planes.At(0));
    REQUIRE(plane != nullptr);
    REQUIRE(plane->Lands.Count() == 1);

    auto * land = static_cast<CLand *>(plane->Lands.At(0));
    REQUIRE(land != nullptr);
    REQUIRE(land->Units.Count() == 1);

    auto * unit = static_cast<CUnit *>(land->Units.At(0));
    REQUIRE(unit != nullptr);
    CHECK(unit->Id == 1);
    CHECK(unit->FactionId == 123);
    CHECK(unit->IsOurs);
}

TEST_CASE("SaveOrders and LoadOrders round-trip a simple unit order", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n"
        "plain (0,0) in Start, 10 peasants (humans), $0.\n"
        "Exits:\n"
        "* Hero (1), Test Faction (123)\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    auto * unit = static_cast<CUnit *>(harness.parser.m_Units.At(0));
    REQUIRE(unit != nullptr);
    unit->Orders = "hold";

    auto saved = makeReport("");
    REQUIRE(harness.parser.SaveOrders(saved.path().c_str(), "pw", false, 123) == ERR_OK);

    unit->Orders.clear();
    int faction_id = 0;
    REQUIRE(harness.parser.LoadOrders(saved.path().c_str(), faction_id) == ERR_OK);

    CHECK(faction_id == 123);
    CHECK(unit->Orders.find("hold") != std::string::npos);
}

TEST_CASE("ParseStructure captures id, kind, and description for hex structures", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/structures.rep", false, false) == ERR_OK);

    auto * land = harness.parser.GetLand(8, 50, 0);
    REQUIRE(land != nullptr);
    REQUIRE(land->Structs.Count() == 3);

    auto * stockade = static_cast<CStruct *>(land->Structs.At(0));
    REQUIRE(stockade != nullptr);
    CHECK(stockade->Id == 1);
    CHECK(stockade->Name.find("Jungle Camp") != std::string::npos);
    CHECK(stockade->Kind == "Stockade");

    auto * timberYard = static_cast<CStruct *>(land->Structs.At(1));
    REQUIRE(timberYard != nullptr);
    CHECK(timberYard->Kind == "Timber Yard");

    auto * ranch = static_cast<CStruct *>(land->Structs.At(2));
    REQUIRE(ranch != nullptr);
    CHECK(ranch->Kind == "Ranch");
}

TEST_CASE("ParseStructure associates the first contained unit as structure owner", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/structures.rep", false, false) == ERR_OK);

    auto * land = harness.parser.GetLand(8, 50, 0);
    REQUIRE(land != nullptr);
    REQUIRE(land->Units.Count() == 2);

    auto * stockade = static_cast<CStruct *>(land->Structs.At(0));
    REQUIRE(stockade != nullptr);
    REQUIRE(stockade->Kind == "Stockade");

    CUnit dummy;
    dummy.Id = 12830;
    int idx = 0;
    REQUIRE(land->Units.Search(&dummy, idx));
    auto * guard = static_cast<CUnit *>(land->Units.At(idx));
    REQUIRE(guard != nullptr);
    CHECK(stockade->OwnerUnitId == guard->Id);
}

TEST_CASE("ParseBattles stores attacker, defender, and casualty text for a battle", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/battle_and_mixed_report.rep", false, false) == ERR_OK);

    REQUIRE(harness.parser.m_Battles.Count() == 1);
    auto * battle = static_cast<CBattle *>(harness.parser.m_Battles.At(0));
    REQUIRE(battle != nullptr);
    CHECK(battle->Name.find("Demon (972) attacks Drones (2170)") != std::string::npos);
    CHECK(battle->Description.find("Attackers:") != std::string::npos);
    CHECK(battle->Description.find("Defenders:") != std::string::npos);
    CHECK(battle->Description.find("Total Casualties:") != std::string::npos);
}

TEST_CASE("ParseRep parses faction, battle, event, skill, attitude, and terrain data from one mixed report", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/battle_and_mixed_report.rep", false, false) == ERR_OK);

    CHECK(harness.parser.m_CrntFactionId == 62);
    // Borg (62, our own faction from the header) plus Mantzikert (39, first
    // referenced only as an alien unit's owning faction) are both tracked.
    CHECK(harness.parser.m_Factions.Count() == 2);
    CHECK(harness.parser.m_Battles.Count() == 1);
    CHECK(harness.parser.m_Events.Description.find("Times reward of 200 silver.") != std::string::npos);
    CHECK(harness.parser.m_Skills.Count() == 1);
    CHECK(hasAttitudeCall(harness.helper.attitude_calls, 0, ATT_NEUTRAL));

    auto * faction = harness.parser.GetFaction(62);
    REQUIRE(faction != nullptr);
    CHECK(faction->UnclaimedSilver == 684);

    auto * land = harness.parser.GetLand(43, 79, 0);
    REQUIRE(land != nullptr);
    REQUIRE(land->Units.Count() == 2);

    CUnit dummy;
    dummy.Id = 1652;
    int idx = 0;
    REQUIRE(harness.parser.m_Units.Search(&dummy, idx));
    auto * ownUnit = static_cast<CUnit *>(harness.parser.m_Units.At(idx));
    REQUIRE(ownUnit != nullptr);
    CHECK(ownUnit->IsOurs);
    CHECK(ownUnit->Orders.find("@work") != std::string::npos);
}

TEST_CASE("ParseRep on an empty file returns ERR_OK with nothing parsed", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/empty_report.rep", false, false) == ERR_OK);

    CHECK(harness.parser.m_Factions.Count() == 0);
    CHECK(harness.parser.m_Units.Count() == 0);
    CHECK(harness.parser.m_Planes.Count() == 0);
    CHECK(harness.parser.m_Battles.Count() == 0);
}

TEST_CASE("ParseRep, LoadOrders, and SaveOrders return ERR_FOPEN for a missing file", "[parser]")
{
    ParserHarness harness;
    const char * missing = "tests/fixtures/does-not-exist.rep";

    CHECK(harness.parser.ParseRep(missing, false, false) == ERR_FOPEN);

    int factionId = 0;
    CHECK(harness.parser.LoadOrders(missing, factionId) == ERR_FOPEN);

    CHECK(harness.parser.SaveOrders("tests/fixtures/does-not-exist-dir/orders.ord", "pw", false, 1) == ERR_FOPEN);
}

TEST_CASE("ParseStructure with no active land is a graceful no-op", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/malformed_structure_no_land.rep", false, false) == ERR_OK);

    CHECK(harness.parser.m_Planes.Count() == 0);
}

TEST_CASE("ParseStructure ignores a structure line with a missing id", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/malformed_structure_no_id.rep", false, false) == ERR_OK);

    REQUIRE(harness.parser.m_Planes.Count() == 1);
    auto * plane = static_cast<CPlane *>(harness.parser.m_Planes.At(0));
    REQUIRE(plane != nullptr);
    REQUIRE(plane->Lands.Count() == 1);

    auto * land = static_cast<CLand *>(plane->Lands.At(0));
    REQUIRE(land != nullptr);
    CHECK(land->Structs.Count() == 0);
}

TEST_CASE("A missing blank line drops the last entry and skips the following section", "[parser]")
{
    ParserHarness harness;

    REQUIRE(harness.parser.ParseRep("tests/fixtures/missing_blank_separator.rep", false, false) == ERR_OK);

    // The second skill has no trailing blank line before "Item reports:", so
    // the header text is silently absorbed into its description instead of
    // being recognized as the start of a new section...
    REQUIRE(harness.parser.m_Skills.Count() == 2);
    auto * corrupted = static_cast<CShortNamedObj *>(harness.parser.m_Skills.At(1));
    REQUIRE(corrupted != nullptr);
    CHECK(corrupted->Description.find("Item reports:") != std::string::npos);

    // ...and because the header line was consumed rather than left for
    // dispatch, the entire Item reports section is skipped.
    CHECK(harness.parser.m_Items.Count() == 0);
}

TEST_CASE("SaveOrders writes only the requested faction's units", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n"
        "plain (0,0) in Start, 10 peasants (humans), $0.\n"
        "Exits:\n"
        "* Hero (1), Test Faction (123)\n"
        "- Ally (2), Ally Faction (456)\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    CUnit ourDummy;
    ourDummy.Id = 1;
    int ourIdx = 0;
    REQUIRE(harness.parser.m_Units.Search(&ourDummy, ourIdx));
    auto * ourUnit = static_cast<CUnit *>(harness.parser.m_Units.At(ourIdx));
    REQUIRE(ourUnit != nullptr);
    ourUnit->Orders = "hold";

    CUnit otherDummy;
    otherDummy.Id = 2;
    int otherIdx = 0;
    REQUIRE(harness.parser.m_Units.Search(&otherDummy, otherIdx));
    auto * otherUnit = static_cast<CUnit *>(harness.parser.m_Units.At(otherIdx));
    REQUIRE(otherUnit != nullptr);
    REQUIRE(otherUnit->FactionId == 456);
    otherUnit->Orders = "hold";

    auto saved = makeReport("");
    REQUIRE(harness.parser.SaveOrders(saved.path().c_str(), "pw", false, 123) == ERR_OK);

    const std::string contents = readFile(saved.path());
    CHECK(contents.find("unit 1") != std::string::npos);
    CHECK(contents.find("unit 2") == std::string::npos);
}

TEST_CASE("SaveOrders with decorate writes a land-description comment before each hex's orders", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n"
        "plain (0,0) in Start, 10 peasants (humans), $0.\n"
        "Exits:\n"
        "* Hero (1), Test Faction (123)\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    auto * unit = static_cast<CUnit *>(harness.parser.m_Units.At(0));
    REQUIRE(unit != nullptr);
    unit->Orders = "hold";

    auto decorated = makeReport("");
    REQUIRE(harness.parser.SaveOrders(decorated.path().c_str(), "pw", true, 123) == ERR_OK);
    const std::string decoratedContents = readFile(decorated.path());
    CHECK(decoratedContents.find("plain (0,0) in Start") != std::string::npos);
    CHECK(decoratedContents.find("unit 1") != std::string::npos);
    CHECK(decoratedContents.find("hold") != std::string::npos);

    auto plain = makeReport("");
    REQUIRE(harness.parser.SaveOrders(plain.path().c_str(), "pw", false, 123) == ERR_OK);
    const std::string plainContents = readFile(plain.path());
    CHECK(plainContents.find("unit 1") != std::string::npos);
    CHECK(plainContents.find("hold") != std::string::npos);
    CHECK(plainContents.find("plain (0,0) in Start") == std::string::npos);
}

TEST_CASE("LoadOrders ingests a real multi-unit order file and infers the faction id", "[parser]")
{
    ParserHarness harness;

    for (long id : {1289L, 1290L, 1288L})
    {
        CUnit * unit = new CUnit;
        unit->Id = id;
        unit->FactionId = 21;
        harness.parser.m_Units.Insert(unit);
    }

    int factionId = 0;
    REQUIRE(harness.parser.LoadOrders("tests/fixtures/orders_multi_unit.ord", factionId) == ERR_OK);

    CHECK(factionId == 21);

    CUnit dummy;
    dummy.Id = 1289;
    int idx = 0;
    REQUIRE(harness.parser.m_Units.Search(&dummy, idx));
    auto * unit = static_cast<CUnit *>(harness.parser.m_Units.At(idx));
    REQUIRE(unit != nullptr);
    CHECK(unit->Orders.find("claim 10") != std::string::npos);
    CHECK(unit->Orders.find("study comb") != std::string::npos);
}

TEST_CASE("LoadOrders on a file with no unit lines leaves FactionId at 0 and existing orders untouched", "[parser]")
{
    ParserHarness harness;
    auto report = makeReport(
        "Atlantis Report For:\n"
        "Test Faction (123)\n"
        "January, Year 2\n"
        "\n"
        "plain (0,0) in Start, 10 peasants (humans), $0.\n"
        "Exits:\n"
        "* Hero (1), Test Faction (123)\n"
        "\n");

    REQUIRE(harness.parser.ParseRep(report.path().c_str(), false, false) == ERR_OK);

    auto * unit = static_cast<CUnit *>(harness.parser.m_Units.At(0));
    REQUIRE(unit != nullptr);
    unit->Orders = "hold";

    int factionId = 999;
    REQUIRE(harness.parser.LoadOrders("tests/fixtures/orders_no_units.ord", factionId) == ERR_OK);

    CHECK(factionId == 0);
    CHECK(unit->Orders == "hold");
}

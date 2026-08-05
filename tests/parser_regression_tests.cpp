#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "files.h"
#include "consts.h"
#include "consts_ah.h"
#include "atlaparser.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{
class TempFile
{
public:
    explicit TempFile(const std::string & contents)
    {
        char pattern[] = "/tmp/alh-parser-XXXXXX";
        const int fd = mkstemp(pattern);
        if (fd < 0)
            throw std::runtime_error("mkstemp failed");

        path_ = pattern;
        close(fd);

        std::ofstream out(path_);
        if (!out)
            throw std::runtime_error("failed to open temp file");
        out << contents;
    }

    ~TempFile()
    {
        if (!path_.empty())
            unlink(path_.c_str());
    }

    const std::string & path() const
    {
        return path_;
    }

private:
    std::string path_;
};

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

struct ParserHarness
{
    ParserHarness() : parser(&helper) {}

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

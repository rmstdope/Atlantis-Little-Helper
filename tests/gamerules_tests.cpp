#include "catch.hpp"

#include "gamerules.h"
#include "configmanager.h"
#include "consts.h"
#include "consts_ah.h"
#include "files.h"
#include "atlaparser.h"

namespace
{
// GameRules methods read through gpConfigManager; point it at a
// locally-owned, freshly constructed ConfigManager for the duration of
// each test so tests can't see each other's state or the real ah.cfg.
class ScopedConfigManager
{
public:
    ScopedConfigManager() : prev_(gpConfigManager)
    {
        gpConfigManager = &cfg_;
    }
    ~ScopedConfigManager()
    {
        gpConfigManager = prev_;
    }

    ConfigManager cfg_;

private:
    ConfigManager * prev_;
};
}

//=============================================================
// Issue #24's target list

TEST_CASE("GameRules GetOrderId resolves the hardcoded order table and rejects unknown orders", "[gamerules]")
{
    ScopedConfigManager scm;
    GameRules rules;
    rules.Init();

    long id;
    REQUIRE(rules.GetOrderId("move", id));
    CHECK(id == O_MOVE);

    REQUIRE(rules.GetOrderId("study", id));
    CHECK(id == O_STUDY);

    CHECK_FALSE(rules.GetOrderId("not-a-real-order", id));
}

TEST_CASE("GameRules GetOrderId is case-insensitive and rejects a null order", "[gamerules]")
{
    ScopedConfigManager scm;
    GameRules rules;
    rules.Init();

    long id;
    REQUIRE(rules.GetOrderId("MOVE", id));
    CHECK(id == O_MOVE);

    REQUIRE(rules.GetOrderId("Move", id));
    CHECK(id == O_MOVE);

    CHECK_FALSE(rules.GetOrderId(nullptr, id));
}

TEST_CASE("GameRules GetStructAttr parses attribute flags and the two-token MaxLoad/MinSailingPower attributes", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_STRUCTS, "Tower", "MOBILE,MAX_LOAD 50,MIN_POWER 2");
    GameRules rules;

    long maxLoad, minSail;
    long attr = rules.GetStructAttr("Tower", maxLoad, minSail);

    CHECK((attr & SA_MOBILE) != 0);
    CHECK(maxLoad == 50);
    CHECK(minSail == 2);
}

TEST_CASE("GameRules GetStructAttr always sets the gate flag for the gate structure kind", "[gamerules]")
{
    ScopedConfigManager scm;
    GameRules rules;

    long maxLoad, minSail;
    long attr = rules.GetStructAttr(STRUCT_GATE, maxLoad, minSail);

    CHECK((attr & SA_GATE) != 0);
}

TEST_CASE("GameRules GetStudyCost resolves aliases before looking up the cost", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_ALIAS, "ZZALIASSRC", "ZZALIASDST");
    scm.cfg_.SetConfig(SZ_SECT_STUDY_COST, "ZZALIASDST", "200");
    GameRules rules;

    CHECK(rules.GetStudyCost("ZZALIASSRC") == 200);
}

TEST_CASE("GameRules ResolveAlias follows an alias chain and stops on a cycle", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_ALIAS, "A", "B");
    scm.cfg_.SetConfig(SZ_SECT_ALIAS, "B", "C");
    GameRules rules;

    CHECK(std::string(rules.ResolveAlias("A")) == "C");

    // A cycle (A -> B -> A) must not hang; it falls back to the original name.
    scm.cfg_.SetConfig(SZ_SECT_ALIAS, "A", "B");
    scm.cfg_.SetConfig(SZ_SECT_ALIAS, "B", "A");
    CHECK(std::string(rules.ResolveAlias("A")) == "A");
}

TEST_CASE("GameRules IsMan/IsTradeItem/IsMagicSkill reflect the config-driven property groups", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_UNITPROP_GROUPS, PRP_MEN, "LEADERS,VIKING");
    scm.cfg_.SetConfig(SZ_SECT_UNITPROP_GROUPS, PRP_TRADE_ITEMS, "IRON,GRAIN");
    scm.cfg_.SetConfig(SZ_SECT_UNITPROP_GROUPS, PRP_MAG_SKILLS, "FORCE_SKILL,PATTERN_SKILL");
    GameRules rules;
    rules.Init();

    CHECK(rules.IsMan("LEADERS"));
    CHECK_FALSE(rules.IsMan("IRON"));

    CHECK(rules.IsTradeItem("IRON"));
    CHECK_FALSE(rules.IsTradeItem("LEADERS"));

    CHECK(rules.IsMagicSkill("FORCE"));
    CHECK_FALSE(rules.IsMagicSkill("LEADERS"));
}

TEST_CASE("GameRules GetWeatherLine picks the correct config key for each current/good/zone combination", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_WEATHER, SZ_KEY_WEATHER_CUR_GOOD_TROPIC, "Clear skies");
    scm.cfg_.SetConfig(SZ_SECT_WEATHER, SZ_KEY_WEATHER_NEXT_BAD_MEDIUM, "Storms ahead");
    GameRules rules;

    CHECK(std::string(rules.GetWeatherLine(true, true, 0)) == "Clear skies");
    CHECK(std::string(rules.GetWeatherLine(false, false, 1)) == "Storms ahead");
}

TEST_CASE("GameRules GetMaxRaceSkillLevel caps a skill at the race's max level and caches the result", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_MAX_SKILL_LVL, "HUMAN", "3,1,COMBAT,STEALTH");
    GameRules rules;

    CHECK(rules.GetMaxRaceSkillLevel("HUMAN", "COMBAT", nullptr, false) == 3);
    CHECK(rules.GetMaxRaceSkillLevel("HUMAN", "FARMING", nullptr, false) == 1);

    // Second call for the same key must return the cached value unchanged
    // even though the underlying config no longer has an entry for it.
    scm.cfg_.RemoveSection(SZ_SECT_MAX_SKILL_LVL);
    CHECK(rules.GetMaxRaceSkillLevel("HUMAN", "COMBAT", nullptr, false) == 3);
}

TEST_CASE("GameRules CanSeeAdvResources matches a skill's resources against a terrain's resources", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_RESOURCE_SKILL, "MINING", "IRON 1,SILVER 3");
    scm.cfg_.SetConfig(SZ_SECT_RESOURCE_LAND, "MOUNTAIN", "IRON,STONE");
    GameRules rules;

    std::vector<long> levels;
    std::vector<std::string> resources;
    bool ok = rules.CanSeeAdvResources("MINING", "MOUNTAIN", levels, resources);

    REQUIRE(ok);
    REQUIRE(resources.size() == 1);
    CHECK(resources[0] == "IRON");
    CHECK(levels[0] == 1);
}

//=============================================================
// Absorbed CGameDataHelper virtuals (previously implemented directly
// against gpApp->GetConfig in ahapp.cpp's out-of-line forwarding block)

TEST_CASE("GameRules attitude tracking defaults to neutral, resolves player faction as friend2, and is settable", "[gamerules]")
{
    ScopedConfigManager scm;
    GameRules rules;
    rules.SetAttitudeForFaction(0, ATT_NEUTRAL); // matches GameRules::Init()

    CHECK(rules.GetAttitudeForFaction(999) == ATT_NEUTRAL);

    rules.SetAttitudeForFaction(42, ATT_FRIEND1);
    CHECK(rules.GetAttitudeForFaction(42) == ATT_FRIEND1);

    scm.cfg_.SetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID, 42L);
    CHECK(rules.GetAttitudeForFaction(42) == ATT_FRIEND2);
}

TEST_CASE("GameRules IsWagon/IsWagonPuller/WagonCapacity read the configured wagon lists", "[gamerules]")
{
    ScopedConfigManager scm;
    scm.cfg_.SetConfig(SZ_SECT_COMMON, SZ_KEY_WAGONS, "WAGON,CART");
    scm.cfg_.SetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_PULLERS, "HORSE,OX");
    scm.cfg_.SetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_CAPACITY, 50L);
    GameRules rules;

    CHECK(rules.IsWagon("CART"));
    CHECK_FALSE(rules.IsWagon("HORSE"));
    CHECK(rules.IsWagonPuller("HORSE"));
    CHECK_FALSE(rules.IsWagonPuller("CART"));
    CHECK(rules.WagonCapacity() == 50);
}

TEST_CASE("GameRules GetTropicZone/SetTropicZone round-trip a plane's y-range", "[gamerules]")
{
    ScopedConfigManager scm;
    GameRules rules;

    long yMin, yMax;
    CHECK_FALSE(rules.GetTropicZone("nowhere", yMin, yMax));

    rules.SetTropicZone("faerun", 10, 20);
    REQUIRE(rules.GetTropicZone("faerun", yMin, yMax));
    CHECK(yMin == 10);
    CHECK(yMax == 20);
}

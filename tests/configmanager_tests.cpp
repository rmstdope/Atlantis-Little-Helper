#include "catch.hpp"

#include "configmanager.h"
#include "consts_ah.h"
#include "test_helpers.h"

namespace
{
// ConfigManager::Init() loads from fixed filenames (SZ_CONFIG_FILE/
// SZ_CONFIG_STATE_FILE) via CConfigFile::Load(), which just leaves the
// object empty when the file doesn't exist - so a freshly constructed,
// un-Init()'d ConfigManager is a safe, empty starting point for these
// tests without needing to touch the filesystem.
}

TEST_CASE("ConfigManager SetConfig/GetConfig round-trips a value", "[configmanager]")
{
    ConfigManager cfg;

    cfg.SetConfig("MySection", "MyKey", "MyValue");
    CHECK(std::string(cfg.GetConfig("MySection", "MyKey")) == "MyValue");
}

TEST_CASE("ConfigManager SetConfig long overload stores a stringified value", "[configmanager]")
{
    ConfigManager cfg;

    cfg.SetConfig("MySection", "Count", 42L);
    CHECK(std::string(cfg.GetConfig("MySection", "Count")) == "42");
}

TEST_CASE("ConfigManager routes sections registered via Init to the state file, others to the config file", "[configmanager]")
{
    ConfigManager cfg;
    cfg.Init();

    // SZ_SECT_REPORTS is one of the sections Init() registers into
    // m_ConfigSectionsState, so it must route to CONFIG_FILE_STATE.
    CHECK(cfg.GetConfigFile(SZ_SECT_REPORTS) == &cfg.m_Config[CONFIG_FILE_STATE]);

    // An arbitrary section not in that set routes to CONFIG_FILE_CONFIG.
    CHECK(cfg.GetConfigFile("SomeArbitrarySection") == &cfg.m_Config[CONFIG_FILE_CONFIG]);
}

TEST_CASE("ConfigManager routes composite ORDERS_<factionid> sections to the state file", "[configmanager]")
{
    ConfigManager cfg;
    cfg.Init();

    std::string section;
    cfg.ComposeConfigOrdersSection(section, 42);

    CHECK(cfg.GetConfigFile(section.c_str()) == &cfg.m_Config[CONFIG_FILE_STATE]);
}

TEST_CASE("ConfigManager GetSectionFirst/GetSectionNext iterate a section's entries", "[configmanager]")
{
    ConfigManager cfg;

    cfg.SetConfig("Sect", "A", "1");
    cfg.SetConfig("Sect", "B", "2");

    const char * name;
    const char * value;
    int idx = cfg.GetSectionFirst("Sect", name, value);
    REQUIRE(idx >= 0);
    CHECK(std::string(name) == "A");
    CHECK(std::string(value) == "1");

    idx = cfg.GetSectionNext(idx, "Sect", name, value);
    REQUIRE(idx >= 0);
    CHECK(std::string(name) == "B");
    CHECK(std::string(value) == "2");
}

TEST_CASE("ConfigManager MoveSectionEntries relocates all entries and removes the source section", "[configmanager]")
{
    ConfigManager cfg;

    cfg.SetConfig("Old", "A", "1");
    cfg.SetConfig("Old", "B", "2");

    cfg.MoveSectionEntries(CONFIG_FILE_CONFIG, "Old", "New");

    const char * name;
    const char * value;
    CHECK(cfg.m_Config[CONFIG_FILE_CONFIG].GetFirstInSection("Old", name, value) == -1);

    int idx = cfg.m_Config[CONFIG_FILE_CONFIG].GetFirstInSection("New", name, value);
    REQUIRE(idx >= 0);
    CHECK(std::string(name) == "A");
    CHECK(std::string(value) == "1");
}

TEST_CASE("ConfigManager RemoveSection removes a section routed to either underlying file", "[configmanager]")
{
    ConfigManager cfg;
    cfg.Init();

    cfg.SetConfig(SZ_SECT_REPORTS, "199901", "report.rep"); // routes to CONFIG_FILE_STATE
    cfg.RemoveSection(SZ_SECT_REPORTS);

    CHECK(cfg.m_Config[CONFIG_FILE_STATE].GetByName(SZ_SECT_REPORTS, "199901") == nullptr);
}

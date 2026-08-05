#include "catch.hpp"

#include "cfgfile.h"
#include "test_helpers.h"

#include <string>
#include <vector>

namespace
{
std::vector<std::string> collectSectionNames(CConfigFile & cfg, const char * section)
{
    std::vector<std::string> names;
    const char * name;
    const char * value;
    int idx = cfg.GetFirstInSection(section, name, value);
    while (idx != -1)
    {
        names.push_back(name);
        idx = cfg.GetNextInSection(idx, section, name, value);
    }
    return names;
}
}

//=============================================================
// Load / GetByName

TEST_CASE("CConfigFile Load parses the sample fixture's sections and values", "[config]")
{
    CConfigFile cfg;
    REQUIRE(cfg.Load("tests/fixtures/sample_config.cfg"));

    CHECK(std::string(cfg.GetByName("General", "AppName")) == "Atlantis Little Helper");
    CHECK(std::string(cfg.GetByName("General", "Version")) == "1.2.3");
    CHECK(std::string(cfg.GetByName("General", "Empty")) == "");
    CHECK(std::string(cfg.GetByName("Display", "Theme")) == "Dark");
    CHECK(std::string(cfg.GetByName("Display", "Formula")) == "a=b+c");
}

TEST_CASE("CConfigFile GetByName is case-insensitive on section and name, and returns null when missing", "[config]")
{
    TempFile file("[Section]\nName = Value\n");
    CConfigFile cfg;
    REQUIRE(cfg.Load(file.path().c_str()));

    CHECK(std::string(cfg.GetByName("Section", "Name")) == "Value");
    CHECK(std::string(cfg.GetByName("section", "name")) == "Value");
    CHECK(std::string(cfg.GetByName("SECTION", "NAME")) == "Value");

    CHECK(cfg.GetByName("Other", "Name") == nullptr);
    CHECK(cfg.GetByName("Section", "Missing") == nullptr);
}

TEST_CASE("CConfigFile Load returns false and leaves the object empty for a missing file", "[config]")
{
    CConfigFile cfg;
    CHECK_FALSE(cfg.Load("tests/fixtures/does-not-exist.cfg"));

    CHECK(cfg.GetByName("Any", "Thing") == nullptr);
    const char * section;
    CHECK_FALSE(cfg.GetNextSection("", section));
}

//=============================================================
// Save / round-trip

TEST_CASE("CConfigFile Save+Load round trip via the sample fixture preserves structure and the multi-line comment", "[config]")
{
    CConfigFile cfg;
    REQUIRE(cfg.Load("tests/fixtures/sample_config.cfg"));

    TempFile output("");
    REQUIRE(cfg.Save(output.path().c_str()));

    const std::string saved = readFile(output.path());

    // The multi-line comment is preserved verbatim and still precedes the key it annotates.
    const auto commentLine1 = saved.find("# Application display name shown in the");
    const auto commentLine2 = saved.find("# window title bar");
    const auto appNameLine = saved.find("AppName");
    REQUIRE(commentLine1 != std::string::npos);
    REQUIRE(commentLine2 != std::string::npos);
    REQUIRE(appNameLine != std::string::npos);
    CHECK(commentLine1 < commentLine2);
    CHECK(commentLine2 < appNameLine);

    // A blank/whitespace-only value ("Empty") is not written back out at all.
    CHECK(saved.find("Empty") == std::string::npos);

    CConfigFile reloaded;
    REQUIRE(reloaded.Load(output.path().c_str()));
    CHECK(std::string(reloaded.GetByName("General", "AppName")) == "Atlantis Little Helper");
    CHECK(std::string(reloaded.GetByName("General", "Version")) == "1.2.3");
    CHECK(std::string(reloaded.GetByName("Display", "Theme")) == "Dark");
    CHECK(std::string(reloaded.GetByName("Display", "Formula")) == "a=b+c");

    // The omitted blank value does not resurrect itself on the next load.
    CHECK(reloaded.GetByName("General", "Empty") == nullptr);
}

TEST_CASE("CConfigFile Save omits blank/whitespace-only values but keeps them queryable in memory beforehand", "[config]")
{
    CConfigFile cfg;
    cfg.SetByName("Sect", "Real", "Value");
    cfg.SetByName("Sect", "Blank", "   ");

    CHECK(std::string(cfg.GetByName("Sect", "Blank")) == "   ");

    TempFile output("");
    REQUIRE(cfg.Save(output.path().c_str()));

    CConfigFile reloaded;
    REQUIRE(reloaded.Load(output.path().c_str()));
    CHECK(std::string(reloaded.GetByName("Sect", "Real")) == "Value");
    CHECK(reloaded.GetByName("Sect", "Blank") == nullptr);
}

TEST_CASE("CConfigFile Save returns false when the destination directory does not exist", "[config]")
{
    CConfigFile cfg;
    cfg.SetByName("Sect", "Key", "Value");

    CHECK_FALSE(cfg.Save("tests/fixtures/does-not-exist-dir/output.cfg"));
}

//=============================================================
// SetByName / RemoveSection

TEST_CASE("CConfigFile SetByName updates an existing key, inserts a new key, and removes a key given a null value", "[config]")
{
    CConfigFile cfg;

    cfg.SetByName("Sect", "Key", "V1");
    CHECK(std::string(cfg.GetByName("Sect", "Key")) == "V1");

    cfg.SetByName("Sect", "Key", "V2");
    CHECK(std::string(cfg.GetByName("Sect", "Key")) == "V2");

    cfg.SetByName("Sect", "Key", nullptr);
    CHECK(cfg.GetByName("Sect", "Key") == nullptr);

    cfg.SetByName("Sect", "NewKey", "New");
    CHECK(std::string(cfg.GetByName("Sect", "NewKey")) == "New");
}

TEST_CASE("CConfigFile RemoveSection removes only the matching section's entries, case-insensitively", "[config]")
{
    CConfigFile cfg;
    cfg.SetByName("SectA", "Key1", "A1");
    cfg.SetByName("SectA", "Key2", "A2");
    cfg.SetByName("SectB", "Key1", "B1");

    cfg.RemoveSection("secta");

    CHECK(cfg.GetByName("SectA", "Key1") == nullptr);
    CHECK(cfg.GetByName("SectA", "Key2") == nullptr);
    CHECK(std::string(cfg.GetByName("SectB", "Key1")) == "B1");
}

//=============================================================
// Iteration

TEST_CASE("CConfigFile GetFirstInSection/GetNextInSection iterate only the entries in that section", "[config]")
{
    CConfigFile cfg;
    cfg.SetByName("Sect", "A", "1");
    cfg.SetByName("Sect", "B", "2");
    cfg.SetByName("Sect", "C", "3");
    cfg.SetByName("Other", "D", "4");

    const auto names = collectSectionNames(cfg, "Sect");
    REQUIRE(names.size() == 3);
    CHECK(names[0] == "A");
    CHECK(names[1] == "B");
    CHECK(names[2] == "C");

    const char * name;
    const char * value;
    CHECK(cfg.GetFirstInSection("Missing", name, value) == -1);
}

TEST_CASE("CConfigFile GetNextSection walks distinct sections in order, distinguishing a name that is a prefix of another", "[config]")
{
    CConfigFile cfg;
    cfg.SetByName("A", "Key", "1");
    cfg.SetByName("AB", "Key", "2");
    cfg.SetByName("B", "Key", "3");

    const char * section;
    REQUIRE(cfg.GetNextSection("", section));
    CHECK(std::string(section) == "A");

    REQUIRE(cfg.GetNextSection("A", section));
    CHECK(std::string(section) == "AB");

    REQUIRE(cfg.GetNextSection("AB", section));
    CHECK(std::string(section) == "B");

    CHECK_FALSE(cfg.GetNextSection("B", section));
}

//=============================================================
// Parsing edge cases

TEST_CASE("CConfigFile Load discards a line with a name but no '=' before the newline, without corrupting later entries", "[config]")
{
    TempFile file("[Sect]\nBadLineNoEquals\nGood = Value\n");
    CConfigFile cfg;
    REQUIRE(cfg.Load(file.path().c_str()));

    CHECK(std::string(cfg.GetByName("Sect", "Good")) == "Value");
    CHECK(collectSectionNames(cfg, "Sect").size() == 1);
}

TEST_CASE("CConfigFile Load keeps the first occurrence of a duplicate key and drops the later one", "[config]")
{
    TempFile file("[Sect]\nKey = First\nKey = Second\n");
    CConfigFile cfg;
    REQUIRE(cfg.Load(file.path().c_str()));

    CHECK(std::string(cfg.GetByName("Sect", "Key")) == "First");
    CHECK(collectSectionNames(cfg, "Sect").size() == 1);
}

TEST_CASE("CConfigFile Load parses CRLF-terminated files the same as LF-terminated files", "[config]")
{
    TempFile file("[Sect]\r\nKey = Value\r\n");
    CConfigFile cfg;
    REQUIRE(cfg.Load(file.path().c_str()));

    CHECK(std::string(cfg.GetByName("Sect", "Key")) == "Value");
}

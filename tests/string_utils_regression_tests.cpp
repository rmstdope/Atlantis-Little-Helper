#include "catch.hpp"

#include "string_utils.h"

#include <string>

TEST_CASE("Given nulls when SafeCmp is called then null ordering is stable", "[string_utils]")
{
    CHECK(SafeCmp(nullptr, nullptr) == 0);
    CHECK(SafeCmp(nullptr, "x") < 0);
    CHECK(SafeCmp("x", nullptr) > 0);
    CHECK(SafeCmp("AbC", "aBc") == 0);
}

TEST_CASE("Given strings with embedded whitespace when SafeCmpNoSpaces is called then space-insensitive compare is stable", "[string_utils]")
{
    CHECK(SafeCmpNoSpaces("combat spell", "combatspell") == 0);
    CHECK(SafeCmpNoSpaces(" alpha\tbeta ", "alpha beta") == 0);
}

TEST_CASE("Given prefix-only equality when SafeCmpNoSpaces is called then longer non-space tails do not compare equal", "[string_utils]")
{
    CHECK(SafeCmpNoSpaces("Combatspell", "CombatspellX") < 0);
    CHECK(SafeCmpNoSpaces("CombatspellX", "Combatspell") > 0);
}

TEST_CASE("Given leading whitespace when SkipSpaces is called then it returns first non-space byte", "[string_utils]")
{
    const char * p = SkipSpaces(" \t\r\nAlpha");
    REQUIRE(p != nullptr);
    CHECK(std::string(p) == "Alpha");
    CHECK(SkipSpaces(nullptr) == nullptr);
}

TEST_CASE("Given chained stream-like appends when operator<< overloads are used then values are appended in order", "[string_utils]")
{
    std::string s;
    s << "A" << std::string("B") << 'C' << 12L << 34UL << 5.2;
    CHECK(s == "ABC12345.20");
}

TEST_CASE("Given add set insert helpers when used then string payload updates match contract", "[string_utils]")
{
    std::string s = "ab";

    AddCh(s, 'c');
    CHECK(s == "abc");

    DelCh(s, 1);
    CHECK(s == "ac");

    SetCh(s, 1, 'Z');
    CHECK(s == "aZ");

    AddStr(s, "12");
    CHECK(s == "aZ12");
    AddStr(s, "xyz", 2);
    CHECK(s == "aZ12xy");

    SetStr(s, "reset");
    CHECK(s == "reset");
    SetStr(s, "buffer", 3);
    CHECK(s == "buf");

    InsStr(s, "++", 1);
    CHECK(s == "b++uf");
    InsStr(s, "XYZZY", 2, 2);
    CHECK(s == "b+XY+uf");

    DelCh(s, -1);
    SetCh(s, 999, 'q');
    CHECK(s == "b+XY+uf");
}

TEST_CASE("Given numeric append helpers when used then decimal formatting is deterministic", "[string_utils]")
{
    std::string s;
    AddLong(s, -7);
    AddULong(s, 42);
    AddDouble(s, 1.234, 6, 2);
    CHECK(s == "-742  1.23");
}

TEST_CASE("Given binary buffer helpers when used then bytes are appended and inserted by position", "[string_utils]")
{
    std::string s = "ace";
    const char data[] = {'b', 'd'};

    AddBuf(s, data, 2);
    CHECK(s == "acebd");

    const char one = 'X';
    InsBuf(s, &one, 1, 1);
    CHECK(s == "aXcebd");
}

TEST_CASE("Given delimited source when GetToken char-limit overload is called then token and next pointer are returned", "[string_utils]")
{
    std::string out;
    const char * src = "\"Alpha Beta\",Rest";
    char * next = GetToken(out, src, ',', TRIM_SPACES, true);

    CHECK(out == "Alpha Beta");
    REQUIRE(next != nullptr);
    CHECK(std::string(next) == ",Rest");
}

TEST_CASE("Given multiple possible delimiters when GetToken set-limit overload is called then LimitUsed reports the matched one", "[string_utils]")
{
    std::string out;
    char limitUsed = 0;
    const char * src = "value1|value2";

    char * next = GetToken(out, src, "|,", limitUsed, TRIM_NONE, false);
    CHECK(out == "value1");
    CHECK(limitUsed == '|');
    REQUIRE(next != nullptr);
    CHECK(std::string(next) == "value2");
}

TEST_CASE("Given numeric and non-numeric prefixes when GetInteger is called then value and validity are reported consistently", "[string_utils]")
{
    std::string out;
    bool valid = false;
    char * next = GetInteger(out, "-123rest", valid);
    CHECK(out == "-123");
    CHECK(valid);
    REQUIRE(next != nullptr);
    CHECK(std::string(next) == "rest");

    next = GetInteger(out, "-", valid);
    CHECK(out == "-");
    CHECK_FALSE(valid);
    REQUIRE(next != nullptr);
    CHECK(*next == '\0');
}

TEST_CASE("Given decimal and malformed prefixes when GetDouble is called then parsed prefix and validity are deterministic", "[string_utils]")
{
    std::string out;
    bool valid = false;
    char * next = GetDouble(out, "-12.50kg", valid);
    CHECK(out == "-12.50");
    CHECK(valid);
    REQUIRE(next != nullptr);
    CHECK(std::string(next) == "kg");

    next = GetDouble(out, ".", valid);
    CHECK(out == ".");
    CHECK_FALSE(valid);
}

TEST_CASE("Given trim modes when TrimLeft and TrimRight are called then only configured whitespace is removed", "[string_utils]")
{
    std::string s1 = "\t  hello \t";
    TrimLeft(s1, TRIM_SPACES);
    CHECK(s1 == "hello \t");
    TrimRight(s1, TRIM_SPACES);
    CHECK(s1 == "hello");

    std::string s2 = "\r\n hello \n";
    TrimLeft(s2, TRIM_ALL);
    TrimRight(s2, TRIM_ALL);
    CHECK(s2 == "hello");
}

TEST_CASE("Given a format string when Format is called then output is generated with printf semantics", "[string_utils]")
{
    std::string out;
    Format(out, "%s-%d-%.1f", "v", 12, 4.25);
    CHECK(out == "v-12-4.2");
}

TEST_CASE("Given case-insensitive haystacks when FindSubStr and FindSubStrR are called then first and last matches are returned", "[string_utils]")
{
    const std::string s = "One two ONE two";
    CHECK(FindSubStr(s, "one") == 0);
    CHECK(FindSubStrR(s, "one") == 8);
    CHECK(FindSubStr(s, "missing") == -1);
    CHECK(FindSubStrR(s, nullptr) == -1);
}

TEST_CASE("Given substring deletion bounds when DelSubStr is called then only in-range bytes are removed", "[string_utils]")
{
    std::string s = "abcdef";
    DelSubStr(s, 2, 10);
    CHECK(s == "ab");

    DelSubStr(s, -1, 2);
    CHECK(s == "ab");
}

TEST_CASE("Given mixed whitespace when Normalize is called then runs are collapsed and edges are trimmed", "[string_utils]")
{
    std::string s = " \tAlpha \r\n  Beta\t ";
    Normalize(s);
    CHECK(s == "Alpha Beta");
}

TEST_CASE("Given line breaks when RemoveLineBreaks is called then embedded and trailing CR/LF are removed while index-0 is preserved", "[string_utils]")
{
    std::string s = "\nA\r\nB\n";
    RemoveLineBreaks(s);
    // Legacy contract: the helper iterates from the end down to index 1.
    CHECK(s == "\nAB");
}

TEST_CASE("Given target characters when Replace is called then matches after index 0 are replaced", "[string_utils]")
{
    std::string s = "a_b_a";
    Replace(s, 'a', 'X');
    CHECK(s == "a_b_X");
}

TEST_CASE("Given signed digit strings when IsInteger is called then current legacy integer classification is preserved", "[string_utils]")
{
    CHECK(IsInteger("0"));
    CHECK(IsInteger("-12"));
    CHECK_FALSE(IsInteger(""));
    // Legacy behavior: a lone '-' is currently classified as integer.
    CHECK(IsInteger("-"));
    CHECK_FALSE(IsInteger("12a"));
}

TEST_CASE("Given mixed-case text when ToLower is called then string is lower-cased in place and returned", "[string_utils]")
{
    std::string s = "AbC123";
    const char * ptr = ToLower(s);
    CHECK(s == "abc123");
    CHECK(ptr == s.c_str());
}

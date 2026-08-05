#include "catch.hpp"

#include "files.h"
#include "test_helpers.h"

#include <string>

//=============================================================
// CFileReader::GetNextLine

TEST_CASE("CFileReader GetNextLine returns each line including its trailing newline", "[fileio]")
{
    TempFile file("First line\nSecond line\n");
    CFileReader reader;
    REQUIRE(reader.Open(file.path().c_str()));

    std::string line;
    REQUIRE(reader.GetNextLine(line));
    CHECK(line == "First line\n");

    REQUIRE(reader.GetNextLine(line));
    CHECK(line == "Second line\n");

    CHECK_FALSE(reader.GetNextLine(line));
}

TEST_CASE("CFileReader GetNextLine returns a final line with no trailing newline once, then reports end of file", "[fileio]")
{
    TempFile file("Only line, no newline");
    CFileReader reader;
    REQUIRE(reader.Open(file.path().c_str()));

    std::string line;
    REQUIRE(reader.GetNextLine(line));
    CHECK(line == "Only line, no newline");

    CHECK_FALSE(reader.GetNextLine(line));
}

TEST_CASE("CFileReader GetNextLine on an empty file returns false immediately", "[fileio]")
{
    TempFile file("");
    CFileReader reader;
    REQUIRE(reader.Open(file.path().c_str()));

    std::string line;
    CHECK_FALSE(reader.GetNextLine(line));
}

//=============================================================
// CFileReader pushback (QueueChar / QueueString)

TEST_CASE("CFileReader QueueChar pushes a single character back onto the stream", "[fileio]")
{
    TempFile file("BC");
    CFileReader reader;
    REQUIRE(reader.Open(file.path().c_str()));

    reader.QueueChar('A');

    char ch;
    REQUIRE(reader.GetNextChar(ch));
    CHECK(ch == 'A');
    REQUIRE(reader.GetNextChar(ch));
    CHECK(ch == 'B');
    REQUIRE(reader.GetNextChar(ch));
    CHECK(ch == 'C');
    CHECK_FALSE(reader.GetNextChar(ch));
}

TEST_CASE("CFileReader QueueString implements pushback: requeued content is read again before the rest of the file", "[fileio]")
{
    TempFile file("Line1\nLine2\n");
    CFileReader reader;
    REQUIRE(reader.Open(file.path().c_str()));

    std::string line1;
    REQUIRE(reader.GetNextLine(line1));
    CHECK(line1 == "Line1\n");

    // Push the line back, mirroring how CAtlaParser::PutLineBack implements lookahead.
    reader.QueueString(line1.c_str(), (int)line1.size());

    std::string reread;
    REQUIRE(reader.GetNextLine(reread));
    CHECK(reread == "Line1\n");

    std::string line2;
    REQUIRE(reader.GetNextLine(line2));
    CHECK(line2 == "Line2\n");
}

//=============================================================
// Internal read-buffer refill boundary

TEST_CASE("CFileReader reassembles a line correctly across an internal buffer refill boundary", "[fileio]")
{
    const size_t markerStart = static_cast<size_t>(RW_BUF_SIZE - 3);
    const std::string prefix(markerStart, 'a');
    const std::string suffix(50, 'b');
    const std::string content = prefix + "MARKER" + suffix + "\n";
    REQUIRE(content.size() > static_cast<size_t>(RW_BUF_SIZE));

    TempFile file(content);
    CFileReader reader;
    REQUIRE(reader.Open(file.path().c_str()));

    std::string line;
    REQUIRE(reader.GetNextLine(line));
    CHECK(line == content);
}

//=============================================================
// CFileWriter buffered writes

TEST_CASE("CFileWriter buffers small writes in memory until Close flushes them to disk", "[fileio]")
{
    TempFile file("");
    {
        CFileWriter writer;
        REQUIRE(writer.Open(file.path().c_str()));
        REQUIRE(writer.WriteBuf("small", 5));

        // Nothing has reached the FILE* yet - the on-disk file is still empty.
        CHECK(readFile(file.path()).empty());
    }
    // Close() (run by the destructor here) flushes the buffered bytes.
    CHECK(readFile(file.path()) == "small");
}

TEST_CASE("CFileWriter auto-flushes once buffered writes cross the internal buffer threshold", "[fileio]")
{
    const std::string big(static_cast<size_t>(RW_BUF_SIZE) + 100, 'x');

    TempFile file("");
    CFileWriter writer;
    REQUIRE(writer.Open(file.path().c_str()));
    REQUIRE(writer.WriteBuf(big.c_str(), (long)big.size()));

    // The threshold was crossed inside WriteBuf, so the data is already on disk
    // even though Close() hasn't been called yet.
    CHECK(readFile(file.path()) == big);

    writer.Close();
    CHECK(readFile(file.path()) == big);
}

//=============================================================
// Missing file / bad path / null filename

TEST_CASE("CFileReader Open returns false for a missing file", "[fileio]")
{
    CFileReader reader;
    CHECK_FALSE(reader.Open("tests/fixtures/does-not-exist.rep"));
}

TEST_CASE("CFileWriter Open returns false when the destination directory does not exist", "[fileio]")
{
    CFileWriter writer;
    CHECK_FALSE(writer.Open("tests/fixtures/does-not-exist-dir/output.txt"));
}

TEST_CASE("CFileWriter Open returns false for a null filename instead of crashing", "[fileio]")
{
    CFileWriter writer;
    CHECK_FALSE(writer.Open(nullptr));
}

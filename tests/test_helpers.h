#ifndef __TEST_HELPERS_H_INCL__
#define __TEST_HELPERS_H_INCL__

#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace
{
class TempFile
{
public:
    explicit TempFile(const std::string & contents)
    {
        char pattern[] = "/tmp/alh-test-XXXXXX";
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

    // Owns a filesystem path and unlinks it on destruction - copying (or
    // moving, since a moved-from path_ would still be unlinked) would let
    // two instances race to unlink the same path.
    TempFile(const TempFile &) = delete;
    TempFile & operator=(const TempFile &) = delete;

    const std::string & path() const
    {
        return path_;
    }

private:
    std::string path_;
};

inline std::string readFile(const std::string & path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("failed to open file: " + path);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
}

#endif

/*
 * This source file is part of the Atlantis Little Helper program.
 */

#include "string_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "compat.h"

#if !defined(_MSC_VER)

char * _strlwr(char * p)
{
    char * ret = p;

    while (*p)
    {
        *p = tolower(*p);
        p++;
    }
    return ret;
}

#endif

static const char * stristr( const char * string, const char * image )
{
    char   first[3];
    int    imagelen;

    if ((NULL==image) || (0==*image))
        return string;

    imagelen = strlen(image);
    first[0] = tolower(image[0]);
    first[1] = toupper(image[0]);
    first[2] = 0;

    while (string)
    {
        string = strpbrk(string, first);
        if (string)
        {
            if (0==strnicmp(string, image, imagelen))
                return string;
            else
                string++;
        }
    }

    return NULL;
}

int SafeCmp(const char * s1, const char * s2)
{
    if (NULL==s1)
        if (NULL==s2)
            return 0;
        else
            return -1;
    else
        if (NULL==s2)
            return 1;
        else
            return stricmp(s1, s2);
}

int SafeCmpNoSpaces(const char * s1, const char * s2)
{
    if (NULL==s1)
        if (NULL==s2)
            return 0;
        else
            return -1;
    else
        if (NULL==s2)
            return 1;
        else
        {
            while (*s1 && *s2)
            {
                while (*s1 && *s1<=' ')
                    s1++;
                while (*s2 && *s2<=' ')
                    s2++;
                if (*s1 < *s2)
                    return -1;
                else if (*s1 > *s2)
                    return 1;
                s1++;
                s2++;
            }
            return 0;
        }
}

const char * SkipSpaces(const char * p)
{
    while (p && *p && (*p<=' '))
        p++;
    return p;
}

std::string & operator<<(std::string & s, const char * psz)
{
    if (psz)
        s += psz;
    return s;
}

std::string & operator<<(std::string & s, const std::string & src)
{
    s += src;
    return s;
}

std::string & operator<<(std::string & s, char ch)
{
    s.push_back(ch);
    return s;
}

std::string & operator<<(std::string & s, long lNum)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", lNum);
    s += buf;
    return s;
}

std::string & operator<<(std::string & s, unsigned long ulNum)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%lu", ulNum);
    s += buf;
    return s;
}

std::string & operator<<(std::string & s, double dNum)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "%.2f", dNum);
    s += buf;
    return s;
}

void AddCh(std::string & s, char ch)
{
    s.push_back(ch);
}

void DelCh(std::string & s, int nPos)
{
    if ((nPos<0) || (nPos>=(int)s.size()))
        return;
    s.erase(nPos, 1);
}

void SetCh(std::string & s, int nPos, char ch)
{
    if ((nPos<0) || (nPos>=(int)s.size()))
        return;
    s[(size_t)nPos] = ch;
}

void AddStr(std::string & s, const char * szS, int iSLen)
{
    if ((NULL==szS) || (0==szS[0]))
        return;

    if (0==iSLen)
        s += szS;
    else
        s.append(szS, iSLen);
}

void SetStr(std::string & s, const char * szS, int iSLen)
{
    s.clear();
    AddStr(s, szS, iSLen);
}

void InsStr(std::string & s, const char * szS, int nPos, int iSLen)
{
    if ((NULL==szS) || (0==szS[0]))
        return;

    if (nPos<0)
        nPos = 0;
    if (nPos>(int)s.size())
        nPos = (int)s.size();

    if (0==iSLen)
        s.insert((size_t)nPos, szS);
    else
        s.insert((size_t)nPos, szS, (size_t)iSLen);
}

void AddLong(std::string & s, long lNum) { s << lNum; }
void AddULong(std::string & s, unsigned long ulNum) { s << ulNum; }

void AddDouble(std::string & s, double dNum, int width, int precision)
{
    char mask[64];
    char buf[512];
    snprintf(mask, sizeof(mask), "%s%d.%df", "%", width, precision);
    snprintf(buf, sizeof(buf), mask, dNum);
    s += buf;
}

void AddBuf(std::string & s, const void * szData, int iDataLen)
{
    if ((NULL==szData) || (0==iDataLen))
        return;

    s.append((const char *)szData, (size_t)iDataLen);
}

void InsBuf(std::string & s, const void * szData, int nPos, int iDataLen)
{
    if ((NULL==szData) || (0==iDataLen))
        return;

    if (nPos<0)
        nPos = 0;
    if (nPos>(int)s.size())
        nPos = (int)s.size();

    s.insert((size_t)nPos, (const char *)szData, (size_t)iDataLen);
}

char * GetToken(std::string & out, const char * Src, char Limit, TrimMode Mode, BOOL StripQuotes)
{
    char * p;

    out.clear();

    if (NULL==Src)
        return NULL;

    if (StripQuotes && '"' == *Src)
    {
        p = (char*)strchr(Src + 1, '"');
        Src++;
    }
    else
        p = (char*)strchr(Src,Limit);

    if (NULL==p)
        out.assign(Src);
    else
    {
        if (p!=Src)
            out.assign(Src, p-Src);
        p++;
    }

    TrimLeft(out, Mode);
    TrimRight(out, Mode);

    return p;
}

char * GetToken(std::string & out, const char * Src, const char * Limit, char & LimitUsed, TrimMode Mode, BOOL StripQuotes)
{
    char * p;

    out.clear();
    LimitUsed = 0;

    if (NULL==Src)
        return NULL;

    if (StripQuotes && '"' == *Src)
    {
        p = (char*)strchr(Src + 1, '"');
        Src++;
    }
    else
        p = (char*)strpbrk(Src,Limit);

    if (NULL==p)
        out.assign(Src);
    else
    {
        if (p!=Src)
            out.assign(Src, p-Src);
        LimitUsed = *p;
        p++;
    }

    TrimLeft(out, Mode);
    TrimRight(out, Mode);

    return p;
}

char * GetInteger(std::string & out, const char * Src, BOOL & Valid)
{
    out.clear();

    while (Src)
    {
        if ( (*Src>='0') && (*Src<='9') )
            out.push_back(*Src);
        else
            if ( ('-'==*Src) && out.empty() )
                out.push_back(*Src);
            else
                break;
        Src++;
    }

    Valid = out.size() > 1 || out.size() == 1 && out[0] != '-';

    return (char*)Src;
}

char * GetDouble(std::string & out, const char * Src, BOOL & Valid)
{
    out.clear();

    while (Src)
    {
        if ( (*Src>='0') && (*Src<='9') || (*Src=='.') )
            out.push_back(*Src);
        else
            if ( ('-'==*Src) && out.empty() )
                out.push_back(*Src);
            else
                break;
        Src++;
    }

    Valid = out.size() > 1 || out.size() == 1 && out[0] != '-' && out[0] != '.';

    return (char*)Src;
}

void TrimLeft(std::string & s, TrimMode Mode)
{
    if (TRIM_NONE==Mode)
        return;

    size_t i = 0;

    if (TRIM_SPACES==Mode)
        while (i<s.size() && (s[i]==' ' || s[i]=='\t'))
            ++i;
    else
        while (i<s.size() && s[i]<=' ')
            ++i;

    if (i>0)
        s.erase(0, i);
}

void TrimRight(std::string & s, TrimMode Mode)
{
    if (TRIM_NONE==Mode)
        return;

    while (!s.empty())
    {
        char ch = s[s.size()-1];
        if (TRIM_SPACES==Mode)
        {
            if (ch!=' ' && ch!='\t')
                break;
        }
        else if (ch>' ')
            break;

        s.erase(s.size()-1, 1);
    }
}

void Format(std::string & out, const char * lpszFormat, va_list argList)
{
    int nMaxLen = 0x0100;

    while (nMaxLen<0x80000)
    {
        std::string tmp;
        tmp.resize((size_t)nMaxLen);

        va_list argListCopy;
#if defined(_MSC_VER)
        argListCopy = argList;
#else
        va_copy(argListCopy, argList);
#endif
        int err = _vsnprintf(&tmp[0], (size_t)nMaxLen, lpszFormat, argListCopy);
        va_end(argListCopy);

        if (err>=0 && err<nMaxLen)
        {
            out.assign(tmp.c_str(), (size_t)err);
            return;
        }

        if (err<0)
            nMaxLen <<= 1;
        else
            nMaxLen = err+1;
    }

    out.clear();
}

void Format(std::string & out, const char * lpszFormat, ...)
{
    va_list argList;

    va_start(argList, lpszFormat);
    Format(out, lpszFormat, argList);
    va_end(argList);
}

int FindSubStr(const std::string & s, const char * szS)
{
    if (!szS)
        return -1;

    const char * pFirst = stristr(s.c_str(), szS);

    if (pFirst)
        return (int)(pFirst - s.c_str());

    return -1;
}

int FindSubStrR(const std::string & s, const char * szS)
{
    if (!szS)
        return -1;

    const char * p1 = NULL;
    const char * base = s.c_str();
    const char * p2 = stristr(base, szS);
    int n = strlen(szS);

    while (p2)
    {
        p1 = p2;
        p2 += n;
        if (p2-base >= (int)s.size())
            break;
        p2 = stristr(p2, szS);
    }

    if (p1)
        return (int)(p1 - base);

    return -1;
}

void DelSubStr(std::string & s, int nPos, int nCount)
{
    if ((nPos < 0) || (nCount <= 0) || ((nPos + 1) > (int)s.size()))
        return;

    if ((nPos + nCount) > (int)s.size())
        nCount = (int)s.size() - nPos;

    s.erase((size_t)nPos, (size_t)nCount);
}

void Normalize(std::string & s)
{
    TrimRight(s, TRIM_ALL);
    TrimLeft(s, TRIM_ALL);

    for (int i=(int)s.size()-1; i>0; i--)
    {
        if (s[(size_t)i] <= ' ')
        {
            if (s[(size_t)i-1] <= ' ')
                DelCh(s, i);
            else
                s[(size_t)i] = ' ';
        }
    }
}

void RemoveLineBreaks(std::string & s)
{
    for (int i=(int)s.size()-1; i>0; i--)
    {
        if ('\r' == s[(size_t)i] || '\n' == s[(size_t)i])
            DelCh(s, i);
    }
}

void Replace(std::string & s, char search, char replace_with)
{
    for (int i=(int)s.size()-1; i>0; i--)
    {
        if (search == s[(size_t)i])
            s[(size_t)i] = replace_with;
    }
}

BOOL IsInteger(const std::string & s)
{
    BOOL Ok = FALSE;

    for (size_t pos = 0; pos < s.size(); ++pos)
    {
        char ch = s[pos];
        if ((ch>='0' && ch<='9') || (0==pos && '-'==ch))
            Ok = TRUE;
        else
            return FALSE;
    }

    return Ok;
}

const char * ToLower(std::string & s)
{
    if (s.empty())
        return s.c_str();

    _strlwr(&s[0]);
    return s.c_str();
}

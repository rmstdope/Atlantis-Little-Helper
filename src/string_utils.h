/*
 * This source file is part of the Atlantis Little Helper program.
 */

#ifndef __AH_STRING_UTILS_H__
#define __AH_STRING_UTILS_H__

#include <stdarg.h>
#include <stddef.h>
#include <string>

#include "bool.h"

enum TrimMode {TRIM_NONE=0, TRIM_SPACES, TRIM_ALL};

int SafeCmp(const char * s1, const char * s2);
int SafeCmpNoSpaces(const char * s1, const char * s2);
const char * SkipSpaces(const char * p);

std::string & operator<<(std::string & s, const char * psz);
std::string & operator<<(std::string & s, const std::string & src);
std::string & operator<<(std::string & s, char ch);
std::string & operator<<(std::string & s, long lNum);
std::string & operator<<(std::string & s, unsigned long ulNum);
std::string & operator<<(std::string & s, double dNum);

void AddCh(std::string & s, char ch);
void DelCh(std::string & s, int nPos);
void SetCh(std::string & s, int nPos, char ch);
void AddStr(std::string & s, const char * szS, int iSLen=0);
void SetStr(std::string & s, const char * szS, int iSLen=0);
void InsStr(std::string & s, const char * szS, int nPos, int iSLen=0);
void AddLong(std::string & s, long lNum);
void AddULong(std::string & s, unsigned long ulNum);
void AddDouble(std::string & s, double dNum, int width, int precision);
void AddBuf(std::string & s, const void * szData, int iDataLen);
void InsBuf(std::string & s, const void * szData, int nPos, int iDataLen);
char * GetToken(std::string & out, const char * Src, char Limit, TrimMode Mode=TRIM_SPACES, bool StripQuotes=true);
char * GetToken(std::string & out, const char * Src, const char * Limit, char & LimitUsed, TrimMode Mode=TRIM_SPACES, bool StripQuotes=true);
char * GetInteger(std::string & out, const char * Src, bool & Valid);
char * GetDouble(std::string & out, const char * Src, bool & Valid);
void TrimLeft(std::string & s, TrimMode Mode=TRIM_SPACES);
void TrimRight(std::string & s, TrimMode Mode=TRIM_SPACES);
void Format(std::string & out, const char * lpszFormat, ...);
void Format(std::string & out, const char * lpszFormat, va_list argList);
int FindSubStr(const std::string & s, const char * szS);
int FindSubStrR(const std::string & s, const char * szS);
void DelSubStr(std::string & s, int nPos, int nCount);
void Normalize(std::string & s);
void RemoveLineBreaks(std::string & s);
void Replace(std::string & s, char search, char replace_with);
bool IsInteger(const std::string & s);
const char * ToLower(std::string & s);

#endif // __AH_STRING_UTILS_H__

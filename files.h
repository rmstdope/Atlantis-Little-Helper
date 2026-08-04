/*
 * This source file is part of the Atlantis Little Helper program.
 * Copyright (C) 2001 Maxim Shariy.
 *
 * Atlantis Little Helper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atlantis Little Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atlantis Little Helper; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FILES_H_INCL__
#define __FILES_H_INCL__

#include <stdio.h>
#include "string_utils.h"



#define RW_BUF_SIZE  8092

//---------------------------------------------------------------

class CFileReader
{
public:
    CFileReader();
    ~CFileReader();

    BOOL Open(const char * szFName);
    void Close();

    BOOL GetNextChar(char & ch);
    BOOL GetNextLine(std::string & s);

    void QueueChar(char ch);
    void QueueString(const char * p, int n=0);

protected:
    BOOL ReadMore();

    FILE   * m_f;
    size_t   m_nPos;
    size_t   m_nSize;
    char     m_Buf[RW_BUF_SIZE];
    std::string     m_Queue;
    std::string     m_FileName;
};

//---------------------------------------------------------------

class CFileWriter
{
public:
    CFileWriter();
    ~CFileWriter();

    BOOL Open(const char * szFName, const char * szMode = "wb");
    void Close();

    BOOL WriteBuf(const char * szData, long nDataSize);

protected:
    BOOL Flush();

    FILE   * m_f;
    std::string     m_s;
};

//---------------------------------------------------------------

#endif

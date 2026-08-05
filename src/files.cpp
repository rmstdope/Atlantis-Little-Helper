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

#include <stdio.h>
#include "files.h"
#include "string_utils.h"

#include "stdhdr.h"
//#include "wx/msgdlg.h"
//#include "wx/dialog.h"




CFileReader::CFileReader()
{
    m_Queue.reserve(256);
    m_f     = nullptr;
    m_nPos  = 0;
    m_nSize = 0;
}

//---------------------------------------------------------------------

CFileReader::~CFileReader()
{
    Close();
}

//---------------------------------------------------------------------


bool  CFileReader::Open(const char * szFName)
{
    Close();

    if (szFName && *szFName)
        m_f = fopen(szFName, "rb");

    m_FileName = szFName;

    return (nullptr != m_f);
}

//---------------------------------------------------------------------

void CFileReader::Close()
{
    if (m_f)
        fclose(m_f);
    m_f = nullptr;
    m_nPos  = 0;
    m_nSize = 0;
}

//---------------------------------------------------------------------


bool CFileReader::GetNextChar(char & ch)
{
    // return first queued char
    if (m_Queue.size()>0)
    {
        ch = m_Queue.c_str()[0];
        DelCh(m_Queue, 0);
        return true;
    }

    if (m_nPos >= m_nSize)
    {
        if ((!ReadMore()) || (m_nPos >= m_nSize))
            return false;
    }


    ch = m_Buf[m_nPos];
    m_nPos++;
    return true;

}


//---------------------------------------------------------------------

void CFileReader::QueueChar(char ch)
{
    AddCh(m_Queue, ch);
}

//---------------------------------------------------------------------

void CFileReader::QueueString(const char * p, int n)
{
    AddStr(m_Queue, p, n);
}

//---------------------------------------------------------------------

bool CFileReader::GetNextLine(std::string & s)
{
    char ch;

    s.clear();

    while (GetNextChar(ch))
    {
        AddCh(s, ch);
        if ('\n'==ch)
            break;
    }

    return (!s.empty());
}

//---------------------------------------------------------------------
/*
bool CFileReader::ReadMore()
{
    m_nPos  = 0;
    m_nSize = 0;

    if (!m_f)
        return false;

    if (feof(m_f))
        return false;

    m_nSize = fread(m_Buf, 1, RW_BUF_SIZE, m_f);
    if (0==m_nSize)
    {
        if (ferror(m_f))
        {
            std::string S;
            Format(S, "Error reading file %s", m_FileName.c_str());
            wxMessageBox(S.c_str());
        }
        else
        {
            std::string S;
            Format(S, "No error reading file %s, but still read 0 bytes", m_FileName.c_str());
            wxMessageBox(S.c_str());
        }
    }
    else
    {
        FILE * f;
        std::string   name;

        Format(name, "%s_read_", m_FileName.c_str());
        if (f=fopen(name.c_str(), "ab"))
        {
            size_t n;
            n = fwrite(m_Buf, 1, m_nSize, f);
            fclose(f);
        }
        else
        {
            std::string S;
            Format(S, "can not open log file %s for writing", name.c_str());
            wxMessageBox(S.c_str());
        }

    }

    return (m_nSize>0);
}
*/
//---------------------------------------------------------------------


bool CFileReader::ReadMore()
{
    int i;

    m_nPos  = 0;
    m_nSize = 0;

    if (!m_f)
        return false;

    if (feof(m_f))
        return false;

    for (i=0; i<3; i++)
    {
        m_nSize = fread(m_Buf, 1, RW_BUF_SIZE, m_f);
        if (0==m_nSize)
        {
            if (ferror(m_f))
            {
                //wxString S;
                // The stupid wxString does not compile on some configurations
                //S = wxString::Format(wxString("Error reading file %s"), m_FileName.c_str());
                //wxMessageBox(S);
                break;
            }
            else
                wxSleep(1); // maybe it will get better?
        }
        break;
    }

    return (m_nSize>0);
}


//=====================================================================


CFileWriter::CFileWriter()
{
    m_s.reserve(1024);
    m_f     = nullptr;
}

//---------------------------------------------------------------------

CFileWriter::~CFileWriter()
{
    Close();
}

//---------------------------------------------------------------------


bool CFileWriter::Open(const char * szFName, const char * szMode)
{
    Close();

    if (szFName && *szFName)
        m_f = fopen(szFName, szMode);

    return (nullptr != m_f);
}

//---------------------------------------------------------------------

void CFileWriter::Close()
{
    if (m_f)
    {
        Flush();
        fclose(m_f);
        m_f     = nullptr;
    }
}

//---------------------------------------------------------------------


bool CFileWriter::WriteBuf(const char * szData, long nDataSize)
{
    if (!m_f)
        return false;

    AddBuf(m_s, szData, nDataSize);
    if (m_s.size() > RW_BUF_SIZE)
        return Flush();

    return true;
}

//---------------------------------------------------------------------

bool CFileWriter::Flush()
{
    size_t n;

    n = fwrite(m_s.c_str(), 1, m_s.size(), m_f);

    if (n < (size_t)m_s.size())
        return false;

    m_s.clear();
    return true;

}

//---------------------------------------------------------------------

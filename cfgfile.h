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

#ifndef __CONFIG_FILE_H_INCL__
#define __CONFIG_FILE_H_INCL__

#include <vector>
#include <string>
#include "bool.h"


typedef struct _CONFIG_PARAM
{
public:
    const char * szSection;
    const char * szName;
    const char * szValue;
    const char * szComment;
    int          SectionLen; // excluding zero
    int          NameLen;    // excluding zero
    int          ValueLen;   // excluding zero
    int          CommentLen; // excluding zero
} CONFIG_PARAM;


class CConfigFile
{
public:
    CConfigFile();
    ~CConfigFile();

    BOOL Load(const char * szFName);
    BOOL Save(const char * szFName);
    const char * GetByName(const char * szSection, const char * szName);
    void         SetByName(const char * szSection, const char * szName, const char * szNewValue, const char * szComment=NULL);

    int          GetFirstInSection(const char * szSection, const char *& szName, const char *& szValue);
    int          GetNextInSection (int idx, const char * szSection, const char *& szName, const char *& szValue);

    void         RemoveSection(const char * szSection);
    BOOL         GetNextSection(const char * szPrevSection, const char *& szNextSection);

private:
    // Internal sorted-vector API matching the old CSortedCollection interface
    int   Count() const { return (int)m_items.size(); }
    void* At(int i) const { return (i >= 0 && i < (int)m_items.size()) ? m_items[i] : nullptr; }
    BOOL  Insert(void* pItem);
    BOOL  Search(void* pItem, int& nIndex) const;
    void  AtFree(int i) { FreeItem(m_items[i]); m_items.erase(m_items.begin()+i); }
    void  FreeAll() { for (auto p : m_items) FreeItem(p); m_items.clear(); }

    void FreeItem(void * pItem);
    int  Compare (void * pItem1, void * pItem2) const;

    std::vector<CONFIG_PARAM*> m_items;
};



#endif

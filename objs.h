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

#ifndef __OBJS_IS_OBJECTS_FOUNDATION_H_INCL__
#define __OBJS_IS_OBJECTS_FOUNDATION_H_INCL__

#include "cstr.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

enum EValueType
{
    eLong,
    eCharPtr,
    eObjPtr
};

enum EPropertyType
{
    eNormal,
    eOriginal,
    eBoth
};

#ifndef min
#define min(x,y) ( ((x) < (y)) ? (x) : (y) )
#endif

#ifndef max
#define max(x,y) ( ((x) > (y)) ? (x) : (y) )
#endif


#define MAX_PROP_COLL_KEYS  16

// property errors

#define PE_OK              ( 0)
#define PE_NOT_IMPLEMENTED (-1)
#define PE_INV_NAME        (-2)
#define PE_INV_VALUE_TYPE  (-3)      
#define PE_INV_PROP_TYPE   (-4)      

//=============================================================

struct TEvent
{
    unsigned long   id;
    void          * data;
};

//-------------------------------------------------------------------

class TObject
{
public:
    TObject() {};
    virtual ~TObject() {};

    virtual BOOL HandleEvent(TEvent * event) {return FALSE;};
};

//-------------------------------------------------------------------

class TProperty
{
public:
    TProperty();
    TProperty(const char * name, EValueType type, const void * value);
    ~TProperty();
    
    int     SetValue(EValueType     type,
                     const void  *  value, 
                     EPropertyType  proptype = eNormal
                    );

    const char * m_name;
    EValueType   m_type;
    void       * m_value;
    void       * m_valueorg;
};

//-------------------------------------------------------------------

// Map from property name to TProperty* (owned)
class TPropertyColl
{
public:
    TPropertyColl()  {};
    ~TPropertyColl() { freeAll(); };

    TProperty * find(const char * name) const;
    void        insert(TProperty * p);
    void        erase(const char * name);
    void        freeAll();
    size_t      count() const { return m_map.size(); }
    TProperty * at(int no) const;   // iterate by position (O(n), used rarely)

private:
    std::map<std::string, TProperty*> m_map;
};

//-------------------------------------------------------------------

class TPropertyHolder : public TObject
{
public: 
    TPropertyHolder();
    virtual ~TPropertyHolder();

    virtual BOOL GetProperty(const char    *  name,
                             EValueType     & valuetype,
                             const void    *& value,
                             EPropertyType    proptype = eNormal
                            );
    virtual int  SetProperty(const char  *  name,
                             EValueType     type,
                             const void  *  value, 
                             EPropertyType  proptype = eNormal
                            );
    virtual void DelProperty(const char  *  name);
    virtual void ResetNormalProperties();
    virtual const char  * ResolveAlias(const char * alias) {return alias;};
    virtual std::multimap<std::string,std::string> * GetPropertyGroups() {return NULL;};
    virtual const char  * GetPropertyName(int no);

protected:
    BOOL GetJustProperty    (const char    *  name,
                             EValueType     & valuetype,
                             const void    *& value,
                             EPropertyType    proptype = eNormal
                            );
    TPropertyColl  m_Properties;

};

//-------------------------------------------------------------------

class TPropertyHolderColl
{
public:
    TPropertyHolderColl()  {};
    TPropertyHolderColl(int /*nDelta*/) {};
    virtual ~TPropertyHolderColl() { DeleteAll(); }

    void SetSortMode(const char ** keys, int keycount);

    // Collection-like API used by callers
    size_t            Count()   const { return m_items.size(); }
    TPropertyHolder * At(int i) const { return m_items[i]; }

    void AtInsert(int i, TPropertyHolder * p) { m_items.insert(m_items.begin()+i, p); }
    void AtDelete(int i)                      { m_items.erase(m_items.begin()+i); }
    void DeleteAll()                          { m_items.clear(); }

protected:
    void ClearKeys();
    int  Compare(TPropertyHolder * a, TPropertyHolder * b) const;

    std::vector<TPropertyHolder*> m_items;
    int    m_KeyCount = 0;
    char * m_Key[MAX_PROP_COLL_KEYS] = {};
};

//-------------------------------------------------------------------

#endif


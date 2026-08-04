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


#include "stdlib.h"
#include "string.h"
#include <stdarg.h>
#include <cstdint>
#include "cstr.h"
#include "objs.h"
#include "compat.h"


//===================================================================

TProperty::TProperty()
{
    m_name      = NULL;
    m_type      = eLong;
    m_value     = (void*)0;
    m_valueorg  = (void*)0;
}

//-------------------------------------------------------------------

TProperty::TProperty(const char * name, EValueType type, const void * value)
{
    m_name      = name?strdup(name):NULL;
    m_type      = type;
    if (eCharPtr==type)
    {
        m_value     = value?strdup((const char *)value):NULL;
        m_valueorg  = value?strdup((const char *)value):NULL;
    }
    else
    {
        m_value     = (void*)value;
        m_valueorg  = (void*)value;
    }
}

//-------------------------------------------------------------------

TProperty::~TProperty()
{
    if (m_name)
        free((void*)m_name);
    if (eCharPtr==m_type) 
    {
        if (m_value)
            free(m_value);
        if (m_valueorg)
            free(m_valueorg);
    }
}

//-------------------------------------------------------------------

int TProperty::SetValue(EValueType     type,
                        const void  *  value, 
                        EPropertyType  proptype
                        )
{
    void     ** pvalue;

    if (type!=m_type)
        return PE_INV_VALUE_TYPE;

    switch (proptype)
    {
    case eNormal:
        pvalue = &m_value;
        break;
    case eOriginal:
        pvalue = &m_valueorg;
        break;
    default:
        return PE_INV_PROP_TYPE;
    }

    if (eCharPtr==type)
    {
        if (*pvalue)
            free(*pvalue);
        *pvalue     = value?strdup((const char *)value):NULL;
    }
    else
        *pvalue     = (void*)value;

    return PE_OK;
}


//===================================================================

//===================================================================
// TPropertyColl

TProperty * TPropertyColl::find(const char * name) const
{
    if (!name) return nullptr;
    auto it = m_map.find(name);
    return (it != m_map.end()) ? it->second : nullptr;
}

void TPropertyColl::insert(TProperty * p)
{
    if (!p || !p->m_name) return;
    m_map[p->m_name] = p;
}

void TPropertyColl::erase(const char * name)
{
    if (!name) return;
    auto it = m_map.find(name);
    if (it != m_map.end()) {
        delete it->second;
        m_map.erase(it);
    }
}

void TPropertyColl::freeAll()
{
    for (auto & kv : m_map)
        delete kv.second;
    m_map.clear();
}

TProperty * TPropertyColl::at(int no) const
{
    if (no < 0 || (size_t)no >= m_map.size()) return nullptr;
    auto it = m_map.begin();
    std::advance(it, no);
    return it->second;
}

//===================================================================

TPropertyHolder::TPropertyHolder()
{

}

//-------------------------------------------------------------------

TPropertyHolder::~TPropertyHolder()
{
    m_Properties.freeAll();
}

//-------------------------------------------------------------------

BOOL TPropertyHolder::GetJustProperty(const char    *  name,
                                      EValueType     & valuetype,
                                      const void    *& value,
                                      EPropertyType    proptype 
                                     )       
{
    TProperty * pProp = m_Properties.find(name);
    if (!pProp) return FALSE;

    valuetype = pProp->m_type;
    switch (proptype)
    {
    case eNormal:   value = pProp->m_value;    break;
    case eOriginal: value = pProp->m_valueorg; break;
    default:        value = NULL; return FALSE;
    }
    return TRUE;
}

//-------------------------------------------------------------------

const char * TPropertyHolder::GetPropertyName(int no)
{
    TProperty * pProp = m_Properties.at(no);
    return pProp ? pProp->m_name : NULL;
}

//-------------------------------------------------------------------

BOOL TPropertyHolder::GetProperty(const char    *  name,
                                  EValueType     & valuetype,
                                  const void    *& value,
                                  EPropertyType    proptype 
                                  )       
{
    BOOL Ok = FALSE;

    name = ResolveAlias(name);

    if (GetJustProperty(name, valuetype, value, proptype))
    {
        Ok = TRUE;
    }
    else
    {
        auto * pSSC = GetPropertyGroups();
        if (pSSC)
        {
            auto range = pSSC->equal_range(name);
            long sum = 0;
            for (auto it = range.first; it != range.second; ++it)
            {
                const void * x;
                EValueType   vt;
                if (GetJustProperty(it->second.c_str(), vt, x, proptype) && (eLong == vt))
                {
                    // TProperty encodes eLong values directly in the pointer (pre-existing convention).
                    // Use intptr_t as the safe intermediary for pointer<->integer round-trips.
                    sum      += static_cast<long>(reinterpret_cast<intptr_t>(x));
                    valuetype = eLong;
                    Ok        = TRUE;
                }
            }
            if (Ok)
                value = reinterpret_cast<const void*>(static_cast<intptr_t>(sum));
        }
    }

    return Ok;
}

//-------------------------------------------------------------------

int  TPropertyHolder::SetProperty(const char  *  name,
                                  EValueType     type,
                                  const void  *  value, 
                                  EPropertyType  proptype 
                                 )
{
    int err = PE_OK;

    name = ResolveAlias(name);

    TProperty * pProp = m_Properties.find(name);
    if (pProp)
    {
        if (eBoth == proptype)
        {
            err = pProp->SetValue(type, value, eNormal);
            if (PE_OK == err)
                err = pProp->SetValue(type, value, eOriginal);
        }
        else
            err = pProp->SetValue(type, value, proptype);
    }
    else
    {
        pProp = new TProperty(name, type, value);
        m_Properties.insert(pProp);
    }

    return err;
}

//-------------------------------------------------------------------

void TPropertyHolder::DelProperty(const char  *  name)
{
    name = ResolveAlias(name);
    m_Properties.erase(name);
}

//-------------------------------------------------------------------

void TPropertyHolder::ResetNormalProperties()
{
    // Iterate all properties and reset normal values from originals
    for (int i = 0; i < (int)m_Properties.count(); i++)
    {
        TProperty * pProp = m_Properties.at(i);
        if (pProp)
            pProp->SetValue(pProp->m_type, pProp->m_valueorg, eNormal);
    }
}

//===================================================================

//===================================================================

void TPropertyHolderColl::ClearKeys()
{
    for (int i = 0; i < min(m_KeyCount, MAX_PROP_COLL_KEYS); i++)
        if (m_Key[i]) { free(m_Key[i]); m_Key[i] = NULL; }
    m_KeyCount = 0;
}

//-------------------------------------------------------------------

void TPropertyHolderColl::SetSortMode(const char ** keys, int keycount)
{
    ClearKeys();
    for (int i = 0; i < min(keycount, MAX_PROP_COLL_KEYS); i++)
        if (keys[i] && *keys[i])
            m_Key[m_KeyCount++] = strdup(keys[i]);

    std::stable_sort(m_items.begin(), m_items.end(),
        [this](TPropertyHolder * a, TPropertyHolder * b) { return Compare(a, b) < 0; });
}

//-------------------------------------------------------------------

int TPropertyHolderColl::Compare(TPropertyHolder * pItem1, TPropertyHolder * pItem2) const
{
    int               n, x;
    const void      * p1,  * p2;
    BOOL              Ok1,   Ok2;
    EValueType        t1,    t2; 

    if (!pItem1) return !pItem2 ? 0 : -1;
    if (!pItem2) return 1;

    for (n = 0; n < min(m_KeyCount, MAX_PROP_COLL_KEYS); n++)
    {
        Ok1 = ((TPropertyHolder*)pItem1)->GetProperty(m_Key[n], t1, p1);
        Ok2 = ((TPropertyHolder*)pItem2)->GetProperty(m_Key[n], t2, p2);

        if (!Ok1) { if (!Ok2) continue; else return -1; }
        else       { if (!Ok2) return 1; }

        if (t1 != t2) continue;

        switch (t1)
        {
        case eLong:
            if ((long)p1 > (long)p2) return  1;
            if ((long)p1 < (long)p2) return -1;
            break;
        case eCharPtr:
            if (!p1) { if (!p2) continue; else return -1; }
            else       { if (!p2) return 1; }
            x = stricmp((const char*)p1, (const char*)p2);
            if (x != 0) return x;
            break;
        default:
            return 0;
        }
    }
    return 0;
}

//===================================================================


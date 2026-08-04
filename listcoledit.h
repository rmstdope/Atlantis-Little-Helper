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

#ifndef __AH_LIST_COL_EDIT_INCL__
#define __AH_LIST_COL_EDIT_INCL__

#include <vector>
#include <algorithm>

//--------------------------------------------------------------------------

class TUnitColData
{
public:
    CStr   PropName;
    CStr   Caption;
    int    width;
    int    flags;
};

//--------------------------------------------------------------------------

class CUnitColDataColl
{
public:
    CUnitColDataColl() {};
    ~CUnitColDataColl() { FreeAll(); };
    void FreeAll() { for (auto* p : m_items) delete p; m_items.clear(); }
    // sorted insert by PropName; returns false if already present
    bool Insert(TUnitColData* p) {
        auto it = std::lower_bound(m_items.begin(), m_items.end(), p,
            [](TUnitColData* a, TUnitColData* b) {
                return stricmp(a->PropName.GetData(), b->PropName.GetData()) < 0;
            });
        if (it != m_items.end() && stricmp((*it)->PropName.GetData(), p->PropName.GetData()) == 0)
            return false;
        m_items.insert(it, p);
        return true;
    }
    bool Search(TUnitColData* key, int& idx) {
        auto it = std::lower_bound(m_items.begin(), m_items.end(), key,
            [](TUnitColData* a, TUnitColData* b) {
                return stricmp(a->PropName.GetData(), b->PropName.GetData()) < 0;
            });
        if (it != m_items.end() && stricmp((*it)->PropName.GetData(), key->PropName.GetData()) == 0) {
            idx = (int)(it - m_items.begin());
            return true;
        }
        return false;
    }
    TUnitColData* At(int i) { return m_items[i]; }
private:
    std::vector<TUnitColData*> m_items;
};

//--------------------------------------------------------------------------

class CListHeaderEditDlg : public CResizableDlg
{
public:
    CListHeaderEditDlg(wxWindow *parent, const char * szWorkSection);
    ~CListHeaderEditDlg();

    void  OnButton       (wxCommandEvent& event);
    void  OnSetNameChange(wxCommandEvent& event);
    void  OnSetNameSelect(wxCommandEvent& event);
    void  OnListSelect   (wxListEvent& event);
    void  OnListActivate (wxListEvent& event);


private:
    void  LoadListSrc();
    void  LoadListDest(const char * szNewName);
    void  SaveListDest();
    void  LoadSetCombo();
    void  ProcessCaption(int oldidx, int newidx);
    
    void  AddItem();
    void  DeleteItem();

    CStr             m_WorkKey;
    CStr             m_SetName;
    CUnitColDataColl m_Fields;
    BOOL             m_SetIsValid;
    int              m_SourceIdx;
    int              m_DestIdx;
    BOOL             m_IsSaving;
    time_t           m_lastselect;

    wxButton   * m_btnOk    ;
    wxButton   * m_btnCancel;
    wxListCtrl * m_lstSource;
    wxListCtrl * m_lstDest;
    wxComboBox * m_cbSetName;
    wxTextCtrl * m_txtCaption;  
    
    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------

#endif


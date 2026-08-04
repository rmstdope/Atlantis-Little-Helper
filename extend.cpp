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

#include "stdhdr.h"

#include "string_utils.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "objs.h"

#include "ahapp.h"

#include "extend.h"


#define CHECK_NULL_PTR(ptr, err, msg) \
if (!ptr)                             \
{                                     \
    ShowError(msg);                   \
    result = err;                     \
    goto quit;                        \
}

#define SZ_UNIT_FILTER_MODULE              "unit_filter_module"
#define SZ_UNIT_FILTER_FUNC                "unit_filter_function"
#define SZ_ALH_PROGRAM_NAME                "alh_extension"
#define SZ_ALH_UNIT_FILTER_MODULE          "alh_unit_filter"
#define SZ_ALH_UNIT_FILTER_FN_GET_PROPERTY "get_property"

//-------------------------------------------------------------------------

CPythonEmbedder::CPythonEmbedder()
{
    m_pAtlantis = nullptr;
    ShowError("Wrong CPythonEmbedder constructor called!");
}

//-------------------------------------------------------------------------

CPythonEmbedder::CPythonEmbedder(CAtlaParser * pAtlantis)
{
    m_pAtlantis       = pAtlantis;
    m_bInitUnitFilter = false;
    m_bInitUnitFilter = false;
    m_bInitGeneric    = false;

    m_pCode   = nullptr;
    m_pModule = nullptr;
    m_pDict   = nullptr;
    m_pFunc   = nullptr;
}

//-------------------------------------------------------------------------

CPythonEmbedder::~CPythonEmbedder()
{
    if (m_bInitUnitFilter)
        DoneUnitFilter();
    if (m_bInitGeneric)
        DoneGeneric();
}

//-------------------------------------------------------------------------

void   CPythonEmbedder::ShowError(const char * msg, int msglen)
{
    if (msglen<=0)
        msglen = strlen(msg);

    gpApp->ShowError (msg, msglen, true);
}


//=========================================================================
//
//  Now all the python-specific functions
//

#ifdef HAVE_PYTHON
//ALH_PYTHON_EXTEND

#include "Python.h"

//-------------------------------------------------------------------------


eEErr  CPythonEmbedder::InitGeneric()
{
    eEErr rc = E_OK;

    if (!m_pAtlantis)
        return E_NULL_POINTERS;

    if (m_bInitGeneric)
        return E_ALREADY_INIT;

    Py_SetProgramName((char*)SZ_ALH_PROGRAM_NAME);
    Py_Initialize();
    m_bInitGeneric = true;

    return rc;
}

//-------------------------------------------------------------------------

void   CPythonEmbedder::DoneGeneric()
{
    Py_Finalize();
    m_bInitGeneric = false;
}

//-------------------------------------------------------------------------

void   CPythonEmbedder::CheckForPythonError()
{
    if (PyErr_Occurred() )
        PyErr_Print();
}

//-------------------------------------------------------------------------

static CUnit * gpUnit = nullptr;  // to be used by unitfltr_getproperty()

extern "C" PyObject * unitfltr_getproperty(PyObject *self, PyObject* args)
{
    char          * propname;
    EValueType      type;
    const void    * value;

    if (!PyArg_ParseTuple(args, "s", &propname) || !gpUnit)
        Py_RETURN_NONE;

    if (!gpUnit->GetProperty(propname, type, value, eNormal) )
    {
        // make default empty value
        {
            auto it__ = gpApp->m_pAtlantis->m_UnitPropertyTypes.find(propname);
            if (it__ != gpApp->m_pAtlantis->m_UnitPropertyTypes.end())
            {
                type = (EValueType)it__->second;
                if (eLong == type) value = 0;
                else               value = "";
            }
            else
                Py_RETURN_NONE;
        }
    }

    if (eLong==type)
        return Py_BuildValue("i", (long)value);
    else
    {
        // python is case sensitive, so lowercase all string values
        std::string S;
        SetStr(S, (const char *)value);
        ToLower(S);
        return Py_BuildValue("s", S.c_str());
    }
}


PyMethodDef unitfltr_methods[] =
{
    {SZ_ALH_UNIT_FILTER_FN_GET_PROPERTY,  unitfltr_getproperty, METH_VARARGS,  "Get unit property."},
    {nullptr, nullptr}   // sentinel
};


void initunitfltr(void)
{
    PyImport_AddModule(SZ_ALH_UNIT_FILTER_MODULE);
    Py_InitModule     (SZ_ALH_UNIT_FILTER_MODULE, unitfltr_methods);
}




eEErr  CPythonEmbedder::InitUnitFilter(const char * userfilter, std::string & sPythonFilter)
{
    eEErr        result = E_OK;
    std::string         sToken;
    const char * p = userfilter;
    char         ch;
    int          idx;
    std::string         sCommand;

    sPythonFilter.clear();
    result = InitGeneric();
    if (E_OK != result)
        return result;

    if (m_bInitUnitFilter)
        return E_ALREADY_INIT;
    m_bInitUnitFilter = true;

    GetCommonCode(sCommand);

    sCommand << "\n"
             << "import " << SZ_ALH_UNIT_FILTER_MODULE << "\n"
             << "def " << SZ_UNIT_FILTER_FUNC << "():\n"
             << "    res = " ;
    while (p && *p)
    {
        p = GetToken(sToken, p, "+-*/<>=!()., \t\r\n", ch, TRIM_ALL, false);

        // python is case sensitive, so lowercase all quoted strings
        if (!sToken.empty() && '\"' == sToken.c_str()[0] && '\"' == sToken.c_str()[sToken.size()-1])
            ToLower(sToken);

        if (gpApp->m_pAtlantis->m_UnitPropertyNames.Search((void*)sToken.c_str(), idx))
            sCommand << SZ_ALH_UNIT_FILTER_MODULE << "." << SZ_ALH_UNIT_FILTER_FN_GET_PROPERTY << "(\"" << sToken << "\")";
        else
            sCommand << sToken;

        if ('\n' == ch)
            sCommand << ' ';
        else if (ch && '\r' != ch)
            sCommand << ch;
    }
    sCommand << "\n"
             << "    return res\n";
    sPythonFilter = sCommand;


    initunitfltr();

    m_pCode   = Py_CompileString((char*)sCommand.c_str(), SZ_UNIT_FILTER_MODULE,  Py_file_input);
                CHECK_NULL_PTR(m_pCode, E_PYTHON, "Py_CompileString()")
    m_pModule = PyImport_ExecCodeModule((char*)SZ_UNIT_FILTER_MODULE, m_pCode);
                CHECK_NULL_PTR(m_pModule, E_PYTHON, "PyImport_ExecCodeModule()")
    m_pDict   = PyModule_GetDict(m_pModule);
                CHECK_NULL_PTR(m_pDict, E_PYTHON, "PyModule_GetDict()")
    m_pFunc   = PyDict_GetItemString(m_pDict, SZ_UNIT_FILTER_FUNC);
                CHECK_NULL_PTR(m_pFunc, E_PYTHON, "PyDict_GetItemString()")


quit:

    if (E_OK != result)
        CheckForPythonError();

    return result;
}

//-------------------------------------------------------------------------

eEErr  CPythonEmbedder::RunUnitFilter(CUnit * pUnit, bool & success)
{
    eEErr result = E_PYTHON;

    success = false;

    CHECK_NULL_PTR(m_pFunc, E_NULL_POINTERS, "m_pFunc not initialised")

    if (PyCallable_Check(m_pFunc))
    {
        PyObject * pValue;

        gpUnit = pUnit; // will be used by the extension function
        pValue = PyObject_CallObject(m_pFunc, nullptr);
        if (pValue)
        {
            success = PyInt_AsLong(pValue);
            Py_DECREF(pValue);
            result = E_OK;
        }
        gpUnit = nullptr;
    }

quit:

    if (E_OK != result)
        CheckForPythonError();

    return result;
}

//-------------------------------------------------------------------------

void   CPythonEmbedder::DoneUnitFilter()
{
    if (m_pCode)    Py_DECREF(m_pCode);
    if (m_pModule)  Py_DECREF(m_pModule);
    /// m_pDict is a borrowed reference
    /// m_pFunc: Borrowed reference
    m_pCode   = nullptr;
    m_pModule = nullptr;
    m_pDict   = nullptr;
    m_pFunc   = nullptr;
    m_bInitUnitFilter = false;
}

//-------------------------------------------------------------------------

void   CPythonEmbedder::GetCommonCode(std::string & code)
{
    CFileReader  F;
    CFileWriter  W;
    std::string         S;
    int          x;

    code.clear();
    if (F.Open(SZ_COMMON_PY_FILE))
    {
        while (F.GetNextLine(S))
            code << S;
        F.Close();
    }
    else
    {
        if (W.Open(SZ_COMMON_PY_FILE))
        {
            code << "import string";
            W.WriteBuf(code.c_str(), code.size());
            W.Close();
        }
    }

    // 0D 0A sequence kills the dumb python parser on windows

    x = FindSubStr(code, "\r");
    while (x>=0)
    {
        DelCh(code, x);
        x = FindSubStr(code, "\r");
    }
}

//-------------------------------------------------------------------------

#endif

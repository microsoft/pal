/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Contains implementation of the SCXodm class for AIX.
    
    \date        2011-08-15 16:05:00

    
*/
/*----------------------------------------------------------------------------*/

// Only for AIX!

#if defined(aix)

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/scxodm.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <libperfstat.h>
#include <vector>
#include <scxcorelib/scxexception.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    
        Creates a SCXodmDependencies object.
    */
    SCXodmDependencies::SCXodmDependencies()
        : m_fInitialized(false)
    {
        m_lock = SCXCoreLib::ThreadLockHandleGet(L"SCXSystemLib::SCXodmDependencies");
    }

    /** Virtual destructor */
    SCXodmDependencies::~SCXodmDependencies()
    {
        if ( m_fInitialized )
        {
            Terminate();
        }
    }

    /** Initialize the ODM database accessor functions.
        \returns Result of the odm_initialize() call
    */
    int SCXodmDependencies::Initialize()
    {
        SCXASSERT( ! m_fInitialized );

        m_lock.Lock();
        (void)setenv("ODMDIR", "/etc/objrepos", 1);
        int status = odm_initialize();
        if ( 0 != status )
        {
            throw SCXodmException(L"odm_initialize failed", odmerrno, SCXSRCLOCATION);
        }
        m_fInitialized = true;
        return status;
    }

    /** Terminate the ODM database accessor functions.
        \returns Result of the odm_terminate() call
    */
    int SCXodmDependencies::Terminate()
    {
        SCXASSERT( m_fInitialized );

        int status = odm_terminate();
        if ( 0 != status )
        {
            SCXodmException e(L"odm_terminate failed", odmerrno, SCXSRCLOCATION);
            SCX_LOGERROR(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(
                    L"scx.core.common.pal.system.scxodm"), e.What());
            SCXASSERTFAIL(e.What().c_str());
        }
        m_fInitialized = false;
        m_lock.Unlock();
        return status;
    }

    /** Gets (first) information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param criteria     Search criteria (string containing qualifying search criteria)
        \param returnData   Pointer to data structure (from header) returned for this class
        \returns -1 on error (odmerrno is set), NULL if no match is found
    */
    void *SCXodmDependencies::GetFirst(CLASS_SYMBOL cs, char *criteria, void *returnData) {
        SCXASSERT( m_fInitialized );
        return odm_get_first(cs, criteria, returnData);
    }

    /** Gets (next) information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param returnData   Pointer to data structure (from header) returned for this class
        \returns -1 on error (odmerrno is set), NULL if no match is found
    */
    void *SCXodmDependencies::GetNext(CLASS_SYMBOL cs, void *returnData) {
        SCXASSERT( m_fInitialized );
        return odm_get_next(cs, returnData);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    
        Creates a SCXodm object.
    */
    SCXodm::SCXodm()
        : m_deps(0),
          m_fGetFirst(true)
    {
        perfInterface_init();
        m_deps = new SCXodmDependencies();
        m_deps->Initialize();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    
        Note that distructor for m_deps will get called, causing cleanup of it's
        structures (and calling odm_terminate() as needed).
    */
    SCXodm::~SCXodm()
    {
    }
    
    /*----------------------------------------------------------------------------*/
    bool SCXodm::perfInterfaced = false;

    /*----------------------------------------------------------------------------*/
    /**
         Dump object as string (for logging).
    
         Parameters:  None
         Retval:      String representation of object.
    */
    std::wstring SCXodm::DumpString() const
    {
        return L"SCXodm: <No data>";
    }

    /** Gets information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param criteria     Search criteria (string containing qualifying search criteria)
        \param returnData   Pointer to data structure (from header) returned for this class
        \param mode         Mode of operation

        \throws  SCXodmException if odm internal error occurred
        \throws  SCXodmException if mode parameter is invalid
        \returns NULL if no match is found
        \NOTE During the existance SCXodm object can operate in two modes. In one mode of operation the mode parameter
              can be only eGetDefault and in the other mode of operation the mode parameter can be either eGetFirst or
              eGetNext.
    */
    void *SCXodm::Get(CLASS_SYMBOL cs, const char *pCriteria, void *returnData, eGetMode mode)
    {
        void *pData;

        if(mode == eGetFirst || (mode == eGetDefault && m_fGetFirst))
        {
             pData = m_deps->GetFirst(cs, (char*)pCriteria, returnData);
             m_fGetFirst = false;
        }
        else if(mode == eGetNext || (mode == eGetDefault && !m_fGetFirst))
        {
            pData = m_deps->GetNext(cs, returnData);
        }
        else
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"mode", L"out of range", SCXSRCLOCATION);
        }
        if (reinterpret_cast<intptr_t>(pData) == -1)
        {
            throw SCXodmException(L"odm_get_first/odm_get_next failed", odmerrno, SCXSRCLOCATION);
        }
        else if (NULL == pData)
        {
            // If there is no "next", then reset so next Get() call will be first

            m_fGetFirst = true;
        }
        return pData;
    }

    /** Gets information from the ODM database.
        \param cs           Pointer to class symbol (generally ClassName_CLASS from header)
        \param criteria     Search criteria (string containing qualifying search criteria)
        \param returnData   Pointer to data structure (from header) returned for this class
        \param mode         Mode of operation

        \throws  SCXodmException if odm internal error occurred
        \throws  SCXodmException if mode parameter is invalid
        \returns NULL if no match is found
        \NOTE During the existance SCXodm object can operate in two modes. In one mode of operation the mode parameter
              can be only eGetDefault and in the other mode of operation the mode parameter can be either eGetFirst or
              eGetNext.
    */
    void *SCXodm::Get(CLASS_SYMBOL cs, const std::wstring &wCriteria, void *returnData, eGetMode mode)
    {
        // Convert criteria into a form for the odm_get_* functions (non-const C-style string)
        std::string sCriteria = SCXCoreLib::StrToUTF8(wCriteria);
        std::vector<char> criteria(sCriteria.c_str(), sCriteria.c_str() + sCriteria.length() + 1);
        return Get(cs, &criteria[0], returnData, mode);
    }

    /*----------------------------------------------------------------------------*/
    /**
       BUG 462269, for some reason ODM can not be used before some system calls that themselves
       use ODM are called. For example, after perfstat_netinterface is called it is possible to use
       ODM database. Workaround to start the ODM database, code from networkinterface.cpp:
    */
    void SCXodm::perfInterface_init()
    {
        if(perfInterfaced == false)
        {
            perfstat_id_t first;
            int structsAvailable = perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);
            if (structsAvailable < 0)
            {
                throw SCXodmException(L"perfstat_netinterface", errno, SCXSRCLOCATION);
            }
            std::vector<char> buffer(structsAvailable * sizeof(perfstat_netinterface_t));
            perfstat_netinterface_t *statp = reinterpret_cast<perfstat_netinterface_t *>(&buffer[0]);
            strcpy(first.name, FIRST_NETINTERFACE);
            int structsReturned = perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), structsAvailable);
            if (structsReturned < 0)
            {
                throw SCXodmException(L"perfstat_netinterface", errno, SCXSRCLOCATION);
            }
            perfInterfaced = true;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
         Format details of violation
    */
    std::wstring SCXodmException::What() const 
    { 
        std::wstring s = L"SCXodm error: ODM error because ";
        s.append(m_Reason);
        s.append(L": ODM error ");
        s.append(SCXCoreLib::StrFrom(GetErrno()));
        return s;
    }
} // SCXSystemLib

#endif // defined(aix)
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

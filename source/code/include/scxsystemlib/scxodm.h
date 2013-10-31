/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file

    \brief       This file contains helper functions for use of the
                 ODM database on IBM AIX systems.

    \date        2011-08-15 16:05:00


*/
/*----------------------------------------------------------------------------*/
#ifndef SCXODM_H
#define SCXODM_H

#if !defined(aix)
#error this file is only meaningful on the AIX platform
#endif

#include <odmi.h>
#include <sys/cfgodm.h>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxthreadlock.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       This class encapsulates dependencies for the ODM system on IBV AIX.
    */
    class SCXodmDependencies
    {
    public:
        SCXodmDependencies();
        virtual ~SCXodmDependencies();

        virtual int Initialize();
        virtual int Terminate();
        virtual void *GetFirst(CLASS_SYMBOL cs, char *criteria, void *returnData);
        virtual void *GetNext(CLASS_SYMBOL cs, void *returnData);

    private:
        bool m_fInitialized;            //!< Have we been initialized properly?
        //! Named lock for this instance to be shared across all threads
        SCXCoreLib::SCXThreadLockHandle m_lock;
    };

    /*----------------------------------------------------------------------------*/
    /**
        This class provides helper functions for use of the ODM database on AIX.

        This class is pretty thin right now (errors from functions are directly exposed
        to callers), but can be improved as needed.
    */

    class SCXodm
    {
    private:
        bool m_fGetFirst;                                       //!< Flag to specify type of get
        static bool perfInterfaced;     //bug 462269, need to call perfstat_netinterface for odm to work
        void perfInterface_init();      //bug 462269, need to call perfstat_netinterface for odm to work

    protected:
        SCXCoreLib::SCXHandle<SCXodmDependencies> m_deps;       //!< Dependency object.

        SCXodm(SCXCoreLib::SCXHandle<SCXodmDependencies> deps)
            : m_deps(deps)
        { }

    public:
        //! Get function mode parameter
        enum eGetMode
        {
            eGetDefault = 0, //!< Default behavior
            eGetFirst,       //!< Get first element
            eGetNext         //!< Get next element
        };

        SCXodm();
        virtual ~SCXodm();

        std::wstring DumpString() const;
        virtual void *Get(CLASS_SYMBOL cs, const char *pCriteria, void *returnData, eGetMode mode = eGetDefault);
        virtual void *Get(CLASS_SYMBOL cs, const std::wstring &wCriteria, void *returnData, eGetMode mode = eGetDefault);
    };

    /** Exception for general kstat error */
    class SCXodmException : public SCXCoreLib::SCXException
    {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] odm_errno (ODM-specific) Errno related to the error.
            \param[in] l Source code location.
        */
        SCXodmException(std::wstring reason, int odm_errno, const SCXCoreLib::SCXCodeLocation& l)
            : SCXException(l),
              m_Reason(reason),
              m_errno(odm_errno)
        { }

        std::wstring What() const;
        /*----------------------------------------------------------------------------*/
        /**
           Return errno related to the error.
        */
        int GetErrno() const { return m_errno; }

    protected:
        //! Description of internal error
        std::wstring   m_Reason;
        int m_errno; //!< Errno related to the error.
    };
};

#endif /* SCXODM_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

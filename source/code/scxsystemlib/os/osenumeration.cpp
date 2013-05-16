/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       Enumeration of Operating System 

   \date        08-03-04 14:12:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>

using namespace SCXCoreLib;

namespace SCXSystemLib
{
    /** Module name string */
    const wchar_t *OSEnumeration::moduleIdentifier = L"scx.core.common.pal.system.os.osenumeration";

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor
    */
    OSEnumeration::OSEnumeration() : EntityEnumeration<OSInstance>()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);

        SCX_LOGTRACE(m_log, L"OSEnumeration default constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Desctructor.
    */
    OSEnumeration::~OSEnumeration()
    {
        SCX_LOGTRACE(m_log, L"OSEnumeration::~OSEnumeration()");
    }

    /*----------------------------------------------------------------------------*/
    /**
        Creates the total OS instances.
    */
    void OSEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"OSEnumeration Init()");

        SetTotalInstance(SCXCoreLib::SCXHandle<OSInstance>(new OSInstance()));
        Update(true);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).
    
        Parameters:  None
        Retval:      The object represented as a string suitable for logging.
    
    */
    const std::wstring OSEnumeration::DumpString() const
    {
        return L"OSEnumeration";
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/


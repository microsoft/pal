/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Enumeration of Memory. Only contains the total instance.

    \date        2007-07-03 10:01:40

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

#include <scxsystemlib/memoryenumeration.h>
#include <scxsystemlib/memoryinstance.h>

using namespace SCXCoreLib;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor
    */
    MemoryEnumeration::MemoryEnumeration() : EntityEnumeration<MemoryInstance>()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.cpu.memoryenumeration");    
        SCX_LOGTRACE(m_log, L"MemoryEnumeration default constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
        Create Memory instances. Only creates the total instance.
    */
    void MemoryEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"MemoryEnumeration Init()");

        SetTotalInstance(SCXCoreLib::SCXHandle<MemoryInstance>(new MemoryInstance()));
        Update(true);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).
    
        Parameters:  None
        Retval:      The object represented as a string suitable for logging.
    
    */
    const std::wstring MemoryEnumeration::DumpString() const
    {
        return L"MemoryEnumeration";
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/


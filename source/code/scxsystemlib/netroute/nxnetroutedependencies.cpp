/*-----------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information

*/
/**
   \file nxnetroutedependencies.cpp

   \brief Dependencies needed by NxNetRoute classes

   \date  06-08-2015 16:20:00
 */
/*-----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/nxnetroutedependencies.h>
#include <sys/sysinfo.h>


using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{
    /*-----------------------------------------------------------------------------*/
    /**
       Constructor
    */
    NxNetRouteDependencies::NxNetRouteDependencies()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.netroute.nxnetroutedependencies"));
        SCX_LOGTRACE(m_log, wstring(L"NxNetRouteDependencies constructor "));
        m_pathToFile = L"/proc/net/route";
    }   

    /**
       Constructor - allows you to pass in a new path for the file normally located in proc/net/route
    */
    NxNetRouteDependencies::NxNetRouteDependencies(std::wstring pathToFile) : m_pathToFile(pathToFile)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.netroute.nxnetroutedependencies"));
        SCX_LOGTRACE(m_log, wstring(L"NxNetRouteDependencies constructor "));
    }   

    /*-----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    NxNetRouteDependencies::~NxNetRouteDependencies()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteDependencies destructor");
    }

    /*-----------------------------------------------------------------------------*/
    /**
       Init
    */
    void NxNetRouteDependencies::Init()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteDependencies Init()");
    }

    void NxNetRouteDependencies::CleanUp()
    {
    }

}

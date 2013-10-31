/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       Enumeration of BIOS 

  \date        11-01-14 15:00:00
 */
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxsystemlib/biosenumeration.h>
#include <scxcorelib/stringaid.h>

using namespace SCXCoreLib;

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
      Destructor

     */
    BIOSEnumeration::BIOSEnumeration()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.bios.biosenumeration"));

        SCX_LOGTRACE(m_log, std::wstring(L"BIOSEnumeration default constructor: "));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Creates the total BIOSEnumeration instance.
     */
    void BIOSEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"BIOSEnumeration Init()");
#if defined(linux) || (defined(sun) && !defined(sparc))
        SetTotalInstance(SCXCoreLib::SCXHandle<BIOSInstance>(new BIOSInstance()));
#elif (defined(sun) && defined(sparc))
        SCXCoreLib::SCXHandle<BiosDependencies> deps(new BiosDependencies());
        SetTotalInstance(SCXCoreLib::SCXHandle<BIOSInstance>(new BIOSInstance(deps)));
#else
        SetTotalInstance(SCXCoreLib::SCXHandle<BIOSInstance>(new BIOSInstance()));
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Cleanup

     */
    void BIOSEnumeration::CleanUp()
    {


    }

    /*----------------------------------------------------------------------------*/
    /**
      Destructor

     */
    BIOSEnumeration::~BIOSEnumeration()
    {
        SCX_LOGTRACE(m_log, std::wstring(L"BIOSEnumeration default destructor: "));
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

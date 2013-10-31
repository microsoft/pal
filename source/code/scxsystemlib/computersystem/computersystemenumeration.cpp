/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       Enumeration of ComputerSystem. 

  \date        11-04-18 14:45:00
 */
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/computersystemenumeration.h>
#include <scxcorelib/stringaid.h>
#include <sstream>
#include <scxsystemlib/computersysteminstance.h>

#if defined(sun)
#include <sys/sysinfo.h>
#endif


using namespace SCXCoreLib;

namespace SCXSystemLib
{


    /*----------------------------------------------------------------------------*/
    /**
      Constructor. 

     */
#if FILTERLINUX
    ComputerSystemEnumeration::ComputerSystemEnumeration(SCXCoreLib::SCXHandle<SCXSmbios> scxsmbios,
                                                      SCXCoreLib::SCXHandle<ComputerSystemDependencies>deps):
        m_deps(deps),
        m_scxsmbios(scxsmbios)
    {
    }
#elif defined(sun) || defined(aix) || defined(hpux)
    ComputerSystemEnumeration::ComputerSystemEnumeration(SCXCoreLib::SCXHandle<ComputerSystemDependencies> deps):
    m_deps(deps)
    {
    }
#else
    ComputerSystemEnumeration::ComputerSystemEnumeration()
    {
    }
#endif  


    /*----------------------------------------------------------------------------*/
    /**
      Create ComputerSystemEnumeration instances.
     */
    void ComputerSystemEnumeration::Init()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.computersystem.computerSystemEnumeration"));
        SCX_LOGTRACE(m_log, L"ComputerSystemEnumeration Init()");

#if FILTERLINUX
        SetTotalInstance(SCXCoreLib::SCXHandle<ComputerSystemInstance>(new ComputerSystemInstance(m_scxsmbios,m_deps)));
#elif defined(sun) || defined(aix) || defined(hpux)
        m_deps->Init();
        SetTotalInstance(SCXCoreLib::SCXHandle<ComputerSystemInstance>(new ComputerSystemInstance(m_deps)));
#endif

        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
      Update all the ComputerSystem instances.  
      
      This is done collectively for all instances by using a platform dependent 
     */
    void ComputerSystemEnumeration::Update(bool updateInstances)
    {
        if (updateInstances)
        {
            UpdateInstances();
        }

    }

    /*----------------------------------------------------------------------------*/
    /**
      Cleanup

     */
    void ComputerSystemEnumeration::CleanUp()
    {

        SCX_LOGTRACE(m_log, L"ComputerSystemEnumeration CleanUp()");

    }

    /*----------------------------------------------------------------------------*/
    /**
      Destructor

    */
    ComputerSystemEnumeration::~ComputerSystemEnumeration()
    {
        SCX_LOGTRACE(m_log, std::wstring(L"ComputerSystemEnumeration default destructor: "));
    }

}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

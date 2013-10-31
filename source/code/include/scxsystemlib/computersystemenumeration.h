/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Enumeration of ComputerSystem. 

   \date        11-04-18 14:00:00
*/
/*----------------------------------------------------------------------------*/
#ifndef COMPUTERSYSTEMENUMERATION_H
#define COMPUTERSYSTEMENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/computersysteminstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxcmn.h>

using namespace SCXCoreLib;


namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents a colletion of ComputerSystem. 

       PAL Holding collection of ComputerSystem. 

    */
    class ComputerSystemEnumeration : public EntityEnumeration<ComputerSystemInstance>
    {
    public:
#if FILTERLINUX
        explicit ComputerSystemEnumeration(SCXHandle<SCXSmbios> scxsmbios = SCXHandle<SCXSmbios>(new SCXSmbios()),
                       SCXHandle<ComputerSystemDependencies> deps = SCXHandle<ComputerSystemDependencies>(new ComputerSystemDependencies()) );
#elif defined(sun) || defined(aix) || defined(hpux)
        explicit ComputerSystemEnumeration(SCXCoreLib::SCXHandle<ComputerSystemDependencies> = SCXCoreLib::SCXHandle<ComputerSystemDependencies>(new ComputerSystemDependencies()) );
#else
        ComputerSystemEnumeration();
#endif  
        virtual ~ComputerSystemEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances = true);
        virtual void CleanUp();

    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
        SCXCoreLib::SCXHandle<ComputerSystemDependencies> m_deps; //!< Collects external dependencies of this class.
#if FILTERLINUX
        SCXCoreLib::SCXHandle<SCXSmbios> m_scxsmbios; //!< Collects external dependencies of this class
#elif defined(sun)

#endif
    };

}

#endif /* COMPUTERSYSTEMENUMERATION_H*/  
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

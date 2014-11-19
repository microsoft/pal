/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Enumeration of Processor. 

   \date        11-03-18 14:00:00
*/
/*----------------------------------------------------------------------------*/
#ifndef PROCESSORENUMERATION_H
#define PROCESSORENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents a colletion of Processor. 

       PAL Holding collection of Processor. 

    */
    class ProcessorEnumeration : public EntityEnumeration<ProcessorInstance>
    {
    public:
#if defined(linux) || (defined(sun) && !defined(sparc))
        explicit ProcessorEnumeration(SCXCoreLib::SCXHandle<SCXSmbios> scxsmbios = SCXCoreLib::SCXHandle<SCXSmbios>(new SCXSmbios()) );
#elif defined(sun) && defined(sparc)
        explicit ProcessorEnumeration(SCXCoreLib::SCXHandle<ProcessorPALDependencies> = SCXCoreLib::SCXHandle<ProcessorPALDependencies>(new ProcessorPALDependencies()) );
#endif  
        virtual ~ProcessorEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances = true);
        virtual void CleanUp();

    private:
#if defined(linux) || (defined(sun) && !defined(sparc))
        void GetDeviceIds(vector<std::wstring>& devicesId);  //!< Get all processors device ID.
#elif defined(sun) && defined(sparc)
        virtual void CreateProcessorInstances(); //!< Read kstat structure and construct processor instances.
#endif  

    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
#if defined(linux) || (defined(sun) && !defined(sparc))
        SCXCoreLib::SCXHandle<SCXSmbios> m_scxsmbios; //!< Collects external dependencies of this class
        struct SmbiosEntry m_smbiosEntry; //!<To get Smbios Table via current SmbiosEntry.
        MiddleData m_smbiosTable; //!< Smbios Table

#elif defined(sun) && defined(sparc)
        SCXCoreLib::SCXHandle<ProcessorPALDependencies> m_deps; //!< Collects external dependencies of this class.
#endif
    };

}

#endif /* PROCESSORENUMERATION_H*/
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

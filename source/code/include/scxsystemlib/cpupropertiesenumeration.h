/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Enumeration of Cpu Properties. 

   \date        11-10-31 14:00:00
*/
/*----------------------------------------------------------------------------*/

#ifndef CPUPROPERTIESENUMERATION_H
#define CPUPROPERTIESENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/cpupropertiesinstance.h>
#include <scxsystemlib/procfsreader.h>
#include <scxcorelib/scxlog.h>
#if defined(aix) 
#include <libperfstat.h>
#endif
#if defined(hpux) 
#include <sys/pstat.h>
#endif

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Class that represents a collection of CPU Properties.

       PAL Holding collection of CPU Properties.

    */
    class CpuPropertiesEnumeration : public EntityEnumeration<CpuPropertiesInstance>
    {
    public:
#if defined(linux) 
        explicit CpuPropertiesEnumeration(SCXCoreLib::SCXHandle<ProcfsCpuInfoReader> cpuinfoTable = SCXCoreLib::SCXHandle<ProcfsCpuInfoReader>(new ProcfsCpuInfoReader()));
#elif defined(sun) 
        explicit CpuPropertiesEnumeration(SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies> = SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies>(new CpuPropertiesPALDependencies()) );
#elif defined(aix) || defined(hpux)
        explicit CpuPropertiesEnumeration();
#endif
        virtual ~CpuPropertiesEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances = true);
        virtual void CleanUp();

    private:
#if defined(linux) 
        SCXCoreLib::SCXHandle<ProcfsCpuInfoReader> m_cpuinfoTable;
#endif
        virtual void CreateCpuPropertiesInstances(); // Construct processor instances.

#if defined(sun) 
        bool GetCpuCount(unsigned int& numCpus);              //!< Get number of physical CPUs present
#endif

    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
#if defined(sun) 
        SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies> m_deps; //!< Collects external dependencies of this class.
#elif defined(aix) 
        private:
            perfstat_partition_total_t m_partTotal;
            perfstat_cpu_total_t       m_cpuTotal;
#elif defined(hpux) 
        private:
            int  m_cpuTotal;
#endif
    };

}

#endif /* CPUPROPERTIESENUMERATION_H  */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

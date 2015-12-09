/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Enumeration of CPU:s

   \date        07-05-21 12:00:00

   \date        08-05-29 12:28:00
*/
/*----------------------------------------------------------------------------*/
#ifndef CPUENUMERATION_H
#define CPUENUMERATION_H

#include <vector>

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/cpuinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxthreadlock.h>

#if defined(sun)

#include <scxsystemlib/scxkstat.h>
#include <sys/types.h>
#include <sys/processor.h>

#elif defined(hpux)

#include <sys/mp.h>
#include <sys/pstat.h>

#elif defined(aix)

#include <sys/systemcfg.h>

#endif

namespace SCXSystemLib
{
    /** Time between each sample in seconds. */
    const int CPU_SECONDS_PER_SAMPLE = 60;

    /*----------------------------------------------------------------------------*/
    /**
     Class representing all external dependencies from the CPU PAL.
    */
    class CPUPALDependencies
    {
    public:
        virtual SCXCoreLib::SCXHandle<std::wistream> OpenStatFile() const;
        virtual SCXCoreLib::SCXHandle<std::wistream> OpenCpuinfoFile() const;
        virtual long sysconf(int name) const;
#if defined(sun)
        virtual const SCXCoreLib::SCXHandle<SCXKstat> CreateKstat() const;
        virtual int p_online(processorid_t processorid, int flag) const;
#elif defined(hpux)
        virtual int pstat_getprocessor(struct pst_processor *buf,
                                       size_t elemsize,
                                       size_t elemcount,
                                       int index) const;
        virtual int pstat_getdynamic(struct pst_dynamic *buf,
                                     size_t elemsize,
                                     size_t elemcount,
                                     int index) const;
#elif defined(aix)
        virtual int perfstat_cpu_total(perfstat_id_t *name,
                                       perfstat_cpu_total_t* buf,
                                       int bufsz,
                                       int number) const;

        virtual int perfstat_cpu(perfstat_id_t *name,
                                 perfstat_cpu_t* buf,
                                 int bugsz,
                                 int number) const;

        virtual int perfstat_partition_total(perfstat_id_t *name,
                                             perfstat_partition_total_t* buf,
                                             int sizeof_struct,
                                             int number) const;
#endif
        virtual ~CPUPALDependencies() {};
    };

    /*----------------------------------------------------------------------------*/
    /**
     Class that represents a colletion of CPU:s.

     PAL Holding collection of CPU:s.

    */
    class CPUEnumeration : public EntityEnumeration<CPUInstance>
    {
    public:
        explicit CPUEnumeration(SCXCoreLib::SCXHandle<CPUPALDependencies> = SCXCoreLib::SCXHandle<CPUPALDependencies>(new CPUPALDependencies()), time_t = CPU_SECONDS_PER_SAMPLE, size_t = MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        ~CPUEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();
        void SampleData();

        //
        // These would normally be protected, but are here for unit test purposes
        // (Being a friend doesn't seem to give access to protected static members)
        //
        static size_t ProcessorCountPhysical(
            SCXCoreLib::SCXHandle<CPUPALDependencies> deps,
            SCXCoreLib::SCXLogHandle& logH,
            bool fForceComputation = false);
        static size_t ProcessorCountLogical(SCXCoreLib::SCXHandle<CPUPALDependencies> deps);

        /**
         Provider access to ProcessorCountPhysical() method

         \param[out]    count  Number of physical cores on this system
         \param[in]     logH  Log handle (for logging purposes)
         \returns       True if supported on this platform, False otherwise

         */
        static bool GetProcessorCountPhysical(scxulong& count, SCXCoreLib::SCXLogHandle& logH)
        {
            SCXCoreLib::SCXHandle<CPUPALDependencies> deps(new CPUPALDependencies());
            count = static_cast<scxulong>(ProcessorCountPhysical(deps, logH));
            return ( count > 0 ? true : false );
        }

        /**
         Provider access to ProcessorCountLogical() method

         \param[out]    count  Number of physical cores on this system
         \returns       True if supported on this platform, False otherwise

         */
        static bool GetProcessorCountLogical(scxulong& count)
        {
            SCXCoreLib::SCXHandle<CPUPALDependencies> deps(new CPUPALDependencies());
            count = static_cast<scxulong>(ProcessorCountLogical(deps));
            return ( count > 0 ? true : false );
        }

    private:
        SCXCoreLib::SCXHandle<CPUPALDependencies> m_deps; //!< Collects external dependencies of this class.
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
        SCXCoreLib::SCXThreadLockHandle m_lock; //!< Handles locking in the cpu enumeration.
        time_t m_sampleSecs;			//!< Number of seconds between samples
        size_t m_sampleSize;                    //!< Number of elements stored in sample set

        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> m_dataAquisitionThread; //!< Thread pointer.
        static void DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param);
        bool IsCPUEnabled(const int cpuid);
#if defined(sun)
        SCXCoreLib::SCXHandle<SCXKstat> m_kstatHandle; //!< Keep a kstat object to avoid expensive kstat_open()
#endif

#if defined(sun) || defined(hpux)
        /*----------------------------------------------------------------------------*/
        /**
         Utility class for gathering all relevant cpu counters for all platforms
         and exposing them through a common interface.

         Instances are short-lived in the SampleData() loop.
        */
        class CPUStatHelper
        {
            friend class CPUEnumeration;
        public:
#if defined(sun)
            CPUStatHelper(unsigned int cpuid, SCXCoreLib::SCXHandle<SCXKstat> kstatHandle, SCXCoreLib::SCXHandle<CPUPALDependencies> deps);
#endif
            CPUStatHelper(unsigned int cpuid, SCXCoreLib::SCXHandle<CPUPALDependencies> deps);
            ~CPUStatHelper();
            void Update();
        protected:
            scxulong User;      //!< User ticks
            scxulong System;    //!< System ticks
            scxulong Idle;      //!< Idle ticks
            scxulong IOWait;    //!< IO Wait ticks
            scxulong Nice;      //!< Nice ticks
            scxulong Irq;       //!< IRQ Ticks
            scxulong SoftIrq;   //!< Soft IRC ticks (Dpc).
            scxulong Total;     //!< Total ticks.
        private:
            /*----------------------------------------------------------------------------*/
            /**
             Private constructor.
            */
            CPUStatHelper();
            void Init();

            SCXCoreLib::SCXLogHandle m_log;     //!< Log handle.
            unsigned int m_cpuid;               //!< The cpuid for which the counters where read.
            SCXCoreLib::SCXHandle<CPUPALDependencies> m_deps; //!< Collects external dependencies of this class.


#if defined(sun)
            SCXCoreLib::SCXHandle<SCXKstat> m_kstat;          //!< Kstat instance to read cpu counters from for Sun; local copy
            scxulong GetValue(const std::wstring & statistic);
#elif defined(hpux)
            struct pst_processor m_pst_processor; //!< CPU data
#else
#error "Not implemented for this platform"
#endif
        };

#elif defined(aix)

        std::vector<perfstat_cpu_t> m_dataarea; //!< Holds output from perfstat_cpu()
        bool m_threadStarted;                   //!< true if the subsidiary thread has run Update() at least one time
        perfstat_cpu_total_t m_dataarea_total;  //!< Holds output from perfstat_cpu_total()
        perfstat_id_t m_cpuid;                  //!< First CPU ID in perfstat_cpu() queries

#endif
    };

}

#endif /* CPUENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

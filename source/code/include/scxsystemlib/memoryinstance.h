/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       PAL representation of system Memory

    \date        2007-07-02 13:21:02

*/
/*----------------------------------------------------------------------------*/
#ifndef MEMORYINSTANCE_H
#define MEMORYINSTANCE_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/datasampler.h>
#include <string>
#include <vector>

#if defined(sun)
#include <scxsystemlib/scxkstat.h>
#endif


namespace SCXSystemLib
{
    /** Number of samples collected in the datasampler for Memory. */
    const int MAX_MEMINSTANCE_DATASAMPER_SAMPLES = 6;

    /** Time between each sample in seconds. */
    const int MEMORY_SECONDS_PER_SAMPLE = 60;

    /** Datasampler for memory information. */
    typedef DataSampler<scxulong> MemoryInstanceDataSampler;

    /*----------------------------------------------------------------------------*/
    /**
       Class representing all external dependencies from the Memory PAL.

    */
    class MemoryDependencies
    {
    public:

        MemoryDependencies();
        virtual ~MemoryDependencies() {};

#if defined(linux)

        virtual std::vector<std::wstring> GetMemInfoLines();
        virtual std::vector<std::wstring> GetVMStatLines();

#elif defined(sun)

        virtual scxulong GetPageSize();
        virtual scxulong GetPhysicalPages();
        virtual scxulong GetAvailablePhysicalPages();
        virtual long GetNumberOfCPUs();
        virtual void GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages);
        virtual SCXCoreLib::SCXHandle<SCXKstat> CreateKstat();
        virtual bool IsProcessorPresent(int id);

#elif defined(hpux)

        virtual void GetStaticMemoryInfo(scxulong& page_size, scxulong& physical_memory);
        virtual void GetDynamicMemoryInfo(scxulong& real_pages, scxulong& free_pages);
        virtual void GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages);
        virtual bool GetPageingData(scxulong& reads, scxulong& writes);

#elif defined(aix)

        virtual void GetMemInfo(scxulong& total_pages, scxulong& free_pages, scxulong& max_swap_pages, scxulong& free_swap_pages);
        virtual bool GetPageingData(scxulong& reads, scxulong& writes);

#endif
    private:
        SCXCoreLib::SCXLogHandle m_log;    //!< Log handle.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents values related to system memory.

        Concrete implementation of an instance of system memory.

        There is a slight difference in the implementation from the pattern
        described in EntityInstance. The difference is in the presence of
        a thread which updates the m_pageReads and m_pageWrites members
        continuously. So all updates are not contained to the Update function.

    */
    class MemoryInstance : public EntityInstance
    {
    public:

        MemoryInstance(SCXCoreLib::SCXHandle<MemoryDependencies> = SCXCoreLib::SCXHandle<MemoryDependencies>(new MemoryDependencies()), bool startThread = true);
        virtual ~MemoryInstance();

        // Return values indicate whether the implementation for this platform
        // supports the value or not.
        bool GetTotalPhysicalMemory(scxulong& totalPhysicalMemory) const;
        bool GetReservedMemory(scxulong& reservedMemory) const;
        bool GetAvailableMemory(scxulong& availableMemory) const;
        bool GetUsedMemory(scxulong& usedMemory) const;
        bool GetPageReads(scxulong& pageReads) const;
        bool GetPageWrites(scxulong& pageWrites) const;
        bool GetTotalSwap(scxulong& totalSwap) const;
        bool GetAvailableSwap(scxulong& availableSwap) const;
        bool GetUsedSwap(scxulong& usedSwap) const;

        virtual bool GetCacheSize(scxulong& cacheSize);
        virtual void Update();
        virtual void CleanUp();

        virtual const std::wstring DumpString() const;

        static bool GetPagingSinceBoot(scxulong& pageReads, scxulong& pageWrites, MemoryInstance* inst, SCXCoreLib::SCXHandle<MemoryDependencies> = SCXCoreLib::SCXHandle<MemoryDependencies>(new MemoryDependencies()));

    private:
#if defined(sun)
        SCXCoreLib::SCXHandle<SCXKstat> GetKstat()
        {
            return m_kstat; 
        }

        SCXCoreLib::SCXThreadLockHandle& GetKstatLockHandle()
        {
            return m_kstat_lock_handle;
        } 
#endif

    private:
        SCXCoreLib::SCXHandle<MemoryDependencies> m_deps; //!< Collects external dependencies of this class.
        SCXCoreLib::SCXLogHandle m_log;                 //!< Log handle.

        static const scxulong ONEKILO = 0x400;

        scxulong m_totalPhysicalMemory;                 //!< Total amount of physical memory.
        scxulong m_reservedMemory;                      //!< Amount of reseved memory unavailable for user processes.
        scxulong m_availableMemory;                     //!< Amount of available memory.
        scxulong m_usedMemory;                          //!< Amount of used memory.
        scxulong m_totalSwap;                           //!< Total amount of swap.
        scxulong m_availableSwap;                       //!< Amount of available swap.
        scxulong m_usedSwap;                            //!< Amount of used swap.
        MemoryInstanceDataSampler m_pageReads;          //!< Data sampler for page reads.
        MemoryInstanceDataSampler m_pageWrites;         //!< Data sampler for page writes.
        bool m_reservedMemoryIsSupported;               //!< Is m_reservedMemory a usable number?

        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> m_dataAquisitionThread;  //!< Pointer to thread body.

#if defined(sun)
        SCXCoreLib::SCXHandle<SCXKstat> m_kstat;         //!< kstat structure used to get data on Solaris
        SCXCoreLib::SCXThreadLockHandle m_kstat_lock_handle; //!< Lock to serialize access to kstat functions
#endif

        static void DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param);

    protected:
#if defined(linux)
        bool m_foundTotalPhysMem;                       //< Was "MemTotal:" line found in /proc/meminfo
        bool m_foundAvailMem;                           //< Was "MemFree:" line found in /proc/meminfo
        bool m_foundTotalSwap;                          //< Was "SwapTotal:" line found in /proc/meminfo
        bool m_foundAvailSwap;                          //< Was "SwapFree:" line found in /proc/meminfo
#endif
    };

}

#endif /* MEMORYINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       PAL representation of system memory

    \date        2007-07-02 17:50:20

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>
#include <scxsystemlib/memoryinstance.h>
#include <scxsystemlib/scxsysteminfo.h>
#include <string>
#include <sstream>

#if defined(sun)
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/swap.h>
#include <sys/sysinfo.h>
#endif

#if defined(hpux)
#include <sys/pstat.h>
#endif

#if defined(aix)
#include <libperfstat.h>
#include <sys/vminfo.h>
#endif

using namespace SCXCoreLib;

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    */
    MemoryDependencies::MemoryDependencies()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
    }

#if defined(linux)

    /*----------------------------------------------------------------------------*/
    /**
        Get all lines from meminfo file

        \returns     vector of lines
    */
    std::vector<std::wstring> MemoryDependencies::GetMemInfoLines()
    {
        const std::wstring meminfoFileName = L"/proc/meminfo";
        std::vector<std::wstring> lines;
        SCXStream::NLFs nlfs;

        SCXFile::ReadAllLines(SCXFilePath(meminfoFileName), lines, nlfs);

        return lines;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get all lines from vmstat file

        \returns     vector of lines
    */
    std::vector<std::wstring> MemoryDependencies::GetVMStatLines()
    {
        const std::wstring vmstatFileName = L"/proc/vmstat";
        std::vector<std::wstring> lines;
        SCXStream::NLFs nlfs;

        SCXFile::ReadAllLines(SCXFilePath(vmstatFileName), lines, nlfs);

        return lines;
    }

#elif defined(sun)

    /*----------------------------------------------------------------------------*/
    /**
        Get page size

        \returns     size of a page
    */
    scxulong MemoryDependencies::GetPageSize()
    {
        long pageSizeL = sysconf(_SC_PAGESIZE);
        SCXASSERT(-1 != pageSizeL && "_SC_PAGESIZE not found");
        return static_cast<scxulong>(pageSizeL);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages of physical memory

        \returns     number of pages
    */
    scxulong MemoryDependencies::GetPhysicalPages()
    {
        long physPagesL = sysconf(_SC_PHYS_PAGES);
        SCXASSERT(-1 != physPagesL && "_SC_PHYS_PAGES not found");
        return static_cast<scxulong>(physPagesL);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages of available physical memory

        \returns     number of pages
    */
    scxulong MemoryDependencies::GetAvailablePhysicalPages()
    {
        long availPhysPagesL = sysconf(_SC_AVPHYS_PAGES);
        SCXASSERT(-1 != availPhysPagesL && "_SC_AVPHYS_PAGES not found");
        return static_cast<scxulong>(availPhysPagesL);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pus.

        \returns     number of cpus
    */
    long MemoryDependencies::GetNumberOfCPUs()
    {
        long number_of_cpus = sysconf(_SC_NPROCESSORS_CONF);
        SCXASSERT(-1 != number_of_cpus && "_SC_NPROCESSORS_CONF not found");
        return number_of_cpus;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total number of pages for swap and number of reserved pages

        \param[out]  max_pages        max number of pages for swap
        \param[out]  reserved_pages   number of reserved pages in swap
    */
    void MemoryDependencies::GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages)
    {
        struct anoninfo swapinfo;
        int result = swapctl(SC_AINFO, &swapinfo);
        SCXASSERT(-1 != result && "swapctl failed");

        max_pages = swapinfo.ani_max;
        reserved_pages = swapinfo.ani_resv;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Tests if a specified processor is installed on system.

        \param[in]   id               Processor id
        \returns     True if specified processor exists on system
        \throws      SCXErrnoException if test fails
    */
    bool MemoryDependencies::IsProcessorPresent(int id)
    {
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"MemoryDependencies::IsProcessorPresent() - calling p_online(", id).append(L", P_STATUS)"));
        int status = ::p_online(id, P_STATUS);

        SCX_LOGHYSTERICAL(m_log, StrAppend(L"MemoryDependencies::IsProcessorPresent() - p_online status: ", status));
        if (-1 == status) {         // Failed, but why?
            if (EINVAL == errno) {
                return false;           // Processor not present
            } else {
                SCX_LOGWARNING(m_log, StrAppend(L"MemoryDependencies::IsProcessorPresent() - p_online status: -1 (", errno).append(L"), the CPU is in an error state"));
                throw SCXErrnoException(L"p_online", errno, SCXSRCLOCATION);
            }
        }

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Creates a new SCXKstat object with cpu/vm information.

       \param[in]  cpuid  Id number of cpu to create kstat object for.

       \returns  Handle to newly created kstat.
    */
    SCXHandle<SCXKstat> MemoryDependencies::CreateKstat()
    {
        return SCXHandle<SCXKstat>(new SCXKstat());
    }

#elif defined(hpux)

        /*
          HP kindly provides an easy way to read all kind of system and kernel data.
          This is collectively known as the pstat interface.
          It's supposed to be relatively upgrade friendly, even without recompilation.
          What is lacking however, is documentation. There is a whitepaper on pstat that
          you can look for at HP's site, which is very readable. But the exact semantics of
          each and every parameter is subject to experimentation and guesswork. I read
          somewhere that, to truly understand them you would need to have access to the
          kernel source. Needless to say, we don't. I've written a document called
          "Memory monitoring on HPUX.docx" that summarizes the needs and what's available.

          These are the system variables that we use together with ALL the documentation that HP provide

          psts.page_size       - page size in bytes/page
          psts.physical_memory - system physical memory in 4K pages
          pstd.psd_rm          - total real memory
          pstd.psd_free        - free memory pages
          pstv.psv_swapspc_max - max pages of on-disk backing store
          pstv.psv_swapspc_cnt - pages of on-disk backing store
          pstv.psv_swapmem_max - max pages of in-memory backing store
          pstv.psv_swapmem_cnt - pages of in-memory backing store
          pstv.psv_swapmem_on  - in-memory backing store enabled

          For usedMemory we use a measure of all real (physical) memory assigned to processes
          For availableMemory we use the size of unassigned memory
        */

    /*----------------------------------------------------------------------------*/
    /**
        Get page size and size in pages of physical memory

        \param[out]  page_size        size of a page
        \param[out]  physical_memory  number of pages of physical memory
    */
    void MemoryDependencies::GetStaticMemoryInfo(scxulong& page_size, scxulong& physical_memory)
    {
        struct pst_static psts;

        if (pstat_getstatic(&psts, sizeof(psts), 1, 0) < 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do pstat_getstatic()", SCXSRCLOCATION);
        }

        physical_memory = static_cast<scxulong>(psts.physical_memory);
        page_size = static_cast<scxulong>(psts.page_size);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages used and free memory

        \param[out]  real_pages  number of used pages
        \param[out]  free_pages  number of free pages
    */
    void MemoryDependencies::GetDynamicMemoryInfo(scxulong& real_pages, scxulong& free_pages)
    {
        struct pst_dynamic pstd;

        if (pstat_getdynamic(&pstd, sizeof(pstd), 1, 0) < 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do pstat_getdynamic()", SCXSRCLOCATION);
        }

        real_pages = static_cast<scxulong>(pstd.psd_rm);
        free_pages = static_cast<scxulong>(pstd.psd_free);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total number of pages for swap and number of reserved pages

        \param[out]  max_pages        max number of pages for swap
        \param[out]  reserved_pages   number of reserved pages in swap
    */
    void MemoryDependencies::GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages)
    {
        struct pst_vminfo pstv;

        if (pstat_getvminfo(&pstv, sizeof(pstv), 1, 0) < 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do pstat_getvminfo()", SCXSRCLOCATION);
        }

        max_pages = static_cast<scxulong>(pstv.psv_swapspc_max + pstv.psv_swapmem_on * pstv.psv_swapmem_max);
        reserved_pages = static_cast<scxulong>(pstv.psv_swapspc_cnt + pstv.psv_swapmem_on * pstv.psv_swapmem_cnt);
    }


    /*----------------------------------------------------------------------------*/
    /**
        Get total number of page reads and writes

        \param[out]  reads   number of reads
        \param[out]  writes  number of writes

        \returns     true if a values gould be retrieved
    */
    bool MemoryDependencies::GetPageingData(scxulong& reads, scxulong& writes)
    {
        struct pst_vminfo pstv;

        /* Get information about the system virtual memory variables */
        if (pstat_getvminfo(&pstv, sizeof(pstv), 1, 0) != 1) {
            return false;
        }

        // These are the system variables that we use together with ALL the documentation that HP provide
        // pstv.psv_spgpgin     - pages paged in
        // pstv.psv_spgpgout    - pages paged out

        reads = pstv.psv_spgpgin;
        writes = pstv.psv_spgpgout;

        /*
          Note: There's a variable that count the total number of faults taken: pstv.psv_sfaults
          There are also measures of the rates for all these. They are, respectively:
          pstv.psv_rpgin, pstv.psv_rpgout, and pstv.psv_rfaults.
        */

        return true;
    }

#elif defined(aix)

    /*----------------------------------------------------------------------------*/
    /**
        Get total memory size, free memory size, swap size and free swap size

        \param[out]  total_pages      Total memory size
        \param[out]  free_pages       Free memory size
        \param[out]  max_swap_pages   Swap size
        \param[out]  free_swap_pages  Free swap size

        All sizes in pages.
        A page on AIX is 4K.
    */
    void MemoryDependencies::GetMemInfo(scxulong& total_pages, scxulong& free_pages, scxulong& max_swap_pages, scxulong& free_swap_pages)
    {
        perfstat_memory_total_t mem;
   
        if (perfstat_memory_total(NULL, &mem, sizeof(perfstat_memory_total_t), 1) != 1)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do perfstat_memory_total()", SCXSRCLOCATION);
        }

        // WI 617621: AIX available memory calculation incorrectly handles FS cache
        //
        // Previously, we consider free pages to be mem.real_free + mem.numperm
        // (the idea is that mem.numperm reflected the amount of FS cache that
        // we wanted to consider as "free").  However, this isn't necessarily
        // accurate.
        //
        // AIX has a concept of "minimum cache size" (based on configuration
        // parameter "lru_free_repage"). Briefly, vm setting minperm is the
        // smallest that the cache will be allowed to go unless things get
        // seriously desperate.  As a result, we've modified free memory
        // calculations to take minperm into account.
        //
        // See "Overview of AIX page replacement" for more info:
        //   http://www.ibm.com/developerworks/aix/library/au-vmm

        // Look up "minperm" setting (minimum size of the cache)
        //   (Interactively, use 'vmo -L' for this)
        struct vminfo vm;
        if (vmgetinfo(&vm, VMINFO_ABRIDGED, sizeof(vm)) != 0)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Could not do vmgetinfo()", SCXSRCLOCATION);
        }

        total_pages = static_cast<scxulong>(mem.real_total);

        // Take into account file buffers.  Algorithm:
        //   if (numperm > minperm), consider (numperm - minperm) to be free
        //   otherwise, consider the cache to be completely empty
        if (mem.numperm > vm.minperm)
        {
            free_pages = static_cast<scxulong>(mem.real_free + (mem.numperm - vm.minperm));
        }
        else
        {
            free_pages = static_cast<scxulong>(mem.real_free);
        }

        max_swap_pages = static_cast<scxulong>(mem.pgsp_total);
        free_swap_pages = static_cast<scxulong>(mem.pgsp_free);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total number of page reads and writes

        \param[out]  reads   number of reads
        \param[out]  writes  number of writes

        \returns     true if a values gould be retrieved
    */
    bool MemoryDependencies::GetPageingData(scxulong& reads, scxulong& writes)
    {
        perfstat_memory_total_t mem;
   
        if (perfstat_memory_total(NULL, &mem, sizeof(perfstat_memory_total_t), 1) != 1)
        {
            return false;
        }

        reads = mem.pgins;
        writes = mem.pgouts;

        return true;
    }

#else
#error "Not implemented for this platform."
#endif

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents values passed between the threads of the memory instance.

        Representation of values passed between the threads of the memory instance.

    */
    class MemoryInstanceThreadParam : public SCXThreadParam
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor.

            \param[in] pageReads   Datasampler for holding measurements of page reads.
            \param[in] pageWrites  Datasampler for holding measurements of page writes.
            \param[in] deps        Dependencies for the Memory data colletion.

        */
        MemoryInstanceThreadParam(MemoryInstanceDataSampler* pageReads,
                                  MemoryInstanceDataSampler* pageWrites,
                                  SCXCoreLib::SCXHandle<MemoryDependencies> deps,
                                  MemoryInstance* inst)
            : SCXThreadParam(),
              m_pageReads(pageReads),
              m_pageWrites(pageWrites),
              m_deps(deps),
              m_inst(inst)
        {}

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the page reads parameter.

            \returns Pointer to datasampler for holding measurements of page reads.

        */
        MemoryInstanceDataSampler* GetPageReads()
        {
            return m_pageReads;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the page writes parameter.

            \returns Pointer to datasampler for holding measurements of page writes.

        */
        MemoryInstanceDataSampler* GetPageWrites()
        {
            return m_pageWrites;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the dependency structure.

            \returns Pointer to the dependency structure

        */
        SCXCoreLib::SCXHandle<MemoryDependencies> GetDeps()
        {
            return m_deps;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the instance pointer.

            \returns Pointer to the memory instance

        */
        MemoryInstance* GetInst()
        {
            return m_inst;
        }

    private:
        MemoryInstanceDataSampler* m_pageReads;           //!< Pointer to datasampler for holding measurements of page reads.
        MemoryInstanceDataSampler* m_pageWrites;          //!< Pointer to datasampler for holding measurements of page writes.
        SCXCoreLib::SCXHandle<MemoryDependencies> m_deps; //!< Collects external dependencies.
        MemoryInstance* m_inst;                           //!< Pointer to to the memory instance
    };


    /*----------------------------------------------------------------------------*/
    /**
        Constructor

       \param[in] deps  Dependencies for the Memory data colletion.

    */
    MemoryInstance::MemoryInstance(SCXCoreLib::SCXHandle<MemoryDependencies> deps, bool startThread /* = true */) :
        EntityInstance(true),
        m_deps(deps),
        m_totalPhysicalMemory(0),
        m_reservedMemory(0),
        m_availableMemory(0),
        m_usedMemory(0),
        m_totalSwap(0),
        m_availableSwap(0),
        m_usedSwap(0),
        m_pageReads(MAX_MEMINSTANCE_DATASAMPER_SAMPLES),
        m_pageWrites(MAX_MEMINSTANCE_DATASAMPER_SAMPLES),
#if defined(sun)
        m_reservedMemoryIsSupported(false),
        m_kstat_lock_handle(std::wstring(L"MemoryInstance")),
#elif defined(hpux)
        m_reservedMemoryIsSupported(true),
#else
        m_reservedMemoryIsSupported(false),
#endif
        m_dataAquisitionThread(0)
#if defined(linux)
        , m_foundTotalPhysMem(false)
        , m_foundAvailMem(false)
        , m_foundTotalSwap(false)
        , m_foundAvailSwap(false)
#endif
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
        SCX_LOGTRACE(m_log, L"MemoryInstance default constructor");

#if defined(sun)
        m_kstat = deps->CreateKstat();
#endif

        if (startThread)
        {
            MemoryInstanceThreadParam* params = new MemoryInstanceThreadParam(&m_pageReads, &m_pageWrites, m_deps, this);
            m_dataAquisitionThread = new SCXCoreLib::SCXThread(MemoryInstance::DataAquisitionThreadBody, params);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
        Destructor

    */
    MemoryInstance::~MemoryInstance()
    {
        SCX_LOGTRACE(m_log, L"MemoryInstance destructor");
        if (NULL != m_dataAquisitionThread) {
            if (m_dataAquisitionThread->IsAlive())
            {
                CleanUp();
            }
            m_dataAquisitionThread = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total memory.

        \param[out]  totalPhysicalMemory Total physical memory of machine in bytes.

        \returns     true if a value is supported by the implementation

    */
    bool MemoryInstance::GetTotalPhysicalMemory(scxulong& totalPhysicalMemory) const
    {
        totalPhysicalMemory = m_totalPhysicalMemory;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get available (free) memory.

        \param[out]  availableMemory Available memory in bytes.

        \returns     true if a value is supported by this implementation

        This is the amount of physical memory that is currently available for
        use by user processes. In other words; memory that is not reported under
        UsedMemory or ReservedMemory.
    */
    bool MemoryInstance::GetAvailableMemory(scxulong& availableMemory) const
    {
        availableMemory = m_availableMemory;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get reserved memory.

        \param[out]  reservedMemory Amount of reserved memory in bytes.

        \returns     true if a value is supported by this implementation

        This is the amount of memory that the system has reserved for special
        purposes, and that will never be available for user processes.
        On some, if not most, systems this figure is unavailabe. In those cases
        any reserved memory will included in the number for UsedMemory.

        \note This was added because HPUX can potentially reserve a huge
        amount of pysical memory for its pseudo-swap feature that would seriosly
        skew the UsedMemory reading.
    */
    bool MemoryInstance::GetReservedMemory(scxulong& reservedMemory) const
    {
        reservedMemory = m_reservedMemory;
        return m_reservedMemoryIsSupported;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get used memory.

        \param[out]  usedMemory Amount of used memory.

        \returns     true if a value is supported by this implementation

        The amount of physical memory that is currenly allocated. If ReservedMemory
        is supported this number is mostly memory used by user processes. If not,
        this figure includes memory reserved by the system.
    */
    bool MemoryInstance::GetUsedMemory(scxulong& usedMemory) const
    {
        usedMemory = m_usedMemory;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages read from disk per second to resolve a hard page fault.

        \param[out] pageReads Number of page reads per second.

        \returns    true if a value is supported by this implementation

    */
    bool MemoryInstance::GetPageReads(scxulong& pageReads) const
    {
        pageReads = m_pageReads.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES)
            / MEMORY_SECONDS_PER_SAMPLE;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of pages written to disk per second to resolve hard page faults.

        \param[out] pageWrites Number of page writes per second.

        \returns    true if a value is supported by this implementation

    */
    bool MemoryInstance::GetPageWrites(scxulong& pageWrites) const
    {
        pageWrites = m_pageWrites.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES)
            / MEMORY_SECONDS_PER_SAMPLE;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the total amount of swap space in bytes.

        \param[out]  totalSwap Total amount of swap space.

        \returns     true if a value is supported by this implementation

    */
    bool MemoryInstance::GetTotalSwap(scxulong& totalSwap) const
    {
        totalSwap = m_totalSwap;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get amount of available (free) swap space in bytes.

        \param[out]  availableSwap Amount of available swap space.

        \returns     true if a value is supported by this implementation

    */
    bool MemoryInstance::GetAvailableSwap(scxulong& availableSwap) const
    {
        availableSwap = m_availableSwap;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the amount of used swap space in bytes.

        \param[out]  usedSwap The amount of used swap space.
        \returns     true if a value is supported by this implementation

    */
    bool MemoryInstance::GetUsedSwap(scxulong& usedSwap) const
    {
        usedSwap = m_usedSwap;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieves the cache size. On non-Solaris systems this is a no-op today; 
       on Solaris systems with zfs installed it returns cache size. 

       \Note Prior versions of this code took c_min into account. However,
             due to WI631566 (Oracle Support SR #3-8264452461), this is not
             correct behavior. In constrained memory situations, Solaris will
             try to achieve a ZFS cache size of zero (0), to free memory for
             programs, thus reducing the cache size well below c_min.

             Note that this is often difficult for Solaris to achieve, as the
             arccache is a write-thru cache (data written to ZFS first goes to
             the arccache and is then written to disk from there), but Solaris
             will do it's best to achieve that over time in constrained memory
             situations.

       \param[out]  cacheSize The total amount of space used by the zfs cache
       \returns     True/false, and populates parameters. 

    */
    bool MemoryInstance::GetCacheSize(scxulong& cacheSize)
    {
#if defined(sun)
        SCXSystemLib::SystemInfo si;
        bool fIsInGlobalZone;

        // According to Oracle SR 3-13482152541, the Solaris ZFS cache lives
        // in the kernal, on the global zone. If the system has both global
        // and local zones, then the ZFS cache is in the global zone only,
        // and is shared among both the global and local zones.
        //
        // As a result, from the local zone perspective, the ZFS cache is
        // essentially "free" (it doesn't come from local zone memory).
        //
        // If we support non-global zones, and we're NOT in the global
        // zone, then just return zero for the cache size.
        if ( si.GetSUN_IsInGlobalZone(fIsInGlobalZone) )
        {
            if ( ! fIsInGlobalZone )
            {
                cacheSize = 0;
                return true;
            }
        }

        scxulong cache_size = 0;
        SCXCoreLib::SCXThreadLock lock(m_kstat_lock_handle);

        m_kstat->Update();
        
        try
        {
            m_kstat->Lookup("zfs", "arcstats", 0);
        }
        catch (SCXKstatNotFoundException)
        {
            cacheSize = 0;
            return false; 
        }
        
        try
        {
            // Get cache size and min cache size, return 0 if not able to find values
            if( !m_kstat->TryGetValue(std::wstring(L"size"), cache_size) )
            {
                cacheSize = 0;
                return false; 
            }
        }
        catch (SCXKstatStatisticNotFoundException)
        {
            cacheSize = 0;
            return false; 
        }
        
        cacheSize = cache_size; 
        return true;
#else
        cacheSize = 0;
        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update the object members with values from hardware.
        This method updates all values that are not time dependent.
        Time dependent values are updated from a separate thread.

    */
    void MemoryInstance::Update()
    {
        SCX_LOGTRACE(m_log, L"MemoryInstance Update()");

#if defined(linux)

    /**
        Update the object members with values from /proc/meminfo.

        \ex
        /proc/meminfo:
          MemTotal:       516400 kB
          MemFree:         91988 kB
          Buffers:         46148 kB
          Cached:         277860 kB
          SwapCached:      64136 kB
          Active:         299804 kB
          Inactive:        97372 kB
          HighTotal:           0 kB
          HighFree:            0 kB
          LowTotal:       516400 kB
          LowFree:         91988 kB
          SwapTotal:      514040 kB
          SwapFree:       402508 kB
          Dirty:             676 kB
          Writeback:           0 kB
          Mapped:          96832 kB
          Slab:            20356 kB
          CommitLimit:    772240 kB
          Committed_AS:   281692 kB
          PageTables:       1700 kB
          VmallocTotal:   507896 kB
          VmallocUsed:      3228 kB
          VmallocChunk:   504576 kB
          HugePages_Total:     0
          HugePages_Free:      0
          HugePages_Rsvd:      0
          Hugepagesize:     4096 kB

        We are interested in the following fields:
          MemTotal
          MemFree
          SwapTotal
          SwapFree
    */
	
	/**
	    The 3.14+ linux kernel has MemAvailable: which gives more appropirate value for available memory. We will be getting the available memory from MemAvailable: (if present) instead of MemFree + Buffers + Cached
	    MemTotal:        3522864 kB
	    MemFree:          175844 kB
	    MemAvailable:    2813432 kB
	    Buffers:              36 kB
	    Cached:           289148 kB
	    SwapCached:          332 kB
	    Active:           234424 kB
	    Inactive:         250828 kB
	    Active(anon):      94120 kB
	    Inactive(anon):   203372 kB
	    Active(file):     140304 kB
	    Inactive(file):    47456 kB
	    Unevictable:           0 kB
	    Mlocked:               0 kB
	    SwapTotal:       6655996 kB
	    SwapFree:        6638924 kB
	    Dirty:                 8 kB
	    Writeback:             0 kB
	    AnonPages:        195772 kB
	    Mapped:            28868 kB
	    Shmem:            101424 kB
	    Slab:            2765564 kB
	    SReclaimable:    2745856 kB
	    SUnreclaim:        19708 kB
	    KernelStack:        4224 kB
	    PageTables:         6588 kB
	    NFS_Unstable:          0 kB
	    Bounce:                0 kB
	    WritebackTmp:          0 kB
	    CommitLimit:     8417428 kB
	    Committed_AS:    1004816 kB
	    VmallocTotal:   34359738367 kB
	    VmallocUsed:       67624 kB
	    VmallocChunk:   34359663604 kB
	    HardwareCorrupted:     0 kB
	    AnonHugePages:     75776 kB
	    HugePages_Total:       0
	    HugePages_Free:        0
	    HugePages_Rsvd:        0
	    HugePages_Surp:        0
	    Hugepagesize:       2048 kB
	    DirectMap4k:       94144 kB
	    DirectMap2M:     3575808 kB

	*/
        std::vector<std::wstring> lines = m_deps->GetMemInfoLines();

        scxulong buffers = 0, cached = 0, reportedAvailableMemory = 0; // KB

        for (size_t i = 0; i < lines.size(); i++)
        {
            std::wstring line = lines[i];

            SCX_LOGHYSTERICAL(m_log, std::wstring(L"UpdateFromMemInfo() - Read line: ").append(line));

            std::vector<std::wstring> tokens;
            StrTokenize(line, tokens);
            if (tokens.size() >= 2)
            {
                if (L"MemTotal:" == tokens[0])
                {
                    try
                    {
                        m_totalPhysicalMemory = StrToULong(tokens[1]) * 1024;  // Resulting units: bytes
                        m_foundTotalPhysMem = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    totalPhysicalMemory = ", m_totalPhysicalMemory));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_totalPhysicalMemory from :").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"MemAvailable:" == tokens[0])
                {
                    try
                    {
                        reportedAvailableMemory = StrToULong(tokens[1]) * 1024;  // Resulting units: bytes
                        m_foundAvailMem = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    availableMemory = ", m_availableMemory));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_availableMemory from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"MemFree:" == tokens[0])
                {
                    try
                    {
                        m_availableMemory = StrToULong(tokens[1]) * 1024;  // Resulting units: bytes
                        m_foundAvailMem = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    availableMemory = ", m_availableMemory));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_availableMemory from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"Buffers:" == tokens[0])
                {
                    try
                    {
                        buffers = StrToULong(tokens[1]) * 1024;  // Resulting units: bytes
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    buffers = ", buffers));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read buffers from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"Cached:" == tokens[0])
                {
                    try
                    {
                        cached = StrToULong(tokens[1]) * 1024;  // Resulting units: bytes
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    Cached = ", cached));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read buffers from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"SwapTotal:" == tokens[0])
                {
                    try
                    {
                        m_totalSwap = StrToULong(tokens[1]) * 1024; // Resulting units: bytes
                        m_foundTotalSwap = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    totalSwap = ", m_totalSwap));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_totalSwap from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
                if (L"SwapFree:" == tokens[0])
                {
                    try
                    {
                        m_availableSwap = StrToULong(tokens[1]) * 1024; // Resulting units: bytes
                        m_foundAvailSwap = true;
                        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    availableSwap = ", m_availableSwap));
                    }
                    catch (const SCXNotSupportedException& e)
                    {
                        SCX_LOGWARNING(m_log, std::wstring(L"Could not read m_availableSwap from: ").append(line).append(L" - ").append(e.What()));
                    }
                }
            }
        }

        // perform some adjustments and calculations. Resulting units: bytes.
        if (reportedAvailableMemory)
        {
            m_availableMemory = reportedAvailableMemory;
        } else {
            m_availableMemory += buffers + cached;
        }
        m_usedMemory = m_totalPhysicalMemory - m_availableMemory;
        m_usedSwap = m_totalSwap - m_availableSwap;

        // This is "weak" in that it can easily be missed; we verify this in unit tests too now
        SCXASSERT(m_foundTotalPhysMem && "MemTotal not found");
        SCXASSERT(m_foundAvailMem && "MemFree not found");
        SCXASSERT(m_foundTotalSwap && "SwapTotal not found");
        SCXASSERT(m_foundAvailSwap && "SwapFree not found");

#elif defined(sun)

        /*
          Update the object members with info from sysconf and swapctl.
        */

        scxulong pageSize = m_deps->GetPageSize();
        m_totalPhysicalMemory = m_deps->GetPhysicalPages() * pageSize;      // Resulting units: bytes
        m_availableMemory = m_deps->GetAvailablePhysicalPages() * pageSize; // Resulting units: bytes

        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Page Size (", pageSize).append(L")"));
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Total Physical Memory (", m_totalPhysicalMemory).append(L")"));
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Memory Available (", m_availableMemory).append(L")"));

        scxulong cacheSize = 0;
        GetCacheSize(cacheSize);
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - ZFS Cache Size (", cacheSize).append(L")"));

        m_availableMemory += cacheSize;
        m_usedMemory = m_totalPhysicalMemory - m_availableMemory;           // Resulting units: bytes

        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - New Memory Available (", m_availableMemory).append(L")"));
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Used Memory (", m_usedMemory).append(L")"));

        scxulong max_pages = 0;
        scxulong reserved_pages = 0;
        m_deps->GetSwapInfo(max_pages, reserved_pages);
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Swap Max Pages (", max_pages).append(L")"));
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Swap Reserved Pages (", reserved_pages).append(L")"));

        m_totalSwap = max_pages * pageSize;                    // Resulting units: bytes
        m_availableSwap = (max_pages - reserved_pages) * pageSize;  // Resulting units: bytes
        m_usedSwap = reserved_pages * pageSize;                // Resulting units: bytes

        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Swap Total (", m_totalSwap).append(L")"));
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Swap Available (", m_availableSwap).append(L")"));
        SCX_LOGTRACE(m_log, StrAppend(L"MemoryInstance::Update() - Swap Used (", m_usedSwap).append(L")"));

#elif defined(hpux)

        scxulong page_size = 0;
        scxulong physical_memory = 0;
        scxulong real_pages = 0;
        scxulong free_pages = 0;

        m_deps->GetStaticMemoryInfo(page_size, physical_memory);
        m_deps->GetDynamicMemoryInfo(real_pages, free_pages);

        m_totalPhysicalMemory = (physical_memory * page_size); // Resulting units: bytes
        m_usedMemory = (real_pages * page_size);               // Resulting units: bytes
        m_availableMemory = (free_pages * page_size);          // Resulting units: bytes

        // The reservedMemory size varies with a few MB up and down, so it's best to recompute 
        // this number every time so that the used and free percentages adds up.
        m_reservedMemory = m_totalPhysicalMemory - m_usedMemory - m_availableMemory;

        scxulong max_pages = 0;
        scxulong reserved_pages = 0;

        m_deps->GetSwapInfo(max_pages, reserved_pages);

        // totalSwap is the total size of all external swap devices plus swap memory, if enabled
        // availableSwap is the size of remaining device swap (with reserved memory subtracted)
        // plus remaining swap memory, if that was enabled in system configuration.
        // usedSwap is the difference between those. This is consistent with the 'total'
        // numbers when you do 'swapinfo -t'.
        m_totalSwap = max_pages * page_size;                   // Resulting units: bytes
        m_availableSwap = reserved_pages * page_size;          // Resulting units: bytes
        m_usedSwap = m_totalSwap - m_availableSwap;            // Resulting units: bytes

#elif defined(aix)

        scxulong total_pages = 0;
        scxulong free_pages = 0;
        scxulong max_swap_pages = 0;
        scxulong free_swap_pages = 0;

        m_deps->GetMemInfo(total_pages, free_pages, max_swap_pages, free_swap_pages);

        // All memory data given in bytes

        m_totalPhysicalMemory = (total_pages * 4) * 1024;      // Resulting units: bytes
        m_availableMemory = (free_pages * 4) * 1024;           // Resulting units: bytes
        m_usedMemory = m_totalPhysicalMemory - m_availableMemory;   // Resulting units: bytes

        m_totalSwap = (max_swap_pages * 4) * 1024;             // Resulting units: bytes
        m_availableSwap = (free_swap_pages * 4) * 1024;        // Resulting units: bytes
        m_usedSwap = m_totalSwap - m_availableSwap;

#else
#error "Not implemented for this platform."
#endif

    }

    /*----------------------------------------------------------------------------*/
    /**
        Clean up the instance. Closes the thread.

    */
    void MemoryInstance::CleanUp()
    {
        SCX_LOGTRACE(m_log, L"MemoryInstance CleanUp()");
        if (NULL != m_dataAquisitionThread) {
        m_dataAquisitionThread->RequestTerminate();
        m_dataAquisitionThread->Wait();
            m_dataAquisitionThread = NULL;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns     The object represented as a string suitable for logging.

    */
    const std::wstring MemoryInstance::DumpString() const
    {
        std::wstringstream ss;
        ss << L"MemoryInstance: totalPhysMem = " << m_totalPhysicalMemory
           << L", availableMem = " << m_availableMemory
           << L", usedMem = " << m_usedMemory
           << L", pageReads = " << m_pageReads.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES) / MEMORY_SECONDS_PER_SAMPLE
           << L", pageWrites = " << m_pageWrites.GetAverageDelta(MAX_MEMINSTANCE_DATASAMPER_SAMPLES) / MEMORY_SECONDS_PER_SAMPLE
           << L", totalSwap = " << m_totalSwap
           << L", availableSwap = " << m_availableSwap
           << L", usedSwap = " << m_usedSwap;
        return ss.str();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Utility function to retrieve the page reads and page writes since boot
        from the system.

        \param[out]  pageReads   Number of page reads since boot.
        \param[out]  pageWrites  Number of page writes since boot.
        \param[in]   deps        Dependencies for the Memory data colletion.

        \returns     true if the values are supported by this implementation

    */
    bool MemoryInstance::GetPagingSinceBoot(scxulong& pageReads, scxulong& pageWrites, MemoryInstance* inst, SCXCoreLib::SCXHandle<MemoryDependencies> deps)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
        SCX_LOGHYSTERICAL(log, L"MemoryInstance::GetPagingSinceBoot()");

        if (NULL == inst)
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"inst", L"MemoryInstance instance parameter to GetPagingSinceBoot() is NULL", SCXSRCLOCATION);
        }

#if defined(linux)

        /**
           Get pageReads and pageWrites from values in /proc/vmstat.

           Example /proc/vmstat:
           nr_dirty 134
           nr_writeback 0
           nr_unstable 0
           nr_page_table_pages 432
           nr_mapped 24087
           nr_slab 5073
           pgpgin 1467244
           pgpgout 7330560
           pswpin 34123
           pswpout 248659
           pgalloc_high 0
           pgalloc_normal 119867240
           pgalloc_dma32 0
           pgalloc_dma 138137
           pgfree 120028486
           pgactivate 1852759
           pgdeactivate 2003853
           pgfault 261199917
           pgmajfault 9456
           pgrefill_high 0
           pgrefill_normal 3227693
           pgrefill_dma32 0
           pgrefill_dma 94902
           pgsteal_high 0
           pgsteal_normal 601920
           pgsteal_dma32 0
           pgsteal_dma 14627
           pgscan_kswapd_high 0
           pgscan_kswapd_normal 683001
           pgscan_kswapd_dma32 0
           pgscan_kswapd_dma 17469
           pgscan_direct_high 0
           pgscan_direct_normal 1641420
           pgscan_direct_dma32 0
           pgscan_direct_dma 44062
           pginodesteal 432
           slabs_scanned 232960
           kswapd_steal 412728
           kswapd_inodesteal 29634
           pageoutrun 8016
           allocstall 2488
           pgrotated 248502
           nr_bounce 0

           We are interested in the following fields:
           pgpgin
           pgpgout

        */
        try 
        {
            std::vector<std::wstring> lines = deps->GetVMStatLines();

            bool foundPgpgin = false;
            bool foundPgpgout = false;
    
            for (size_t i=0; (!foundPgpgin || !foundPgpgout) && i<lines.size(); i++)
            {
                std::wstring line = lines[i];

                SCX_LOGHYSTERICAL(log, std::wstring(L"DataAquisitionThreadBody() - Read line: ").append(line));
    
                std::vector<std::wstring> tokens;
                StrTokenize(line, tokens);
                if (tokens.size() >= 2)
                {
                    if (L"pgpgin" == tokens[0])
                    {
                        try
                        {
                            pageReads = StrToULong(tokens[1]);
                            foundPgpgin = true;
                            SCX_LOGHYSTERICAL(log, StrAppend(L"    pageReads = ", pageReads));
                        }
                        catch (const SCXNotSupportedException& e)
                        {
                            SCX_LOGWARNING(log, std::wstring(L"Could not read pageReads from: ").append(line).append(L" - ").append(e.What()));
                        }
                    }
                    if (L"pgpgout" == tokens[0])
                    {
                        try
                        {
                            pageWrites = StrToULong(tokens[1]);
                            foundPgpgout = true;
                            SCX_LOGHYSTERICAL(log, StrAppend(L"    pageWrites = ", pageWrites));
                        }
                        catch (const SCXNotSupportedException& e)
                        {
                            SCX_LOGWARNING(log, std::wstring(L"Could not read pageWrites from: ").append(line).append(L" - ").append(e.What()));
                        }
                    }
                }
            }
            SCXASSERT(foundPgpgin && "pgpgin not found.");
            SCXASSERT(foundPgpgout && "pgpgout not found.");
        } 
        catch (SCXFileSystemException &e) 
        {
            SCX_LOGERROR(log, std::wstring(L"Could not open /proc/vmstat for reading: ").append(e.What()));
            return false;            
        }

#elif defined(sun)

        pageReads = 0;
        pageWrites = 0;
        SCXThreadLock lock(inst->GetKstatLockHandle());
 
        SCXHandle<SCXSystemLib::SCXKstat> kstat = inst->GetKstat();
        kstat->Update();

        std::vector<int> cpuInstances;
        for (kstat_t* cur = kstat->ResetInternalIterator(); cur; cur = kstat->AdvanceInternalIterator())
        {
            if (strcmp(cur->ks_module, "cpu_info") != 0 || cur->ks_type != KSTAT_TYPE_NAMED )
                continue;

            // Get the instance number of this CPU
            int cpuInstance = cur->ks_instance;

            // Is this CPU online and present?
            if (!deps->IsProcessorPresent(cpuInstance)) {
                continue;
            }

            cpuInstances.push_back(cpuInstance);
        }

        for (std::vector<int>::const_iterator it = cpuInstances.begin(); it != cpuInstances.end(); ++it)
        {
            try {
                std::wostringstream id;
                id << L"cpu_stat" << *it;
                kstat->Lookup(L"cpu_stat", id.str(), *it);

                const cpu_stat *cpu_stat_p = 0;
                kstat->GetValueRaw(cpu_stat_p);

                pageReads  += cpu_stat_p->cpu_vminfo.pgpgin;
                pageWrites += cpu_stat_p->cpu_vminfo.pgpgout;

            } catch (const SCXKstatException& e) {
                SCX_LOGWARNING(log, std::wstring(L"Kstat failed for memory: ").append(e.What()));
                return false;
            }
        }

#elif defined(hpux) || defined(aix)

        if (!deps->GetPageingData(pageReads, pageWrites))
        {
            return false;
        }

#else
#error "Not implemented on this platform"
#endif

        return true;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Thread body that updates values that are time dependent.

        \param param Must contain a parameter named "ParamValues" of type MemoryInstanceThreadParam*

        The thread updates all members that are time dependent. Like for example
        page reads per second.

    */
    void MemoryInstance::DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.memory.memoryinstance");
        SCX_LOGTRACE(log, L"MemoryInstance::DataAquisitionThreadBody()");

        if (0 == param.GetData())
        {
            SCXASSERT( ! "No parameters to DataAquisitionThreadBody");
            return;
        }
        MemoryInstanceThreadParam* params = static_cast<MemoryInstanceThreadParam*>(param.GetData());

        if (0 == params)
        {
            SCXASSERT( ! "Parameters to DagaAquisitionThreadBody not instance of MemoryInstanceThreadParam");
            return;
        }

        MemoryInstanceDataSampler* pageReadsParam = params->GetPageReads();
        if (0 == pageReadsParam)
        {
            SCXASSERT( ! "PageReads not set");
            return;
        }
        MemoryInstanceDataSampler* pageWritesParam = params->GetPageWrites();
        if (0 == pageWritesParam)
        {
            SCXASSERT( ! "PageWrites not set");
            return;
        }

        SCXCoreLib::SCXHandle<MemoryDependencies> deps = params->GetDeps();

        bool bUpdate = true;
        params->m_cond.SetSleep(MEMORY_SECONDS_PER_SAMPLE * 1000);
        {
            SCXConditionHandle h(params->m_cond);
            while ( ! params->GetTerminateFlag())
            {
                if (bUpdate)
                {
                    scxulong pageReads = 0;
                    scxulong pageWrites = 0;

                    if ( ! GetPagingSinceBoot(pageReads, pageWrites, params->GetInst(), deps))
                    {
                        return;
                    }

                    pageReadsParam->AddSample(pageReads);
                    pageWritesParam->AddSample(pageWrites);
                    bUpdate = false;
                }

                enum SCXCondition::eConditionResult r = h.Wait();
                if (SCXCondition::eCondTimeout == r)
                {
                    bUpdate = true;
                }
            }
        }
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

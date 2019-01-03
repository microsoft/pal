/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2007-07-02 12:49:28

  Memory data colletion test class.

  This class tests the Linux, Solaris, and HP/UX implementations.

  It compares a 10 second average against top output to a margin of 5 units

  A longer period will give a smaller margin, but will also make the tests
  take longer time to run. 10 seconds seemes to give a good enough error
  margin for these tests.
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>

#include <scxsystemlib/memoryinstance.h>

#include <cppunit/extensions/HelperMacros.h>

#if defined(WIN32)
# include <windows.h>
#elif defined(Linux) || defined(sun) || defined(hpux)
# include <unistd.h>
# include <pthread.h>
#endif

#if defined(hpux)
# include <sys/pstat.h>
#endif

#if defined(sun)
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

#include <errno.h>
#include <math.h>
#include <iomanip>

using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;

const scxulong c_pageSize = 4096;

#if defined(sun)

// For test purposes, lets create a 64MB cache;
// This affects available memory, so adjust remaining figures
const scxulong c_zfsCacheSize = 64 * 1024 * 1024;

// ZFS isn't supported on Sparc 9
#if (PF_MAJOR == 5 && PF_MINOR >= 10) || (PF_MAJOR > 5)
// Following is in MB
const scxulong c_zfsMemoryAdj = c_zfsCacheSize / 1024 / 1024;
#else
const scxulong c_zfsMemoryAdj = 0;
#endif

#else

const scxulong c_zfsMemoryAdj = 0;

#endif // defined(sun)

const scxulong c_totalPhysicalMemory = 512;
const scxulong c_availableMemory = 128;
const scxulong c_usedMemory = c_totalPhysicalMemory - c_availableMemory;

const scxulong c_totalSwap = 256;
const scxulong c_availableSwap = 64;
const scxulong c_usedSwap = c_totalSwap - c_availableSwap;

const scxulong c_totalPageReads = 4000;
const scxulong c_totalPageWrites = 8000;

class TestMemoryDependencies : public MemoryDependencies
{
public:

    TestMemoryDependencies() {};
    virtual ~TestMemoryDependencies() {};

#if defined(linux)

    std::vector<wstring> GetMemInfoLines()
    {
        std::vector<wstring> meminfo;

        /*
          We are interested in the following fields:

          MemTotal
          MemFree
          SwapTotal
          SwapFree
        */

        meminfo.push_back(StrAppend(StrAppend(L"MemTotal:       ", c_totalPhysicalMemory * 1024), L"kB"));
        meminfo.push_back(StrAppend(StrAppend(L"MemFree:        ", c_availableMemory * 1024), L"kB"));
        meminfo.push_back(L"Buffers:             0 kB");
        meminfo.push_back(L"Cached:              0 kB");
        meminfo.push_back(L"SwapCached:          0 kB");
        meminfo.push_back(L"Active:              0 kB");
        meminfo.push_back(L"Inactive:            0 kB");
        meminfo.push_back(L"HighTotal:           0 kB");
        meminfo.push_back(L"HighFree:            0 kB");
        meminfo.push_back(L"LowTotal:            0 kB");
        meminfo.push_back(L"LowFree:             0 kB");
        meminfo.push_back(StrAppend(StrAppend(L"SwapTotal:      ", c_totalSwap * 1024), L"kB"));
        meminfo.push_back(StrAppend(StrAppend(L"SwapFree:        ", c_availableSwap * 1024), L"kB"));
        meminfo.push_back(L"Dirty:               0 kB");
        meminfo.push_back(L"Writeback:           0 kB");
        meminfo.push_back(L"Mapped:              0 kB");
        meminfo.push_back(L"Slab:                0 kB");
        meminfo.push_back(L"CommitLimit:         0 kB");
        meminfo.push_back(L"Committed_AS:        0 kB");
        meminfo.push_back(L"PageTables:          0 kB");
        meminfo.push_back(L"VmallocTotal:        0 kB");
        meminfo.push_back(L"VmallocUsed:         0 kB");
        meminfo.push_back(L"VmallocChunk:        0 kB");
        meminfo.push_back(L"HugePages_Total:     0");
        meminfo.push_back(L"HugePages_Free:      0");
        meminfo.push_back(L"HugePages_Rsvd:      0");
        meminfo.push_back(L"Hugepagesize:        0 kB");

        return meminfo;
    }

    std::vector<wstring> GetVMStatLines()
    {
        std::vector<wstring> vmstat;

        /*
          We are interested in the following fields:

          pgpgin
          pgpgout
        */

        vmstat.push_back(L"nr_dirty 0");
        vmstat.push_back(L"nr_writeback 0");
        vmstat.push_back(L"nr_unstable 0");
        vmstat.push_back(L"nr_page_table_pages 0");
        vmstat.push_back(L"nr_mapped 0");
        vmstat.push_back(L"nr_slab 0");
        vmstat.push_back(StrAppend(L"pgpgin ", c_totalPageReads));
        vmstat.push_back(StrAppend(L"pgpgout ", c_totalPageWrites));
        vmstat.push_back(L"pswpin 0");
        vmstat.push_back(L"pswpout 0");
        vmstat.push_back(L"pgalloc_high 0");
        vmstat.push_back(L"pgalloc_normal 0");
        vmstat.push_back(L"pgalloc_dma32 0");
        vmstat.push_back(L"pgalloc_dma 0");
        vmstat.push_back(L"pgfree 0");
        vmstat.push_back(L"pgactivate 0");
        vmstat.push_back(L"pgdeactivate 0");
        vmstat.push_back(L"pgfault 0");
        vmstat.push_back(L"pgmajfault 0");
        vmstat.push_back(L"pgrefill_high 0");
        vmstat.push_back(L"pgrefill_normal 0");
        vmstat.push_back(L"pgrefill_dma32 0");
        vmstat.push_back(L"pgrefill_dma 0");
        vmstat.push_back(L"pgsteal_high 0");
        vmstat.push_back(L"pgsteal_normal 0");
        vmstat.push_back(L"pgsteal_dma32 0");
        vmstat.push_back(L"pgsteal_dma 0");
        vmstat.push_back(L"pgscan_kswapd_high 0");
        vmstat.push_back(L"pgscan_kswapd_normal 0");
        vmstat.push_back(L"pgscan_kswapd_dma32 0");
        vmstat.push_back(L"pgscan_kswapd_dma 0");
        vmstat.push_back(L"pgscan_direct_high 0");
        vmstat.push_back(L"pgscan_direct_normal 0");
        vmstat.push_back(L"pgscan_direct_dma32 0");
        vmstat.push_back(L"pgscan_direct_dma 0");
        vmstat.push_back(L"pginodesteal 0");
        vmstat.push_back(L"slabs_scanned 0");
        vmstat.push_back(L"kswapd_steal 0");
        vmstat.push_back(L"kswapd_inodesteal 0");
        vmstat.push_back(L"pageoutrun 0");
        vmstat.push_back(L"allocstall 0");
        vmstat.push_back(L"pgrotated 0");
        vmstat.push_back(L"nr_bounce 0");

        return vmstat;
    }

#elif defined(sun)

    class MockKstat : public SCXKstat
    {
    public:
        MockKstat() : SCXKstat(), m_instance(0), m_bCpuReturned(false)
        {
        }

        void Lookup(const wstring& module, const wstring& name, int instance = -1)
        {
            SCXKstat::Lookup(module, name, instance);
            m_instance = instance;
            mock_statistics.cpu_vminfo.pgpgin = c_totalPageReads;
            mock_statistics.cpu_vminfo.pgpgout = c_totalPageWrites;
        }
        void Lookup(const wstring& module, int instance = -1)
        {
            // Never called, so just bomb out
            CPPUNIT_ASSERT_MESSAGE("Lookup() without name parameter not implemented", false);
        }
        void Lookup(const char *module, const char *name, int instance = -1)
        {
            // We need to return only one CPU instance
            if ( strcasecmp(module, "cpu_stat") || false == m_bCpuReturned )
            {
                if ( 0 == strcasecmp(module, "cpu_stat") )
                    m_bCpuReturned = true;

                SCXKstat::Lookup(module, name, instance);
            }
            else
            {
                throw SCXKstatNotFoundException(L"kstat_lookup() could not find kstat",
                                                ENOENT,
                                                SCXCoreLib::StrFromUTF8(module),
                                                instance,
                                                ( name != NULL ? SCXCoreLib::StrFromUTF8(name) : L""),
                                                SCXSRCLOCATION);
            }
        }

        scxulong GetValue(const wstring& statistic) const
        {
            if (m_instance != 0)
            {
                return 0;
            }

            if (statistic == L"pgpgin")
            {
                return c_totalPageReads;
            }
            if (statistic == L"pgpgout")
            {
                return c_totalPageWrites;
            }

            return 0;
        }

        bool TryGetValue(const std::wstring& statistic, scxulong& value) const
        {
            if (m_instance != 0)
            {
                return false;
            }

            if (statistic == L"size")
            {
                value = c_zfsCacheSize;
                return true;
            }

            return false;
        }

        void* GetExternalDataPointer() { return &mock_statistics; }

    private:
        int m_instance;
        bool m_bCpuReturned;
        cpu_stat_t mock_statistics;
    };

    scxulong GetPageSize()
    {
        return c_pageSize;
    }

    scxulong GetPhysicalPages()
    {
        return c_totalPhysicalMemory * 1024 * 1024 / c_pageSize;
    }

    scxulong GetAvailablePhysicalPages()
    {
        return c_availableMemory * 1024 * 1024 / c_pageSize;
    }

    long GetNumberOfCPUs()
    {
        return 1;
    }

    void GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages)
    {
        max_pages = c_totalSwap * 1024 * 1024 / c_pageSize ;
        reserved_pages = c_usedSwap * 1024 * 1024 / c_pageSize;
    }

    SCXCoreLib::SCXHandle<SCXKstat> CreateKstat()
    {
        return SCXCoreLib::SCXHandle<SCXKstat>(new MockKstat());
    }

#elif defined(hpux)

    void GetStaticMemoryInfo(scxulong& page_size, scxulong& physical_memory)
    {
        page_size = c_pageSize;
        physical_memory = c_totalPhysicalMemory * 1024 * 1024 / c_pageSize;
    }

    void GetDynamicMemoryInfo(scxulong& real_pages, scxulong& free_pages)
    {
        real_pages = c_usedMemory * 1024 * 1024 / c_pageSize;
        free_pages = c_availableMemory * 1024 * 1024 / c_pageSize;
    }

    void GetSwapInfo(scxulong& max_pages, scxulong& reserved_pages)
    {
        max_pages = c_totalSwap * 1024 * 1024 / c_pageSize;
        reserved_pages = c_availableSwap * 1024 * 1024 / c_pageSize;
    }

    bool GetPageingData(scxulong& reads, scxulong& writes)
    {
        reads = c_totalPageReads;
        writes = c_totalPageWrites;

        return true;
    }

#elif defined(aix)

    // AIX page size is always 4KB
    void GetMemInfo(scxulong& total_pages, scxulong& free_pages, scxulong& max_swap_pages, scxulong& free_swap_pages)
    {
        total_pages = c_totalPhysicalMemory * 1024 * 1024 / 4096;
        free_pages = c_availableMemory * 1024 * 1024 / 4096;
        max_swap_pages = c_totalSwap * 1024 * 1024 / 4096;
        free_swap_pages = c_availableSwap * 1024 * 1024 / 4096;
    }

    bool GetPageingData(scxulong& reads, scxulong& writes)
    {
        reads = c_totalPageReads;
        writes = c_totalPageWrites;

        return true;
    }

#endif

};


// Testable class to verify proper behavior when reading /proc/meminfo file on Linux
class TestableMemoryInstance : public MemoryInstance
{
public:
    TestableMemoryInstance(SCXCoreLib::SCXHandle<MemoryDependencies> deps = SCXCoreLib::SCXHandle<MemoryDependencies>(new MemoryDependencies()), bool startThread = true)
        : MemoryInstance(deps, startThread)
    {}

    void VerifyMeminfoFileReadProperly()
    {
#if defined(linux)
        CPPUNIT_ASSERT_EQUAL(static_cast<bool>(true), m_foundTotalPhysMem);
        CPPUNIT_ASSERT_EQUAL(static_cast<bool>(true), m_foundAvailMem);
        CPPUNIT_ASSERT_EQUAL(static_cast<bool>(true), m_foundTotalSwap);
        CPPUNIT_ASSERT_EQUAL(static_cast<bool>(true), m_foundAvailSwap);
#endif
    }
};


#if defined(linux)
class TestWI11691MemoryDependencies : public MemoryDependencies
{
    scxulong  m_totalMemory,
        m_freeMemory,
        m_buffers,
        m_cached;
public:
    TestWI11691MemoryDependencies( scxulong  totalMemory, scxulong freeMemory,
            scxulong buffers, scxulong cached ) :
            m_totalMemory(totalMemory),
            m_freeMemory(freeMemory),
            m_buffers(buffers),
            m_cached(cached) {}

    virtual ~TestWI11691MemoryDependencies() {}

    std::vector<wstring> GetMemInfoLines()
    {
        std::vector<wstring> meminfo;

        /*
          We are interested in the following fields:

          MemTotal
          MemFree
          Buffers
          Cached
        */

        meminfo.push_back(StrAppend(StrAppend(L"MemTotal:       ", m_totalMemory * 1024), L"kB"));
        meminfo.push_back(StrAppend(StrAppend(L"MemFree:        ", m_freeMemory * 1024), L"kB"));
        meminfo.push_back(StrAppend(StrAppend(L"Buffers:        ", m_buffers * 1024), L"kB"));
        meminfo.push_back(StrAppend(StrAppend(L"Cached:         ", m_cached * 1024), L"kB"));
        meminfo.push_back(L"SwapCached:          0 kB");
        meminfo.push_back(L"Active:              0 kB");
        meminfo.push_back(L"Inactive:            0 kB");
        meminfo.push_back(L"HighTotal:           0 kB");
        meminfo.push_back(L"HighFree:            0 kB");
        meminfo.push_back(L"LowTotal:            0 kB");
        meminfo.push_back(L"LowFree:             0 kB");
        meminfo.push_back(StrAppend(StrAppend(L"SwapTotal:      ", c_totalSwap * 1024), L"kB"));
        meminfo.push_back(StrAppend(StrAppend(L"SwapFree:        ", c_availableSwap * 1024), L"kB"));
        meminfo.push_back(L"Dirty:               0 kB");
        meminfo.push_back(L"Writeback:           0 kB");
        meminfo.push_back(L"Mapped:              0 kB");
        meminfo.push_back(L"Slab:                0 kB");
        meminfo.push_back(L"CommitLimit:         0 kB");
        meminfo.push_back(L"Committed_AS:        0 kB");
        meminfo.push_back(L"PageTables:          0 kB");
        meminfo.push_back(L"VmallocTotal:        0 kB");
        meminfo.push_back(L"VmallocUsed:         0 kB");
        meminfo.push_back(L"VmallocChunk:        0 kB");
        meminfo.push_back(L"HugePages_Total:     0");
        meminfo.push_back(L"HugePages_Free:      0");
        meminfo.push_back(L"HugePages_Rsvd:      0");
        meminfo.push_back(L"Hugepagesize:        0 kB");

        return meminfo;
    }

    std::vector<wstring> GetVMStatLines()
    {
        std::vector<wstring> vmstat;

        vmstat.push_back(L"nr_dirty 0");
        vmstat.push_back(L"nr_writeback 0");
        vmstat.push_back(L"nr_unstable 0");
        vmstat.push_back(L"nr_page_table_pages 0");
        vmstat.push_back(L"nr_mapped 0");
        vmstat.push_back(L"nr_slab 0");
        vmstat.push_back(StrAppend(L"pgpgin ", c_totalPageReads));
        vmstat.push_back(StrAppend(L"pgpgout ", c_totalPageWrites));
        vmstat.push_back(L"pswpin 0");
        vmstat.push_back(L"pswpout 0");
        vmstat.push_back(L"pgalloc_high 0");
        vmstat.push_back(L"pgalloc_normal 0");
        vmstat.push_back(L"pgalloc_dma32 0");
        vmstat.push_back(L"pgalloc_dma 0");
        vmstat.push_back(L"pgfree 0");
        vmstat.push_back(L"pgactivate 0");
        vmstat.push_back(L"pgdeactivate 0");
        vmstat.push_back(L"pgfault 0");
        vmstat.push_back(L"pgmajfault 0");
        vmstat.push_back(L"pgrefill_high 0");
        vmstat.push_back(L"pgrefill_normal 0");
        vmstat.push_back(L"pgrefill_dma32 0");
        vmstat.push_back(L"pgrefill_dma 0");
        vmstat.push_back(L"pgsteal_high 0");
        vmstat.push_back(L"pgsteal_normal 0");
        vmstat.push_back(L"pgsteal_dma32 0");
        vmstat.push_back(L"pgsteal_dma 0");
        vmstat.push_back(L"pgscan_kswapd_high 0");
        vmstat.push_back(L"pgscan_kswapd_normal 0");
        vmstat.push_back(L"pgscan_kswapd_dma32 0");
        vmstat.push_back(L"pgscan_kswapd_dma 0");
        vmstat.push_back(L"pgscan_direct_high 0");
        vmstat.push_back(L"pgscan_direct_normal 0");
        vmstat.push_back(L"pgscan_direct_dma32 0");
        vmstat.push_back(L"pgscan_direct_dma 0");
        vmstat.push_back(L"pginodesteal 0");
        vmstat.push_back(L"slabs_scanned 0");
        vmstat.push_back(L"kswapd_steal 0");
        vmstat.push_back(L"kswapd_inodesteal 0");
        vmstat.push_back(L"pageoutrun 0");
        vmstat.push_back(L"allocstall 0");
        vmstat.push_back(L"pgrotated 0");
        vmstat.push_back(L"nr_bounce 0");

        return vmstat;
    }

};
#endif  //defined(linux)

#if defined(linux)
class TestMemAvailableMemoryDependencies : public MemoryDependencies
{

public:
    TestMemAvailableMemoryDependencies() {}

    virtual ~TestMemAvailableMemoryDependencies() {}

    std::vector<wstring> GetMemInfoLines()
    {
        std::vector<wstring> meminfo;

        /*
          We are interested in the following fields:

          MemTotal
          MemAvailable
        */

        meminfo.push_back(L"MemTotal:        3522864 kB");
        meminfo.push_back(L"MemFree:          175844 kB");
        meminfo.push_back(L"MemAvailable:    2813432 kB");
        meminfo.push_back(L"Buffers:              36 kB");
        meminfo.push_back(L"Cached:           289148 kB");
        meminfo.push_back(L"SwapCached:          332 kB");
        meminfo.push_back(L"Active:           234424 kB");
        meminfo.push_back(L"Inactive:         250828 kB");
        meminfo.push_back(L"Active(anon):      94120 kB");
        meminfo.push_back(L"Inactive(anon):   203372 kB");
        meminfo.push_back(L"Active(file):     140304 kB");
        meminfo.push_back(L"Inactive(file):    47456 kB");
        meminfo.push_back(L"Unevictable:           0 kB");
        meminfo.push_back(L"Mlocked:               0 kB");
        meminfo.push_back(L"SwapTotal:       6655996 kB");
        meminfo.push_back(L"SwapFree:        6638924 kB");
        meminfo.push_back(L"Dirty:                 8 kB");
        meminfo.push_back(L"Writeback:             0 kB");
        meminfo.push_back(L"AnonPages:        195772 kB");
        meminfo.push_back(L"Mapped:            28868 kB");
        meminfo.push_back(L"Shmem:            101424 kB");
        meminfo.push_back(L"Slab:            2765564 kB");
        meminfo.push_back(L"SReclaimable:    2745856 kB");
        meminfo.push_back(L"SUnreclaim:        19708 kB");
        meminfo.push_back(L"KernelStack:        4224 kB");
        meminfo.push_back(L"PageTables:         6588 kB");
        meminfo.push_back(L"NFS_Unstable:          0 kB");
        meminfo.push_back(L"Bounce:                0 kB");
        meminfo.push_back(L"WritebackTmp:          0 kB");
        meminfo.push_back(L"CommitLimit:     8417428 kB");
        meminfo.push_back(L"Committed_AS:    1004816 kB");
        meminfo.push_back(L"VmallocTotal:   34359738367 kB");
        meminfo.push_back(L"VmallocUsed:       67624 kB");
        meminfo.push_back(L"VmallocChunk:   34359663604 kB");
        meminfo.push_back(L"HardwareCorrupted:     0 kB");
        meminfo.push_back(L"AnonHugePages:     75776 kB");
        meminfo.push_back(L"HugePages_Total:       0");
        meminfo.push_back(L"HugePages_Free:        0");
        meminfo.push_back(L"HugePages_Rsvd:        0");
        meminfo.push_back(L"HugePages_Surp:        0");
        meminfo.push_back(L"Hugepagesize:       2048 kB");
        meminfo.push_back(L"DirectMap4k:       94144 kB");
        meminfo.push_back(L"DirectMap2M:     3575808 kB");

        return meminfo;
    }

};
#endif  //defined(linux)

class MemoryInstance_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( MemoryInstance_Test  );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( testAllMembers );
    CPPUNIT_TEST( testAllMembersDepInj );
    CPPUNIT_TEST( testAvailablemem_wi11691 );
    CPPUNIT_TEST( testMemAvailable );
    SCXUNIT_TEST_ATTRIBUTE( testAllMembers, SLOW);
    CPPUNIT_TEST_SUITE_END();

    public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void callDumpStringForCoverage()
    {
        SCXCoreLib::SCXHandle<MemoryInstance> memInstance( new MemoryInstance() );
        CPPUNIT_ASSERT(memInstance->DumpString().find(L"MemoryInstance") != wstring::npos);
    }

    void testAllMembersDepInj()
    {
        SCXCoreLib::SCXHandle<TestableMemoryInstance> memInstance( new TestableMemoryInstance(
            SCXCoreLib::SCXHandle<MemoryDependencies>(new TestMemoryDependencies()), false ));

        memInstance->Update();

        scxulong totalPhysicalMemory = 0;
        scxulong availableMemory = 0;
        scxulong usedMemory = 0;
        scxulong totalSwap = 0;
        scxulong availableSwap = 0;
        scxulong usedSwap = 0;
        scxulong totalPageReads = 0;
        scxulong totalPageWrites = 0;

        memInstance->GetTotalPhysicalMemory(totalPhysicalMemory);
        memInstance->GetAvailableMemory(availableMemory);
        memInstance->GetUsedMemory(usedMemory);
        memInstance->GetTotalSwap(totalSwap);
        memInstance->GetAvailableSwap(availableSwap);
        memInstance->GetUsedSwap(usedSwap);
        MemoryInstance::GetPagingSinceBoot(totalPageReads, totalPageWrites, memInstance.GetData(), SCXCoreLib::SCXHandle<MemoryDependencies>(new TestMemoryDependencies()));

        // std::cout << endl << endl << "totalPhysicalMemory = " << totalPhysicalMemory << endl;
        // std::cout << "totalSwap = " << totalSwap << endl;
        // std::cout << "availableMemory = " << availableMemory << endl;
        // std::cout << "usedMemory = " << usedMemory << endl;
        // std::cout << "availableSwap = " << availableSwap << endl;
        // std::cout << "usedSwap = " << usedSwap << endl;
        // std::cout << "totalPageReads = " << totalPageReads << endl;
        // std::cout << "totalPageWrites = " << totalPageWrites << endl << endl;

        CPPUNIT_ASSERT_EQUAL( c_totalPhysicalMemory, totalPhysicalMemory/(1024*1024) );
        CPPUNIT_ASSERT_EQUAL( c_totalSwap, totalSwap/(1024*1024) );
        CPPUNIT_ASSERT_EQUAL( c_availableMemory + c_zfsMemoryAdj, availableMemory/(1024*1024) );
        CPPUNIT_ASSERT_EQUAL( c_usedMemory - c_zfsMemoryAdj, usedMemory/(1024*1024) );
        CPPUNIT_ASSERT_EQUAL( c_availableSwap, availableSwap/(1024*1024) );
        CPPUNIT_ASSERT_EQUAL( c_usedSwap, usedSwap/(1024*1024) );
        CPPUNIT_ASSERT_EQUAL( c_totalPageReads, totalPageReads );
        CPPUNIT_ASSERT_EQUAL( c_totalPageWrites, totalPageWrites );

        memInstance->VerifyMeminfoFileReadProperly();

        memInstance->CleanUp();
    }

    void testAllMembers()
    {

        std::map<std::string, scxulong> keyValuesBefore;
        std::map<std::string, scxulong> keyValuesAfter;

        GetPagingData(keyValuesBefore);

        SCXCoreLib::SCXHandle<TestableMemoryInstance> memInstance( new TestableMemoryInstance(SCXCoreLib::SCXHandle<MemoryDependencies>(new MemoryDependencies()), false ));
        memInstance->Update();

        // Values from MemoryInstance.
        scxulong totalPhysicalMemory;     memInstance->GetTotalPhysicalMemory(totalPhysicalMemory);
        scxulong availableMemory; memInstance->GetAvailableMemory(availableMemory);
        scxulong usedMemory;      memInstance->GetUsedMemory(usedMemory);
        scxulong totalSwap;       memInstance->GetTotalSwap(totalSwap);
        scxulong availableSwap;   memInstance->GetAvailableSwap(availableSwap);
        scxulong usedSwap;        memInstance->GetUsedSwap(usedSwap);
        scxulong totalPageReads;
        scxulong totalPageWrites;
        CPPUNIT_ASSERT( true == memInstance->GetPagingSinceBoot(totalPageReads, totalPageWrites, memInstance.GetData()) );

#if defined(sun)
        GetSolarisAvailableMem(keyValuesAfter);
        GetSolarisPRTConfData(keyValuesAfter);
        GetSolarisSwapData(keyValuesAfter);
        keyValuesAfter["UsedMemory"] = keyValuesAfter["TotalMemory"] - keyValuesAfter["AvailableMemory"];
#elif defined(linux)
        GetLinuxTopData(keyValuesAfter);
#elif defined(hpux)
        GetHPUXPstatData(keyValuesAfter);       // TotalMemory
        GetHPUXTopData(keyValuesAfter);         // AvailableMemory, UsedMemory
        GetHPUXSwapinfoData(keyValuesAfter);    // TotalSwap, AvailableSwap, UsedSwap
#elif defined(aix)
        GetAIXData(keyValuesAfter);
#endif

        GetPagingData(keyValuesAfter);

        // Compare the values.
        CPPUNIT_ASSERT_DOUBLES_EQUAL(static_cast<double>(keyValuesAfter["TotalMemory"]), static_cast<double>(totalPhysicalMemory/1024), 1024);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(static_cast<double>(keyValuesAfter["TotalSwap"]), static_cast<double>(totalSwap/1024), 1024);
        SCXUNIT_ASSERT_BOTH_AT_OR_BOTH_ABOVE(availableMemory/1024, keyValuesAfter["AvailableMemory"], 0);
        SCXUNIT_ASSERT_BOTH_AT_OR_BOTH_ABOVE(usedMemory/1024,      keyValuesAfter["UsedMemory"], 0);
        SCXUNIT_ASSERT_BOTH_AT_OR_BOTH_ABOVE(availableSwap/1024,   keyValuesAfter["AvailableSwap"], 0);
#if defined(aix)
        SCXUNIT_ASSERT_BOTH_AT_OR_BOTH_ABOVE(keyValuesAfter["UsedSwap"]?usedSwap/1024:(usedSwap*100)/totalSwap,  keyValuesAfter["UsedSwap"]?keyValuesAfter["UsedSwap"]:keyValuesAfter["UsedSwapPercentage"], 0);
#else
        SCXUNIT_ASSERT_BOTH_AT_OR_BOTH_ABOVE(usedSwap/1024,        keyValuesAfter["UsedSwap"], 0);
#endif
        CPPUNIT_ASSERT(keyValuesBefore["PageReads"] <= totalPageReads);
        CPPUNIT_ASSERT(totalPageReads <= keyValuesAfter["PageReads"]);
        CPPUNIT_ASSERT(keyValuesBefore["PageWrites"] <= totalPageWrites);
        CPPUNIT_ASSERT(totalPageWrites <= keyValuesAfter["PageWrites"]);

        memInstance->VerifyMeminfoFileReadProperly();

        memInstance->CleanUp();
    }

    void testAvailablemem_wi11691()
    {
#if defined(linux)
        // available memory on linux platforms should be calculated as
        // MemFree + Buffers + Cached
        const scxulong c_totalMemory = 512;
        const scxulong c_free = 26;
        const scxulong c_buffer = 214;
        const scxulong c_cached = 189;


        SCXCoreLib::SCXHandle<TestableMemoryInstance> memInstance( new TestableMemoryInstance(
            SCXCoreLib::SCXHandle<MemoryDependencies>(new TestWI11691MemoryDependencies(
            c_totalMemory, c_free, c_buffer, c_cached )) ) );

        memInstance->Update();

        scxulong availableMemory = 0;
        scxulong usedMemory = 0;

        memInstance->GetAvailableMemory(availableMemory);
        memInstance->GetUsedMemory(usedMemory);

        CPPUNIT_ASSERT( availableMemory/(1024*1024) == ( c_free + c_buffer + c_cached ));
        CPPUNIT_ASSERT( usedMemory/(1024*1024) == ( c_totalMemory - (c_free + c_buffer + c_cached) ));

        memInstance->VerifyMeminfoFileReadProperly();
#endif
    }

    void testMemAvailable()
    {
#if defined(linux)
        // available memory on linux platforms with Kernel 3.14+ should Taken from MemAvailable

        const scxulong c_totalMemory = 3522864;
        const scxulong c_memAvail = 2813432;


        SCXCoreLib::SCXHandle<TestableMemoryInstance> memInstance( new TestableMemoryInstance(SCXCoreLib::SCXHandle<MemoryDependencies>(new TestMemAvailableMemoryDependencies())));

        memInstance->Update();

        scxulong availableMemory = 0;
        scxulong usedMemory = 0;

        memInstance->GetAvailableMemory(availableMemory);
        memInstance->GetUsedMemory(usedMemory);

        CPPUNIT_ASSERT( availableMemory/1024 == ( c_memAvail ));
        CPPUNIT_ASSERT( usedMemory/1024 == ( c_totalMemory - (c_memAvail) ));

        memInstance->VerifyMeminfoFileReadProperly();
#endif
    }

    private:

    void GetLinuxTopData(std::map<std::string, scxulong>& keyValues)
    {
        char buf[256];

        FILE* topFile = popen("TERM=xterm top -b -n 1 | egrep \"Mem:|Mem :|Swap:\"", "r");
        if (topFile)
        {
            CPPUNIT_ASSERT(fgets(buf, 256, topFile) != NULL);
            std::string topOutput = buf;
            CPPUNIT_ASSERT(fgets(buf, 256, topFile) != NULL);
            topOutput.append(buf);

            pclose(topFile);

            std::vector<std::string> topTokens;
            Tokenize(topOutput, topTokens);

            // We may have "Mem:" or "Mem :", which affects the offset (RH vs. CentOS, strangely)
            size_t toffset = 0;
            if ( ":" == topTokens[2] )
            {
                toffset++;
            }

#if defined(ppc)
            /*
            ----------------------------------------------------------
            ----- Output from PowerPC Redhat 7 Linux systems: --------
            ----------------------------------------------------------
            KiB Mem :  1716736 total,   400064 free,   276288 used,  1040384 buff/cache
            KiB Swap:  2097088 total,  2097088 free,        0 used.  1212416 avail Mem
            */
            keyValues["TotalMemory"]     = ToSCXUlong(topTokens[2 + toffset]);
            keyValues["TotalSwap"]       = ToSCXUlong(topTokens[12 + toffset]);
            keyValues["AvailableMemory"] = ToSCXUlong(topTokens[4 + toffset]);
            keyValues["UsedMemory"]      = ToSCXUlong(topTokens[6 + toffset]);
            keyValues["AvailableSwap"]   = ToSCXUlong(topTokens[14 + toffset]);
            keyValues["UsedSwap"]        = ToSCXUlong(topTokens[16 + toffset]);
#else
            /*
            ----------------------------------------------------------
            ---------- Output from most Linux systems: ---------------
            ----------------------------------------------------------
            Mem:   2047248k total,  1164684k used,   882564k free,    11456k buffers
            Swap:  4128760k total,        0k used,  4128760k free,   589628k cached
            ----------------------------------------------------------
            ---------- Output from Redhat 7 systems: -----------------
            ----------------------------------------------------------
            KiB Mem:   2043048 total,  1781884 used,   261164 free,      108 buffers
            KiB Swap:  2113532 total,        8 used,  2113524 free.  1093844 cached Mem
            ----------------------------------------------------------
            */

            if ( (topTokens.size() >= 21) && (topTokens[0] == "KiB") )
            {
                keyValues["TotalMemory"]     = ToSCXUlong(topTokens[2 + toffset]);
                keyValues["AvailableMemory"] = ToSCXUlong(topTokens[6 + toffset]);
                keyValues["UsedMemory"]      = ToSCXUlong(topTokens[4 + toffset]);
                keyValues["TotalSwap"]       = ToSCXUlong(topTokens[12 + toffset]);
                keyValues["AvailableSwap"]   = ToSCXUlong(topTokens[16 + toffset]);
                keyValues["UsedSwap"]        = ToSCXUlong(topTokens[14 + toffset]);
            }
            else if (topTokens.size() >= 18)
            {
                keyValues["TotalMemory"]     = ToSCXUlong(topTokens[1]);
                keyValues["AvailableMemory"] = ToSCXUlong(topTokens[5]);
                keyValues["UsedMemory"]      = ToSCXUlong(topTokens[3]);
                keyValues["TotalSwap"]       = ToSCXUlong(topTokens[10]);
                keyValues["AvailableSwap"]   = ToSCXUlong(topTokens[14]);
                keyValues["UsedSwap"]        = ToSCXUlong(topTokens[12]);
            }
#endif
        }

        // We did look up the value for UsedMemory/UsedSwap above
        //
        // However, on Linux, the memory provider computes these values by
        // taking used=Total-Available, as this is all that's available in
        // the /proc/meminfo file.  We do the same here to avoid any
        // rounding problems ...

        keyValues["UsedMemory"] = keyValues["TotalMemory"] - keyValues["AvailableMemory"];
        keyValues["UsedSwap"] = keyValues["TotalSwap"] - keyValues["AvailableSwap"];
    }

    void GetAIXData(std::map<std::string, scxulong>& keyValues)
    {
        FILE* file = popen("vmstat -vs", "r");
        if (file)
        {
            char buf[256];

            for (char *o = fgets(buf, 256, file); o != NULL; o = fgets(buf, 256, file))
            {
                std::string output = buf;

                std::vector<std::string> tokens;
                Tokenize(output, tokens);

                if (tokens.size() > 2)
                {
                    if (("memory" == tokens[1] && "pages" == tokens[2])
                        || ("@" == tokens[1] && "memory" == tokens[2] && "pages" == tokens[3]))
                    {
                        keyValues["TotalMemory"] = (ToSCXUlong(tokens[0])) * 4;
                    }
                    else if (("free" == tokens[1] && "pages" == tokens[2])
                             || ("@" == tokens[1] && "free" == tokens[2] && "pages" == tokens[3]))
                    {
                        keyValues["AvailableMemory"] = (ToSCXUlong(tokens[0])) * 4;
                        break;
                    }
                }
            }
            pclose(file);

            keyValues["UsedMemory"] = keyValues["TotalMemory"] - keyValues["AvailableMemory"];
        }

        file = popen("lsps -s", "r");
        if (file)
        {
            char buf[256];
            if (fgets(buf, 256, file) && fgets(buf, 256, file))
            {
                std::string output = buf;

                std::vector<std::string> tokens;
                Tokenize(output, tokens);

                if (tokens.size() == 2)
                {
                    keyValues["TotalSwap"] = ToSCXUlong(tokens[0]) * 1024;
                    scxulong p = ToSCXUlong(tokens[1]);
                    keyValues["UsedSwap"] = keyValues["TotalSwap"] * p / 100;
                    keyValues["UsedSwapPercentage"] = p;
		}
            }
            pclose(file);

            keyValues["AvailableSwap"] = keyValues["TotalSwap"] - keyValues["UsedSwap"];
        }
    }

    void GetPagingData(std::map<std::string, scxulong>& keyValues)
    {
        FILE* file = popen("vmstat -s", "r");
        if (file)
        {
            char buf[256];

            for (char *o = fgets(buf, 256, file); o != NULL; o = fgets(buf, 256, file))
            {
                std::string output = buf;

                std::vector<std::string> tokens;
                Tokenize(output, tokens);

#if defined(aix)

                if (tokens.size() > 2)
                {
                    if ("page" == tokens[1] && "ins" == tokens[2])

#else

                if (tokens.size() > 3)
                {
                    if ("pages" == tokens[1] && "paged" == tokens[2] && "in" == tokens[3])

#endif

                    {
                        keyValues["PageReads"] = ToSCXUlong(tokens[0]);
                    }

#if defined(aix)

                    else if ("page" == tokens[1] && "outs" == tokens[2])

#else

                    else if ("pages" == tokens[1] && "paged" == tokens[2] && "out" == tokens[3])

#endif
                    {
                        keyValues["PageWrites"] = ToSCXUlong(tokens[0]);
                        break;
                    }
                }

            }
            pclose(file);
        }
    }

    void GetSolarisAvailableMem(std::map<std::string, scxulong>& keyValues)
    {
        FILE* vmstatFile = popen("vmstat", "r");

        if (vmstatFile)
            {
                char buf[256];
                fgets(buf, 256, vmstatFile);
                fgets(buf, 256, vmstatFile);
                fgets(buf, 256, vmstatFile);
                std::string vmstatOutput = buf;

                pclose(vmstatFile);

                std::vector<std::string> vmstatTokens;
                Tokenize(vmstatOutput, vmstatTokens);

                if (vmstatTokens.size() >= 17)
                    {
                        keyValues["AvailableMemory"] = ToSCXUlong(vmstatTokens[4]);
                    }
            }
    }

    void GetSolarisPRTConfData(std::map<std::string, scxulong>& keyValues)
    {
        char buf[256];

        FILE* prtconfFile = popen("/usr/sbin/prtconf | grep Memory", "r");

        if (prtconfFile)
            {
                fgets(buf, 256, prtconfFile);
                std::string prtconfOutput = buf;

                pclose(prtconfFile);

                std::vector<std::string> prtconfTokens;
                Tokenize(prtconfOutput, prtconfTokens);

                if (prtconfTokens.size() == 4)
                    {
                        if ("Megabytes" == prtconfTokens[3])
                            {
                                keyValues["TotalMemory"] = ToSCXUlong(prtconfTokens[2]) * 1024;
                            }
                    }
            }
    }

    void GetSolarisSwapData(std::map<std::string, scxulong>& keyValues)
    {
        char buf[256];

        FILE* swapFile = popen("/usr/sbin/swap -s", "r");

        if (swapFile)
            {
                fgets(buf, 256, swapFile);
                std::string swapOutput = buf;

                pclose(swapFile);

                std::vector<std::string> swapTokens;
                Tokenize(swapOutput, swapTokens);

                if (swapTokens.size() >= 12)
                    {
                        bool usedFound = false;
                        bool availableFound = false;
                        for (size_t i = 0; i < swapTokens.size(); ++i)
                            {
                                if ("used," == swapTokens[i])
                                    {
                                        keyValues["UsedSwap"] = ToSCXUlong(swapTokens[i-1]);
                                        usedFound = true;
                                    }
                                if ("available" == swapTokens[i])
                                    {
                                        keyValues["AvailableSwap"] = ToSCXUlong(swapTokens[i-1]);
                                        availableFound = true;
                                    }
                            }
                        if (usedFound && availableFound)
                            {
                                keyValues["TotalSwap"] = keyValues["UsedSwap"] + keyValues["AvailableSwap"];
                            }
                    }
            }
    }

#if defined(hpux)
    void GetHPUXPstatData(std::map<std::string, scxulong>& keyValues)
    {
        // We would like to read the property TotalMemory from an external source
        // but we can only find it in the tool 'glance' which isn't standard.

        struct pst_static psts;
        errno = 0;
        CPPUNIT_ASSERT(pstat_getstatic(&psts, sizeof(psts), (size_t)1, 0) > 0);
        CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(errno), errno == 0);

        keyValues["TotalMemory"] = psts.physical_memory * 4; // From 4K pages to 1K.

        CPPUNIT_ASSERT_MESSAGE("Strange pagesize. Don't trust any results!",
                               psts.page_size == 4096); // Check pagesize, just to make sure.
    }
#endif

    void GetHPUXTopData(std::map<std::string, scxulong>& keyValues)
    {
        // This is the line that we've after.
        // Memory: 513116K (383616K) real, 1988200K (1611388K) virtual, 416604K free  Page# 1/181

        // Top on HP/UX can't write to stdout except if it's a terminal.
        // So we need to write to a file to get sensible output.
        // top -d 1 -n 1 -u -f <filename>

        std::string topcmd("/usr/bin/top -d 1 -n 1 -u -f ");
        char tmpnambuf[L_tmpnam];
        char topbuf[256];
        bool done = false;
        int eno = 0;

        errno = 0;

        // Generate a temporary filename
        CPPUNIT_ASSERT(tmpnam(tmpnambuf));

        // Compose command
        std::string cmd = topcmd + std::string(tmpnambuf);

        if (system(cmd.c_str()) < 0) {
            CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(errno), errno == 0);
            return;
        }

        /* Now read the output */
        FILE *topFile = fopen(tmpnambuf, "r");
        if (topFile == 0) {
            CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(errno), errno == 0);
            return;
        }

        while(fgets(topbuf, sizeof(topbuf), topFile)) {
            if (topbuf[0] != 'M') continue; // 'M' as i "Memory".

            std::string topOutput(topbuf);
            std::vector<std::string> topTokens;
            Tokenize(topOutput, topTokens, " ()");

            if ("Memory:" == topTokens[0])
                {
                    keyValues["UsedMemory"]      = ToSCXUlong(topTokens[1])  / 1024; // 513116K above
                    keyValues["AvailableMemory"] = ToSCXUlong(topTokens[7])  / 1024; // 416604K above
                    done = true;
                }
        }
        /* If this wasn't end of file then save errno. */
        if (!feof(topFile)) { eno = errno; }
        fclose(topFile);
        remove(tmpnambuf);
        errno = eno;            // Just for documentation purposes!
        CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(eno), errno == 0);
        CPPUNIT_ASSERT_MESSAGE("Didn't get expected values from top", done);
    }

    void GetHPUXSwapinfoData(std::map<std::string, scxulong>& keyValues)
    {
        // Extract TotalSwap, AvailableSwap, UsedSwap
        // They map to AVAIL, FREE, and USED in the total row, respectively.
        // /usr/sbin/swapinfo -t
        //              Kb      Kb      Kb   PCT  START/      Kb
        // TYPE      AVAIL    USED    FREE  USED   LIMIT RESERVE  PRI  NAME
        // dev     4194304  560688 3633616   13%       0       -    1  /dev/vg00/lvol2
        // reserve       -  577736 -577736
        // memory  2076824  962068 1114756   46%
        // total   6271128 2100492 4170636   33%       -       0    -

        char buf[256];
        bool done = false;
        int eno = 0;

        errno = 0;
        FILE* swapinfoFile = popen("/usr/sbin/swapinfo -t", "r");
        CPPUNIT_ASSERT_MESSAGE("Can't do popen", swapinfoFile);
        if (!swapinfoFile) return;

        while(fgets(buf, sizeof(buf), swapinfoFile)) {
            if (buf[0] != 't') continue; // 't' as i "total".

            std::string swapinfoOutput(buf);
            std::vector<std::string> swapinfoTokens;
            Tokenize(swapinfoOutput, swapinfoTokens, " \t");

            if ("total" == swapinfoTokens[0]) {
                keyValues["TotalSwap"] = ToSCXUlong(swapinfoTokens[1]);          // The AVAIL column
                keyValues["UsedSwap"] = ToSCXUlong(swapinfoTokens[2]);           // The USED column
                keyValues["AvailableSwap"] = ToSCXUlong(swapinfoTokens[3]);      // The FREE column
                done = true;
                break;
            }
        }
        /* If this wasn't end of file then save errno. */
        if (!feof(swapinfoFile)) { eno = errno; }
        pclose(swapinfoFile);
        errno = eno;            // Just for documentation purposes!
        CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(eno), errno == 0);
        CPPUNIT_ASSERT_MESSAGE("Didn't get expected values from swapinfo", done);
    }

    static scxulong ToSCXUlong(const std::string& str)
    {
        scxulong tmp;
        std::stringstream ss(str);

        ss >> tmp;

        return tmp;
    }


    static void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " \n")
    {
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos = str.find_first_of(delimiters, lastPos);

        while (std::string::npos != pos || std::string::npos != lastPos)
            {
                tokens.push_back(str.substr(lastPos, pos - lastPos));
                lastPos = str.find_first_not_of(delimiters, pos);
                pos = str.find_first_of(delimiters, lastPos);
            }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( MemoryInstance_Test );

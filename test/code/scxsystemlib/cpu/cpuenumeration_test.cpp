/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    Created date    2007-05-28 08:10:00

    CPU data colletion test class.

    This class tests the Linux implementation, no need to even try to build on Windows.

    It compares a 10 second average against top output to a maring of 5 units

    A longer period will give a smaller margin, but will also make the tests take longer time to run.
    10 seconds seemes to give a good enough error margin for these tests.
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>

#include <scxsystemlib/cpuenumeration.h>

#if defined(sun)
#include <sys/sysinfo.h>
#include <scxsystemlib/scxkstat.h>
#endif // defined(sun)

#include <testutils/scxunit.h>
#include <string>
#include <vector>
#include <sstream>

#include <math.h>
#include <stdlib.h>

#if defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include <iostream>

// Sparc 8 doesn't have round() function ...
#if (defined(sun) && (PF_MAJOR==5) && (PF_MINOR<=9))
// This is a "bastardized" function suitable only for this test.  The real round() function
// returns a double; we don't.  Thus, this shouldn't be included in a compatibility function.
inline static scxulong round(double x)
{
    x += 0.5;
    return static_cast<scxulong>(x);
}
#endif

using namespace std;
using namespace SCXCoreLib;
using namespace SCXSystemLib;

static const wstring s_LogModuleName = L"scx.core.common.pal.system.cpu.cpuenumeration";

// Make it easy to determine if we support physical processor counts on HP or not

#if defined(hpux) && ((PF_MAJOR > 11) || (PF_MINOR >= 31))
#define HPUX_PHYSICAL_PROC_COUNT_SUPPORTED 1
#endif

class CPUPALTestDependencies;

extern "C"
{
    typedef void* (*pThreadFn)(void *);
}

#if defined(sun)

/**
    Mock SCXKstat
*/

class MockKstat : public SCXKstat
{
public:
    MockKstat(const CPUPALTestDependencies* pDeps) :
        SCXKstat(),
        m_pTestdeps(NULL),
        iteratorPosition(0)
    {
        // Need to remove constness introduced since the CreateKstat() is const-declared
        m_pTestdeps = const_cast<CPUPALTestDependencies*>(pDeps);
    }

    void SetStatistic(const wstring& statistic, scxulong value, int tag)
    {
        statisticMap[statistic] = value;
        mock_statistics.cpu_sysinfo.cpu[tag] = value;
    }

    scxulong GetValue(const wstring& statistic) const;

    void Lookup(const wstring& module, const wstring& name, int instance = -1);

    void Lookup(const wstring& module, int instance = -1);

    void Lookup(const char* module, const char* name, int instance = -1)
    {
        SCXKstat::Lookup(module, name, instance);
    }

    void* GetExternalDataPointer()
    {
        return &mock_statistics;
    }

protected:
    virtual kstat_t* ResetInternalIterator();
    virtual kstat_t* AdvanceInternalIterator();

    map<wstring, scxulong> statisticMap;
    cpu_stat_t mock_statistics;

    CPUPALTestDependencies* m_pTestdeps;
    size_t iteratorPosition;

    friend class CPUPALTestDependencies;
};

#endif // defined(sun)

/**
    Class for injecting test behavior into the CPU PAL.
 */

// Notes for Solaris:
//
// This class provides a framework for dynamic CPU support on Solaris. Solaris
// zones can support dynamic CPUs. In this configuration, CPU IDs need not start
// at zero, and need not be monotonically increasing. This class allows a kstat
// chain to be created in which CPUs can be created and deleted at will, and then
// passed on to implementation to check for proper behavior.
//
// Note that we don't implement a "complete" kstat chain in this code. For
// simplicity, we implement just what we need to test the production code on
// Solaris. In particular, we do not implement "real" independent data lookup
// of values (only specific ones), complete kstat_t data (unique ks_kid), nor
// actually chaining of kstat values (ks_next).  Instead, we overload the
// existing iterators to make things work.

class CPUPALTestDependencies : public CPUPALDependencies
{
public:
    CPUPALTestDependencies() :
        m_user(0),
        m_system(0),
        m_idle(0),
        m_iowait(0),
        m_nice(0),
        m_irq(0),
        m_softirq(0),
        m_numProcs(1),
        m_disabledProcs(0),
        m_cpuInfoFileType(-1)
    {}

    virtual SCXHandle<std::wistream> OpenStatFile() const
    {
        SCXHandle<std::wstringstream> statfilecontent( new std::wstringstream );
        *statfilecontent << L"cpu  " << m_numProcs * m_user
                         << L" " << m_numProcs * m_nice
                         << L" " << m_numProcs * m_system
                         << L" " << m_numProcs * m_idle
                         << L" " << m_numProcs * m_iowait
                         << L" " << m_numProcs * m_irq
                         << L" " << m_numProcs * m_softirq << L" 0" << endl;
        for (int i = 0; i < m_numProcs; ++i)
        {
            // In this simple mock every cpu is equal.
            *statfilecontent << L"cpu" << i
                             << L" " << m_user
                             << L" " << m_nice
                             << L" " << m_system
                             << L" " << m_idle
                             << L" " << m_iowait
                             << L" " << m_irq
                             << L" " << m_softirq << L" 0" << endl;
        }
        *statfilecontent << L"intr 925622655 892108154 78 0 2 2 0 4 0 2 0 0 0 1057 0 0 28275035 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1852436 0 0 0 0 0 0 0 3385885 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl
                         << L"ctxt 168393795" << endl
                         << L"btime 1208301855" << endl
                         << L"processes 343202" << endl
                         << L"procs_running 2" << endl
                         << L"procs_blocked 0" << endl;

        return statfilecontent;
    }

#if defined(linux)
    virtual SCXHandle<std::wistream> OpenCpuinfoFile() const
    {
        SCXHandle<wstringstream> cpufilecontent( new wstringstream );

        switch (m_cpuInfoFileType)
        {
            case 0:
                // In the first case, we give the whole shebang.
                // In the future, we cheat (only give enough for the caller to be
                // satisfied) to avoid needlessly increasing length.

                *cpufilecontent << L"processor       : 0" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"cpu family      : 6" << endl
                                << L"model           : 15" << endl
                                << L"model name      : Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz" << endl
                                << L"stepping        : 7" << endl
                                << L"cpu MHz         : 2333.414" << endl
                                << L"cache size      : 4096 KB" << endl
                                << L"fdiv_bug        : no" << endl
                                << L"hlt_bug         : no" << endl
                                << L"f00f_bug        : no" << endl
                                << L"coma_bug        : no" << endl
                                << L"fpu             : yes" << endl
                                << L"fpu_exception   : yes" << endl
                                << L"cpuid level     : 10" << endl
                                << L"wp              : yes" << endl
                                << L"flags           : fpu de tsc msr pae cx8 apic cmov pat clflush acpi mmx fxsr sse sse2 ss ht nx constant_tsc pni" << endl
                                << L"bogomips        : 5838.53" << endl
                                << endl
                                << L"processor       : 1" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"cpu family      : 6" << endl
                                << L"model           : 15" << endl
                                << L"model name      : Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz" << endl
                                << L"stepping        : 7" << endl
                                << L"cpu MHz         : 2333.414" << endl
                                << L"cache size      : 4096 KB" << endl
                                << L"fdiv_bug        : no" << endl
                                << L"hlt_bug         : no" << endl
                                << L"f00f_bug        : no" << endl
                                << L"coma_bug        : no" << endl
                                << L"fpu             : yes" << endl
                                << L"fpu_exception   : yes" << endl
                                << L"cpuid level     : 10" << endl
                                << L"wp              : yes" << endl
                                << L"flags           : fpu de tsc msr pae cx8 apic cmov pat clflush acpi mmx fxsr sse sse2 ss ht nx constant_tsc pni" << endl
                                << L"bogomips        : 5838.53" << endl;
                break;

            case 1:
                // One physical processor, one logical processor
                *cpufilecontent << L"processor       : 0" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 0" << endl
                                << L"siblings        : 1" << endl
                                << L"core id         : 0" << endl
                                << L"cpu cores       : 1" << endl;
                break;

            case 2:
                // One physical processor, two logical processors
                *cpufilecontent << L"processor       : 0" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 0" << endl
                                << L"siblings        : 2" << endl
                                << L"core id         : 0" << endl
                                << L"cpu cores       : 2" << endl
                                << endl
                                << L"processor       : 1" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 0" << endl
                                << L"siblings        : 2" << endl
                                << L"core id         : 0" << endl
                                << L"cpu cores       : 2" << endl;
                break;

            case 3:
                // Two physical processors, four logical processors
                *cpufilecontent << L"processor       : 0" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 0" << endl
                                << L"siblings        : 3" << endl
                                << L"core id         : 0" << endl
                                << L"cpu cores       : 4" << endl
                                << endl
                                << L"processor       : 1" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 0" << endl
                                << L"siblings        : 3" << endl
                                << L"core id         : 1" << endl
                                << L"cpu cores       : 4" << endl
                                << endl
                                << L"processor       : 2" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 1" << endl
                                << L"siblings        : 3" << endl
                                << L"core id         : 2" << endl
                                << L"cpu cores       : 4" << endl
                                << endl
                                << L"processor       : 3" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"physical id     : 1" << endl
                                << L"siblings        : 3" << endl
                                << L"core id         : 3" << endl
                                << L"cpu cores       : 4" << endl;
                break;

            case 4:
                // CPU info file from the PM's test system. No "physical id" line.
                // This appears to happen on VMware, but can happen on Hyper-V as well.
                *cpufilecontent << L"processor       : 0" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"cpu family      : 6" << endl
                                << L"model           : 26" << endl
                                << L"model name      : Intel(R) Xeon(R) CPU           W3530  @ 2.80GHz" << endl
                                << L"stepping        : 5" << endl
                                << L"cpu MHz         : 2799.948" << endl
                                << L"cache size      : 8192 KB" << endl
                                << L"fpu             : yes" << endl
                                << L"fpu_exception   : yes" << endl
                                << L"cpuid level     : 11" << endl
                                << L"wp              : yes" << endl
                                << L"flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss syscall nx lm constant_tsc up rep_good nopl pni ssse3 cx16 sse4_1 sse4_2 popcnt hypervisor lahf_lm" << endl
                                << L"bogomips        : 5599.89" << endl
                                << L"clflush size    : 64" << endl
                                << L"cache_alignment : 64" << endl
                                << L"address sizes   : 36 bits physical, 48 bits virtual" << endl
                                << L"power management:" << endl;
                break;

            case 44326:
                // WI 44326: 3 physical cores with two physical processors?
                //
                // Turns out that physical IDs of CPU Cores need not be
                // monotonically increasing.  Changed algorithm to accomodate.

                *cpufilecontent << L"processor       : 0" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"cpu family      : 6" << endl
                                << L"model           : 26" << endl
                                << L"model name      : Intel(R) Xeon(R) CPU           E5530  @ 2.40GHz" << endl
                                << L"stepping        : 5" << endl
                                << L"cpu MHz         : 2400.357" << endl
                                << L"cache size      : 8192 KB" << endl
                                << L"physical id     : 0" << endl
                                << L"siblings        : 1" << endl
                                << L"core id         : 0" << endl
                                << L"cpu cores       : 1" << endl
                                << L"apicid          : 0" << endl
                                << L"initial apicid  : 0" << endl
                                << L"fpu             : yes" << endl
                                << L"fpu_exception   : yes" << endl
                                << L"cpuid level     : 11" << endl
                                << L"wp              : yes" << endl
                                << L"flags           : fpu vme de pse tsc msr pae mce cx8 apic mtrr pge mca cmov pat clflush mmx fxsr sse sse2 ht syscall nx rdtscp lm constant_tsc rep_good unfair_spinlock pni ssse3 cx16 sse4_1 sse4_2 popcnt hypervisor lahf_lm" << endl
                                << L"bogomips        : 4800.71" << endl
                                << L"clflush size    : 64" << endl
                                << L"cache_alignment : 64" << endl
                                << L"address sizes   : 40 bits physical, 48 bits virtual" << endl
                                << L"power management:" << endl
                                << endl
                                << L"processor       : 1" << endl
                                << L"vendor_id       : GenuineIntel" << endl
                                << L"cpu family      : 6" << endl
                                << L"model           : 26" << endl
                                << L"model name      : Intel(R) Xeon(R) CPU           E5530  @ 2.40GHz" << endl
                                << L"stepping        : 5" << endl
                                << L"cpu MHz         : 2400.357" << endl
                                << L"cache size      : 8192 KB" << endl
                                << L"physical id     : 2" << endl
                                << L"siblings        : 1" << endl
                                << L"core id         : 0" << endl
                                << L"cpu cores       : 1" << endl
                                << L"apicid          : 2" << endl
                                << L"initial apicid  : 2" << endl
                                << L"fpu             : yes" << endl
                                << L"fpu_exception   : yes" << endl
                                << L"cpuid level     : 11" << endl
                                << L"wp              : yes" << endl
                                << L"flags           : fpu vme de pse tsc msr pae mce cx8 apic mtrr pge mca cmov pat clflush mmx fxsr sse sse2 ht syscall nx rdtscp lm constant_tsc rep_good unfair_spinlock pni ssse3 cx16 sse4_1 sse4_2 popcnt hypervisor lahf_lm" << endl
                                << L"bogomips        : 4799.68" << endl
                                << L"clflush size    : 64" << endl
                                << L"cache_alignment : 64" << endl
                                << L"address sizes   : 40 bits physical, 48 bits virtual" << endl
                                << L"power management:" << endl;
                break;

            default:
                std::stringstream ss;
                ss << "Unknown stat filetype during call to OpenCpuinfoFile(): " << m_cpuInfoFileType;
                CPPUNIT_FAIL( ss.str() );
        }

        return cpufilecontent;
    }
#endif // defined(linux)

// Solaris has it's own version - keeps things easier
#if !defined(sun)
    virtual long sysconf(int name) const
    {
#if defined(linux) || defined(aix)
        switch (name)
        {
            case _SC_NPROCESSORS_ONLN:
                return m_numProcs - m_disabledProcs;
#if defined(aix)
            case _SC_NPROCESSORS_CONF:
                return m_numProcs;
#endif /* aix */

            default:
                std::stringstream ss;
                ss << "CPUPALTestDependencies::sysconf - the mock is not designed to handle sysconf ID: " << name;
                CPPUNIT_FAIL( ss.str() );
                return -1;
        }
#else
        return -1;
#endif // defined(linux) || defined(aix)
    }
#endif // !defined(sun)

#if defined(sun)
    void Reset()
    {
        m_vKstat.clear();
        m_vChipID.clear();
        m_numProcs = 0;
    }

    void AddInstance(int instanceID, int chipID)
    {
        kstat_t kst;

        // Populate the kstat structure
        memset( &kst, 0, sizeof(kst) );
        strncpy( kst.ks_module, "cpu_info", KSTAT_STRLEN );
        strncpy( kst.ks_class, "misc", KSTAT_STRLEN );
        kst.ks_instance = instanceID;
        kst.ks_type = KSTAT_TYPE_NAMED;

        std::ostringstream stream;
        stream << "cpu_info" << instanceID;
        strncpy( kst.ks_name, stream.str().c_str(), KSTAT_STRLEN );

        // And insert (not bothering to check for duplicates)
        CPPUNIT_ASSERT( m_vKstat.size() == m_numProcs );
        m_vKstat.push_back( kst );

        CPPUNIT_ASSERT( m_vChipID.size() == m_numProcs );
        m_vChipID.push_back( chipID );

        m_numProcs++;
    }

    // The Dynamic CPU implementation for Solaris no longer relies on sysconf() API.
    // Let's be certain that's actually the case.

    virtual long sysconf(int name) const
    {
        switch (name)
        {
            case _SC_NPROCESSORS_CONF:
                return m_vKstat.size();

            case _SC_NPROCESSORS_ONLN:
            case _SC_CPUID_MAX:
            default:
                std::stringstream ss;
                ss << "CPUPALTestDependencies::sysconf - the mock is not designed to handle sysconf ID: " << name;
                CPPUNIT_FAIL( ss.str() );
                return -1;
        }
    }

    virtual int p_online(processorid_t processorid, int flag) const
    {
        // Check if the processor ID is in our kstat chain
        int returnState = P_OFFLINE;

        for (std::vector<kstat_t>::const_iterator it = m_vKstat.begin(); it != m_vKstat.end(); ++it)
        {
            if (processorid == (*it).ks_instance)
            {
                returnState = P_ONLINE;
                break;
            }
        }

        return returnState;
    }

    virtual const SCXHandle<SCXKstat> CreateKstat() const
    {
        // Despite its name, the MockKstat depends on the running system
        // in that sense that the "module" and "name" parameters must map to
        // existing entities. And "sys" and "cpu" does not exist on Sparc V8.
        // Before: SCXHandle<MockKstat> kstat = new MockKstat(L"cpu", L"sys", cpuid);
        SCXHandle<MockKstat> kstat( new MockKstat(this) );
        return kstat;
    }

#elif defined(hpux)
#if defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
    void Reset()
    {
        m_vChipID.clear();
        m_numProcs = 0;
    }

    void AddInstance(int instanceID, int chipID)
    {
        (void) instanceID;
        CPPUNIT_ASSERT( m_vChipID.size() == m_numProcs );

        m_vChipID.push_back( chipID );
        m_numProcs++;
    }
#endif // defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
    virtual int pstat_getprocessor(struct pst_processor* buf,
                                   size_t elemsize,
                                   size_t elemcount,
                                   int index) const
    {
        for (int i = 0; i < elemcount; ++i)
        {
            buf[i].psp_logical_id = i;
            buf[i].psp_processor_state = (i < m_numProcs - m_disabledProcs ? PSP_SPU_ENABLED : PSP_SPU_DISABLED);
            buf[i].psp_cpu_time[CP_USER] = m_user;
            buf[i].psp_cpu_time[CP_NICE] = m_nice;
            buf[i].psp_cpu_time[CP_IDLE] = m_idle;
            buf[i].psp_cpu_time[CP_SYS]  = m_system;
            buf[i].psp_cpu_time[CP_WAIT] = m_iowait;
#if HPUX_PHYSICAL_PROC_COUNT_SUPPORTED
            buf[i].psp_socket_id = m_vChipID[i];
#endif
        }
        return elemcount;
    }

    virtual int pstat_getdynamic(struct pst_dynamic* buf,
                                 size_t elemsize,
                                 size_t elemcount,
                                 int index) const
    {
        buf[0].psd_max_proc_cnt = m_numProcs;
        return 1;
    }

#elif defined(aix)
    virtual int perfstat_cpu_total(perfstat_id_t* name,
                                   perfstat_cpu_total_t* buf,
                                   int bufsz,
                                   int number) const
    {
        int n = m_numProcs - m_disabledProcs; // Number of online procs

        buf->user = m_user * n;
        buf->sys = m_system * n;
        buf->idle = m_idle * n;
        buf->wait = m_iowait * n;
        buf->runque = 2;
        return 1;
    }

    virtual int perfstat_cpu(perfstat_id_t* name,
                             perfstat_cpu_t* buf,
                             int bugsz,
                             int number) const
    {
        int n = m_numProcs - m_disabledProcs; // Number of online procs

        // If more CPUs requested than we have (working), return what we've got
        if (number > n)
        {
            number = n;
        }

        for (int i = 0; i < number; ++i)
        {
            buf[i].user = m_user;
            buf[i].sys = m_system;
            buf[i].idle = m_idle;
            buf[i].wait = m_iowait;
            buf[i].runque = 5;
        }

        return n;
    }

    virtual int perfstat_partition_total(perfstat_id_t* name,
                                         perfstat_partition_total_t* buf,
                                         int sizeof_struct,
                                         int number) const
    {
        // Check for bad parameters
        if (name != NULL || number != 1)
        {
            return 0;
        }

        memset(buf, 0, sizeof (perfstat_partition_total_t));
        buf->online_cpus = m_numProcs - m_disabledProcs;
        buf->min_cpus = 1;
        buf->max_cpus = m_numProcs;

        return 1;
    }

#endif

    void SetUser(scxulong val) { m_user = val; }
    void SetSystem(scxulong val) { m_system = val; }
    void SetIdle(scxulong val) { m_idle = val; }
    void SetIOWait(scxulong val) { m_iowait = val; }
    void SetNice(scxulong val) { m_nice = val; }
    void SetIrq(scxulong val) { m_irq = val; }
    void SetSoftIrq(scxulong val) { m_softirq = val; }

    void SetNumProcs(int numProcs)
    {
        m_numProcs = numProcs;
#if defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        Reset();
        for (int i = 0; i < numProcs; i++)
            AddInstance(i, 1);
#endif // defined(sun)
    }

    void SetDisabledProcs(int disabledProcs)
    {
        m_disabledProcs = disabledProcs;

#if defined(sun)
        // Let's make sure we're not disabling more than we've got
        CPPUNIT_ASSERT(m_vKstat.size() >= disabledProcs);

        for (int i = 0; i < disabledProcs; i++)
            m_vKstat.pop_back();
#endif // defined(sun)
    }

    void SetCpuInfoFileType(int type) { m_cpuInfoFileType = type; }

public:
    scxulong m_user;
    scxulong m_system;
    scxulong m_idle;
    scxulong m_iowait;

#if defined(sun)
    vector<kstat_t> m_vKstat;           //!< Vector of kstat_t data structures
    vector<int> m_vChipID;              //!< Vector of chip ID's (named data)

    friend class MockKstat;
#elif defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
    vector<int> m_vChipID;              //!< Vector of chip ID's
#endif

private:
    scxulong m_nice;
    scxulong m_irq;
    scxulong m_softirq;
    int m_numProcs;
    int m_disabledProcs;
    int m_cpuInfoFileType;
};

#if defined(sun)
// Need to be here due to mutal dependencies
scxulong MockKstat::GetValue(const wstring& statistic) const
{
    if (L"chip_id" == statistic)
    {
        // Validate we're called properly
        SCXUNIT_ASSERT_BETWEEN_MESSAGE("iteratorPosition out of range",
            iteratorPosition, 0, m_pTestdeps->m_vKstat.size() - 1);

        return m_pTestdeps->m_vChipID[iteratorPosition];
    }

    return statisticMap.find(statistic)->second;
}

void MockKstat::Lookup(const wstring& module, const wstring& name, int instance)
{
    // Call the real kstat lookup to make sure accesses of m_KstatPointer in SCXKstat::GetValueRaw()
    // will get proper values for all but the mock values below
    SCXKstat::Lookup(module, name, instance);

    SetStatistic(L"user", m_pTestdeps->m_user, CPU_USER);
    SetStatistic(L"kernel", m_pTestdeps->m_system, CPU_KERNEL);
    SetStatistic(L"idle", m_pTestdeps->m_idle, CPU_IDLE);
    SetStatistic(L"wait", m_pTestdeps->m_iowait, CPU_WAIT);
}

void MockKstat::Lookup(const wstring& module, int instance)
{
    // Call the real kstat lookup to make sure that the accesses are valid, but
    // fake the return values (Note: This is used for Physical CPU counts; we
    // only call with instance 0 since we don't know how many processors the
    // real (underlying) system has.)
    SCXKstat::Lookup(module, 0);

    SetStatistic(L"chip_id",
                 m_pTestdeps->m_vChipID[instance],
                 0 /* "Zero is to satisfy "fake" CPU statistic */);
}

kstat_t* MockKstat::ResetInternalIterator()
{
    iteratorPosition = 0;
    if (iteratorPosition >= m_pTestdeps->m_vKstat.size())
    {
        return NULL;
    }
    return &m_pTestdeps->m_vKstat[iteratorPosition];
}

kstat_t* MockKstat::AdvanceInternalIterator()
{
    if (++iteratorPosition >= m_pTestdeps->m_vKstat.size())
    {
        return NULL;
    }
    return &m_pTestdeps->m_vKstat[iteratorPosition];
}
#endif



class CPUPALTestDependenciesWI11678 : public CPUPALTestDependencies
{
public:
    CPUPALTestDependenciesWI11678(){
        SetNumProcs(8);
    }

    virtual SCXHandle<std::wistream> OpenStatFile() const
    {
        SCXHandle<std::wstringstream> statfilecontent( new std::wstringstream );
        *statfilecontent <<
        L"cpu  91932320 79411 2311540 7259234600 323686 19333 79380 0" << endl <<
        L"cpu0 1521515 1067 270995 906917730 113406 8908 71567 0" << endl <<
        L"cpu1 1608830 15162 285905 906949703 42217 2131 1179 0" << endl <<
        L"cpu2 505780 872 253619 908093644 42759 4229 4234 0" << endl <<
        L"cpu3 1727636 31344 374383 906755767 11595 4063 349 0" << endl <<
        L"cpu4 480444 628 276461 908133093 14284 0 233 0" << endl <<
        L"cpu5 1528135 3999 327597 907034134 10952 0 325 0" << endl <<
        L"cpu6 432151 481 201196 908238460 32373 0 480 0" << endl <<
        L"cpu7 1388738 25853 321378 907112065 56097 0 1009 0" << endl <<
        L"intr 9532421188 500146530 3 0 3 3 0 0 0 3 0 0 0 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 26209011 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 94619924 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 142848381 0 0 0 0 0 0 0 178662734 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0" << endl <<
        L"ctxt 39309945745" << endl <<
        L"btime 1225989933" << endl <<
        L"processes 8223456" << endl <<
        L"procs_running 1" << endl <<
        L"procs_blocked 0";

        return statfilecontent;
    }
};


#if defined(sun)

class CPUPALTestDependenciesWI367214 : public CPUPALTestDependencies
{
public:
    CPUPALTestDependenciesWI367214(int maxProcessorId)
        : CPUPALTestDependencies(),
          m_status(-1),
          m_maxCallCount(-1),
          m_currentCallCount(0)
    {
        for (int i = 0; i < maxProcessorId; i++)
            AddInstance(i, 1);
    }

    void SetStatus(int status) { m_status = status; }
    int GetCurrentPOnlineCallCount(void) const { return m_currentCallCount; }
    void EnablePOnlineCallChecking(int maxCallCount) { m_maxCallCount = maxCallCount; }

    virtual int p_online(processorid_t processorid, int flag) const
    {
        const_cast <CPUPALTestDependenciesWI367214 *>(this)->m_currentCallCount;

        // If hit, these indicates that a loop condition is failing.
        CPPUNIT_ASSERT((m_maxCallCount == -1) || (m_currentCallCount < m_maxCallCount));
        CPPUNIT_ASSERT(processorid <= CPUPALTestDependencies::m_vKstat.size());

        // If hit, this would indicate that the mock is being used for something
        // it doesn't currently handle.
        if (flag != P_STATUS)
        {
            std::stringstream ss;
            ss << "CPUPALTestDependenciesWI367214::p_online - the mock is not designed to handle flag: " << flag;
            CPPUNIT_FAIL( ss.str() );
        }

        if (m_status == -1)
        {
            errno = EINVAL;
        }

        return m_status;
    }

private:
    int m_status;
    int m_maxCallCount;
    int m_currentCallCount;
};

#endif


class CPUEnumeration_Test : public CPPUNIT_NS::TestFixture
{
    friend class SCXSystemLib::CPUEnumeration;

    CPPUNIT_TEST_SUITE( CPUEnumeration_Test  );

    CPPUNIT_TEST( testMockedValues );
    CPPUNIT_TEST( testRemoveProc );
    CPPUNIT_TEST( testRealValues );
    CPPUNIT_TEST( testMockedValues_WI11678 );
    CPPUNIT_TEST( testLogicalProcCount );
    CPPUNIT_TEST( testGetProcessorCountLogical );
    CPPUNIT_TEST( testPhysicalProcCountWithNoCpuinfoData );
    CPPUNIT_TEST( testPhysicalProcCountWithOneCoreProcessor );
    CPPUNIT_TEST( testPhysicalProcCountWithOneCoreProcessorTwoProcs );
    CPPUNIT_TEST( testPhysicalProcCountWithTwoCoreProcessors );
    CPPUNIT_TEST( testPhysicalProcCountWithNoPhysicalID );
    CPPUNIT_TEST( testPhysicalProcCountWithGapInPhysicalIDs );
    CPPUNIT_TEST( testPhysicalProcCountWithMassiveProcessors );
    CPPUNIT_TEST( testPhysicalLogicalProcCountsWithDynamicCPUs );
    CPPUNIT_TEST( TestNoProcessorsOnlineDuringUpdate );

    SCXUNIT_TEST_ATTRIBUTE(testMockedValues,SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testRemoveProc,SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testRealValues,SLOW);

    CPPUNIT_TEST_SUITE_END();

public:

        CPUEnumeration* m_pEnum;

    void setUp(void)
    {
        m_pEnum = 0;
    }

    void tearDown(void)
    {
        if (m_pEnum != 0)
        {
            m_pEnum->CleanUp();
            delete m_pEnum;
            m_pEnum = 0;
        }
    }

    bool MeetsPrerequisites(std::wstring testName)
    {
#if defined(hpux)
        /* HP needs privileges because sudo doesn't work reliably when already sudo'ed */
        if (0 == geteuid())
        {
            return true;
        }

        std::wstring warnText;

        warnText = L"Platform needs privileges to run CPUEnumeration_Test::" + testName + L" test";

        SCXUNIT_WARNING(warnText);
        return false;
#else
        /* No privileges needed */
        (void) testName;

        return true;
#endif
    }

    void testMockedValues()
    {
        // Set up some values to use for testing
        scxulong user1    = 0;
        scxulong user2    = 0;
        scxulong system1  = 0;
        scxulong system2  = 0;
        scxulong idle1    = 0;
        scxulong idle2    = 0;
        scxulong iowait1  = 0;
        scxulong iowait2  = 0;
        scxulong nice1    = 0;
        scxulong nice2    = 0;
        scxulong irq1     = 0;
        scxulong irq2     = 0;
        scxulong softirq1 = 0;
        scxulong softirq2 = 0;

        user1    = 866380;
        user2    = 866489;
        system1  = 2276265;
        system2  = 2276621;
        idle1    = 352845757;
        idle2    = 352847702;
        iowait1  = 285749;
        iowait2  = 285754;
#if !defined(sun) && !defined(aix)
        nice1    = 8090;
        nice2    = 8090;
#if !defined(hpux)
        irq1     = 74027;
        irq2     = 74027;
        softirq1 = 509292;
        softirq2 = 509312;
#endif
#endif

        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
#if defined(sun)
        // On Solaris, let's have at least one CPU (probably harmless elsewhere)
        deps->SetNumProcs(1);
#endif

        // Set up values for first snapshot
        deps->SetUser(user1);
        deps->SetSystem(system1);
        deps->SetIdle(idle1);
        deps->SetIOWait(iowait1);
        deps->SetNice(nice1);
        deps->SetIrq(irq1);
        deps->SetSoftIrq(softirq1);

        // Take first snapshot
        m_pEnum = new CPUEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Set up values for second snapshot
        deps->SetUser(user2);
        deps->SetSystem(system2);
        deps->SetIdle(idle2);
        deps->SetIOWait(iowait2);
        deps->SetNice(nice2);
        deps->SetIrq(irq2);
        deps->SetSoftIrq(softirq2);

        // Take second snapshot
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Calculate total time between the two snapshots
        double totalDelta = static_cast<double>(user2 - user1 + nice2 - nice1
                                                + system2 - system1 + idle2 - idle1
                                                + iowait2 - iowait1 + irq2 - irq1
                                                + softirq2 - softirq1);

        // Get the cpu0 instance from the cpu pal.
        SCXCoreLib::SCXHandle<CPUInstance> inst = m_pEnum->GetInstance(0);
        CPPUNIT_ASSERT(0 != inst);
        scxulong data;

#if defined(sun) || defined(linux) || defined(hpux) || defined(aix)

        CPPUNIT_ASSERT(inst->GetProcessorTime(data));
        scxulong computed = static_cast<scxulong>(100 - round(100 * static_cast<double>(idle2 - idle1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);

        CPPUNIT_ASSERT(inst->GetIdleTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(idle2 - idle1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);

        CPPUNIT_ASSERT(inst->GetUserTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(user2 - user1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);

        CPPUNIT_ASSERT(inst->GetPrivilegedTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(system2 - system1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);

        CPPUNIT_ASSERT(inst->GetIowaitTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(iowait2 - iowait1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);

#if defined(sun) || defined(aix)
        CPPUNIT_ASSERT(inst->GetNiceTime(data) == false);
#else
        CPPUNIT_ASSERT(inst->GetNiceTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(nice2 - nice1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);
#endif

#if defined(hpux) || defined(sun) || defined(aix)
        CPPUNIT_ASSERT(inst->GetInterruptTime(data) == false);
#else
        CPPUNIT_ASSERT(inst->GetInterruptTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(irq2 - irq1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);
#endif

#if defined(hpux) || defined(sun) || defined(aix)
        CPPUNIT_ASSERT(inst->GetDpcTime(data) == false);
#else
        CPPUNIT_ASSERT(inst->GetDpcTime(data));
        computed = static_cast<scxulong>(round(100 * static_cast<double>(softirq2 - softirq1)/totalDelta));
        CPPUNIT_ASSERT_EQUAL(data, computed);
#endif

#if defined(linux) || defined(hpux) || defined(sun)
        CPPUNIT_ASSERT(inst->GetQueueLength(data) == false);
#else
        CPPUNIT_ASSERT(inst->GetQueueLength(data));
        CPPUNIT_ASSERT(data > 0);
#endif

#else

#error "Not implemented for this platform"

#endif
    }

    void testMockedValues_WI11678()
    {
        // this test is linux specific and was found on machine with 8 CPUs
        //  where 'total' numbers get greater than uint32
#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependenciesWI11678());

        // Take first snapshot
        m_pEnum = new CPUEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Get the cpu0 instance from the cpu pal.
        SCXCoreLib::SCXHandle<CPUInstance> inst = m_pEnum->GetInstance(3);
        SCXCoreLib::SCXHandle<CPUInstance> inst_tot = m_pEnum->GetTotalInstance();
        CPPUNIT_ASSERT(0 != inst);
        CPPUNIT_ASSERT(0 != inst_tot);

        // verify that int64 values can be read without trancation
        // magic numbers correspond to sample file from WI (see CPUPALTestDependenciesWI11678 above)

        CPPUNIT_ASSERT( inst->GetIdleLastTick() == 906755767l );
        // -pedantic option prohibits declaration like 7259234600ll, 0x1B0AF2128
        // so this strange number is 7259234600l
        CPPUNIT_ASSERT( inst_tot->GetIdleLastTick() == ( (((scxlong) 0x1 ) << 32) + ((scxlong) 0xB0AF2128)) );

#endif
    }

    void testRemoveProc()
    {
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(2);

        // Take first snapshot
        m_pEnum = new CPUEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Verify the second processor exists.
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), m_pEnum->Size());

        // Set up values for second snapshot
        deps->SetDisabledProcs(1);

        // Take second snapshot
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Verify that the second processor no longer exists.
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), m_pEnum->Size());
    }

    void testRealValues()
    {
        pthread_t t_loadcpu;

        bool stopLoad = false;

        if (pthread_create(&t_loadcpu, NULL, (pThreadFn) LoadCPU, &stopLoad) != 0)
        {
            CPPUNIT_FAIL("Failed to create CPU load thread");
        }
        else
        {
            m_pEnum = new CPUEnumeration();

            scxulong user;
            scxulong system;
            scxulong idle;
            scxulong iowait;
            scxulong nice;
            scxulong irq;
            scxulong softirq;
            scxulong total;

            // initiate and do first sample
            m_pEnum->Init();
            m_pEnum->SampleData();
            m_pEnum->Update();

            // Get the total instance from the cpu pal.
            SCXCoreLib::SCXHandle<CPUInstance> inst = m_pEnum->GetTotalInstance();

            // retrieve counters
            user = inst->GetUserLastTick();
            system = inst->GetPrivilegedLastTick();
            idle = inst->GetIdleLastTick();
            iowait = inst->GetIowaitLastTick();
            nice = inst->GetNiceLastTick();
            irq = inst->GetInterruptLastTick();
            softirq = inst->GetSWInterruptLastTick();
            total = inst->GetTotalLastTick();

            // sleep
            DoSleep(interval);

            // get second sample
            m_pEnum->SampleData();
            m_pEnum->Update();

            stopLoad = true;
            pthread_join(t_loadcpu, NULL);

            scxulong data;

#if defined(sun) || defined(linux) || defined(hpux) || defined(aix)

            CPPUNIT_ASSERT(inst->GetProcessorTime(data));

#if !defined(aix)
            CPPUNIT_ASSERT(total > 0);
            CPPUNIT_ASSERT(total < inst->GetTotalLastTick());
#endif
            CPPUNIT_ASSERT(inst->GetIdleTime(data));
            CPPUNIT_ASSERT(idle <= inst->GetIdleLastTick());

            CPPUNIT_ASSERT(inst->GetUserTime(data));
            CPPUNIT_ASSERT(user <= inst->GetUserLastTick());

            CPPUNIT_ASSERT(inst->GetPrivilegedTime(data));
            CPPUNIT_ASSERT(system <= inst->GetPrivilegedLastTick());

            CPPUNIT_ASSERT(inst->GetIowaitTime(data));
            CPPUNIT_ASSERT(iowait <= inst->GetIowaitLastTick());

#if defined(sun) || defined(aix)
            CPPUNIT_ASSERT(inst->GetNiceTime(data) == false);
#else
            CPPUNIT_ASSERT(inst->GetNiceTime(data));
            CPPUNIT_ASSERT(nice <= inst->GetNiceLastTick());
#endif

#if defined(hpux) || defined(sun) || defined(aix)
            CPPUNIT_ASSERT(inst->GetInterruptTime(data) == false);
#else
            CPPUNIT_ASSERT(inst->GetInterruptTime(data));
            CPPUNIT_ASSERT(irq <= inst->GetInterruptLastTick());
#endif

#if defined(hpux) || defined(sun) || defined(aix)
            CPPUNIT_ASSERT(inst->GetDpcTime(data) == false);
#else
            CPPUNIT_ASSERT(inst->GetDpcTime(data));
            CPPUNIT_ASSERT(softirq <= inst->GetSWInterruptLastTick());
#endif

#if defined(linux) || defined(hpux) || defined(sun)
            CPPUNIT_ASSERT(inst->GetQueueLength(data) == false);
#else
            CPPUNIT_ASSERT(inst->GetQueueLength(data));
#endif

#else

#error "Not implemented for this plattform"

#endif
        }
    }

    void testLogicalProcCount()
    {
        // Test that, if we look up the number of logical processors, that will
        // match the number of instances created via the CPU PAL.

        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(4);
        deps->SetDisabledProcs(0);

        // Verify that the logical count of processors matches
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), CPUEnumeration::ProcessorCountLogical(deps));

        // Take snapshot
        m_pEnum = new CPUEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Verify the count of processors
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), m_pEnum->Size());
    }

    void testGetProcessorCountLogical()
    {
        scxulong logicalProcessor = 0;
        CPPUNIT_ASSERT(SCXSystemLib::CPUEnumeration::GetProcessorCountLogical(logicalProcessor));

        unsigned int numberOfLogicalProcessors = 0;
        wstring cmdStrNumOfLogProcessor;
        vector<wstring> tokens;
        std::istringstream streamInstr;
        std::ostringstream streamOutstr;
        std::ostringstream streamErrstr;

        if ( ! MeetsPrerequisites(L"testGetProcessorCountLogical"))
            return;

        /*--------sample output on AIX------------------------*/
        /*--cmd: bindprocessor -q        
            output: The available processors are:  0 1 2 3 4 5 6 7
        ---------------------------------------*/
       
        /*--------sample output on HPUX------------------------*/
        /*--cmd: sudo ioscan -fnC processor
           output: 
         Class       I  H/W Path  Driver    S/W State   H/W Type     Description
         ========================================================================
         processor   0  128       processor   CLAIMED     PROCESSOR    Processor
         processor   1  129       processor   CLAIMED     PROCESSOR    Processor
        ------------------------------*/

        /*--------sample output on SUN------------------------*/
        /*--cmd: psrinfo
           output:
          0       on-line   since 09/11/2012 11:39:14
          1       on-line   since 09/11/2012 11:39:17
        -----------------------------------------------*/

        /*--------sample output on Linux------------------------*/
        /*--cmd: cat /proc/cpuinfo |grep processor
          output: 
          processor       : 0
          processor       : 1
        -----------------------------------------------*/
  
        #if defined(aix)
            cmdStrNumOfLogProcessor = L"bindprocessor -q";
        #elif defined(hpux)
            cmdStrNumOfLogProcessor = L"ioscan -fnC processor";
        #elif defined(sun)
            cmdStrNumOfLogProcessor = L"psrinfo";
        #elif defined(linux)
            cmdStrNumOfLogProcessor = L"cat /proc/cpuinfo";
        #endif
            
        int procRet = SCXCoreLib::SCXProcess::Run(cmdStrNumOfLogProcessor, streamInstr, streamOutstr, streamErrstr, 150000);
      
        #if defined(hpux) && PF_MAJOR == 11 && PF_MINOR == 31
           if (procRet == 0)
        #else
           if (procRet == 0 && streamErrstr.str().empty())
        #endif 
        {
            std::string stdoutStr = streamOutstr.str();
            std::istringstream stdInStr(stdoutStr);
            SCXCoreLib::SCXStream::NLFs nlfs;
            vector<wstring> outLines;
            SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stdInStr, outLines, nlfs);
          
            #if defined(hpux) || defined(linux) || defined(sun) 
                for(unsigned int i=0; i<outLines.size(); i++)
                {
                #if defined(hpux) || defined(sun)
                    StrTokenize(outLines[i], tokens, L" ");
                #elif defined(linux)
                    StrTokenize(outLines[i], tokens, L":");
                #endif
                    if (tokens.size() > 1)
                    {
                    #if defined(hpux) || defined(linux)
                        if (0 == tokens[0].compare(L"processor"))
                    #endif
                        {
                            numberOfLogicalProcessors++;
                        }
                    }
                }

            #elif defined(aix)
                for(unsigned int i=0; i<outLines.size(); i++)
                {
                    StrTokenize(outLines[i], tokens, L":");
                    if (tokens.size() > 1)
                    {
                        if (0 == tokens[0].compare(L"The available processors are"))
                        {
                            vector<wstring> lcputokens;
                            StrTokenize(tokens[1], lcputokens, L" "); 
                            numberOfLogicalProcessors = lcputokens.size();
                            break;
                        }
                }
                }
            #endif
        }
        else
        {
            cout << "Command Run failed. The return value is : " << procRet << endl;
            cout << "The ErrorString is : " << streamErrstr.str() << endl;
        }

        CPPUNIT_ASSERT_EQUAL(numberOfLogicalProcessors, logicalProcessor);
    }

    void testPhysicalProcCountWithNoCpuinfoData()
    {
        // We can't test HP-UX here - it will throw an exception with no CPU information

#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetCpuInfoFileType(0);
#elif defined(aix)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(0);
#elif defined(sun)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
#endif

#if defined(linux) || defined(aix) || defined(sun)
        // Verify that the physical count of processors matches
        // (Note that we must always have at least one processor)
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif
    }

    void testPhysicalProcCountWithOneCoreProcessor()
    {
#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetCpuInfoFileType(1);
#elif defined(aix)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(1);
#elif defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->Reset();
        deps->AddInstance(1, 1);
#endif

#if defined(linux) || defined(aix) || defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif
    }

    void testPhysicalProcCountWithOneCoreProcessorTwoProcs()
    {
#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetCpuInfoFileType(2);
#elif defined(aix)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(1);
#elif defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->Reset();
        deps->AddInstance(1, 1);
        deps->AddInstance(2, 1);
#endif

#if defined(linux) || defined(aix) || defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif
    }

    void testPhysicalProcCountWithTwoCoreProcessors()
    {
#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetCpuInfoFileType(3);
#elif defined(aix)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(2);
#elif defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->Reset();
        deps->AddInstance(1, 1);
        deps->AddInstance(2, 2);
        deps->AddInstance(3, 1);
        deps->AddInstance(4, 2);
        deps->AddInstance(5, 1);
        deps->AddInstance(6, 2);
#endif

#if defined(linux) || defined(aix) || defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif
    }

    void testPhysicalProcCountWithNoPhysicalID()
    {
#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetCpuInfoFileType(4);

        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif
    }

    void testPhysicalProcCountWithGapInPhysicalIDs()
    {
        // Test for WI 44326: Gaps in physical core IDs should not confuse count

#if defined(linux)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetCpuInfoFileType(44326);
#elif defined(aix)
        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(2);
#elif defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->Reset();
        deps->AddInstance(1, 1);
        deps->AddInstance(2, 5);
        deps->AddInstance(3, 1);
        deps->AddInstance(4, 5);
#endif

#if defined(linux) || defined(aix) || defined(sun) || defined(HPUX_PHYSICAL_PROC_COUNT_SUPPORTED)
        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif
    }

    void testPhysicalProcCountWithMassiveProcessors()
    {
        // AIX-specific test

#if defined(aix)
        // 'lsdev -c processor' shows:
        //    proc0  Available 00-00 Processor
        //    proc4  Available 00-04 Processor
        //    proc8  Available 00-08 Processor
        //    proc12 Available 00-12 Processor
        //    proc16 Available 00-16 Processor
        //    proc20 Available 00-20 Processor
        //    proc24 Available 00-24 Processor
        //    proc28 Available 00-28 Processor
        //    proc32 Available 00-32 Processor
        //    proc36 Available 00-36 Processor
        //    proc40 Available 00-40 Processor
        //    proc44 Available 00-44 Processor
        //
        // With this implemention, the sort of bug we had is no longer possible.
        // But at least set a large number of disabled processors and validate.

        // Mock dependencies object
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->SetNumProcs(44);
        deps->SetDisabledProcs(32);

        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(12), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));
#endif // defined(aix)
    }

    void testPhysicalLogicalProcCountsWithDynamicCPUs()
    {
        // Solaris-specific test for wi592494
        //
        // On Solaris, dynamic CPUs need not start with instance 0, and need not
        // be monotonically increasing.  Build a test case for this and be certain
        // that we get the correct counts of physical and logical CPUs.
        //
        // See associated WI for actual kstat output on associated system.

#if defined(sun)
        SCXHandle<CPUPALTestDependencies> deps(new CPUPALTestDependencies());
        deps->Reset();
        deps->AddInstance(  7, 0);
        deps->AddInstance( 13, 0);
        deps->AddInstance( 30, 0);
        deps->AddInstance( 41, 0);
        deps->AddInstance( 52, 0);
        deps->AddInstance( 65, 1);
        deps->AddInstance( 67, 1);
        deps->AddInstance( 71, 1);
        deps->AddInstance( 88, 1);
        deps->AddInstance( 91, 1);
        deps->AddInstance( 95, 1);
        deps->AddInstance( 97, 1);
        deps->AddInstance(110, 1);
        deps->AddInstance(112, 1);
        deps->AddInstance(113, 1);
        deps->AddInstance(124, 1);
        deps->AddInstance(131, 2);
        deps->AddInstance(134, 2);
        deps->AddInstance(147, 2);
        deps->AddInstance(152, 2);
        deps->AddInstance(154, 2);
        deps->AddInstance(158, 2);
        deps->AddInstance(159, 2);
        deps->AddInstance(210, 3);
        deps->AddInstance(211, 3);
        deps->AddInstance(213, 3);
        deps->AddInstance(217, 3);
        deps->AddInstance(219, 3);
        deps->AddInstance(222, 3);
        deps->AddInstance(223, 3);
        deps->AddInstance(234, 3);
        deps->AddInstance(242, 3);

        // Verify that the physical count of processors matches
        SCXCoreLib::SCXLogHandle logH = SCXLogHandleFactory::GetLogHandle(s_LogModuleName);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), CPUEnumeration::ProcessorCountPhysical(deps, logH, true));

        // Verify that the logical count of processors matches
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(32), CPUEnumeration::ProcessorCountLogical(deps));

        // Finally, take snapshot, and verify the count of processors that way
        m_pEnum = new CPUEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->SampleData();
        m_pEnum->Update();

        // Verify the count of processors
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(32), m_pEnum->Size());

#endif // defined(sun)
    }

    void TestNoProcessorsOnlineDuringUpdate()
    {
        // Solaris specific test fow WI# 367214
        //
        // Note: Other platforms may have a similar bug, but there is currently
        //       no time to investigate this issue.

#if defined(sun)
        // Mock dependencies object
        static const int MAX_PROCESSORS = 1;
        SCXHandle<CPUPALTestDependenciesWI367214> deps(new CPUPALTestDependenciesWI367214(MAX_PROCESSORS - 1));

        // Setup the first call to update so that at least one CPU is added to
        // the enumeration.
        deps->SetStatus(P_ONLINE);

        // This test only *needs* to test the Update path.  However CPUEnumeration
        // requires a call to Init first.  On the other hand, Init calls Update.
        // So this first call tests Update and causes numProcessorsAvail to be added
        // to the enumeration.
        m_pEnum = new CPUEnumeration(deps);
        m_pEnum->Init();

        // Setup the second call to Update so that the system expects at least
        // one processor available, but because of "timing" there are none
        // available.  This will validate that both the remove loop and add
        // loops terminate normally.
        deps->SetStatus(-1);

        // Note: For any given call to Update, deps->p_online should not be called
        // more than 3 times per total number of processors.
        int maxCallCount = (MAX_PROCESSORS * 3) + deps->GetCurrentPOnlineCallCount();
        deps->EnablePOnlineCallChecking(maxCallCount);

        m_pEnum->Update(false);
#endif
    }

private:

    static void DoSleep(int seconds)
    {
#if defined(WIN32)
        Sleep(seconds*1000);
#else
        sleep(seconds);
#endif
    }

    static void *FullLoadCPU(void *arg_p)
    {
        bool volatile * stopLoad = static_cast<bool volatile *>(arg_p);
        while (!*stopLoad) ;
        return NULL;
    }

    static void *LoadCPU(void *arg_p)
    {
        bool volatile * stopLoad = static_cast<bool volatile *>(arg_p);
        pthread_t t1;

        while (!*stopLoad)
        {
            bool innerStopLoad = false;
            if (pthread_create(&t1, NULL, (pThreadFn) FullLoadCPU, &innerStopLoad) == 0)
            {
                DoSleep(1);
                innerStopLoad = true;
                pthread_join(t1, NULL);
            }

            if (!*stopLoad)
            {
                DoSleep(1);
            }
        }

        return NULL;
    }

    double ToDouble(const string& str)
    {
        double tmp;
        stringstream ss(str);

        ss >> tmp;

        return tmp;
    }

    unsigned int ToRound(const string& str)
    {
        unsigned int tmp = static_cast<int>(round(ToDouble(str)));

        return tmp;
    }

    void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " \n")
    {
        string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        string::size_type pos = str.find_first_of(delimiters, lastPos);

        while (string::npos != pos || string::npos != lastPos)
        {
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

private:
    static const unsigned int interval=10;

};

CPPUNIT_TEST_SUITE_REGISTRATION( CPUEnumeration_Test );

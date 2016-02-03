/*----------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Instances of Process Items

    \date        07-10-29 15:27:00

    PAL representation of a Process instance

*/
/*----------------------------------------------------------------------------*/
#ifndef PROCESSINSTANCE_H
#define PROCESSINSTANCE_H

#if defined(sun)
#include <unistd.h>
#include <procfs.h>
#endif // defined(sun)

#include <sys/param.h>

#if defined(linux)
#include <unistd.h>
#include <scxsystemlib/procfsreader.h>
#endif // defined(linux)

#if defined(hpux)
#include <sys/pstat.h>
#endif

#if defined(aix)

#ifdef _DEBUG
// workaround to make build going; proper fix to be provided later (wi 9584)
#define SCX_UNDEF_DEBUG
#undef _DEBUG
#endif

#include <sys/procfs.h>
#include <procinfo.h>

#ifdef SCX_UNDEF_DEBUG
#undef _DEBUG
#define _DEBUG
#undef SCX_UNDEF_DEBUG
#endif

#endif // defined(aix)

#include <string>

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/datasampler.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>

namespace SCXSystemLib
{
#ifdef linux

    struct LinuxProcStat {
        int processId;                           //!< %d  1
        char command[30];                        //!< %s
        char state;             //!< %c
        int parentProcessId;                     //!< %d
        int processGroupId;                      //!< %d  5
        int sessionId;                           //!< %d
        int controllingTty;                      //!< %d
        int terminalProcessId;                   //!< %d
        unsigned long flags;    //!< %lu
        unsigned long minorFaults;               //!< %lu 10
        unsigned long childMinorFaults;          //!< %lu
        unsigned long majorFaults;               //!< %lu
        unsigned long childMajorFaults;          //!< %lu
        unsigned long userTime;                  //!< %lu
        unsigned long systemTime;                //!< %lu 15
        long childUserTime;                      //!< %ld
        long childSystemTime;                    //!< %ld
        long priority;          //!< %ld
        long nice;              //!< %ld
        // dummy at this position, not read;    //!< %ld 20
        long intervalTimerValue;                 //!< %ld
        unsigned long startTime;                 //!< %lu
        unsigned long virtualMemSizeBytes;       //!< %lu
        long residentSetSize;                    //!< %ld
        unsigned long residentSetSizeLimit;      //!< %lu 25
        unsigned long startAddress;              //!< %lu
        unsigned long endAddress;                //!< %lu
        unsigned long startStackAddress;         //!< %lu
        unsigned long kernelStackPointer;        //!< %lu
        unsigned long kernelInstructionPointer;  //!< %lu 30
        unsigned long signal;   //!< %lu
        unsigned long blocked;  //!< %lu
        unsigned long sigignore;//!< %lu
        unsigned long sigcatch; //!< %lu
        unsigned long waitChannel;               //!< %lu 35
        unsigned long numPagesSwapped;           //!< %lu
        unsigned long cumNumPagesSwapped;        //!< %lu
        int exitSignal;                          //!< %d
        int processorNum;                        //!< %d
        unsigned long realTimePriority;          //!< %lu 40 (Since 2.5.19)
        unsigned long schedulingPolicy;          //!< %lu    (Since 2.5.19)

    private:
        static const int procstat_len = 40; //!< Number of fields not counting dummy

        /** The format string that should be supplied to fscanf() to read procstat_fields */
        static const char *scanstring;  /* =
        "%d %s %c %d %d %d %d %d %lu %lu " // 1 to 10
        "%lu %lu %lu %lu %lu %ld %ld %ld %ld %*ld " // 11 to 20
        "%ld %lu %lu %ld %lu %lu %lu %lu %lu %lu " // 21 to 30
        "%lu %lu %lu %lu %lu %lu %lu %d %d %lu %lu"; // 31 to 41
                                       */
    public:
        bool ReadStatFile(FILE *filePointer, const char* filename);
    };

    /** Holds Linux memory statistics */
    struct LinuxProcStatM {

        unsigned long size;     //!< total program size
        unsigned long resident; //!< resident set size
        unsigned long share;    //!< shared pages
        unsigned long text;     //!< text (code)
        unsigned long lib;      //!< library
        unsigned long data;     //!< data/stack

    private:
        static const int procstat_len = 6; //!< Number of fields

        /** The format string that should be supplied to fscanf() to read procstat_fields */
        static const char *scanstring;  /* = "%lu %lu %lu %lu %lu %lu" */
    public:
        bool ReadStatMFile(FILE *filePointer, const char* filename);
    };

#endif /* Linux */
}

/** Implements subtraction for system type struct timeval.
    \param tv1 A timeval
    \param tv2 Another timeval
    \returns Difference between tv1 and tv2
    This is present so that the DataSampler class can work correctly.
    std::vector<std::string> commands;
*/
inline struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2)
{
    struct timeval tmp;
    if (tv2.tv_usec > tv1.tv_usec) {
        tmp.tv_usec = tv1.tv_usec + 1000000 - tv2.tv_usec;
        tmp.tv_sec = tv1.tv_sec - tv2.tv_sec - 1;
    } else {
        tmp.tv_usec = tv1.tv_usec - tv2.tv_usec;
        tmp.tv_sec = tv1.tv_sec - tv2.tv_sec;
    }
    return tmp;
}

namespace SCXSystemLib
{
#if defined(sun)
    /** Datatype used for time in /proc is redefined into a custom type. */
    typedef timestruc_t scx_timestruc_t;
#elif defined(aix)
    /** Datatype used for time in /proc is redefined into a custom type. */
    typedef pr_timestruc64_t scx_timestruc_t;
#endif
}

#if defined(sun) || defined(aix)
    /** Implements subtraction for the type scx_timestruc_t.
        \param ts1 A scx_timestruc_t
        \param ts2 Another scx_timestruc_t
        \returns Difference between ts1 and ts2
        This is present so that the DataSampler class can work correctly.
    */
    inline SCXSystemLib::scx_timestruc_t operator-(const SCXSystemLib::scx_timestruc_t& ts1, const SCXSystemLib::scx_timestruc_t& ts2)
    {
        SCXSystemLib::scx_timestruc_t tmp;
        if (ts2.tv_nsec > ts1.tv_nsec) {
            tmp.tv_nsec = ts1.tv_nsec + 1000000000UL - ts2.tv_nsec;
            tmp.tv_sec = ts1.tv_sec - ts2.tv_sec - 1;
        } else {
            tmp.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
            tmp.tv_sec = ts1.tv_sec - ts2.tv_sec;
        }
        return tmp;
    }


    /** Implements addition for the type scx_timestruc_t.
        \param ts1 A scx_timestruc_t
        \param ts2 Another scx_timestruc_t
        \returns Sum of ts1 and ts2
    */
    inline SCXSystemLib::scx_timestruc_t operator+(const SCXSystemLib::scx_timestruc_t& ts1, const SCXSystemLib::scx_timestruc_t& ts2)
    {
        SCXSystemLib::scx_timestruc_t tmp;
        tmp.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
        tmp.tv_sec = ts1.tv_sec + ts2.tv_sec;

        if (tmp.tv_nsec > 1000000000UL) {
            tmp.tv_nsec -= 1000000000UL;
            tmp.tv_sec += 1;
        std::vector<std::string> commands;
        }
        return tmp;
    }
#endif // defined(sun) || defined(aix)


/*----------------------------------------------------------------------------*/

namespace SCXSystemLib
{
    typedef scxulong scxpid_t;  //!< Internal type of process id

    /** Number of samples collected in the datasampler for CPU. */
    const size_t MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES = 6;

    /** Datasampler for CPU information. */
    typedef DataSampler<scxulong> ScxULongDataSampler_t;
    /** Datasampler for time stored as a struct timeval */
    typedef DataSampler<struct timeval> TvDataSampler_t;
#if defined(sun) || defined(aix)
    /** Datasampler for time stored as a scx_timestruc_t (which is specific for solaris&aix) */
    typedef DataSampler<scx_timestruc_t> TsDataSampler_t;
#endif

    /*----------------------------------------------------------------------------*/

    /**
        Class that represents an instance of a unix process.

        Concrete implementation of an instance of a Process entity
    */
    class ProcessInstance : public EntityInstance
    {
        friend class ProcessEnumeration;

        static const wchar_t *moduleIdentifier;         //!< Shared module string

    protected:
        // This constructor is added for unit-test purposes (See WI 516119).
        // Never use this for general use; it is solely for unit testing specific issues!
        ProcessInstance(const std::string &cmd, const std::string &params)
#if defined(linux)
          : m_RealTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_UserTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_SystemTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_HardPageFaults_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES)
#elif defined(sun)
          : m_RealTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_UserTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_SystemTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_BlockOut_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_BlockInp_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_HardPageFaults_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES)
#elif defined(hpux)
          : m_RealTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_UserTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_SystemTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_BlockOut_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_BlockInp_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_HardPageFaults_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES)
#elif defined(aix)
          : m_RealTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_UserTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES),
            m_SystemTime_tics(MAX_PROCESSINSTANCE_DATASAMPER_SAMPLES)
#endif
        {
#if defined(linux)
            strncpy(m.command, cmd.c_str(), sizeof(m.command));
            m.command[sizeof(m.command)-1]='\0';
#elif defined(aix) || defined(sun)
            strncpy(m_psinfo.pr_fname, cmd.c_str(), sizeof(m_psinfo.pr_fname));
            m_psinfo.pr_fname[sizeof(m_psinfo.pr_fname)-1]='\0';
#elif defined(hpux)
            strncpy(m_pstatus.pst_ucomm, cmd.c_str(), sizeof(m_pstatus.pst_ucomm));
            m_pstatus.pst_ucomm[sizeof(m_pstatus.pst_ucomm)-1]='\0';
#endif
            if (!params.empty())
            {
                m_params.push_back(params);
            }
        }

#if defined(sun)
    protected:
        // These functions are virtual for refactoring and/or test purposes

        virtual bool ReadProcessInfo();
        virtual bool ReadUsageInfo();
        virtual bool ReadStatusInfo();
        virtual bool isInGlobalZone();
#endif // defined(sun)

#if defined(linux)
    private:
        void SetBootTime(void);
#endif

#if defined(linux) || defined(sun)
    protected:
        ProcessInstance(scxpid_t pid, const char* basename);
        bool UpdateInstance(const char* basename, bool initial);
#endif // defined(linux) || defined(sun)

#if defined(aix)
    protected:
        ProcessInstance(scxpid_t pid, struct procentry64 *pinfo);
        bool UpdateInstance(struct procentry64 *pinfo, bool initial);
#endif // defined(aix)

#if defined(hpux)
    protected:
        ProcessInstance(scxpid_t pid, struct pst_status *pstatus);
        bool UpdateInstance(struct pst_status *pstatus, bool initial);
#endif // defined(hpux)

    private:
        bool UpdateParameters(void);
        std::vector<std::string> m_params;

    public:
        /** Gets the process ID which this instance represents. */
        scxpid_t getpid() { return m_pid; }

        virtual ~ProcessInstance();

        static bool m_inhibitAccessViolationCheck;

        /* Properties in SCX_UnixProcess */
        bool GetPID(scxulong& x) const;
        bool GetName(std::string&) const;
        bool GetUserName(std::wstring& username) const;
        bool GetNormalizedWin32Priority(unsigned int& prio) const;
        bool GetNativePriority(int& prio) const;
        bool GetExecutionState(unsigned short& state) const;
        bool GetCreationDate(SCXCoreLib::SCXCalendarTime& cre) const;
        bool GetTerminationDate(SCXCoreLib::SCXCalendarTime& term) const;
        bool GetParentProcessID(int& pid) const;
        bool GetRealUserID(scxulong& uid) const;
        bool GetProcessGroupID(scxulong& pgid) const;
        bool GetProcessNiceValue(unsigned int& nice) const;

        /* Properties in SCX_UnixProcess, Phase 2 */
        bool GetOtherExecutionDescription(std::wstring& description) const;
        bool GetKernelModeTime(scxulong& kmt) const;
        bool GetUserModeTime(scxulong& umt) const;
        bool GetWorkingSetSize(scxulong& wss) const;
        bool GetProcessSessionID(scxulong& sid) const;
        bool GetProcessTTY(std::string& tty) const;
        bool GetModulePath(std::string& modpath) const;
        bool GetParameters(std::vector<std::string>& params) const;
        bool GetProcessWaitingForEvent(std::string& event) const;

        /* Properties in SCX_UnixProcessStatisticalInformation */
        bool GetCPUTime(unsigned int& cpu) const;
        bool GetBlockWritesPerSecond(scxulong &bws) const;
        bool GetBlockReadsPerSecond(scxulong &bwr) const;
        bool GetBlockTransfersPerSecond(scxulong &bts) const;
        bool GetPercentUserTime(scxulong &put) const;
        bool GetPercentPrivilegedTime(scxulong &ppt) const;
        bool GetUsedMemory(scxulong &um) const;
        bool GetPercentUsedMemory(scxulong &) const;
        bool GetPagesReadPerSec(scxulong &prs) const;

        /* Properties in SCX_UnixProcessStatisticalInformation, Phase 2 */
        bool GetRealText(scxulong &rt) const;
        bool GetRealData(scxulong &rd) const;
        bool GetRealStack(scxulong &rs) const;
        bool GetVirtualText(scxulong &vt) const;
        bool GetVirtualData(scxulong &vd) const;
        bool GetVirtualStack(scxulong &vs) const;
        bool GetVirtualMemoryMappedFileSize(scxulong &vmmfs) const;
        bool GetVirtualSharedMemory(scxulong &vsm) const;
        bool GetVirtualSize(scxulong &vs) const;
        bool GetCpuTimeDeadChildren(scxulong &ctdc) const;
        bool GetSystemTimeDeadChildren(scxulong &stdc) const;

        /* Utility stuff */
        bool SendSignal(int signl) const;

        std::wstring DumpString(void);
    private:
        /** Tests if this instance was detected when scanning live processes. */
        bool WasFound() { bool found = m_found; m_found = false; return found; }
        void UpdateDataSampler(struct timeval& realtime);
        void UpdateTimedValues(void);
        void CheckRootAccess(void) const;

        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
        scxpid_t m_pid;                         //!< Process ID of this instance
        bool m_found;                           //!< Found during iteration
        bool m_accessViolationEncountered;      //!< Flag that we've had problems with access
        struct timeval m_timeOfDeath;           //!< When did process die

        bool m_scxPriorityValid;                //!< Native priority successfuly mapped to windows priority levels.
        unsigned int m_scxPriority;             //!< Value of the native priority mapped to windows priority levels.
        template<class t> void PriorityOutOfRangeError(t rawPriority);  // Error handling helper.
#if defined(linux)
        char m_procStatName[MAXPATHLEN];        //!< Name of /proc/#/stat file
        char m_procStatMName[MAXPATHLEN];       //!< Name of /proc/#/statm file
        uid_t     m_uid;                        //!< User ID of owner
        gid_t     m_gid;                        //!< Group ID of owner
        LinuxProcStat m;                        //!< Linux specific process information
        LinuxProcStatM n;                       //!< Linux specific process information
        static SCXCoreLib::SCXCalendarTime m_system_boot; //!< Time of system boot
        unsigned int m_jiffies_per_second;              //!< Time base for PC Linux
        static const unsigned int m_pageSize = 4;       //!< Page size in KB on Linux

        TvDataSampler_t       m_RealTime_tics;          //!< Data sampler for real time.
        ScxULongDataSampler_t m_UserTime_tics;          //!< Data sampler for user time.
        ScxULongDataSampler_t m_SystemTime_tics;        //!< Data sampler for system time.
        ScxULongDataSampler_t m_HardPageFaults_tics;    //!< Data sampler for hard page faults.

        /* These are updated when UpdateTimedValues() is run. */
        struct timeval m_delta_RealTime;                //!< Elapsed real time at update
        scxulong m_delta_UserTime;                      //!< Consumed user time at update
        scxulong m_delta_SystemTime;                    //!< Consumed system time at update
        scxulong m_delta_HardPageFaults;                //!< Executed page faults at update

        scxulong ComputeItemsPerSecond(scxulong delta_item,             // Defined below
                                       const struct timeval& elapsedTime) const;
        unsigned int ComputePercentageOfTime(scxulong consumedTime,     // Defined below
                                             const struct timeval& elapsedTime) const;

        bool LinuxProcessPriority2SCXProcessPriority(long linuxPriority, unsigned int &scxPriority);
#endif

#if defined(sun)
        char m_procPsinfoName[MAXPATHLEN];      //!< Name of /proc/#/psinfo file
        char m_procStatusName[MAXPATHLEN];      //!< Name of /proc/#/status file (protected)
        char m_procUsageName[MAXPATHLEN];       //!< Name of /proc/#/usage file

        bool m_logged64BitError;                //!< Has a warning log about problems reading 64-bit process info been issued

    protected:
        // These structures are protected for test purposes

        psinfo_t m_psinfo;                      //!< Solaris specific process information
        pstatus_t m_pstat;                      //!< Solaris specific process information
        prusage_t m_puse;                       //!< Solaris specific process information

    private:
        unsigned long m_clocksPerSecond;        //!< System clock ticks per second

        TvDataSampler_t m_RealTime_tics;                //!< Data sampler for real time.
        TsDataSampler_t m_UserTime_tics;                //!< Data sampler for user time.
        TsDataSampler_t m_SystemTime_tics;              //!< Data sampler for system time.
        ScxULongDataSampler_t m_BlockOut_tics;          //!< Data sampler for written blocks.
        ScxULongDataSampler_t m_BlockInp_tics;          //!< Data sampler for read blocks.
        ScxULongDataSampler_t m_HardPageFaults_tics;    //!< Data sampler for hard page faults.

        /* These are updated when UpdateTimedValues() is run. */
        struct timeval m_delta_RealTime;                //!< Elapsed real time at update
        scx_timestruc_t m_delta_UserTime;               //!< Consumed user time at update
        scx_timestruc_t m_delta_SystemTime;             //!< Consumed system time at update
        scxulong m_delta_BlockOut;                      //!< Elapsed block outputs at update
        scxulong m_delta_BlockInp;                      //!< Elapsed block inputs at update
        scxulong m_delta_HardPageFaults;                //!< Executed page faults at update

        scxulong ComputeItemsPerSecond(scxulong delta_item,             // Defined below
                                       const timeval& elapsedTime) const;
        unsigned int ComputePercentageOfTime(const scx_timestruc_t& consumedTime,//  Below
                                             const struct timeval& elapsedTime) const;

        bool SolarisProcessPriority2SCXProcessPriority(int solarisPriority, unsigned int &scxPriority);

        /**
           Helper class to insure that a file descriptor is closed properly regardless of exceptions
        */

        class AutoClose {
        public:
            /*
              AutoClose constructor
              \param[in]    fd      File descriptor to close
            */
            AutoClose(SCXCoreLib::SCXLogHandle log, int fd) : m_log(log), m_fd(fd) {}
            ~AutoClose();

            SCXCoreLib::SCXLogHandle m_log;     //!< Log handle.
            int m_fd;       //!< File descriptor
        };
#endif // defined(sun)

     private:
        inline bool StoreModuleAndArgs(const std::wstring & module, const std::wstring & args);

#if defined(hpux)
    private:
        struct pst_status m_pstatus;            //!< HP/UX specific process information
        static const unsigned int m_pageSize = 4; //!< Page size in KB on HP/UX

        TvDataSampler_t       m_RealTime_tics;          //!< Data sampler for real time.
        ScxULongDataSampler_t m_UserTime_tics;          //!< Data sampler for user time.
        ScxULongDataSampler_t m_SystemTime_tics;        //!< Data sampler for system time.
        //ScxULongDataSampler_t m_CPUTime_tics;           //!< Data sampler for cpu time.
        //ScxULongDataSampler_t m_CPUTime_total_tics;     //!< Data sampler for process life time.
        ScxULongDataSampler_t m_BlockOut_tics;          //!< Data sampler for written blocks.
        ScxULongDataSampler_t m_BlockInp_tics;          //!< Data sampler for read blocks.
        ScxULongDataSampler_t m_HardPageFaults_tics;    //!< Data sampler for hard page faults.

        /* These are updated when UpdateTimedValues() is run. */
        struct timeval m_delta_RealTime;                //!< Elapsed real time at update
        scxulong m_delta_UserTime;                      //!< Consumed user time at update
        scxulong m_delta_SystemTime;                    //!< Consumed system time at update

        //scxulong m_delta_CPUTime;
        //scxulong m_delta_CPUTime_total;

        scxulong m_delta_BlockOut;                      //!< Elapsed block outputs at update
        scxulong m_delta_BlockInp;                      //!< Elapsed block inputs at update
        scxulong m_delta_HardPageFaults;                //!< Executed page faults at update

        scxulong ComputeItemsPerSecond(scxulong delta_item,             // Defined below
                                       const struct timeval& elapsedTime) const;
        unsigned int ComputePercentageOfTime(scxulong consumedTime,     // Defined below
                                             const struct timeval& elapsedTime) const;

        bool HPUXProcessPriority2SCXProcessPriority(_T_LONG_T hpuxPriority, unsigned int &scxPriority);
#endif

#if defined(aix) || defined(hpux)
    private:
        bool ModulePathFromCommand(std::wstring  const &exeFName, std::wstring const &fullCommand);
        bool FindModuleFromPath(std::wstring const &fname);
        std::wstring m_modulePath;                 //   As derived from m_psinfo
        std::wstring m_name;                       //   As derived from the command line
#endif

#if defined(aix)
    private:
        char m_procPsinfoName[MAXPATHLEN];        //!< Name of /proc/#/psinfo file
        char m_procStatusName[MAXPATHLEN];        //!< Name of /proc/#/status file (protected)

        // Sparse version of much larger procentry64 struct (5024 byte).
        struct ProcEntry
        {
            unsigned int pi_pri;
            unsigned int pi_nice;
        };
        ProcEntry m_procentry;

        psinfo_t m_psinfo;                      //!< AIX specific process information

        // Sparse version of much larger pstatus_t type (1520 bytes).
        struct PStatus
        {
           uint64_t pr_brksize;
           uint64_t pr_stksize;
           pr_timestruc64_t pr_cstime;
           pr_timestruc64_t pr_cutime;
           pr_timestruc64_t pr_utime;
           pr_timestruc64_t pr_stime;
        };
        PStatus m_pstat;

        unsigned long m_clocksPerSecond;        //!< System clock ticks per second

        TvDataSampler_t m_RealTime_tics;                //!< Data sampler for real time.
        TsDataSampler_t m_UserTime_tics;                //!< Data sampler for user time.
        TsDataSampler_t m_SystemTime_tics;              //!< Data sampler for system time.

        /* These are updated when UpdateTimedValues() is run. */
        struct timeval m_delta_RealTime;                //!< Elapsed real time at update
        scx_timestruc_t m_delta_UserTime;              //!< Consumed user time at update
        scx_timestruc_t m_delta_SystemTime;            //!< Consumed system time at update

        scxulong ComputeItemsPerSecond(scxulong delta_item,             // Defined below
                                       const timeval& elapsedTime) const;
        unsigned int ComputePercentageOfTime(const scx_timestruc_t& consumedTime,// Inline
                                             const struct timeval& elapsedTime) const;

        bool AIXProcessPriority2SCXProcessPriority(uint aixPriority, unsigned int &scxPriority);
#endif

    };

    /**
       Constructs a identity string for debug printouts

       This is exclusively meant for debug output. It will output pid and short name
       for process.
     */
    inline std::wstring ProcessInstance::DumpString(void)
    {
#if defined(linux)
        std::string name(m.command);
        long pid = m.processId;
#elif defined(sun)
        std::string name(m_psinfo.pr_fname);
        long pid = m_psinfo.pr_pid;
#elif defined(hpux)
        std::string name(m_pstatus.pst_ucomm);
        long pid = m_pstatus.pst_pid;
#elif defined(aix)
        std::string name(m_psinfo.pr_fname);
        long pid = m_psinfo.pr_pid;
#else
        #error "Not supported"
#endif
        std::stringstream ss;
        if (static_cast<long>(m_pid) != pid) {
            ss << "<" << m_pid << ":" << pid << ">" << name;
        } else {
            ss << "<" << m_pid << ">" << name;
        }
        return SCXCoreLib::StrFromUTF8(ss.str());
    }

#if defined(linux)
    /**
       Computes a parameter for which we have a delta of some kind of item
       and a time delta.

       \param delta_item Delta of usage
       \param elapsedTime Delta of time
       \returns A measure of items per second

       \note If we ever need this in something else that scxulong then
       make this a template.
    */
    inline scxulong
    ProcessInstance::ComputeItemsPerSecond(scxulong delta_item,
                                           const struct timeval& elapsedTime) const
    {
        // Convert elapsed time to milliseconds
        scxulong el = 1000 * elapsedTime.tv_sec + elapsedTime.tv_usec / 1000;
        if (el == 0) return 0;  // Avoid divide by zero

        return 1000 * delta_item / el;
    }


    /**
       Computes the percentage of measure of time in relation to another measure of time.

       \param consumedTime Consumed time
       \param elapsedTime Elapsed time
       \returns A percentage number on how consumed time relates to elapsed time

       \note Delta time on Linux is in jiffies. Like it or not.
    */
    inline unsigned int
    ProcessInstance::ComputePercentageOfTime(scxulong consumedTime,
                                             const struct timeval& elapsedTime) const
    {
        // Convert both to milliseconds to get a resonable resolution WO overflow
        unsigned long el = 1000 * elapsedTime.tv_sec + elapsedTime.tv_usec / 1000;
        if (el == 0) { return 0; } // Avoid divide by zero
        scxulong co = 1000 * consumedTime / m_jiffies_per_second;
        return static_cast<unsigned int>(100 * co / el);
    }

#endif /* linux */

#if defined(sun) || defined(aix)
    /**
       Computes a parameter for which we have a delta of some kind of item
       and a time delta.

       \param delta_item Delta of usage
       \param elapsedTime Delta of time
       \returns A measure of items per second

       \note If we ever need this in something else that scxulong then
       make this a template.
    */
    inline scxulong
    ProcessInstance::ComputeItemsPerSecond(scxulong delta_item,
                                           const timeval& elapsedTime) const
    {
        // Convert elapsed time to milliseconds
        scxulong el = 1000 * elapsedTime.tv_sec + elapsedTime.tv_usec / 1000;
        if (el == 0) return 0;  // Avoid divide by zero

        return 1000 * delta_item / el;
    }


    /**
       Computes the percentage of measure of time in relation to another measure of time.

       \param consumedTime Consumed time
       \param elapsedTime Elapsed time
       \returns A percentage number on how consumed time relates to elapsed time
    */
    inline unsigned int
    ProcessInstance::ComputePercentageOfTime(const scx_timestruc_t& consumedTime,
                                             const struct timeval& elapsedTime) const
    {
        if (elapsedTime.tv_sec == 0 && elapsedTime.tv_usec == 0) { return 0; }
        // We convert both values to floating point to get around rounding problems
        double el = elapsedTime.tv_sec + elapsedTime.tv_usec / 1000000.0L;
        if (!(el > 0)) { return 0; } // Avoid floating exceptions
        double co = consumedTime.tv_sec + consumedTime.tv_nsec / 1000000000.0L;
        return static_cast<unsigned int>(100.0L * co / el);
    }

#endif /* sun || aix */

#if defined(hpux)
    /**
       Computes a parameter for which we have a delta of some kind of item
       and a time delta.

       \param delta_item Delta of usage
       \param elapsedTime Delta of time
       \returns A measure of items per second

       \note If we ever need this in something else that scxulong then
       make this a template.
    */
    inline scxulong
    ProcessInstance::ComputeItemsPerSecond(scxulong delta_item,
                                           const struct timeval& elapsedTime) const
    {
        // Convert elapsed time to milliseconds
        scxulong el = 1000 * elapsedTime.tv_sec + elapsedTime.tv_usec / 1000;
        if (el == 0) return 0;  // Avoid divide by zero

        return 1000 * delta_item / el;
    }

    /**
       Computes the percentage of measure of time in relation to another measure of time.

       \param consumedTime Consumed time
       \param elapsedTime Elapsed time
       \returns A percentage number on how consumed time relates to elapsed time
    */
    inline unsigned int
    ProcessInstance::ComputePercentageOfTime(scxulong consumedTime,
                                             const struct timeval& elapsedTime) const
    {
        // Convert both to milliseconds to get a resonable resolution WO overflow
        unsigned long el = 1000 * elapsedTime.tv_sec + elapsedTime.tv_usec / 1000;
        if (el == 0) { return 0; } // Avoid divide by zero

        unsigned long co = 1000 * consumedTime;
        return static_cast<unsigned int>(100 * co / el);
    }
#endif /* hpux */
}

#endif /* PROCESSINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

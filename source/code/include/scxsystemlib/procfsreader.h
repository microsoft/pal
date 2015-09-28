/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        procfsreader.h

    \brief       Declares the abstraction of a procfs table on Linux. 
                 Example: /proc/cpuinfo is a table of properties each with form "property\t: value"
                 The /proc/cpuinfo can be read with ProcfsCpuInfoReader and stored as ProcfsCpuInfo.
    
    \date        2011-11-15 15:09:00

    
*/
/*----------------------------------------------------------------------------*/
#ifndef PROCCPUINFO_H
#define PROCCPUINFO_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxfile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>


namespace SCXSystemLib
{

    typedef scxulong scxpid_t;

/*----------------------------------------------------------------------------*/
/**
 * ProcfsTableReader declaration.
 * ProcfsTableReader class: Helper class to load data from cpu or process info
 * 
 */
template<class T>
class ProcfsTableReader 
{
private:
    static const unsigned short MAX_KEY_CHARS = 0x20;
    static const unsigned short MAX_PROPERTY_CHARS = 0x200;
    //vector to restore cpu/process info
    std::vector<T> m_procfsVector;
public:
    typedef T* iterator;
        typedef const T* const_iterator;

    ProcfsTableReader(void) {}
    virtual ~ProcfsTableReader(void) {}
        
    T * begin()
        {
            return &m_procfsVector[0];
        }

        T* end()
        {
        return &m_procfsVector[0] + m_procfsVector.size();
        }
        
protected:
    /**
    \brief     Load objects and their properties from procfs file.
    \returns   true = success, false = failure
    */
    bool LoadFile(std::wistream & ifs)
    {
        bool fRet = false;
        std::vector<std::wstring> matches;
        wchar_t cpuInfoProperty[ProcfsTableReader<T>::MAX_PROPERTY_CHARS];
        wchar_t cpuInfoKey[ProcfsTableReader<T>::MAX_KEY_CHARS];
        wchar_t cpuInfoValue[ProcfsTableReader<T>::MAX_PROPERTY_CHARS];
        bool fRecordStart = false;

        if ( !ifs.good() )
        {
            return fRet;
        }

        m_procfsVector.push_back(T());

        while ( true )
        {
            const char keyValueSep = ':';
            const size_t MAXLINECC = (sizeof(cpuInfoProperty)/sizeof(cpuInfoProperty[0]));
            const size_t MAXKEYCC = (sizeof(cpuInfoKey)/sizeof(cpuInfoKey[0]));

            ifs.getline(cpuInfoProperty, MAXLINECC);
            if ( !ifs.good() )
            {
                break;
            }

            // Processor records are terminated by double new line characters,
            // including the last record.
            if (cpuInfoProperty[0] == '\0')
            {
                fRecordStart = true;
                continue;
            }

            if (fRecordStart)
            {
                m_procfsVector.push_back(T());
                fRecordStart = false;
            }

            std::wstringstream ss(cpuInfoProperty);
            
            cpuInfoKey[0] = '\0';
            ss.getline(cpuInfoKey, MAXKEYCC, keyValueSep);
            if (!ss)
            {
                break;
            }
            const std::wstring& key = SCXCoreLib::StrTrim(cpuInfoKey);

            cpuInfoValue[0] = '\0';
            ss.getline(cpuInfoValue, MAXLINECC);
            // Values can be empty; bad bit not checked.
            const std::wstring& value = SCXCoreLib::StrTrim(cpuInfoValue);

            if (m_procfsVector.back().AddPair(key, value))
            {
                // ProcfsTableReader success on at least one successful
                // property.
                fRet = true;
            }
        }

        return fRet;
    }

};

/*----------------------------------------------------------------------------*/
/**
 * ProcfsTable declaration.
 */
class ProcfsTable
{
public:
    ProcfsTable(void) : m_fEmpty(true) {}
    virtual ~ProcfsTable(void) {}

    virtual bool AddPair(const std::wstring& sProperty, const std::wstring& sValue);

    bool empty(void) const { return m_fEmpty; }

protected:
    
    // Key type.
    typedef unsigned short propertyid;
    
    // The look-up table enforces membership in a set of key properties
    typedef std::map<std::wstring, propertyid> LookupTable;
    
    // Iterator for LookupTable
    typedef LookupTable::const_iterator LookupTableCIT;
    
    // To avoid redundant string compares, we translate keys to enum type.
    typedef LookupTable::value_type LookupEntry;
    
    // The properties are mapped from property id to string value.
    typedef std::map<propertyid, std::wstring> PropertyTable;

    virtual LookupTableCIT LookupProperty(const std::wstring&) = 0;
    virtual bool End(LookupTableCIT) = 0;
    virtual void Insert(propertyid, const std::wstring&) = 0;

    template<class T> bool GetSimpleField(T&, propertyid) const;
    bool GetCompoundField(std::wstring&, propertyid) const;

    PropertyTable            m_Properties;
    bool  m_fEmpty;
};

/*----------------------------------------------------------------------------*/
/**
 * ProcfsCpuInfo declaration.
 *
 * No need to have an iterator because each property will
 * have an accessor.
 */
class ProcfsCpuInfo : public ProcfsTable
{
    friend class ProcfsTableReader<ProcfsCpuInfo>;

public:
    ProcfsCpuInfo(void);
    virtual ~ProcfsCpuInfo(void);

    // Accessors
    inline const std::wstring& Id(void) const { return m_Id; }
    bool Processor(unsigned short&) const;     // "processor"
    bool ProcessorType(unsigned short&) const; // calculated field
    bool AddressSizePhysical(unsigned short&); // "address sizes" Example: 38 bits physical, 48 bits virtual
    bool AddressSizeVirtual(unsigned short&);
    bool Architecture(unsigned short&);        // calculated field
    bool Bogomips(unsigned long&);             // "bogomips" Example: 4989.74
    bool CacheAlignment(unsigned short&);      // "cache_alignment" Example: 64
    bool CacheSize(unsigned long&);            // "cache size" Units KB
    bool CpuidLevel(unsigned short&);          // "cpuid level"
    bool CoreId(unsigned short&);              // "core id"
    bool ClFlushSize(unsigned long&);          // "clflush size" Example: 64
    bool CpuCores(unsigned int&) const;        // "cpu cores"
    bool CpuFamily(unsigned short&) const;     // "cpu family" Example: 6
    bool CpuSpeed(unsigned int&) const;        // "cpu MHz" Example: 2493.774
    bool Fpu(bool&);                           // "fpu"
    bool FpuException(bool&);                  // "fpu_exception"
    bool PhysicalId(unsigned short&) const;    // "physical id"
    bool Model(unsigned short&) const;         // "model" Example: 23
    bool ModelName(std::wstring&) const;          // "model name" Example: Intel(R) Xeon(R) CPU           E5410  @ 2.33GHz
    bool Siblings(unsigned short&);            // "siblings"
    bool Stepping(unsigned short&) const;      // "stepping" Example: 10
    bool VendorId(std::wstring&) const;           // "vendor_id" Example: GenuineIntel
    bool Wp(bool&);                            // "wp"
    bool Is64Bit(void) const;                  // flags lm
    bool IsHyperthreadingCapable(void) const;  // flags ht (it's possible to have a ht CPU with no siblings)
    bool IsHyperthreadingEnabled(void) const;  // # physical cpus != # logical cpus
    bool IsVirtualizationCapable(void) const;  // flags vme
    bool Version(std::wstring& version) const;    // Has form "Model # Stepping #"
    bool Role(unsigned short&) const;          // calculated field
    const std::wstring& CpuKey(void) const;       // Has form "CPU #" where # is from processor

    // Mutators
    void HyperthreadingEnabled(bool);          // mutator

protected:
    virtual ProcfsTable::LookupTableCIT LookupProperty(const std::wstring&);
    virtual bool End(LookupTableCIT);
    virtual void Insert(propertyid key, const std::wstring& value);

private:

    void LoadFlags(void);


    static const LookupTable m_PropertyLookup;
    static const LookupEntry m_PropertyPairs[];

    std::wstring                m_Id;

    bool                     m_HyperThreadingEnabled;

    typedef enum CpuPropertyId_t
    {
        PROCESSOR,
        ADDRESS_SIZES,
        BOGOMIPS,
        CACHE_ALIGNMENT,
        CACHE_SIZE,
        CORE_ID,
        CPUID_LEVEL,
        CLFLUSH_SIZE,
        CPU_CORES,
        CPU_FAMILY,
        CPU_SPEED,
        FLAGS,
        FPU,
        FPU_EXCEPTION,
        PHYSICAL_ID,
        MODEL,
        MODEL_NAME,
        SIBLINGS,
        STEPPING,
        VENDOR_ID,
        WP,
        POWER_MANAGEMENT
    } CpuPropertyId;

    typedef enum ProcessorPrimaryType_t
    {
        OTHER_PPROCESSOR   = 1,
        UNKNOWN_PPROCESSOR = 2, 
        CENTRAL_PPROCESSOR = 3, 
        MATH_PPROCESSOR    = 4,
        DSP_PPROCESSOR     = 5,
        VIDEO_PPROCESSOR   = 6
    } ProcessorPrimaryType;

    std::set<std::wstring> m_Flags;

    static const unsigned short CENTRAL_PROCESSOR_ROLE=2;

    SCXCoreLib::SCXLogHandle m_log;
};

/*----------------------------------------------------------------------------*/
/**
 * ProcfsProcStatus represents one process status as read from /proc/[pid]/status
 *
 * Each property will have an accessor.
 */
class ProcfsProcStatus : public ProcfsTable
{
    friend class ProcfsTableReader<ProcfsProcStatus>;

public:
    ProcfsProcStatus(void);
    virtual ~ProcfsProcStatus(void);

    // Accessors
    bool Name(std::wstring&) const;
    bool State(unsigned short& state) const;
    bool Tgid(scxulong&) const;
    bool Pid(scxpid_t&) const;
    bool PPid(scxpid_t&) const;
    bool TracerPid(scxpid_t&) const;
    bool Uid(uid_t &real, uid_t & effective, uid_t & saved, uid_t & filesystem) const;
    bool Gid(scxulong & real, scxulong & effective, scxulong & saved, scxulong & filesystem) const;
    bool Utrace(scxulong& trace) const;
    bool FDSize(scxulong&) const;
    bool Groups(std::vector<unsigned int> &) const;
    bool VmPeak(scxulong&) const;
    bool VmSize(scxulong&) const;
    bool VmLck(scxulong&) const;
    bool VmHWM(scxulong&) const;
    bool VmRSS(scxulong&) const;
    bool VmData(scxulong&) const;
    bool VmStk(scxulong&) const;
    bool VmExe(scxulong&) const;
    bool VmLib(scxulong&) const;
    bool VmPTE(scxulong&) const;
    bool VmSwap(scxulong&) const;
    bool Threads(scxulong&) const;
    bool SigQ(scxulong& current, scxulong& max) const;
    bool Cpus_allowed(scxulong& map) const;
    bool VoluntaryContextSwitches(scxulong &) const;
    bool NonVoluntaryContextSwitches(scxulong &) const;
    // Defined in /proc/[pid]/status but not yet supported:
    // SigPnd, ShdPnd, SigBlk, SigIgn, SigCgt, CapInh, CapPrm, CapEff, CapBnd, Cpus_allowed_list, Mems_allowed, Mems_allowed_list

    // Process state as returned by public accessor State() 
    typedef enum ProcessState_t
    {
        SLEEP_UNINTERRUPT, // D  The process is in uninterruptible sleep.  
        RUNNABLE,          // R  The process is running.
        SLEEPING,          // S  The process is sleeping, waiting for some event. it does not consume CPU time.  
        STOPPED,           // T  The process is stopped.
        TERMINATED,        // X  The process has terminated.
        ZOMBIE             // Z  A zombie process.  
    } ProcessState;

    // Mutators
    //  * None *

protected:
    virtual ProcfsTable::LookupTableCIT LookupProperty(const std::wstring&);
    virtual bool End(LookupTableCIT);
    virtual void Insert(propertyid key, const std::wstring& value);

private:

    static const LookupTable m_PropertyLookup;
    static const LookupEntry m_PropertyPairs[];

    typedef enum ProcStatusId_t
    {
        NAME,
        STATE,
        TGID,
        PID,
        PPID,
        TRACERPID,
        UID,
        GID,
        UTRACE,
        FDSIZE,
        GROUPS,
        VMPEAK,
        VMSIZE,
        VMLCK,
        VMHWM,
        VMRSS,
        VMDATA,
        VMSTK,
        VMEXE,
        VMLIB,
        VMPTE,
        VMSWAP,
        THREADS,
        SIGQ,
        CPUS_ALLOWED,
        VOLUNTARY_CTXT_SWITCHES,
        NONVOLUNTARY_CTXT_SWITCHES
    } ProcStatusId;

    SCXCoreLib::SCXLogHandle m_log;

}; // ProcfsProcStatus 

class CPUInfoDependencies
{
    public:
            virtual SCXCoreLib::SCXHandle<std::wistream> OpenCpuinfoFile() const;
                virtual ~CPUInfoDependencies() {};
};
/*----------------------------------------------------------------------------*/
/**
 * ProcfsCpuInfoReader declaration.
 */
class ProcfsCpuInfoReader : public ProcfsTableReader<ProcfsCpuInfo>
{
public:
    ProcfsCpuInfoReader(SCXCoreLib::SCXHandle<CPUInfoDependencies> deps= SCXCoreLib::SCXHandle<CPUInfoDependencies>(new CPUInfoDependencies())):m_deps(deps)
    {
    }
    virtual ~ProcfsCpuInfoReader(void) {}

    bool Init(void);

    virtual bool Load(void);

    void DetectHTEnabled(void);

private:
    SCXCoreLib::SCXHandle<CPUInfoDependencies> m_deps;
};

/*----------------------------------------------------------------------------*/
/**
 * ProcfsProcStatusReader declaration.
 */
class ProcfsProcStatusReader : public ProcfsTableReader<ProcfsProcStatus>
{
public:
    ProcfsProcStatusReader(void) {}
    virtual ~ProcfsProcStatusReader(void) {}

    bool Load(scxpid_t pid);

};
    
} /*SCXSystemLib*/

typedef enum _ProcessorArchitecture
{
    parchX86      = 0,
    parchMIPS     = 1,
    parchALPHA    = 2,
    parchPOWERPC  = 3,
    parchITANIUM  = 6,
    parchX64      = 9
} ProcessorArchitecture;

#endif  /* PROCCPUINFO_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

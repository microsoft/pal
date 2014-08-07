/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        procfsreader.cpp

    \brief       This file defines the abstraction of a procfs table in general on Linux. 
                 Specifically, it implements the /proc/cpuinfo property container and reader.
    
    \date        2011-11-15 15:09:00

    
*/
/*----------------------------------------------------------------------------*/

#include <scxsystemlib/procfsreader.h>
#include <scxcorelib/scxstream.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{

/*----------------------------------------------------------------------------*/
/**
   Declare the property association of label to identifier.  This array can
   be used to declare a lookup map.
 */
const ProcfsCpuInfo::LookupEntry ProcfsCpuInfo::m_PropertyPairs[] = {
    LookupEntry(L"processor", ProcfsCpuInfo::PROCESSOR),
    LookupEntry(L"address sizes", ProcfsCpuInfo::ADDRESS_SIZES),
    LookupEntry(L"bogomips", ProcfsCpuInfo::BOGOMIPS),
    LookupEntry(L"cache_alignment", ProcfsCpuInfo::CACHE_ALIGNMENT),
    LookupEntry(L"cache size", ProcfsCpuInfo::CACHE_SIZE),
    LookupEntry(L"cpuid level", ProcfsCpuInfo::CPUID_LEVEL),
    LookupEntry(L"core id", ProcfsCpuInfo::CORE_ID),
    LookupEntry(L"clflush size", ProcfsCpuInfo::CLFLUSH_SIZE),
    LookupEntry(L"cpu cores", ProcfsCpuInfo::CPU_CORES),
    LookupEntry(L"cpu family", ProcfsCpuInfo::CPU_FAMILY),
    LookupEntry(L"cpu MHz", ProcfsCpuInfo::CPU_SPEED),
    LookupEntry(L"flags", ProcfsCpuInfo::FLAGS),
    LookupEntry(L"fpu", ProcfsCpuInfo::FPU),
    LookupEntry(L"fpu_exception", ProcfsCpuInfo::FPU_EXCEPTION),
    LookupEntry(L"physical id", ProcfsCpuInfo::PHYSICAL_ID),
    LookupEntry(L"model", ProcfsCpuInfo::MODEL),
    LookupEntry(L"model name", ProcfsCpuInfo::MODEL_NAME),
    LookupEntry(L"siblings", ProcfsCpuInfo::SIBLINGS),
    LookupEntry(L"stepping", ProcfsCpuInfo::STEPPING),
    LookupEntry(L"vendor_id", ProcfsCpuInfo::VENDOR_ID),
    LookupEntry(L"wp", ProcfsCpuInfo::WP),
    LookupEntry(L"power management", ProcfsCpuInfo::POWER_MANAGEMENT)
};


/*----------------------------------------------------------------------------*/
/**
   These flags in /proc/cpuinfo are usually published in cpufeature.h as bit
   offsets.  The labels themselves are not published in a header but are defined
   in documentation.  Here we give them 
 */
#define  FEATURE_FPU       L"fpu"       // Onboard Floating Point Unit
#define  FEATURE_VME       L"vme"       // Virtual Mode Extensions
#define  FEATURE_VMX       L"vmx"       // Supports Virtual Mode
#define  FEATURE_SVM       L"svm"       // Supports Virtual Mode
#define  FEATURE_DE        L"de"        // Debugging Extensions
#define  FEATURE_PSE       L"pse"       // Page Size Extensions
#define  FEATURE_TSC       L"tsc"       // Time Stamp Counter
#define  FEATURE_MSR       L"msr"       // Model-Specific Registers
#define  FEATURE_PAE       L"pae"       // Physical Address Extensions
#define  FEATURE_MCA       L"mce"       // Machine Check Architecture
#define  FEATURE_CX8       L"cx8"       // CMPXCHG8 instruction
#define  FEATURE_APIC      L"apic"      // Onboard APIC
#define  FEATURE_SEP       L"sep"       // SYSENTER/SYSEXIT
#define  FEATURE_MTRR      L"mtrr"      // Memory Type Range Registers
#define  FEATURE_PGE       L"pge"       // Page Global Enable
#define  FEATURE_CMOV      L"cmov"      // CMOV instruction (FCMOVCC and FCOMI too if FPU present)
#define  FEATURE_PAT       L"pat"       // Page Attribute Table
#define  FEATURE_PSE36     L"pse36"     // 36-bit PSEs
#define  FEATURE_PN        L"pn"        //  Processor serial number
#define  FEATURE_CLFLSH    L"clflsh"    // Supports the CLFLUSH instruction
#define  FEATURE_DTES      L"dtes"      // Debug Trace Store
#define  FEATURE_ACPI      L"acpi"      // ACPI via MSR
#define  FEATURE_MMX       L"mmx"       // Multimedia Extensions
#define  FEATURE_FXSR      L"fxsr"      // FXSAVE and FXRSTOR instructions (fast save and restore of FPU context), and CR4.OSFXSR available
#define  FEATURE_XMM       L"xmm"       // Streaming SIMD Extensions
#define  FEATURE_XMM2      L"xmm2"      // Streaming SIMD Extensions-2
#define  FEATURE_SELFSNOOP L"selfsnoop" // CPU self snoop
#define  FEATURE_HT        L"ht"        // Hyper-Threading
#define  FEATURE_ACC       L"acc"       // Automatic clock control
#define  FEATURE_IA64      L"ia64"      // IA-64 processor
#define  FEATURE_SYSCALL   L"syscall"   // SYSCALL/SYSRET
#define  FEATURE_MMXEXT    L"mmext"     // AMD MMX extensions
#define  FEATURE_FXSR_OPT  L"fxsr"      // FXSR optimizations
#define  FEATURE_RDTSCP    L"rdtscp"    // RDTSCP
#define  FEATURE_LM        L"lm"        // Long Mode (x86-64)
#define  FEATURE_3DNOWEXT  L"3dnowext"  // AMD 3DNow! extensions
#define  FEATURE_3DNOW     L"3dnow"     // 3DNow!
#define  FEATURE_RECOVERY  L"recovery"  // CPU in recovery mode
#define  FEATURE_LONGRUN   L"longrun"   // Longrun power control
#define  FEATURE_LRTI      L"lrti"      // LongRun table interface
#define  FEATURE_CXMMX     L"cxmmx"     // Cyrix MMX extensions
#define  FEATURE_K6_MTRR   L"k6_mtrr"   // AMD K6 nonstandard MTRRs
#define  FEATURE_CYRIX_ARR L"cyrix_arr" // Cyrix ARRs (= MTRRs)
#define  FEATURE_CENTAUR_MCR L"centaur_mcr" // Centaur MCRs (= MTRRs)
#define  FEATURE_REP_GOOD  L"rep_good"  // rep microcode works well on this CPU
#define  FEATURE_CONSTANT_TSC L"constant_tsc" // TSC runs at constant rate
#define  FEATURE_SYNC_RDTSC L"sync_rdtsc" // RDTSC syncs CPU core
#define  FEATURE_FXSAVE_LEAK L"fxsave_leak" // FIP/FOP/FDP leaks through FXSAVE
#define  FEATURE_UP        L"up"        // SMP kernel running on UP
#define  FEATURE_ARCH_PERFMON L"arch_perfmon" // Intel Architectural PerfMon
#define  FEATURE_XMM3      L"xmm3"      // Streaming SIMD Extensions-3
#define  FEATURE_MWAIT     L"mwait"     // Monitor/Mwait support
#define  FEATURE_DSCPL     L"dscpl"     // CPL Qualified Debug Store
#define  FEATURE_EST       L"est"       // Enhanced SpeedStep
#define  FEATURE_TM2       L"tm2"       // Thermal Monitor 2
#define  FEATURE_CID       L"cid"       // Context ID
#define  FEATURE_CX16      L"cx16"      // CMPXCHG16B
#define  FEATURE_XTPR      L"xtpr"      // Send Task Priority Messages
#define  FEATURE_XSTORE    L"xstore"    // on-CPU RNG present (xstore insn)
#define  FEATURE_XSTORE_EN L"xstore_en" // on-CPU RNG enabled
#define  FEATURE_XCRYPT    L"xcrypt"    // on-CPU crypto (xcrypt insn)
#define  FEATURE_XCRYPT_EN L"xcrypt_en" // on-CPU crypto enabled
#define  FEATURE_LAHF_LM   L"lahf_lm"    // LAHF/SAHF in long mode

/**
 *  Declare a lookup map for properties in /proc/cpuinfo
 */
const ProcfsCpuInfo::LookupTable ProcfsCpuInfo::m_PropertyLookup(m_PropertyPairs, m_PropertyPairs + (sizeof(m_PropertyPairs)/sizeof(m_PropertyPairs[0])));

/*----------------------------------------------------------------------------*/
/**
 * ProcfsTable implementation
 */

/*----------------------------------------------------------------------------*/
/**
   Adds a property key and value to the collection as implemented in derived class
   \param     sProperty - (in) string as key
   \param     sValue    - (in) string as value
   \returns   true = success, false = fail
*/
bool
ProcfsTable::AddPair(const std::wstring & sProperty, const std::wstring & sValue)
{
    bool fRet = false;
    
    LookupTableCIT cit = LookupProperty(sProperty);
    if (!End(cit))
    {
        Insert(cit->second, sValue);
        m_fEmpty = false;
        fRet = true;
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
 * ProcfsCpuInfo implementation
 */

/*----------------------------------------------------------------------------*/
/**
   Constructor
*/
ProcfsCpuInfo::ProcfsCpuInfo(void)
  : m_Id(L"CPU."),
    m_HyperThreadingEnabled(false),
    m_log(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.common.procfscpuinfo")))

{
}

/*----------------------------------------------------------------------------*/
/**
   Destructor
*/
ProcfsCpuInfo::~ProcfsCpuInfo(void)
{
}

/*----------------------------------------------------------------------------*/
/**
   Find a property in the property collection using key. 
   \param     sProperty - (in) string as key
   \returns   const iterator. end of collection if not found.
*/
ProcfsTable::LookupTableCIT
ProcfsCpuInfo::LookupProperty(const std::wstring& sProperty)
{
    return ProcfsCpuInfo::m_PropertyLookup.find(sProperty);
}

/*----------------------------------------------------------------------------*/
/**
   Determines if the const iterator references the end of the collection.
   \param     cit - (in) const iterator.
   \returns   true = cit is at the end of the collection, false = it is not.
*/
bool
ProcfsCpuInfo::End(LookupTableCIT cit)
{
    return (cit == m_PropertyLookup.end());
}

/*----------------------------------------------------------------------------*/
/**
   Adds map-ready property as key and value pair.
   \param     propKey - (in) key
   \param     sValue - (in) value
*/
void
ProcfsCpuInfo::Insert(propertyid propKey, const std::wstring & sValue)
{
    m_Properties.insert(PropertyTable::value_type(propKey, sValue));
    if (propKey == PROCESSOR)
    {
        m_Id = L"CPU ";
        m_Id += sValue;
    }
    else if (propKey == FLAGS)
    {
        LoadFlags();
    }
}

/*----------------------------------------------------------------------------*/
/**
   Splits a space separated list of cpuinfo flags into an STL collection.
   \brief  Example: fpu de tsc msr pae cx8 apic sep cmov pat clflush acpi mmx fxsr
   sse sse2 ss ht syscall nx lm constant_tsc rep_good pni ssse3 cx16 sse4_1 lahf_lm
*/
void
ProcfsCpuInfo::LoadFlags(void)
{
    std::wstring sFlags;
    std::wstring sFlag;
    if (GetCompoundField(sFlags, FLAGS))
    {
        std::wstringstream ssFlags(sFlags);

        m_Flags.clear();

        while (!!ssFlags)
        {
            ssFlags >> sFlag;
            m_Flags.insert(sFlag.c_str());
            sFlag.clear();
        }
    }
    else
    {
        SCX_LOGWARNING(m_log, L"LoadFlags found no flags property.");
    }
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Processor property.
   \param     processor - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::Processor(unsigned short& processor) const
{
    return GetSimpleField<unsigned short>(processor, PROCESSOR);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the ProcessorType property.
   \param     processortype - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::ProcessorType(unsigned short& processortype) const
{
    processortype = CENTRAL_PPROCESSOR;  // /proc/cpuinfo only has central processors.
    return true;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the AddressSizePhysical property.
   \param     addresssizephysical - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::AddressSizePhysical(unsigned short& addresssizephysical)
{
    return GetSimpleField<unsigned short>(addresssizephysical, ADDRESS_SIZES);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the AddressSizeVirtual property.
   \param     addresssizevirtual - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::AddressSizeVirtual(unsigned short& addresssizevirtual)
{
    bool fRet = false;
        PropertyTable::const_iterator cit = m_Properties.find(ADDRESS_SIZES);
    if (cit != m_Properties.end())
    {
        std::size_t len = cit->second.length();
        std::wstringstream ss(cit->second);

        // Example: "38 bits physical, 48 bits virtual"
        ss.ignore(len, ' '); // #
        ss.ignore(len, ' '); // bits
        ss.ignore(len, ' '); // physical,
        ss >> addresssizevirtual;
        
        fRet = !!ss;
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the AddressSizeVirtual property.
   \param     addresssizevirtual - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::Architecture(unsigned short& architecture)
{
    bool fRet = false;
    unsigned short model = 0;

    if (GetSimpleField<unsigned short>(model, MODEL))
    {
        if (model == 1)
        {
            architecture = parchITANIUM;
            fRet = true;
        } else if (model > 1 && model < 23)
        {
            architecture = parchX64;
            fRet = true;
        } else
        {
            architecture = parchX86;
            fRet = true;
        }
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Bogomips property.
   \param     bogomips - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::Bogomips(unsigned long& bogomips)
{
    return GetSimpleField<unsigned long>(bogomips, BOGOMIPS);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CacheAlignment property.
   \param     cachealignment - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CacheAlignment(unsigned short& cachealignment)
{
    return GetSimpleField<unsigned short>(cachealignment, CACHE_ALIGNMENT);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CacheSize property.
   \param     cachesize - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CacheSize(unsigned long& cachesize)
{
    return GetSimpleField<unsigned long>(cachesize, CACHE_SIZE);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CpuidLevel property.
   \param     cpuidlevel - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CpuidLevel(unsigned short& cpuidlevel)
{
    return GetSimpleField<unsigned short>(cpuidlevel, CPUID_LEVEL);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CoreId property.
   \param     coreid - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CoreId(unsigned short& coreid)
{
    return GetSimpleField<unsigned short>(coreid, CORE_ID);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the ClFlushSize property.
   \param     clflushsize - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::ClFlushSize(unsigned long& clflushsize)
{
    return GetSimpleField<unsigned long>(clflushsize, CLFLUSH_SIZE);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CpuCores property.
   \param     cpucores - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CpuCores(unsigned int& cpucores) const
{
    return GetSimpleField<unsigned int>(cpucores, CPU_CORES);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CpuFamily property.
   \param     cpufamily - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CpuFamily(unsigned short& cpufamily) const
{
    return GetSimpleField<unsigned short>(cpufamily, CPU_FAMILY);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the CpuSpeed property.
   \param     cpuspeed - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::CpuSpeed(unsigned int& cpuspeed) const
{
    return GetSimpleField<unsigned int>(cpuspeed, CPU_SPEED);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Fpu property.
   \param     fpu - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::Fpu(bool& fpu)
{
    return GetSimpleField<bool>(fpu, FPU);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the FpuException property.
   \param     fpuexception - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::FpuException(bool& fpuexception)
{
    return GetSimpleField<bool>(fpuexception, FPU_EXCEPTION);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the PhysicalId property.
   \param     physicalid - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::PhysicalId(unsigned short& physicalid) const
{
    return GetSimpleField<unsigned short>(physicalid, PHYSICAL_ID);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Model property.
   \param     model - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::Model(unsigned short & model) const
{
    return GetSimpleField<unsigned short>(model, MODEL);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the ModelName property.
   \param     modelname - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::ModelName(std::wstring& modelname) const
{
    return GetCompoundField(modelname, MODEL_NAME);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Siblings property.
   \param     siblings - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::Siblings(unsigned short& siblings)
{
    return GetSimpleField<unsigned short>(siblings, SIBLINGS);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Stepping property.
   \param     stepping - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::Stepping(unsigned short& stepping) const
{
    return GetSimpleField<unsigned short>(stepping, STEPPING);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the VendorId property.
   \param     vendorid - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::VendorId(std::wstring& vendorid) const
{
    return GetCompoundField(vendorid, VENDOR_ID);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Wp property.
   \param     wp - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::Wp(bool& wp)
{
    return GetSimpleField<bool>(wp, WP);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Is64Bit property.
   \returns   true = cpu is 64 bit, false = it was not.
 */
bool
ProcfsCpuInfo::Is64Bit(void) const
{
    bool fRet = false;
    std::set<std::wstring>::const_iterator cit = m_Flags.end();
    if (m_Flags.end() != (cit = m_Flags.find(L"lm")))  // Long Mode (x86-64)
    {
        fRet = true;
    }
    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the IsHyperthreadingCapable property.
   \param     ishyperthreadingcapable - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::IsHyperthreadingCapable(void) const
{
    bool fRet = false;
    std::set<std::wstring>::const_iterator cit = m_Flags.end();
    if (m_Flags.end() != (cit = m_Flags.find(FEATURE_HT)))  // Hyper-Threading
    {
        fRet = true;
    }
    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the IsHyperthreadingEnabled property.
   \param     ishyperthreadingenabled - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::IsHyperthreadingEnabled(void) const
{
    return m_HyperThreadingEnabled;
}

void
ProcfsCpuInfo::HyperthreadingEnabled(bool fEnabled)
{
    m_HyperThreadingEnabled = fEnabled;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the IsVirtualizationCapable property.
   \param     isvirtualizationcapable - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::IsVirtualizationCapable(void) const
{
    bool fRet = false;
    std::set<std::wstring>::const_iterator cit = m_Flags.end();
    if (m_Flags.end() != (cit = m_Flags.find(FEATURE_VMX))
    ||  m_Flags.end() != (cit = m_Flags.find(FEATURE_SVM))
    ||  m_Flags.end() != (cit = m_Flags.find(FEATURE_VME)))
    {
        fRet = true;
    }
    return fRet;
}

const std::wstring &
ProcfsCpuInfo::CpuKey(void) const
{
    return m_Id;
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for the Version property.
   \param     version - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
 */
bool
ProcfsCpuInfo::Version(std::wstring& version) const
{
    bool fRet = false;
    unsigned short model = 0, stepping = 0;
    std::wstringstream ssVersion(std::wstringstream::out);

    if(GetSimpleField<unsigned short>(model, MODEL)
    && GetSimpleField<unsigned short>(stepping, STEPPING))
    {
        ssVersion << L"Model " << model << L" Stepping " << stepping;
        version = ssVersion.str();
        fRet = true;
    }
    else
    {
        SCX_LOGERROR(m_log, L"Model/Stepping properties not found.");
    }
    return fRet;
}
 
/*----------------------------------------------------------------------------*/
/**
   Accessor for the Role property.
   \param     role - (out)
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsCpuInfo::Role(unsigned short& role) const
{
    role = CENTRAL_PROCESSOR_ROLE;  // /proc/cpuinfo only ever lists central processor
    return true;
}

/*----------------------------------------------------------------------------*/
/**
   Collect the value belonging to the property id.
   The value type is templated for the input operator.

   \param     t - (out) receives value found in property.
   \param     propid - (in) key to property collection.
   \returns   true = property was found and assigned to arg, false = it was not.
*/
template<class T>
bool
ProcfsTable::GetSimpleField(T& t, propertyid propid) const
{
    bool fRet = false;
    PropertyTable::const_iterator cit = m_Properties.find(propid);
    if (cit != m_Properties.end())
    {
        std::wstringstream ss(cit->second);
            
        ss >> t;
        
        fRet = !!ss;
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Collect the value belonging to the property id.
   The whole value is copied without interpretation.

   \param     s - (out) receives value found in property.
   \param     propid - (in) key to property collection.
   \returns   true = property was found and assigned to arg, false = it was not.
*/
bool
ProcfsTable::GetCompoundField(std::wstring& s, propertyid propid) const
{
    bool fRet = false;

    PropertyTable::const_iterator cit = m_Properties.find(propid);
    
    if (cit != m_Properties.end())
    {
        s = cit->second;
        
        fRet = true;
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
* ProcfsCpuInfoReader implementation
*/
SCXHandle<std::wistream> CPUInfoDependencies::OpenCpuinfoFile() const
{
    return SCXFile::OpenWFstream(SCXFilePath(L"/proc/cpuinfo"), ios::in);
}

/*----------------------------------------------------------------------------*/
/**
   Read in the /proc/cpuinfo file.  After reading properties, calculate fields.

   \returns   true = cpuinfo was found, false = it was not.
*/
bool 
ProcfsCpuInfoReader::Init(void)
{
    bool fRet = Load();

    if (fRet)
    {
        DetectHTEnabled();
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Calculate HyperThreadingEnabled property.  
*/
void
ProcfsCpuInfoReader::DetectHTEnabled(void)
{
    bool fHyperThreadingEnabled = false;
    std::map<unsigned short, unsigned short> logicalCPUs; // key = logical, value = physical
    ProcfsTableReader<ProcfsCpuInfo>::iterator  itCpuInfo = begin();

    // Check for hyperthreading enabled (physical cpu's belong to multiple logical cpu's).
    for (itCpuInfo = begin(); itCpuInfo != end(); itCpuInfo++)
    {
        unsigned short logicalCPU = 0, physicalCPU = 0;
        if (itCpuInfo->Processor(logicalCPU) && itCpuInfo->PhysicalId(physicalCPU))
        {
            std::map<unsigned short, unsigned short>::const_iterator citLogical = logicalCPUs.find(logicalCPU);
            if (citLogical->second == physicalCPU)
            {
                fHyperThreadingEnabled = true;
                break;
            }
            logicalCPUs.insert(std::make_pair(logicalCPU, physicalCPU));
        }
    }

    // Assign HyperThreading enabled flag in each member ProcfsCpuInfo.
    for (itCpuInfo = begin(); itCpuInfo != end(); itCpuInfo++)
    {
        itCpuInfo->HyperthreadingEnabled(fHyperThreadingEnabled);
    }

}

/*----------------------------------------------------------------------------*/
/**
   Calculate HyperThreadingEnabled property.  
*/
bool
ProcfsCpuInfoReader::Load(void)
{
    SCXHandle<std::wistream> cpuFile = m_deps->OpenCpuinfoFile();
    return ProcfsTableReader<ProcfsCpuInfo>::LoadFile(*cpuFile);
}


/*----------------------------------------------------------------------------*/
/**
 * ProcfsProcStatus implementation
 */

/*----------------------------------------------------------------------------*/
/**
   Declare the property association of label to identifier.  This array can
   be used to declare a lookup map.
 */
const ProcfsTable::LookupEntry ProcfsProcStatus::m_PropertyPairs[] = {
    ProcfsTable::LookupEntry(L"Name",      ProcfsProcStatus::NAME),     // Filename (not the complete path[1] of the executable being run by the process.  
    ProcfsTable::LookupEntry(L"State",     ProcfsProcStatus::STATE),    // Process state as a single character (D/R/S/T/Z)  
    ProcfsTable::LookupEntry(L"Tgid",      ProcfsProcStatus::TGID),     //  Thread group ID, equal to the PID of the process (and this is also the TID of the first thread started in the process).  
    ProcfsTable::LookupEntry(L"Pid",       ProcfsProcStatus::PID),      //  PID, equal to TGID.  
    ProcfsTable::LookupEntry(L"PPid",      ProcfsProcStatus::PPID),     //  PID of the parent process.  
    ProcfsTable::LookupEntry(L"TracerPid", ProcfsProcStatus::TRACERPID),//  PID of process tracing this process (0 if the process is not traced).  
    ProcfsTable::LookupEntry(L"Uid",       ProcfsProcStatus::UID),      //  Set of four UIDs: real, effective, saved, filesystem.  
    ProcfsTable::LookupEntry(L"Gid",       ProcfsProcStatus::GID),      //  Set of four GIDs: real, effective, saved, filesystem.  
    ProcfsTable::LookupEntry(L"FDSize",    ProcfsProcStatus::FDSIZE),   //  Number of file descriptors in use.  
    ProcfsTable::LookupEntry(L"Groups",    ProcfsProcStatus::GROUPS),   //  List of GIDs of supplementary groups.  
    ProcfsTable::LookupEntry(L"VmPeak",    ProcfsProcStatus::VMPEAK),   //  Peak size of the virtual memory of the process.  
    ProcfsTable::LookupEntry(L"VmSize",    ProcfsProcStatus::VMSIZE),   //  Current total size of the virtual memory.  
    ProcfsTable::LookupEntry(L"VmLck",     ProcfsProcStatus::VMLCK),    //  Size of locked memory.  
    ProcfsTable::LookupEntry(L"VmHWM",     ProcfsProcStatus::VMHWM),    //  Peak size of the resident set ("high water mark").  
    ProcfsTable::LookupEntry(L"VmRSS",     ProcfsProcStatus::VMRSS),    //  Current size of the resident set.  
    ProcfsTable::LookupEntry(L"VmData",    ProcfsProcStatus::VMDATA),   //  Size of the data segment.  
    ProcfsTable::LookupEntry(L"VmStk",     ProcfsProcStatus::VMSTK),    //  Size of the stack segment.  
    ProcfsTable::LookupEntry(L"VmExe",     ProcfsProcStatus::VMEXE),    //  Size of the executable ("text") segment.  
    ProcfsTable::LookupEntry(L"VmLib",     ProcfsProcStatus::VMLIB),    //  Size of shared libraries loaded by this process (and possibly shared with others).  
    ProcfsTable::LookupEntry(L"VmPTE",     ProcfsProcStatus::VMPTE),    //  Size of page table entries (number of swap entries).  
    ProcfsTable::LookupEntry(L"VmSwap",    ProcfsProcStatus::VMSWAP),   //  Size of swap space used by this process.  
    ProcfsTable::LookupEntry(L"Threads",   ProcfsProcStatus::THREADS),  //  Number of threads.  
    ProcfsTable::LookupEntry(L"SigQ",      ProcfsProcStatus::SIGQ),     //  Two values separated by "/": Current number of queued signals/maximum allowed number of queued signals.  
    ProcfsTable::LookupEntry(L"Cpus_allowed", ProcfsProcStatus::CPUS_ALLOWED), //  A bitmap (as a hexadecimal value) of CPUs where the process may run.  
    ProcfsTable::LookupEntry(L"voluntary_ctxt_switches", ProcfsProcStatus::VOLUNTARY_CTXT_SWITCHES), //  Number of voluntary context switches.  
    ProcfsTable::LookupEntry(L"nonvoluntary_ctxt_switches", ProcfsProcStatus::NONVOLUNTARY_CTXT_SWITCHES) //  Number of involuntary context switches.  
};

/**
 *  Declare a lookup map for property keys in /proc/[pid]/status
 */
const ProcfsProcStatus::LookupTable ProcfsProcStatus::m_PropertyLookup(m_PropertyPairs, m_PropertyPairs + (sizeof(m_PropertyPairs)/sizeof(m_PropertyPairs[0])));

/*----------------------------------------------------------------------------*/
/**
   Constructor
*/
ProcfsProcStatus::ProcfsProcStatus(void)
  : m_log(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.common.procfsprocstatus")))

{
}

/*----------------------------------------------------------------------------*/
/**
   Destructor
*/
ProcfsProcStatus::~ProcfsProcStatus(void)
{
}

/*----------------------------------------------------------------------------*/
/**
   Find a property in the property collection using key. 
   \param     sProperty - (in) string as key
   \returns   const iterator. end of collection if not found.
*/
ProcfsTable::LookupTableCIT
ProcfsProcStatus::LookupProperty(const std::wstring& sProperty)
{
    return ProcfsProcStatus::m_PropertyLookup.find(sProperty);
}

/*----------------------------------------------------------------------------*/
/**
   Determines if the const iterator references the end of the collection.
   \param     cit - (in) const iterator.
   \returns   true = cit is at the end of the collection, false = it is not.
*/
bool
ProcfsProcStatus::End(LookupTableCIT cit)
{
    return (cit == m_PropertyLookup.end());
}

/*----------------------------------------------------------------------------*/
/**
   Adds map-ready property as key and value pair.
   \param     propKey - (in) key
   \param     sValue - (in) value
*/
void
ProcfsProcStatus::Insert(propertyid propKey, const std::wstring & sValue)
{
    m_Properties.insert(PropertyTable::value_type(propKey, sValue));
}

/*----------------------------------------------------------------------------*/
/**
   Filename (not the complete path) of the executable being run by the process.
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::Name(std::wstring& name) const
{
    return GetCompoundField(name, NAME);
}

/*----------------------------------------------------------------------------*/
/**
   Process state by code:
     SLEEP_UNINTERRUPT, // D  The process is in uninterruptible sleep.
     RUNNABLE,          // R  The process is running.
     SLEEPING,          // S  The process is sleeping, waiting for some event. it does not consume CPU time.
     STOPPED,           // T  The process is stopped.
     TERMINATED,        // X  The process has terminated.
     ZOMBIE             // Z  A zombie process.
   \param     state - (out) name of process.
*/
bool
ProcfsProcStatus::State(unsigned short& state) const
{
    bool fRet = false;
    std::wstring sState;

    fRet = GetCompoundField(sState, STATE);
    if (fRet)
    {
        switch(sState[0])
        {
            case 'D':
                state = SLEEP_UNINTERRUPT;
                break;
            case 'R':
                state = RUNNABLE;
                break;
            case 'S':
                state = SLEEPING;
                break;
            case 'T':
                state = STOPPED;
                break;
            case 'X':
                state = TERMINATED;
                break;
            case 'Z':
                state = ZOMBIE;
                break;
            default:
                fRet = false;
                break;
        }
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Thread group ID (PID of the process and TID of the first thread started in the process).
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::Tgid(scxulong& tgid) const
{
    return GetSimpleField<scxulong>(tgid, TGID);
}

/*----------------------------------------------------------------------------*/
/**
   Process id.
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::Pid(scxpid_t& pid) const
{
    return GetSimpleField<scxpid_t>(pid, PID);
}

/*----------------------------------------------------------------------------*/
/**
   Parent process id.
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::PPid(scxpid_t& ppid) const
{
    return GetSimpleField<scxpid_t>(ppid, PPID);
}

/*----------------------------------------------------------------------------*/
/**
   Process id of process tracing this process (0 if not traced).
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::TracerPid(scxpid_t& tracerPid) const
{
    return GetSimpleField<scxpid_t>(tracerPid, TRACERPID);
}

/*----------------------------------------------------------------------------*/
/**
   Accessor for user id properties in proc status.
   \param     real - (out) user id
   \param     effective - (out) user id
   \param     saved - (out) user id
   \param     filesystem - (out) user id
*/
bool
ProcfsProcStatus::Uid(uid_t &real, uid_t & effective, uid_t & saved, uid_t & filesystem) const
{
    bool fRet = false;
    std::wstring sUserIds;

    if (GetCompoundField(sUserIds, UID))
    {
        std::wstringstream ssUserIds(sUserIds);

        if (!!ssUserIds)
        {
            ssUserIds >> real >> effective >> saved >> filesystem;
            fRet = true;
        }
    }

    return fRet;
}
/*----------------------------------------------------------------------------*/
/**
   Set of four GIDs: real, effective, saved, filesystem
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::Gid(scxulong & real, scxulong & effective, scxulong & saved, scxulong & filesystem) const
{
    bool fRet = false;
    std::wstring sGroupIds;

    if (GetCompoundField(sGroupIds, UID))
    {
        std::wstringstream ssGroupIds(sGroupIds);

        if (!!ssGroupIds)
        {
            ssGroupIds >> real >> effective >> saved >> filesystem;
            fRet = true;
        }
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   utrace id of UTRACE API.
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::Utrace(scxulong& utrace) const
{
    return GetSimpleField<scxulong>(utrace, UTRACE);
}

/*----------------------------------------------------------------------------*/
/**
   Number of file descriptors in use.
   \param     fdsize - (out)
*/
bool
ProcfsProcStatus::FDSize(scxulong& fdsize) const
{
    return GetSimpleField<scxulong>(fdsize, FDSIZE);
}

/*----------------------------------------------------------------------------*/
/**
   List of GIDs of supplementary groups.
   \param     groups - (out)
*/
bool
ProcfsProcStatus::Groups(std::vector<unsigned int> & groups) const
{
    bool fRet = false;
    std::wstring sGroups;

    if (GetCompoundField(sGroups, GROUPS))
    {
        std::wstringstream ssGroups(sGroups);

        while (true)
        {
            unsigned int group = 0;
            ssGroups >> group;
            if (!ssGroups)
            {
                break;
            }
            else
            {
                groups.push_back(group);
                fRet = true;
            }
        }
    }

    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   Peak size of the virtual memory of the process.
   \param     vmpeak - (out)
*/
bool
ProcfsProcStatus::VmPeak(scxulong& vmpeak) const
{
    return GetSimpleField<scxulong>(vmpeak, VMPEAK);
}

/*----------------------------------------------------------------------------*/
/**
   Current total size of the virtual memory
   \param     vmsize - (out)
*/
bool
ProcfsProcStatus::VmSize(scxulong& vmsize) const
{
    return GetSimpleField<scxulong>(vmsize, VMSIZE);
}

/*----------------------------------------------------------------------------*/
/**
   Size of locked memory
   \param     vmlck - (out)
*/
bool
ProcfsProcStatus::VmLck(scxulong& vmlck) const
{
    return GetSimpleField<scxulong>(vmlck, VMLCK);
}

/*----------------------------------------------------------------------------*/
/**
   Peak size of the resident set (high water mark)
   \param     vmhwm - (out)
*/
bool
ProcfsProcStatus::VmHWM(scxulong& vmhwm) const
{
    return GetSimpleField<scxulong>(vmhwm, VMHWM);
}

/*----------------------------------------------------------------------------*/
/**
   Current size of the resident set.
   \param     vmrss - (out)
*/
bool
ProcfsProcStatus::VmRSS(scxulong& vmrss) const
{
    return GetSimpleField<scxulong>(vmrss, VMRSS);
}

/*----------------------------------------------------------------------------*/
/**
   Size of the data segment.
   \param     vmdata - (out)
*/
bool
ProcfsProcStatus::VmData(scxulong& vmdata) const
{
    return GetSimpleField<scxulong>(vmdata, VMDATA);
}

/*----------------------------------------------------------------------------*/
/**
   Size of stack segment.
   \param     vmstk - (out)
*/
bool
ProcfsProcStatus::VmStk(scxulong& vmstk) const
{
    return GetSimpleField<scxulong>(vmstk, VMSTK);
}

/*----------------------------------------------------------------------------*/
/**
   Size of executable segment.
   \param     name - (out) name of process.
*/
bool
ProcfsProcStatus::VmExe(scxulong& vmexe) const
{
    return GetSimpleField<scxulong>(vmexe, VMEXE);
}

/*----------------------------------------------------------------------------*/
/**
   Size of shared libraries loaded by this process 
   \param     vmlib - (out)
*/
bool
ProcfsProcStatus::VmLib(scxulong& vmlib) const
{
    return GetSimpleField<scxulong>(vmlib, VMLIB);
}

/*----------------------------------------------------------------------------*/
/**
Size of page table entries (number of swap entries)
   \param     vmpte - (out)
*/
bool
ProcfsProcStatus::VmPTE(scxulong& vmpte) const
{
    return GetSimpleField<scxulong>(vmpte, VMPTE);
}

/*----------------------------------------------------------------------------*/
/**
   Size of swap space used by this process.
   \param     vmswap - (out)
*/
bool
ProcfsProcStatus::VmSwap(scxulong& vmswap) const
{
    return GetSimpleField<scxulong>(vmswap, VMSWAP);
}

/*----------------------------------------------------------------------------*/
/**
   Number of threads.
   \param     threads - (out)
*/
bool
ProcfsProcStatus::Threads(scxulong& threads) const
{
    return GetSimpleField<scxulong>(threads, THREADS);
}

/*----------------------------------------------------------------------------*/
/**
    Slash separated queued signals, current/maximum
   \param     current - (out)
   \param     maximum - (out)
*/
bool
ProcfsProcStatus::SigQ(scxulong& current, scxulong& max) const
{
    bool fRet = false;
    std::wstring sSigQ;

    if (GetCompoundField(sSigQ, SIGQ))
    {
        scxulong n1 = 0;
        scxulong n2 = 0;
        wchar_t cSlash;
        std::wstringstream ssSigQ(sSigQ);
        if (ssSigQ >> n1 >> cSlash >> n2)
        {
            current = n1;
            max = n2;
            fRet = true;
        }
    }
    return fRet;
}

/*----------------------------------------------------------------------------*/
/**
   A bitmap of CPUs where the process may run
   \param     threads - (out)
*/
bool
ProcfsProcStatus::Cpus_allowed(scxulong& cpusAllowed) const
{
    bool fRet = false;
    std::wstring sCpusAllowed;

    if (GetCompoundField(sCpusAllowed, CPUS_ALLOWED))
    {
        scxulong ca = 0;
        std::wstringstream ssCpusAllowed(sCpusAllowed);
        if (ssCpusAllowed >> std::hex >> ca)
        {
            cpusAllowed = ca;
            fRet = true;
        }
    }

    return fRet;

}

/*----------------------------------------------------------------------------*/
/**
   Number of voluntary context switches
   \param     volSwitches - (out)
*/
bool
ProcfsProcStatus::VoluntaryContextSwitches(scxulong &volSwitches) const
{
    return GetSimpleField<scxulong>(volSwitches, VOLUNTARY_CTXT_SWITCHES);
}

/*----------------------------------------------------------------------------*/
/**
   Number of involuntary context switches
   \param     nonVolSwitches - (out)
*/
bool
ProcfsProcStatus::NonVoluntaryContextSwitches(scxulong &nonVolSwitches) const
{
    return GetSimpleField<scxulong>(nonVolSwitches, NONVOLUNTARY_CTXT_SWITCHES);
}

/*----------------------------------------------------------------------------*/
/**
 * ProcfsProcStatusReader implementation
 */

bool ProcfsProcStatusReader::Load(scxpid_t pid)
{
    std::wstring procfsPath = L"/proc/" + SCXCoreLib::StrFrom(pid) + L"/status";
    std::wfstream ifs(SCXCoreLib::StrToUTF8(procfsPath).c_str(), ios_base::in);
    return ProcfsTableReader<ProcfsProcStatus>::LoadFile(ifs);
}


} /*SCXSystemLib*/

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

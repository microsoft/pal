/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file        cpupropertiesinstance.cpp

  \brief       PAL representation of CPU Properties 

  \date        11-10-31 14:30:00 

 */
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <vector>
#include <scxsystemlib/cpupropertiesinstance.h>
#include <sstream>
#if defined(__x86_64__) || defined(__i386__) || (defined(sun) && !defined(sparc))
#include <sys/utsname.h>
#elif defined(hpux)
#include <unistd.h>
#include <scxcorelib/logsuppressor.h>
#endif

using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{
#if defined(linux) 
    const wstring cRoleStr[] = {
           L"Other",
           L"Unknown",
           L"Central Processor",
           L"Math Processor",
           L"DSP Processor",
           L"Video Processor"
           };
    /** The value of string index when a string field references no string*/
    size_t cStrIndexNull = 0;
#elif defined(sun)  
     /** Convert hz to MHz */
     const scxulong cMhzLevel = 1000000;
#endif

#if defined(linux) 

    /*----------------------------------------------------------------------------*/
    /**
      Constructor

      \param[in]        deps - Dependencies for the BIOS data colletion.
     */
    CpuPropertiesInstance::CpuPropertiesInstance(const  wstring& id, const ProcfsCpuInfo &cpuinfo) :
        EntityInstance(id),
        m_cpuinfo(cpuinfo),
        m_family(2), // Unknown family.
        m_log(SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.cpupropertiesinstance")))

    {
        SCX_LOGTRACE(m_log, L"Enter CpuPropertiesInstance(ProcfsCpuInfo) constructor");
        
        m_family = GetFamily();
    }

    /*----------------------------------------------------------------------------*/
    /**
      GetFamily gets the family number as defined by Win32_Processor class.

      Returns:     processor family.
     */
    unsigned short CpuPropertiesInstance::GetFamily()
    {
        std::wstring vendorId;
        bool vendorIdResult = m_cpuinfo.VendorId(vendorId);
        std::wstring modelName;
        bool modelNameResult = m_cpuinfo.ModelName(modelName);

        if ((vendorIdResult == false) || (modelNameResult == false))
        {
            return 2; // Unknown family.
        }

        if (vendorId == L"GenuineIntel")
        {
            // Typical brand string for Intel processor is:
            // "Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz"

            // Find terminating "CPU"
            size_t pos = modelName.find(L"CPU");
            if (pos == wstring::npos)
            {
                return 2; // Unknown family.
            }
            modelName = modelName.substr(0, pos);
            modelName = StrToUpper(modelName);

            // Remove from brand string parts we don't care about.
            StrReplaceAll(modelName, L"(R)", L" ");
            StrReplaceAll(modelName, L"(TM)", L" ");
            StrReplaceAll(modelName, L"MOBILE", L" ");
            StrReplaceAll(modelName, L"GENUINE", L" ");

            std::vector<std::wstring> tokens;
            StrTokenize(modelName, tokens);
            if ((tokens.size() < 2) || (tokens[0] != L"INTEL"))
            {
                return 2; // Unknown family.
            }

            if (tokens[1] == L"XEON")
            {
                return 179; // Intel Xeon family.
            }
            else if (tokens[1] == L"PENTIUM")
            {
                if (tokens.size() >= 3)
                {
                    if (tokens[2] == L"III")
                    {
                        if ((tokens.size() >= 4) && tokens[3] == L"XEON")
                        {
                            return 176; // Intel Pentium III Xeon family.
                        }
                        return 17; // Intel Pentium III family.
                    }
                    else if (tokens[2] == L"4")
                    {
                        return 178; // Intel Pentium 4 family.
                    }                
                    else if (tokens[2] == L"M")
                    {
                        return 185; // Intel Pentium M family.
                    }                
                }
                return 11; // Intel Pentium Brand family.
            }
            else if (tokens[1] == L"CELERON")
            {
                return 15; // Intel Celeron Family.
            }
        }
        else if (vendorId == L"AuthenticAMD")
        {
            // Typical brand string for AMD processor is:
            // "Dual-Core AMD Opteron(tm) Processor 2210"

            modelName = StrToUpper(modelName);

            // Remove from processor name parts we don't care about.
            StrReplaceAll(modelName, L"(R)", L" ");
            StrReplaceAll(modelName, L"(TM)", L" ");
            StrReplaceAll(modelName, L"MOBILE", L" ");
            StrReplaceAll(modelName, L"DUAL CORE", L" ");
            StrReplaceAll(modelName, L"DUAL-CORE", L" ");

            std::vector<std::wstring> tokens;
            StrTokenize(modelName, tokens);
            if (tokens.size() == 0)
            {
                return 2; // Unknown family.
            }
            if (tokens[0] == L"AMD-K5")
            {
                return 25; // AMD K5 family.
            }
            else if (tokens[0] == L"AMD-K6")
            {
                return 26; // AMD K6 family.
            }
            else if (tokens[0] == L"AMD-K7")
            {
                return 190; // AMD K7 family.
            }
            else if ((tokens.size() >= 2) && tokens[0] == L"AMD")
            {
                if (tokens[1] == L"ATHLON")
                {
                    if (tokens.size() >= 3)
                    {
                        if (tokens[2] == L"64")
                        {
                            return 131; // AMD Athlon 64 family.
                        }
                        else if (tokens[2] == L"XP")
                        {
                            return 182; // AMD Athlon XP family.
                        }                            
                    }
                    return 29; // AMD Athlon family.
                }
                else if (tokens[1] == L"DURON")
                {
                    return 24; // AMD Duron family.
                }
                else if (tokens[1] == L"OPTERON")
                {
                    return 132; // AMD Opteron family.
                }
            }
        }
        return 2; // Unknown family.
    }

#elif defined(sun) 
    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        \param[in]      cpuId - Processor mark id
        \param[in]      CpuPropertiesPALDependencies - Dependencies to get data
    */
    CpuPropertiesInstance::CpuPropertiesInstance(const wstring& cpuInfoIndex, SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies> deps) :
        m_deps(deps),
        m_cpuInfoIndex(cpuInfoIndex),
        m_log(SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.cpupropertiesinstance")))
    {
        m_deps->Init();
    }
#elif defined(aix)
    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        \param[in]      cpuTotal - Processor speed, core count, logical processor count.
        \param[in]     partTotal - 64 bit, hyperthreading properties.
    */          
    CpuPropertiesInstance::CpuPropertiesInstance(const perfstat_cpu_total_t & cpuTotal, const perfstat_partition_total_t & partTotal) :
        m_log(SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.cpupropertiesinstance")))
    {
        SCX_LOGTRACE(m_log, L"Enter CpuPropertiesInstance() constructor");
        m_processorAttr.is64Bit = (bool)(partTotal.type.b.kernel_is_64);
        m_processorAttr.isHyperthreadCapable = (bool)(partTotal.type.b.smt_capable != 0);
        m_processorAttr.isHyperthreadEnabled = (bool)(partTotal.type.b.smt_enabled != 0);
        m_processorAttr.isVirtualizationCapable = true;
        m_processorAttr.manufacturer = L"IBM";
        m_processorAttr.family = 32; // Power PC Family = 32

        m_processorAttr.currentClockSpeed =
            m_processorAttr.maxClockSpeed =
                m_processorAttr.normSpeed = (unsigned int)(cpuTotal.processorHZ / 1000000);
        if (partTotal.online_cpus >= 1)
        {
            // On AIX, never return number of cores per processor since we don't know that
            m_processorAttr.numberOfCores = 0;
            m_processorAttr.numberOfLogicalProcessors = cpuTotal.ncpus / partTotal.online_cpus;
        }
        m_processorAttr.processorType = CENTRAL_PROCESSOR;
        m_processorAttr.role = L"Central Processor";
        m_processorAttr.upgradeMethod = 2; //Unknown
        m_processorAttr.cpuStatus = 0;   // Unknown. System metrics are gathered on a per logical cpu basis rather than physical.
    }
#elif defined(hpux)

    #define HZ_PER_MHZ 1000000

    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        \param[in]      cpuId - Processor mark id
        \param[in]      CpuPropertiesPALDependencies - Dependencies to get data
    */
    CpuPropertiesInstance::CpuPropertiesInstance(const wstring& socketId, const pst_processor& cpu, const pst_dynamic& psd) :
        m_log(SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.cpupropertiesinstance")))
    {
        m_processorAttr.is64Bit = true; // Both supported processors, Itanium and PA_RISC, are 64 bit.
        m_socketId = socketId;
#if (PF_MINOR >= 31)
        m_processorAttr.manufacturer = SCXCoreLib::StrFromUTF8(cpu.psp_cpu_vendor);

        m_processorAttr.currentClockSpeed =
            m_processorAttr.maxClockSpeed =
                m_processorAttr.normSpeed = cpu.psp_cpu_frequency/HZ_PER_MHZ;
        m_processorAttr.extClock = cpu.psp_bus_frequency/HZ_PER_MHZ;        

        switch (cpu.psp_cpu_architecture)
        {
            case PSP_ARCH_PA_RISC:
                m_processorAttr.family = 144 /*RISC*/;
                break;
            case PSP_ARCH_IPF:
                m_processorAttr.family = 130 /*Itanium*/;
                break;
            default:
                break;
        }

#else
#if defined(__ia64)
        m_processorAttr.manufacturer = L"Intel";
        m_processorAttr.family = 130 /*Itanium*/;
#elif defined(__hppa)
        m_processorAttr.manufacturer = L"HP";
        m_processorAttr.family = 144 /*RISC*/;
#endif
#endif
        m_processorAttr.processorType = CENTRAL_PROCESSOR;
        m_processorAttr.role = L"Central Processor";
        m_processorAttr.upgradeMethod = 2; //Unknown
        m_processorAttr.cpuStatus = 0;   // Unknown. System metrics are gathered on a per logical cpu basis rather than physical.
#if (PF_MINOR < 31)
        m_processorAttr.numberOfCores = psd.psd_proc_cnt;
        m_processorAttr.numberOfLogicalProcessors = psd.psd_proc_cnt;
#endif
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
      Default destructor
     */
    CpuPropertiesInstance::~CpuPropertiesInstance()
    {
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesInstance default destructor: "));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Update current Processor instance to latest values
     */
    void CpuPropertiesInstance::Update()
    {
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesInstance update with cpuinfo"));
#if defined(linux)

#elif (defined(sun)) 
        //
        //Go to first kstat
        //
        m_deps->Lookup(cModul_Name, m_cpuInfoIndex, cInstancesNum);

        //
        //Count Of Logical Processors
        //
        unsigned int logicalProcessorsCount = 0;

        //
        //Find the kstat by m_chipid
        //
        scxulong uChipId;
        if (!m_deps->TryGetValue(cAttrName_ChipID, uChipId))
        {
            throw SCXNotSupportedException(L"Chip Id not exist", SCXSRCLOCATION);
        }
        scxlong chipId = static_cast<scxlong>( uChipId );
        //
        //Get clock_MHz and current_clock_Hz
        //
        scxulong tmpNormSpeed = 0;
        if (m_deps->TryGetValue(cAttrName_ClockMHz, tmpNormSpeed))
        {
            m_processorAttr.normSpeed = static_cast<unsigned short>(tmpNormSpeed);
            scxulong tmpCurrentClockSpeed = 0;
            if (m_deps->TryGetValue(cAttrName_CurrentClockHz, tmpCurrentClockSpeed))
            {
                m_processorAttr.currentClockSpeed = static_cast<unsigned int>((tmpCurrentClockSpeed) / cMhzLevel);
            }
            else
            {
                //
                //NO current_clock_Hz means same as normSpeed.
                //
                m_processorAttr.currentClockSpeed = m_processorAttr.normSpeed;
            }
        }

        //
        //Get processor name and family
        //
#if !defined(sparc)
        //For x86 kstat provides the correct Intel family number
        scxulong familyNum;

        if (m_deps->TryGetValue(cAttrName_Family, familyNum))
        {
            m_processorAttr.family = familyNum;
        }

        //
        // Get Manufacturer from the Vendor_id
        //
        wstring mfr;

        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesInstance::Update TryGetStringValue ") + cAttrName_Vendor);
        if (m_deps->TryGetStringValue(cAttrName_Vendor, mfr))
        {
            m_processorAttr.manufacturer = mfr;
        }

        //
        // Get Stepping info
        //
        wstring steppingInfo;
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesInstance::Update TryGetStringValue ") + cAttrName_Stepping);
        if (m_deps->TryGetStringValue(cAttrName_Stepping, steppingInfo))
        {
            m_processorAttr.stepping = steppingInfo;
        }
   
        //
        // Get Version info
        //
        wstring model;
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesInstance::Update TryGetStringValue ") + cAttrName_Model);
        if (m_deps->TryGetStringValue(cAttrName_Model, model))
        {
            std::wstringstream version(std::wstringstream::out);
            version << L"Model " << model << L" Stepping " << steppingInfo;
            m_processorAttr.version = version.str();
        }

#endif

        wstring familyName;

#if PF_Major > 5 || PF_MINOR >= 10
        if (m_deps->TryGetStringValue(cAttrName_Brand, familyName))
#else
        if (m_deps->TryGetStringValue(cAttrName_Implementation, familyName))
#endif
        {
            m_processorAttr.name = familyName;
#if defined(sparc)
            m_processorAttr.family = cFamily_Sparc_Value[0];
            for (int i =1; i< cFamily_Sparc_Array_Length; i++)
            {
                if (cFamily_Sparc_Name[i] == familyName)
                {
                    m_processorAttr.family = cFamily_Sparc_Value[i];
                    break;
                }
            }
#endif
        }

#if defined(sparc)
        //
        //Get processor stepping by get "ver xxx" string in processor implementation information
        //
        wstring implementationInfo = L"";

        if (m_deps->TryGetStringValue(cAttrName_Implementation, implementationInfo))
        {
            if (implementationInfo == L"")
            {
                    m_processorAttr.stepping = L"";
            }
            else
            {
                //
                //find "ver xxx" string
                //
                wstring strFindString(L"ver");
                wstring::size_type strPos = implementationInfo.find(strFindString);
                if (wstring::npos != strPos)
                {
                    implementationInfo = implementationInfo.substr(strPos);
                    strFindString = L" ";
                    strPos = implementationInfo.find(strFindString);
                    if (wstring::npos != strPos)
                    {
                        strPos = implementationInfo.find(strFindString, strPos+1);
                        implementationInfo = implementationInfo.substr(0, strPos);
                    }
                    m_processorAttr.stepping = implementationInfo;
                }
            }
        }
#endif

        //
        //Make cpu+id string to be a unique id.
        //
        wstringstream os;
        os << "CPU " << chipId;
        m_processorAttr.deviceID = os.str();

       
        std::set<scxulong> coreid;
        //
        //get number Of Logical Processors count the chip_id
        //
        unsigned int cpuIndex =0;
        for ( ; ; )
        {
            wstringstream cpuInfoName;
            cpuInfoName << cModul_Name.c_str() << cpuIndex;


            if (!m_deps->Lookup(cModul_Name, cpuInfoName.str(), cInstancesNum)) break;

            scxulong uNewChipId = 0;
            //
            // Check ChipId. If chip_id not exist, give a default value 1.
            //
            if (!m_deps->TryGetValue(cAttrName_ChipID, uNewChipId))
            {
                logicalProcessorsCount++;
                break;
            }
            scxlong newChipId = static_cast<scxlong>( uNewChipId );
            //
            // Check ChipId. If equal, number Of Logical Processors add 1.
            //
            if (newChipId == chipId)
            {
                logicalProcessorsCount++;

                // Get Core Id
                scxulong coreID;
                if (m_deps->TryGetValue(cAttrName_CoreID, coreID))
                {
                    coreid.insert(coreID); 
                }

            }
           
            cpuIndex++;
        }
        m_processorAttr.numberOfLogicalProcessors = logicalProcessorsCount;
        if (coreid.size() > 0)
        {
            m_processorAttr.numberOfCores = coreid.size();
        }
        else
        {
            m_processorAttr.numberOfCores = m_processorAttr.numberOfLogicalProcessors;
        }

#elif defined(aix)
        SCX_LOGTRACE(m_log, wstring(L"Calling FillAttributes")); 
        if (!FillAttributes())
        {
            SCX_LOGERROR(m_log, L"FillAttributes failed.");
        }
        SCX_LOGTRACE(m_log, wstring(L"After FillAttributes")); 
#elif defined(hpux)
        m_processorAttr.cpuKey = GetId();
        m_processorAttr.processorId = m_processorAttr.cpuKey;
        m_processorAttr.deviceID = m_processorAttr.cpuKey;

        unsigned int numberOfCoresperCPU = 0;
        struct pst_processor psp[PST_MAX_PROCS] = { { 0 } };
        wstring  tmpPhysicalID;

        int cpuTotal = pstat_getprocessor(psp, sizeof(struct pst_processor), PST_MAX_PROCS, 0);

        if (-1 == cpuTotal)
        {
            SCX_LOGTRACE(m_log, L"pstat_getprocessor failed. errno " + errno);
            throw SCXCoreLib::SCXInvalidStateException(L"pstat_getprocessor failed. errno " + errno, SCXSRCLOCATION);
        }
#if (PF_MINOR >= 31)            
        for (unsigned int i= 0; i< cpuTotal; i++)
        {
            tmpPhysicalID = StrFrom(psp[i].psp_socket_id);    
            if (tmpPhysicalID == m_socketId)
            {
                numberOfCoresperCPU++;
            }
        }
        m_processorAttr.numberOfCores = numberOfCoresperCPU;
        m_processorAttr.numberOfLogicalProcessors = numberOfCoresperCPU;
#endif
        int64_t cpuChipType ;
        if ((cpuChipType = sysconf(_SC_CPU_CHIP_TYPE)) == -1)
        {
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eInfo);
            std::wstring msg = L"sysconf _SC_CPU_CHIP_TYPE failed. the errno is : " + errno;
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(msg));
            SCX_LOG(m_log, severity, msg);
   
            throw SCXCoreLib::SCXErrnoException(L"sysconf _SC_CPU_CHIP_TYPE failed. errno ", errno, SCXSRCLOCATION);
        }
        else
        {
            /*---------------------------------
             31     24 23    16 15    8 7     0
             ----------------------------------
             | family | model  | rev   |number|
            ----------------------------------
            */

            unsigned short stepping = (cpuChipType >> 8) & 0xFF;
            m_processorAttr.stepping = StrFrom<int>(stepping); 
            unsigned short model =  (cpuChipType >>  16) & 0xFF ;
            std::wstringstream ssVersion(std::wstringstream::out);
            ssVersion << L"Model " << model << L" Stepping " << stepping;
            m_processorAttr.version = ssVersion.str(); //Model 2 Stepping 12
        }
#endif

    }

    /*----------------------------------------------------------------------------*/
    /**
      Clean up.
     */
    void CpuPropertiesInstance::CleanUp()
    {
    }

#if defined(aix)
                                                      
    /*----------------------------------------------------------------------------*/
    /**
        Fill in processor known attributes and attributes from
        _system_configuration and instance id.
     */
    bool CpuPropertiesInstance::FillAttributes(void)
    {
        bool          fRet = true;
        int           rc   = 0;
        MODEL_MAP_CIT cit;

        wstringstream ssId(L"CPU ");
        SCX_LOGTRACE(m_log, L"Begin FillAttributes");
        ssId << GetId();
        SCX_LOGTRACE(m_log, L"GetId successful");

        m_processorAttr.cpuKey = ssId.str();
        m_processorAttr.processorId = ssId.str();
        m_processorAttr.deviceID = ssId.str();

        SCX_LOGTRACE(m_log, L"Set m_processorAttr cpuKey, processorId, deviceId");
        cit = SysConfigModelImplLookup.find(_system_configuration.model_impl);
        SCX_LOGTRACE(m_log, L"Called find on _system_configuration.model_impl");
        if (cit != SysConfigModelImplLookup.end())
        {
            m_processorAttr.stepping = cit->second;
            SCX_LOGTRACE(m_log, L"Set m_processorAttr stepping");
        }
        else
        {
            SCX_LOGERROR(m_log, L"FillAttributes failed to find stepping from model_impl " + StrFrom(_system_configuration.model_impl));
        }

        cit = SysConfigImplLookup.find(_system_configuration.implementation);
        SCX_LOGTRACE(m_log, L"Called find on _system_configuration.implementation");
        if (cit != SysConfigImplLookup.end())
        {
            m_processorAttr.name = cit->second;
            SCX_LOGTRACE(m_log, L"Set m_processorAttr name");
        }
        else
        {
            SCX_LOGERROR(m_log, L"FillAttributes failed to find name from implementation " + StrFrom(_system_configuration.implementation));
        }

        cit = SysConfigVersionLookup.find(_system_configuration.version);
        SCX_LOGTRACE(m_log, L"Called find on _system_configuration.version");
        if (cit != SysConfigVersionLookup.end())
        {
            m_processorAttr.version = cit->second;
            SCX_LOGTRACE(m_log, L"Set m_processorAttr version");
        }
        else
        {
            SCX_LOGERROR(m_log, L"FillAttributes failed to find version name from code " + StrFrom(_system_configuration.version));
        }

        SCX_LOGTRACE(m_log, L"Finish FillAttibutes");
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Lookup tables for _system_configuration codes.
       Used for properties stepping, name, version.
     */
    const CpuPropertiesInstance::MODEL_MAP::value_type CpuPropertiesInstance::aSysConfigImplPairs[] = {
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_RS1, L"POWER_RS1" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_RSC, L"POWER_RSC" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_RS2, L"POWER_RS2" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_601, L"POWER_601" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_603, L"POWER_603" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_604, L"POWER_604" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_620, L"POWER_620" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_630, L"POWER_630" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_A35, L"POWER_A35" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_RS64II, L"POWER_RS64II" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_RS64III, L"POWER_RS64III" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_4, L"POWER_4" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_MPC7450, L"POWER_MPC7450" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_5, L"POWER_5" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_6, L"POWER_6" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(POWER_7, L"POWER_7" )
    };

    const CpuPropertiesInstance::MODEL_MAP CpuPropertiesInstance::SysConfigImplLookup(
            CpuPropertiesInstance::aSysConfigImplPairs,
            CpuPropertiesInstance::aSysConfigImplPairs + (sizeof(CpuPropertiesInstance::aSysConfigImplPairs)/sizeof(CpuPropertiesInstance::aSysConfigImplPairs[0])));

    const CpuPropertiesInstance::MODEL_MAP::value_type CpuPropertiesInstance::aSysConfigVersionPairs[] = {
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_601, L"PV_601" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_601a, L"PV_601a" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_603, L"PV_603" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_604, L"PV_604" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_620, L"PV_620" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_630, L"PV_630" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_A35, L"PV_A35" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RS64II, L"PV_RS64II" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RS64III, L"PV_RS64III" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_4, L"PV_4" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RS64IV, L"PV_RS64IV" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_MPC7450, L"PV_MPC7450" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_4_2, L"PV_4_2" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_4_3, L"PV_4_3" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_5, L"PV_5" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_5_2, L"PV_5_2" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_5_3, L"PV_5_3" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_6, L"PV_6" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_6_1, L"PV_6_1" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_7, L"PV_7" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_5_Compat, L"PV_5_Compat" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_6_Compat, L"PV_6_Compat" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_7_Compat, L"PV_7_Compat" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RESERVED_2, L"PV_RESERVED_2" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RESERVED_3, L"PV_RESERVED_3" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RS2, L"PV_RS2" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RS1, L"PV_RS1" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_RSC, L"PV_RSC" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_M1, L"PV_M1" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(PV_M2, L"PV_M2" )
    };

    const CpuPropertiesInstance::MODEL_MAP CpuPropertiesInstance::SysConfigVersionLookup(
            CpuPropertiesInstance::aSysConfigVersionPairs,
            CpuPropertiesInstance::aSysConfigVersionPairs + (sizeof(CpuPropertiesInstance::aSysConfigVersionPairs)/sizeof(CpuPropertiesInstance::aSysConfigVersionPairs[0])));

    const CpuPropertiesInstance::MODEL_MAP::value_type CpuPropertiesInstance::aSysConfigModelImplPairs[] = {
        CpuPropertiesInstance::MODEL_MAP_ENTRY(RS6K_UP_MCA, L"RS6K_UP_MCA" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(RS6K_SMP_MCA, L"RS6K_SMP_MCA" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(RSPC_UP_PCI, L"RSPC_UP_PCI" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(RSPC_SMP_PCI, L"RSPC_SMP_PCI" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(CHRP_UP_PCI, L"CHRP_UP_PCI" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(CHRP_SMP_PCI, L"CHRP_SMP_PCI" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(IA64_COM, L"IA64_COM" ),
        CpuPropertiesInstance::MODEL_MAP_ENTRY(IA64_SOFTSDV, L"IA64_SOFTSDV" )

    };

    const CpuPropertiesInstance::MODEL_MAP CpuPropertiesInstance::SysConfigModelImplLookup(
            CpuPropertiesInstance::aSysConfigModelImplPairs,
            CpuPropertiesInstance::aSysConfigModelImplPairs + (sizeof(CpuPropertiesInstance::aSysConfigModelImplPairs)/sizeof(CpuPropertiesInstance::aSysConfigModelImplPairs[0])));
#endif
    /*----------------------------------------------------------------------------*/
    /**
      Check that whether the maximum data width capability of the processor is 64-bit.

      Parameters:  is64Bit- whether the processor is 64-bit.
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetIs64Bit(bool &is64Bit) const
    {
        bool fRet = false;
#if defined(linux)

        // Implemented platform,and the property can be supported,return true;
        // Implemented by procfs cpuinfo
        // otherwise not support return false.
        is64Bit = m_cpuinfo.Is64Bit();
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) 
        is64Bit = m_processorAttr.is64Bit; 
        fRet = true;
#elif defined(hpux) 
        long kbits = sysconf(_SC_KERNEL_BITS);
        if (kbits > 0)
        {
            is64Bit = (bool)(kbits == 64);
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Is64Bit", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
      Check that whether the processor supports multiple hardware threads per core.

      Parameters:  isHyperthreadCapable- whether the processor is support multiple threads.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetIsHyperthreadCapable(bool &isHyperthreadCapable) const
    {
        bool fRet = false;
#if defined(linux) 
        isHyperthreadCapable = m_cpuinfo.IsHyperthreadingCapable();
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) 
        isHyperthreadCapable = m_processorAttr.isHyperthreadCapable; 
        fRet = true;
#elif defined(hpux) 
        // Property not supported.
        fRet = false;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"IsHyperthreadCapable", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Check that whether the processor is capable of executing enhanced virtualization instructions.

      Parameters:  isVirtualizationCapable- whether the processor is capable of executing enhanced virtualization instructions.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetIsVirtualizationCapable(bool &isVirtualizationCapable) const
    {
        bool fRet = false;
#if defined(linux) 
        isVirtualizationCapable = m_cpuinfo.IsVirtualizationCapable();
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) 
        isVirtualizationCapable = m_processorAttr.isVirtualizationCapable; 
        fRet = true;
#elif defined(hpux) 
        // Property not supported.
        fRet = false;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"IsVirtualizationCapable", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Check that whether the Hyperthread function is enabled.

      Parameters:  IsHyperthreadEnabled - whether the Hyperthread function is enabled.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetIsHyperthreadEnabled(bool &isHyperthreadEnabled) const
    {
        bool fRet = false;
#if defined(linux)
        isHyperthreadEnabled = m_cpuinfo.IsHyperthreadingEnabled();
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) 
        isHyperthreadEnabled = m_processorAttr.isHyperthreadEnabled; 
        fRet = true;
#elif defined(hpux) 
        // Property not supported.
        fRet = false;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"IsHyperthreadEnabled", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor manufacturer.

      Parameters:  manufacturer- Array of characteristics from kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetManufacturer(wstring& manufacturer) const
    {
        bool fRet = false;
#if defined(linux)
        fRet = m_cpuinfo.VendorId(manufacturer);
#elif defined(sun) && !defined(sparc)
        if (!m_processorAttr.manufacturer.empty())
        {
           manufacturer = m_processorAttr.manufacturer; 
           fRet = true;
        }
#elif defined(sun) && defined(sparc)
        fRet = false;
#elif defined(aix) || defined(hpux)
        if (!m_processorAttr.manufacturer.empty())
        {
            manufacturer = m_processorAttr.manufacturer; 
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Manufacturer", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor Id.

      Parameters:  processorId- Array of characteristics from kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetProcessorId(wstring& processorId) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun)
        processorId = L"";
        fRet = false;
#elif defined(aix) || defined(hpux)
        if (!m_processorAttr.processorId.empty())
        {
            processorId = m_processorAttr.processorId;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"ProcessorId", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Processor Version.

      Parameters:  version- Processor Version.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetVersion(wstring &version) const
    {
        bool fRet = false;
#if defined(linux) 
        fRet = m_cpuinfo.Version(version);
#elif defined(aix) || defined(hpux) || defined(sun)
        if (!m_processorAttr.version.empty())
        {
            version = m_processorAttr.version; 
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Version", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Processor Status.

      Parameters:  cpustatus- Processor status.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetCpuStatus(unsigned short& cpustatus) const
    {
        bool fRet = false;
#if defined(linux) 
        cpustatus = 1; // On Linux, processor is always active/online
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) 
        cpustatus = m_processorAttr.cpuStatus;
        fRet = true;
#elif defined(hpux) 
        // Property not supported.
        fRet = false;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"CpuStatus", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Processor External Clock.

      Parameters:  externalClock- External Clock.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetExternalClock(unsigned int& externalClock) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) 
        // The frequency is unknown.
        // Returning false results in this property being posted as NULL.
        externalClock = 0;
        fRet = false;
#elif defined(hpux) 
        if (m_processorAttr.extClock > 0)
        {
            externalClock = m_processorAttr.extClock;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"ExternalClock", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
      Core Count.

      Parameters:  numberOfCores- core count.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetNumberOfCores(unsigned int& numberOfCores) const
    {
        bool fRet = false;
#if defined(linux) 
        fRet = m_cpuinfo.CpuCores(numberOfCores);
#elif defined(aix) || defined(hpux) || defined(sun)
        if (0 < m_processorAttr.numberOfCores)
        {
            numberOfCores = m_processorAttr.numberOfCores; 
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"NumberOfCores", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Max Clock Speed.

      Parameters:  maxSpeed-  Max Clock Speed.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetMaxClockSpeed(unsigned int& maxSpeed) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun)
        maxSpeed = 0;
        fRet = false;
#elif defined(aix) || defined(hpux)
        if (0 < m_processorAttr.maxClockSpeed)
        {
            maxSpeed = m_processorAttr.maxClockSpeed; 
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"MaxClockSpeed", SCXSRCLOCATION);
#endif
        return fRet;

    }

    /*----------------------------------------------------------------------------*/
    /**
      Processor upgrade.

      Parameters:  upgradeMethod-  Processor upgrade.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetUpgradeMethod(unsigned short& upgradeMethod) const
    {
        bool fRet = false;
#if defined(linux) 
        upgradeMethod = 2/*unknown*/;
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) 
        fRet = false;
#elif defined(hpux) 
        // Property not supported.
        fRet = false;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"UpgradeMethod", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Processor Role.

      Parameters:  role-  Processor role.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.

      Role just is the same to processor type.
     */
    bool CpuPropertiesInstance::GetRole(wstring& role) const
    {
        bool fRet = false;
#if defined(linux) 
        role = cRoleStr[2]; // /proc/cpuinfo only has central processors
        fRet = true;
#elif defined(sun) 
        fRet = false;
#elif defined(aix) || defined(hpux)
        if (!m_processorAttr.role.empty())
        {
            role = m_processorAttr.role;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Role", SCXSRCLOCATION);
#endif
       return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Device ID.

      Parameters:  deviceId-  Device ID.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetDeviceID(wstring& deviceId) const
    {
        bool fRet = false;
#if defined(linux) 
        deviceId = GetId();;
        fRet = true;
#elif defined(sun) 
        deviceId = m_processorAttr.deviceID;
        fRet = true;
#elif defined(aix) || defined(hpux)
        if (!m_processorAttr.deviceID.empty())
        {
            deviceId = m_processorAttr.deviceID;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"DeviceID", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      CPU Key.

      Parameters:  cpuKey-  cpu key.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.

      CPU Key just is a key to associate with CPU,so here is equal to DeviceId.
     */
    bool CpuPropertiesInstance::GetCPUKey(wstring& cpuKey) const
    {
        bool fRet = false;
#if defined(linux) 
        cpuKey = GetId();
        fRet = true;
#elif defined(sun) 
        cpuKey = m_processorAttr.deviceID;
        fRet = true;
#elif defined(aix) || defined(hpux)
        if (!m_processorAttr.deviceID.empty())
        {
            cpuKey = m_processorAttr.deviceID;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"CPUKey", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get cpu Description.

      Parameters:  description- Description string : manufacturer Family # Model # Stepping #
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
    */
    bool CpuPropertiesInstance::GetDescription(wstring& description) const
    {
        wstring manufacturer, version;
        unsigned short ufamily;
        bool gotManufacturer, gotFamily, gotVersion;
        bool fRet = false;

        gotManufacturer = GetManufacturer(manufacturer);
        gotFamily = GetFamily(ufamily);
        gotVersion = GetVersion(version);

        if (gotManufacturer && gotFamily && gotVersion)
        {
            std::wstringstream ss;
            ss << manufacturer << L" Family " << ufamily << L" " << version;
            description = ss.str();
            fRet = true;
        } 

        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor Type.

      Parameters:  processorType- Processor type.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetProcessorType(unsigned short& processorType) const
    {
        bool fRet = false;
#if defined(linux) //|| (defined(sun) && !defined(sparc))
        fRet = m_cpuinfo.ProcessorType(processorType);
#elif defined(sun) 
        fRet = false;
#elif defined(aix) || defined(hpux)
        if (0 < m_processorAttr.processorType)
        {
            processorType = m_processorAttr.processorType;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"ProcessorType", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get architecture

      Parameters:  architecture
      Returns:     The numeric code for the platform architecture: 0 x86, 1 MIPS, 2 Alpha, 3 PowerPC, 6 Itanium, 9 x64

      ThrowException: SCXNotSupportedException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetArchitecture(unsigned short& architecture) const
    {
        bool fRet = false;

#if defined(__x86_64__) || defined(__i386__) || (defined(sun) && !defined(sparc))
        struct utsname uname_buf;
        int rc = uname( & uname_buf );
        if (0 <= rc)
        {
            wstring smachine(SCXCoreLib::StrFromUTF8(uname_buf.machine));

            // uname machine must have enough characters to identify the architecture.
            static const unsigned short MINMACHINENAMECC = 4;

            // For x86, uname machine has form "i?86", "i?86-<brand>", or "i86pc"
            if (0 == smachine.compare(0, 6, L"x86_64"))
            {
                architecture = parchX64;
                fRet = true;
            }
            else if (smachine.length() >= MINMACHINENAMECC && smachine[0] == L'i'
             && ((smachine[2] == L'8' && smachine[3] == L'6') || (smachine[1] == L'8' && smachine[2] == L'6')))

            {
                architecture = parchX86;
                fRet = true;
            }
        }
#elif defined(__ia64__)
        architecture = parchITANIUM;
        fRet = true;
#elif defined(__powerpc__)
        architecture = parchPOWERPC;
        fRet = true;
#elif defined(__mips__)
        architecture = parchMIPS;
        fRet = true;
#elif defined(__alpha__)
        architecture = parchALPHA;
        fRet = true;
#elif defined(sparc)
        // sparc is not supported by the specification but it must be supported by the processor provider.
        // We can only return NULL.
        fRet = false;
#elif defined(__hppa)
        // PA_RISC is not supported by the specification but it must be supported by the processor provider.
        // We can only return NULL.
        fRet = false;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Architecture", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor normSpeed.

      Parameters:  normSpeed- value from kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetNormSpeed(unsigned int& normSpeed) const
    {
        bool fRet = false;
#if defined(linux) 
        fRet = m_cpuinfo.CpuSpeed(normSpeed);
#elif defined(sun) 
        normSpeed = m_processorAttr.normSpeed;
        fRet = true;
#elif defined(aix) || defined(hpux)
        if (0 < m_processorAttr.normSpeed)
        {
            normSpeed = m_processorAttr.normSpeed;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"NormSpeed", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor numberOfLogicalProcessors.

      Parameters:  numberOfLogicalProcessors- numbers of kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetNumberOfLogicalProcessors(unsigned int& numberOfLogicalProcessors) const
    {
        bool fRet = false;
#if defined(linux) 
        fRet = m_cpuinfo.CpuCores(numberOfLogicalProcessors);
#elif defined(sun) 
        numberOfLogicalProcessors = m_processorAttr.numberOfLogicalProcessors;
        fRet = true;
#elif defined(aix) || defined(hpux)
        if (0 < m_processorAttr.numberOfLogicalProcessors)
        {
            numberOfLogicalProcessors = m_processorAttr.numberOfLogicalProcessors;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"NumberOfLogicalProcessors", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor currentClockSpeed.

      Parameters:  currentClockSpeed- value from kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetCurrentClockSpeed(unsigned int& currentClockSpeed) const
    {
        bool fRet = false;
#if defined(linux)
        fRet = m_cpuinfo.CpuSpeed(currentClockSpeed);
#elif defined(sun) 
        currentClockSpeed = m_processorAttr.currentClockSpeed; 
        fRet = true;
#elif defined(aix) || defined(hpux)
        if(0 < m_processorAttr.currentClockSpeed)
        {
            currentClockSpeed = m_processorAttr.currentClockSpeed; 
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"CurrentClockSpeed", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor family.

      Parameters:  family- Array of characteristics from kstat_t structure tranform to unsigned shor.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetFamily(unsigned short& family) const
    {
        bool fRet = false;
#if defined(linux) 
        family = m_family;
        fRet = true;
#elif defined(sun) 
        family = m_processorAttr.family; 
        fRet = true;
#elif defined(aix) || defined(hpux)
        if (0 < m_processorAttr.family)
        {
            family = m_processorAttr.family;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Family", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor stepping.

      Parameters:  stepping- Array of characteristics from kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetStepping(wstring& stepping) const
    {
        bool fRet = false;
#if defined(linux) 
        wstringstream ssStepping(wstringstream::out);
        unsigned short usStepping = 0;
        fRet = m_cpuinfo.Stepping(usStepping);
        if (fRet)
        {
            ssStepping << usStepping;
            stepping = ssStepping.str();
        }
#elif defined(sun) || defined(hpux)
        if (m_processorAttr.stepping != L"")
        {
            stepping = m_processorAttr.stepping;
            fRet = true;
        }
        else
        {
            fRet = false;
        }
#elif defined(aix) 
        if (!m_processorAttr.stepping.empty())
        {
            stepping = m_processorAttr.stepping;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Stepping", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get processor name.

      Parameters:  name- Array of characteristics from kstat_t structure.
      Returns:     whether the implementation for this platform supports the value.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool CpuPropertiesInstance::GetName(wstring& name) const
    {
        bool fRet = false;
#if defined(linux) 
        fRet = m_cpuinfo.ModelName(name);
#elif defined(sun) //&& defined(sparc)
        name = m_processorAttr.name;
        fRet = true;
#elif defined(aix) || defined(hpux)
        if (!m_processorAttr.name.empty())
        {
            name = m_processorAttr.name;
            fRet = true;
        }
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Name", SCXSRCLOCATION);
#endif
        return fRet;
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

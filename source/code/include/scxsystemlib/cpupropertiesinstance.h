/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       PAL representation of a PhysicalCpu.

  \date        11-10-31 14:00:00

 */
/*----------------------------------------------------------------------------*/

#ifndef CPUPROPERTIESINSTANCE_H
#define CPUPROPERTIESINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>

#if defined(linux)
#include <scxsystemlib/procfsreader.h>
#include <iomanip>

#elif defined(sun)

#if defined(ia32)
#include <scxsystemlib/procfsreader.h>
#endif

#include <scxsystemlib/cpupropertiesdependencies.h>
#include <scxsystemlib/scxkstat.h>

#elif defined(aix)

#include <libperfstat.h>
#include <sys/utsname.h>
#include <sys/systemcfg.h>

#elif defined(hpux) 
#include <sys/pstat.h>
#endif

using namespace SCXCoreLib;
using std::vector;

namespace SCXSystemLib
{
#if defined(sun) 
    /* Family array length */
    const int cFamily_Sparc_Array_Length = 9;
    /* Sparc name string value array */
    const std::wstring cFamily_Sparc_Name[] = {L"SPARC Family",L"SuperSPARC",L"microSPARC-II",L"microSPARC-IIep",L"UltraSPARC",L"UltraSPARC-II",L"UltraSPARC-IIi",L"UltraSPARC-III",L"UltraSPARC-IIIi"}; //!< Family_Sparc name array
    /* Family_Sparc name-value mapping */
    const unsigned short cFamily_Sparc_Value[] = {80,81,82,83,84,85,86,87,88};

#endif

    typedef enum _ProcessorArchitecture
    {
                    parchX86        = 0,
                    parchMIPS       = 1,
                    parchALPHA      = 2,
                    parchPOWERPC    = 3,
                    parchITANIUM    = 6,
                    parchX64        = 9
    } ProcessorArchitecture;

    typedef enum ProcessorType_t
    {
        OTHER_PROCESSOR   = 1,
        UNKNOWN_PROCESSOR = 2, 
        CENTRAL_PROCESSOR = 3, 
        MATH_PROCESSOR    = 4,
        DSP_PROCESSOR     = 5,
        VIDEO_PROCESSOR   = 6
    } ProcessorType;

    /*----------------------------------------------------------------------------*/
    /**
      Struct representing all implemented attributes for Processor.

     */
    struct ProcessorAttributes
    {
        bool is64Bit;
        bool isHyperthreadCapable;
        bool isHyperthreadEnabled;
        bool isVirtualizationCapable;
        std::wstring cpuKey;
        std::wstring manufacturer;
        std::wstring processorId;
        std::wstring version;
        bool cpuSocketPopulated;
        unsigned short cpuStatus;
        unsigned short processorType;
        unsigned int extClock;
        unsigned int numberOfCores;
        unsigned int numberOfLogicalProcessors;
        unsigned int currentClockSpeed;
        unsigned short family;
        unsigned int maxClockSpeed;
        std::wstring role;
        unsigned short upgradeMethod;
        std::wstring creationClassName;
        std::wstring deviceID;
        unsigned short normSpeed;
        std::wstring stepping;
        std::wstring name;

        ProcessorAttributes(void) :
            is64Bit(false),
            isHyperthreadCapable(false),
            isHyperthreadEnabled(false),
            isVirtualizationCapable(false),
            cpuKey(L""),
            manufacturer(L""),
            processorId(L""),
            version(L""),
            cpuSocketPopulated(false),
            cpuStatus(0),
            processorType(0),
            extClock(0),
            numberOfCores(0),
            numberOfLogicalProcessors(0),
            currentClockSpeed(0),
            family(0),
            maxClockSpeed(0),
            role(L""),
            upgradeMethod(0),
            creationClassName(L""),
            deviceID(L""),
            normSpeed(0),
            stepping(L""),
            name(L"")
        {}
    };

    /*----------------------------------------------------------------------------*/
    /**
      Class that represents values related to Processor.
      Concrete implementation of an instance of a Processor.
     */
    class CpuPropertiesInstance : public EntityInstance
    {
        friend class CpuPropertiesEnumeration;

        public:

#if defined(linux) 
        explicit CpuPropertiesInstance(const std::wstring& id, const ProcfsCpuInfo &cpuinfo);
#elif defined(sun) 
        explicit CpuPropertiesInstance(const std::wstring& cpuInfoIndex, SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies> deps = SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies>(new CpuPropertiesPALDependencies()));
#elif defined(aix) 
        CpuPropertiesInstance(const perfstat_cpu_total_t & cpuTotal, const perfstat_partition_total_t & partTotal);
#elif defined(hpux) 
        CpuPropertiesInstance(const std::wstring& id, const pst_processor& proc, const pst_dynamic& psd);
#endif
        virtual ~CpuPropertiesInstance();

        virtual void Update();
        virtual void CleanUp();



        /*----------------------------------------------------------------------------*/
        /**
        Get NormSpeed.

        Parameters:  norm Speed - RETURN:normSpeed of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetNormSpeed(unsigned int& normSpeed) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get stepping.

        Parameters:  stepping - RETURN: stepping of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetStepping(std::wstring& stepping) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get name.

        Parameters:  name - RETURN: name of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetName(std::wstring& name) const;
        /*----------------------------------------------------------------------------*/
        /**
          Check that whether the maximum data width capability of the processor is 64-bit.

          Parameters:  is64Bit- whether the processor is 64-bit.
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetIs64Bit(bool &is64Bit) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get Manufacturer

        Parameters:  manufacturer - RETURN: manufacturer of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetManufacturer(std::wstring& manufacturer) const ;

        /*----------------------------------------------------------------------------*/
        /**
        Get ProcessorId

        Parameters:  processorId - RETURN: processorId of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetProcessorId(std::wstring& processorId) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get processorType

        Parameters:  processor Type - RETURN: processorType of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetProcessorType(unsigned short& processorType) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get architecture

        Parameters:  architecture - RETURN: architecture of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetArchitecture(unsigned short& architecture) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get numberOfLogicalProcessors.

        Parameters:  numberOfLogicalProcessors - RETURN: numberOfLogicalProcessors of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetNumberOfLogicalProcessors(unsigned int& numberOfLogicalProcessors) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get currentClockSpeed.

        Parameters:  currentClockSpeed - RETURN: currentClockSpeed of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetCurrentClockSpeed(unsigned int& currentClockSpeed) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get family.

        Parameters:  family - RETURN: family of CpuProperties instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetFamily(unsigned short& family) const;
        /*----------------------------------------------------------------------------*/
        /**
          Check that whether the processor supports multiple hardware threads per core.

          Parameters:  isHyperthreadCapable- whether the processor is support multiple threads.
          Returns:     whether the implementation for this platform supports this attribute.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetIsHyperthreadCapable(bool &isHyperthreadCapable) const;
        /*----------------------------------------------------------------------------*/
        /**
          Check that whether the processor is capable of executing enhanced virtualization instructions.

          Parameters:  isVirtualizationCapable- whether the processor is capable of executing enhanced virtualization instructions.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetIsVirtualizationCapable(bool &isVirtualizationCapable) const;
        /*----------------------------------------------------------------------------*/
        /**
          Check that whether the Hyperthread function is enabled.

          Parameters:  IsHyperthreadEnabled - whether the Hyperthread function is enabled.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetIsHyperthreadEnabled(bool &isHyperthreadEnabled) const;
        /*----------------------------------------------------------------------------*/
        /**
          Processor Version.

          Parameters:  version- Processor Version.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetVersion(std::wstring &version) const;
        /*----------------------------------------------------------------------------*/
        /**
          Processor Status.

          Parameters:  cpustatus- Processor status.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetCpuStatus(unsigned short& cpustatus) const;
        /*----------------------------------------------------------------------------*/
        /**
          CPU Socket Populated.

          Parameters:  cpusocketpopulated- CPU Socket Populated.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetCpuSocketPopulated(bool& cpusocketpopulated) const;
        /*----------------------------------------------------------------------------*/
        /**
          Processor External Clock.

          Parameters:  externalClock- External Clock.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetExternalClock(unsigned int& externalClock) const;
        /*----------------------------------------------------------------------------*/
        /**
          Core Count.

          Parameters:  numberOfCores- core count.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetNumberOfCores(unsigned int& numberOfCores) const;
        /*----------------------------------------------------------------------------*/
        /**
          Max Clock Speed.

          Parameters:  maxSpeed-  Max Clock Speed.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetMaxClockSpeed(unsigned int& maxSpeed) const;
        /*----------------------------------------------------------------------------*/
        /**
          Processor upgrade.

          Parameters:  upgradeMethod-  Processor upgrade.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetUpgradeMethod(unsigned short& upgradeMethod) const;
        /*----------------------------------------------------------------------------*/
        /**
          Processor Role.

          Parameters:  role-  Processor role.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetRole(std::wstring& role) const;
        /*----------------------------------------------------------------------------*/
        /**
           Device ID.
                
           Parameters:  deviceId-  Device ID.
           Returns:     whether the implementation for this platform supports the value.
           ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetDeviceID(std::wstring& deviceId) const;
        /*----------------------------------------------------------------------------*/
        /**
          CPU Key.

          Parameters:  cpuKey-  cpu key.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetCPUKey(std::wstring& cpuKey) const;
        /**
          CPU Description

          Parameters:  description -  CPU description.
          Returns:     whether the implementation for this platform supports the value.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetDescription(std::wstring& description) const;

        private:

#if defined(linux)
        ProcfsCpuInfo m_cpuinfo;
        ProcfsCpuInfoReader m_cpuinfoTable;
        unsigned short m_family;
        unsigned short GetFamily();
#elif defined(hpux)
        std::wstring m_socketId;
#elif defined(sun) 
        SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies> m_deps; //!< Collects external dependencies of this class.
        std::wstring m_cpuInfoIndex;
#elif defined(aix) 
        bool FillAttributes(void);

// _system_configuration.implementation
#if !defined(POWER_7)
#define POWER_7     0x8000
#endif

// _system_configuration.version
#if !defined(PV_5_2)
#define PV_5_2          0x0F0001      /* Power PC 5 */
#endif
#if !defined(PV_5_3)
#define PV_5_3          0x0F0002      /* Power PC 5 */
#endif
#if !defined(PV_6)
#define PV_6            0x100000      /* Power PC 6 */
#endif
#if !defined(PV_6_1)
#define PV_6_1          0x100001      /* Power PC 6 DD1.x */
#endif
#if !defined(PV_7)
#define PV_7            0x200000      /* Power PC 7 */
#endif
#if !defined(PV_5_Compat)
#define PV_5_Compat     0x0F8000      /* Power PC 5 */
#endif
#if !defined(PV_6_Compat)
#define PV_6_Compat     0x108000      /* Power PC 6 */
#endif

#if !defined(PV_7_Compat)
#define PV_7_Compat     0x208000      /* Power PC 7 */
#endif

#if !defined(RS6K_UP_MCA)
#define RS6K_UP_MCA  1
#endif

#if !defined(RS6K_SMP_MCA)
#define RS6K_SMP_MCA    2
#endif

#if !defined(RSPC_UP_PCI)
#define RSPC_UP_PCI 3
#endif

#if !defined(RSPC_SMP_PCI)
#define RSPC_SMP_PCI    4
#endif

#if !defined(CHRP_UP_PCI)
#define CHRP_UP_PCI    5
#endif

#if !defined(CHRP_SMP_PCI)
#define CHRP_SMP_PCI    6
#endif

#if !defined(IA64_COM)
#define IA64_COM     7
#endif

#if !defined(IA64_SOFTSDV)
#define IA64_SOFTSDV     8
#endif
        typedef std::map<int, std::wstring>  MODEL_MAP;
        typedef MODEL_MAP::value_type     MODEL_MAP_ENTRY;
        typedef MODEL_MAP::iterator       MODEL_MAP_IT;
        typedef MODEL_MAP::const_iterator MODEL_MAP_CIT;

        static const MODEL_MAP_ENTRY aSysConfigImplPairs[];
        static const MODEL_MAP SysConfigImplLookup;

        static const MODEL_MAP_ENTRY aSysConfigVersionPairs[];
        static const MODEL_MAP SysConfigVersionLookup;

        static const MODEL_MAP_ENTRY aSysConfigModelImplPairs[];
        static const MODEL_MAP SysConfigModelImplLookup;
#endif
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
        struct ProcessorAttributes m_processorAttr;   //!< Processor attributes.
    };

}

#endif /* CPUPROPERTIESINSTANCE_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

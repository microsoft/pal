/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       PAL representation of a Processor. 

  \date        11-03-18 14:00:00 

 */
/*----------------------------------------------------------------------------*/
#ifndef PROCESSORINSTANCE_H
#define PROCESSORINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>


#if defined(linux)
#include <scxsystemlib/scxsmbios.h>
#include <iomanip>
#elif defined(sun)

#if defined(sparc)
#include <scxsystemlib/processordependencies.h>
#include <scxsystemlib/scxkstat.h>
#else
#include <scxsystemlib/scxsmbios.h>
#include <iomanip>
#endif

#endif


using namespace SCXCoreLib;
using std::vector;

namespace SCXSystemLib
{
#if defined(sun) && defined(sparc)
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
    };

    /*----------------------------------------------------------------------------*/
    /**
      Class that represents values related to Processor.
      Concrete implementation of an instance of a Processor. 
     */
    class ProcessorInstance : public EntityInstance
    {
        friend class ProcessorEnumeration;

        public:

#if defined(linux) || (defined(sun) && !defined(sparc))
        explicit ProcessorInstance(const std::wstring& id,const struct SmbiosEntry &curSmbiosEntry,const MiddleData& smbiosTable);
#elif defined(sun) && defined(sparc) 
        explicit ProcessorInstance(const std::wstring& cpuInfoIndex, SCXCoreLib::SCXHandle<ProcessorPALDependencies> deps = SCXCoreLib::SCXHandle<ProcessorPALDependencies>(new ProcessorPALDependencies()));
#endif
        virtual ~ProcessorInstance();

        virtual void Update();
        virtual void CleanUp();



        /*----------------------------------------------------------------------------*/
        /**
        Get NormSpeed. 

        Parameters:  norm Speed - RETURN: processorType  of normSpeed instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetNormSpeed(unsigned int& normSpeed) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get stepping. 

        Parameters:  stepping - RETURN: stepping  of Processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetStepping(std::wstring& stepping) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get name. 

        Parameters:  name - RETURN: name  of Processor instance

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

        Parameters:  manufacturer - RETURN: manufacturer  of Processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetManufacturer(std::wstring& manufacturer) const ;

        /*----------------------------------------------------------------------------*/
        /**
        Get ProcessorId

        Parameters:  processorId - RETURN: processorId  of Processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetProcessorId(std::wstring& processorId) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get processorType

        Parameters:  processor Type - RETURN: processorType  of Processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetProcessorType(unsigned short& processorType) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get architecture

        Parameters:  architecture - RETURN: architecture of processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetArchitecture(unsigned short& architecture) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get numberOfLogicalProcessors. 

        Parameters:  numberOfLogicalProcessors - RETURN: numberOfLogicalProcessors  of Processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetNumberOfLogicalProcessors(unsigned int& numberOfLogicalProcessors) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get currentClockSpeed. 

        Parameters:  currentClockSpeed - RETURN: currentClockSpeed  of Processor instance

        Retval:      true if a value is supported by this implementation
        */
        bool GetCurrentClockSpeed(unsigned int& currentClockSpeed) const;
        /*----------------------------------------------------------------------------*/
        /**
        Get family. 

        Parameters:  family - RETURN: family  of Processor instance

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




        private:
#if defined(linux) || (defined(sun) && !defined(sparc))
        /*----------------------------------------------------------------------------*/
        /**
          Parse SMBIOS Structure Table. 

          Returns:     whether it's successful to parse it. 
         */
        bool ParseSmbiosTable();
        /**
          Read the attributes of Processor Information . 

          Parameters:  offsetProcessor- offset where the Processor Info structure is in SMBIOS Table. 
          Parameters:  structureStart- offset where the string-set is in SMBIOS Table. 
          Returns:     true,successful to read it;otherwise,false. 
         */
        bool ReadProcessorAttr(const size_t offsetProcessor,const size_t offsetStringSet);
#endif

        private:

#if defined(linux) || (defined(sun) && !defined(sparc))
        struct SmbiosEntry m_smbiosEntry; //!<To get Smbios Table via current SmbiosEntry.
        SCXCoreLib::SCXHandle<SCXSmbios> m_scxsmbios; //!< Collects external dependencies of this class
        MiddleData m_smbiosTable; //!< Smbios Table
        unsigned short m_smbiosVersion;//Smbios Version multiplied by 10
#elif defined(sun) && defined(sparc) 
        SCXCoreLib::SCXHandle<ProcessorPALDependencies> m_deps; //!< Collects external dependencies of this class.
        std::wstring m_cpuInfoIndex;
#endif
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
        struct ProcessorAttributes m_processorAttr;   //!< Processor attributes. 
    };

}

#endif /* PROCESSORINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

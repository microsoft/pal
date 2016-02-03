/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
  \file        computersysteminstance.cpp

  \brief       PAL representation of a ComputerSystem

  \date        2011-04-18 14:30:00

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxsystemlib/scxdatadef.h>
#include <scxsystemlib/computersysteminstance.h>
#include <scxsystemlib/cpupropertiesenumeration.h>
#include <sstream>
#if defined(aix)
    #include <sys/utsname.h>
    #include <time.h>
    extern int daylight;
    #include <cf.h>
    #include <scxsystemlib/scxodm.h>
#endif
#if defined(hpux)
    #include <time.h>
    #include <scxsystemlib/scxostypeinfo.h>
    #include <unistd.h>
    #include <stddef.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

using namespace SCXCoreLib;


namespace SCXSystemLib
{
#if FILTERLINUX
      /** The anchor-string to confirm whether the line represents a logical processor*/
      const std::wstring cAnchorLogicalProcessor = L"processor";

      /** The type value of System Reset structure*/
      const unsigned short cSystemReset = 23;
      /** bit offset where 1st bit to indicate Boot Option on Limit is in System Reset structure*/
      const int cBootOptionFirst = 0x04;
      /** bit offset where 2nd bit to indicate Boot Option on Limit is in System Reset structure*/
      const int cBootOptionSecond = 0x03;
      /** bit offset where 1st bit to indicate Boot Option on Watch Dog is in System Reset structure*/
      const int cBootOptionDogSt = 0x02;
      /** bit offset where 2nd bit to indicate Boot Option on Watch Dog is in System Reset structure*/
      const int cBootOptionDogNd = 0x01;
      /** offset where the attribute "Capabilities" is in System Reset structure*/
      const int cCapabilities = 0x04;
      /** offset where the attribute "Reset Count" is in System Reset structure*/
      const int cResetCount = 0x05;
      /** offset where the attribute "Reset Limit" is in System Reset structure*/
      const int cResetLimit = 0x07;

      /** The type value of System Information structure*/
      const unsigned short cSystemInfo = 1;
      /** offset where the attribute "Manufacturer" is in System Information structure*/
      const int cSystemManufacturer = 0x04;
      /** offset where the attribute "Product Name" is in System Information structure*/
      const int cProductName = 0x05;
      /** offset where the attribute "Wake-up Type" is in System Information structure*/
      const int cWakeupType = 0x18;

      /** The type value of System Enclosure or Chassis structure*/
      const unsigned short cSystemEnclosureOrChassis = 3;
      /** offset where the attribute "Manufacturer" is in System Enclosure or Chassis structure*/
      const int cManufacturer = 0x04;
      /** offset where the attribute "Type" is in System Enclosure or Chassis structure*/
      const int cType = 0x05;
      /** offset where the attribute "Boot-up State" is in System Enclosure or Chassis structure*/
      const int cBootUpState = 0x09;
      /** offset where the attribute "PowerSupplyState" is in System Enclosure or Chassis structure*/
      const int cPowerSupplyState = 0x0A;
      /** offset where the attribute "ThermalState" is in System Enclosure or Chassis structure*/
      const int cThermalState = 0x0B;
      /**The value of attribute "Type" for Peripheral device enclosure.*/
      const unsigned short cPeripheralDevice = 0x15;
#elif defined(sun)
        /** Run level description*/
    const std::wstring cRunLevel3 = L"run-level 3";
#endif

#if FILTERLINUX
    /*----------------------------------------------------------------------------*/
    /**
      Constructor

      Parameters: scxsmbios-  Dependencies in SMBIOS standard ,which is used for the ComputerSystem data colletion.
      Parameters: deps-  Dependencies only used for the ComputerSystem data colletion.

     */
    ComputerSystemInstance::ComputerSystemInstance(SCXHandle<SCXSmbios> scxsmbios,
                                        SCXHandle<ComputerSystemDependencies> deps) :
        m_scxsmbios(scxsmbios),
        m_hasSystemReset(false),
        m_deps(deps)
#elif defined(sun) || defined(aix) || defined(hpux)
    /*----------------------------------------------------------------------------*/
    /**
      Constructor

          Parameters:  ComputerSystemDependencies - Dependencies to get data
     */
    ComputerSystemInstance::ComputerSystemInstance(SCXCoreLib::SCXHandle<ComputerSystemDependencies> deps) :
        m_deps(deps)
#else
    ComputerSystemInstance::ComputerSystemInstance()
#endif
    {
        m_computersystemAttr.automaticResetCapability = false;
        m_computersystemAttr.bootOptionOnLimit = 0;
        m_computersystemAttr.bootOptionOnWatchDog = 0;
        m_computersystemAttr.chassisBootupState = 0;
        m_computersystemAttr.daylightInEffect = false;
        m_computersystemAttr.dnsHostName = L"";
        m_computersystemAttr.manufacturer = L"";
        m_computersystemAttr.model = L"";
        m_computersystemAttr.networkServerModeEnabled = false;
        m_computersystemAttr.powerSupplyState = 0;
        m_computersystemAttr.powerManagementCapabilities.clear();
        m_computersystemAttr.powerManagementSupported = false;
        m_computersystemAttr.resetCount = 0;
        m_computersystemAttr.resetLimit = 0;
        m_computersystemAttr.thermalState = 0;
        m_computersystemAttr.wakeUpType = 0;

        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.computersystem.computersysteminstance"));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Default destructor
    */
    ComputerSystemInstance::~ComputerSystemInstance()
    {
        SCX_LOGTRACE(m_log, std::wstring(L"CpuPropertiesInstance default destructor: "));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Update current Computer Sytem instance to latest values
     */
    void ComputerSystemInstance::Update()
    {
#if FILTERLINUX
        struct SmbiosEntry smbiosEntry;
        smbiosEntry.tableAddress = 0;
        smbiosEntry.tableLength = 0;
        smbiosEntry.structureNumber = 0;
        m_scxsmbios->ParseSmbiosEntryStructure(smbiosEntry);
        ParseSmbiosTable(smbiosEntry);
#endif
#if defined(sun) || defined(aix) || defined(hpux)
        std::wstring tmpRes =L"";
        // ------------------------------------
        if(m_deps->GetSystemRunLevel(tmpRes))
        {
            m_runLevel = tmpRes;
        }
#endif
#if defined(sun)
        if(m_deps->GetSystemInfo(SI_PLATFORM, tmpRes) >0)
        {
            m_computersystemAttr.model = tmpRes;
        }

        // Newer Sun hardware and system revisions return unexpected bytes,
        // (perhaps special characters), so just hard code what we expect
        m_computersystemAttr.manufacturer = L"Oracle Corporation";

        if (m_deps->GetPowerCfg(m_powerConfAllLines))
        {
            for(vector<std::wstring>::iterator it = m_powerConfAllLines.begin(); it != m_powerConfAllLines.end(); it++)
            {
                size_t pos = it->find(L"autopm");
                if (pos != std::wstring::npos)
                {
                    if (it->find(L"default", 5) != std::wstring::npos)
                    {
                        m_computersystemAttr.powerManagementCapabilities.push_back(eEnabled);
                    }
                    else if (it->find(L"disable", 5) != std::wstring::npos)
                    {
                        m_computersystemAttr.powerManagementCapabilities.push_back(eDisabled);
                    }else
                    {
                        m_computersystemAttr.powerManagementCapabilities.push_back(eUnknown);
                    }
                    break;
                }
            }
        }
        m_isGetDayLightFlag = m_deps->GetSystemTimeZone(m_computersystemAttr.daylightInEffect);
#elif defined(hpux)
        m_computersystemAttr.manufacturer = L"Hewlett-Packard Company";
#elif defined(aix)
        m_computersystemAttr.manufacturer = L"International Business Machines Corporation";
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Clean up.
    */
    void ComputerSystemInstance::CleanUp()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
      Get AutomaticResetCapability.

      Parameters:  automaticResetCapability
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetAutomaticResetCapability(bool &automaticResetCapability) const
    {
        automaticResetCapability = m_computersystemAttr.automaticResetCapability;
#if FILTERLINUX
        if (m_hasSystemReset)
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(sun)
        return false;
#elif defined(aix)
        utsname unameInfo;
        if (!(uname(&unameInfo) < 0))
        {
            return strcmp(unameInfo.version, "5.3") >= 0;
        }
        else
        {
            return false;
        }
#elif defined(hpux)
        // Automatic System reboot is configured in the GSP/MP, if it exists
        struct stat buf;
        automaticResetCapability = (stat("/dev/GSPdiag1", &buf) == 0);
        return true;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"AutomaticResetCapability", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get BootOptionOnLimit.

      Parameters:  bootOptionOnLimit
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetBootOptionOnLimit(unsigned short &bootOptionOnLimit) const
    {
        bootOptionOnLimit = m_computersystemAttr.bootOptionOnLimit;
#if FILTERLINUX
                if (m_hasSystemReset)
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"BootOptionOnLimit", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get BootOptionOnWatchDog.

      Parameters:  bootOptionOnWatchDog
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetBootOptionOnWatchDog(unsigned short &bootOptionOnWatchDog) const
    {

        bootOptionOnWatchDog =  m_computersystemAttr.bootOptionOnWatchDog;
#if FILTERLINUX
        if (m_hasSystemReset)
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"BootOptionOnWatchDog", SCXSRCLOCATION);
#endif
    }


    /*----------------------------------------------------------------------------*/
    /**
      Get ChassisBootupState.

      Parameters:  chassisBootupState
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetChassisBootupState(unsigned short &chassisBootupState) const
    {

        chassisBootupState = m_computersystemAttr.chassisBootupState;
#if FILTERLINUX
        return true;
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"ChassisBootupState", SCXSRCLOCATION);
#endif
        }

    /*----------------------------------------------------------------------------*/
    /**
      Get DaylightInEffect.

      Parameters:  daylightInEffect
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetDaylightInEffect(bool &daylightInEffect) const
    {
        daylightInEffect = false;
#if FILTERLINUX
        return false;
#elif defined(sun)
        if (m_isGetDayLightFlag)
        {
            daylightInEffect = m_computersystemAttr.daylightInEffect;
            return true;
        }
        else
        {
            return false;
        }
#elif defined(aix) || defined(hpux)
        tzset();
        return daylight != 0;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"DaylightInEffect", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get DNSHostName.

      Parameters:  dnsHostName
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetDNSHostName(std::wstring &dnsHostName) const
    {
        dnsHostName = L"";
#if !FILTERLINUX && !defined(sun) && !defined(aix)
        // Not supported platform
        throw SCXNotSupportedException(L"DNSHostName", SCXSRCLOCATION);
#endif

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get description.

      Parameters:  description
      Returns:     The old-style description of the CPU type
      ThrowException: SCXNotSupportException - platform is not supported
     */
    bool ComputerSystemInstance::GetDescription(std::wstring& description) const
    {
#if defined(linux)
        description = L"AT/AT COMPATIBLE";    // The Windows client returns this for all Pentium & x64 CPUs
#elif defined(sun)
    #if defined(sparc)
        description = L"SPARC";
    #else
        description = L"AT/AT COMPATIBLE";
    #endif
#elif defined(hpux)
    #if defined(__hppa)
        description = L"PA RISC";
    #else
        description = L"Itanium";
    #endif
#elif defined(aix)
        description = L"POWER PC";
#else
        // Platform is not supported
        throw SCXNotSupportedException(L"Description", SCXSRCLOCATION);
#endif
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get Manufacturer.

      Parameters:  manufacturer
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetManufacturer(std::wstring &manufacturer) const
    {
        manufacturer = m_computersystemAttr.manufacturer;
#if FILTERLINUX
        return true;
#elif defined(sun) || defined(aix) || defined(hpux)
        if (!m_computersystemAttr.manufacturer.empty())
        {
            manufacturer = m_computersystemAttr.manufacturer;
            return true;
        }
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"Manufacturer", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get Model.

      Parameters:  model
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetModel(std::wstring &model) const
    {
        model = L"";
#if defined(sun) || defined(linux)
        if (!m_computersystemAttr.model.empty())
        {
            model = m_computersystemAttr.model;
            return true;
        }
        return false;
#elif defined(hpux)
        size_t bufsize;
        bufsize = confstr(_CS_MACHINE_MODEL,NULL,(size_t)0);
        char *buffer = new char[bufsize+1];
        int status = confstr(_CS_MACHINE_MODEL,buffer,bufsize+1);
        model = StrFromUTF8(buffer);
        delete[] buffer;
        if (status <= 0)
            return false;
        SCX_LOGINFO(m_log, model);
        return true;
#elif defined(aix)
       
        SCXHandle<SCXSystemLib::SCXodm> odm = SCXCoreLib::SCXHandle<SCXodm>(new SCXodm()); 
        struct CuAt dvData;
        void * pResult;

        if(odm != NULL)
            pResult= odm->Get(CuAt_CLASS, "attribute = modelname", &dvData, SCXodm::eGetFirst);
        else
            SCX_LOGINFO(m_log, L"odm is NULL");
 
        if (pResult != NULL)
        {
            model = StrFromUTF8(((CuAt *)pResult)->value);
            return true;
        }
        else
        {
            SCX_LOGWARNING(m_log, L"Look up machine modelname failed.");
            return false;
        }
#else
        // Not supported platform
        throw SCXNotSupportedException(L"Model", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get NetworkServerModeEnabled.

      Parameters:  networkServerModeEnabled
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetNetworkServerModeEnabled(bool &networkServerModeEnabled) const
    {
                // This is a Windows 98 property, horribly mislabeled, referring to FileSystem enhancement.
                // It does not map to the Linux/Unix world.
        networkServerModeEnabled = false;
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get PowerSupplyState.

      Parameters:  powerSupplyState
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetPowerSupplyState(unsigned short &powerSupplyState) const
    {
        powerSupplyState = m_computersystemAttr.powerSupplyState;
#if FILTERLINUX
        return true;
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"PowerSupplyState", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get PowerManagementCapabilities.

      Parameters:  powerManagementCapabilities
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetPowerManagementCapabilities(vector<unsigned int> &powerManagementCapabilities) const
    {
        //Avoid compiling error.
        powerManagementCapabilities.clear();

#if FILTERLINUX || defined(aix) || defined(hpux)
        return false;
#elif defined(sun)
        if (m_computersystemAttr.powerManagementCapabilities.size() >0)
        {
            powerManagementCapabilities = m_computersystemAttr.powerManagementCapabilities;
            return true;
        }
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"PowerManagementCapabilities", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get PowerManagementSupported.

      Parameters:  powerManagementSupported
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetPowerManagementSupported(bool &powerManagementSupported) const
    {
        powerManagementSupported = false;
#if FILTERLINUX
        return false;
#elif defined(sun)
        //
        //For Solaris sparc platform, the architecture support power management, so the return value always equals true.
        //
        powerManagementSupported = true;
        return true;
#elif defined(aix)
        return false;
#elif defined(hpux)
        struct stat buf;
        powerManagementSupported = (stat("/dev/GSPdiag1", &buf) == 0);
        return true;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"PowerManagementSupported", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get ResetCount.

      Parameters:  resetCount
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetResetCount(short &resetCount) const
    {
        resetCount = m_computersystemAttr.resetCount;
#if FILTERLINUX
        if (m_hasSystemReset)
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"ResetCount", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get ResetLimit.

      Parameters:  resetLimit
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetResetLimit(short &resetLimit) const
    {
        resetLimit = m_computersystemAttr.resetLimit;
#if FILTERLINUX
                if (m_hasSystemReset)
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"ResetLimit", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get ThermalState.

      Parameters:  thermalState
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetThermalState(unsigned short &thermalState) const
    {
        thermalState = m_computersystemAttr.thermalState;
#if FILTERLINUX
        return true;
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"ThermalState", SCXSRCLOCATION);
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get WakeUpType.

      Parameters:  wakeUpType
      Returns:     whether the implementation for this platform supports the value or not.
      ThrowException: SCXNotSupportException - For not implemented platform.
     */
    bool ComputerSystemInstance::GetWakeUpType(unsigned short &wakeUpType) const
    {
        wakeUpType = m_computersystemAttr.wakeUpType;
#if FILTERLINUX
        return true;
#elif defined(sun) || defined(aix) || defined(hpux)
        return false;
#else
        // Not supported platform
        throw SCXNotSupportedException(L"WakeUpType", SCXSRCLOCATION);
#endif
    }

#if FILTERLINUX
        /*----------------------------------------------------------------------------*/
    /**
      Parse SMBIOS Structure Table.

      Parameters:  curSmbiosEntry- strcuture holding part fields of SMBIOS Structure Table Entry Point to parse SMBIOS Table.
      Returns:     whether it's successful to parse it.
     */
    bool ComputerSystemInstance::ParseSmbiosTable(const struct SmbiosEntry &curSmbiosEntry)
    {
        //
        //Search in SMBIOS table to find the structures related to SCXCM_ComputerSystem.
        //
        try
        {
            if(1 > curSmbiosEntry.tableLength)
            {
                throw SCXCoreLib::SCXInternalErrorException(L"The length of SMBIOS Table is invalid.", SCXSRCLOCATION);
            }
            //
            //Get content of SMBIOS Table via Entry Point.
            //
            MiddleData smbiosTable(curSmbiosEntry.tableLength);
            m_scxsmbios->GetSmbiosTable(curSmbiosEntry,smbiosTable);
            if(1 > smbiosTable.size())
            {
                throw SCXCoreLib::SCXInternalErrorException(L"The length of SMBIOS Table is invalid.", SCXSRCLOCATION);
            }
            //
            //Search SMBIOS Table to find Processor Information structure.
            //
            unsigned char* ptableBuf = &(smbiosTable[0]);
            size_t curNumber = 0;
            size_t curLength = 0;
            while((curNumber<curSmbiosEntry.structureNumber) && (curSmbiosEntry.tableLength - curLength>=cHeaderLength))
            {
                SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosTable() - curLength: ", curLength));
                unsigned char *pcurHeader = ptableBuf + curLength;
                //
                //Type Indicator and length of current SMBIOS structure.
                //
                unsigned short type = pcurHeader[cTypeStructure];
                SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosTable() - type: ", type));
                unsigned short length = pcurHeader[cLengthStructure];
                SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosTable() - length: ", length));
                //
                //If length less than cHeaderLength,it's a absolute error,and can't find exactly the next SMBIOS structure via length.
                //
                if(cHeaderLength > length)
                {
                    throw SCXCoreLib::SCXInternalErrorException(L"The SMBIOS Table is broken.", SCXSRCLOCATION);
                }

                //
                //Read related Smbios structures separately.
                //
                switch(type)
                {
                    case cSystemReset:
                        {
                            m_hasSystemReset = true;
                            ReadSystemResetAttr(ptableBuf,curLength);
                            break;
                        }
                    case cSystemInfo:
                        {
                            ReadSystemInfoAttr(smbiosTable,curLength,curLength+length);
                            break;
                        }
                    case cSystemEnclosureOrChassis:
                        {
                            ReadSystemEnclosureOrChassisAttr(smbiosTable,curLength,curLength+length);
                            break;
                        }
                    default :
                        break;
                }

                //
                //otherwise,find the next SMBIOS structure for the one whose type is related to SCXCM_ComputerSystem.
                //the start address of next structure is after cur structure and it's corresponding string section following.
                //the following string section terminated with two null (00h) BYTES.
                //
                curLength += length;
                curNumber++;
                while(curLength < curSmbiosEntry.tableLength)
                {
                    if((0 != (ptableBuf+curLength)[0]) ||
                            0!= (ptableBuf+curLength)[1])
                    {
                        curLength++;
                    }
                    else /*otherwise,the terminal bytes appear(two null together).*/
                    {
                        SCX_LOGTRACE(m_log, std::wstring(L"ParseSmbiosTable textstring terminate"));
                        curLength += 2;
                        break;
                    }
                }
            }
        }
        catch(const SCXException& e)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"failed to ParseSmbiosTable.", SCXSRCLOCATION);
        }

       return true;
    }

    /**
      Read the attributes of System Reset structure Information .

      Parameters:  psmbiosTable- Point to the beginning of Smbios Table Buffer.
      Parameters:  offsetStructure- offset where the System Reset structure is in SMBIOS Table.
      Returns:     true,successful to read it;otherwise,false.
     */
    bool ComputerSystemInstance::ReadSystemResetAttr(const unsigned char* psmbiosTable,const size_t offsetStructure)
    {
        const unsigned char* psystemReset = psmbiosTable + offsetStructure;

        //
        //AutomaticResetCapability,BootOptionOnLimit,BootOptionOnWatchDog,ResetCount,ResetLimit.
        //Bits 4:3 indicate Boot Option on Limit;Bits 2:1 indicate Boot Option on WactchDog,Bit 0 Status(Automatic Reset).
        //
        unsigned short flag = 0x01;
        unsigned short capabilities = psystemReset[cCapabilities];
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemResetAttr() - capabilities: ", capabilities));
        if(FILTER(capabilities,flag)) m_computersystemAttr.automaticResetCapability = true;
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemResetAttr() - automaticResetCapability: ", m_computersystemAttr.automaticResetCapability));
        int sumLimit = FILTERBIT(capabilities,flag,cBootOptionFirst) + FILTERBIT(capabilities,flag,cBootOptionSecond);
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemResetAttr() - sumLimit: ", sumLimit));
        switch(sumLimit)
        {
            case eDoubleZero:
                {
                    m_computersystemAttr.bootOptionOnLimit = eReserve;
                    break;
                }
            case eZeroOne:
                {
                    m_computersystemAttr.bootOptionOnLimit = eOperatingSystem;
                    break;
                }
            case eOneZero:
                {
                    m_computersystemAttr.bootOptionOnLimit = eSystemUtilities;
                    break;
                }
            case eDoubleOne:
                {
                    m_computersystemAttr.bootOptionOnLimit = eDotreboot;
                    break;
                }
            default:
                    break;
        }
        int sumWatchDog = FILTERBIT(capabilities,flag,cBootOptionDogSt) + FILTERBIT(capabilities,flag,cBootOptionDogNd);
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemResetAttr() - sumWatchDog: ", sumWatchDog));
        switch(sumWatchDog)
        {
            case eDoubleZeroDog:
                {
                    m_computersystemAttr.bootOptionOnWatchDog = eReserve;
                    break;
                }
            case eZeroOneDog:
                {
                    m_computersystemAttr.bootOptionOnWatchDog = eOperatingSystem;
                    break;
                }
            case eOneZeroDog:
                {
                    m_computersystemAttr.bootOptionOnWatchDog = eSystemUtilities;
                    break;
                }
            case eDoubleOneDog:
                {
                    m_computersystemAttr.bootOptionOnWatchDog = eDotreboot;
                    break;
                }
            default:
                    break;
        }

        m_computersystemAttr.resetCount = MAKEWORD(psystemReset+cResetCount,psystemReset+cResetCount+1);
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemResetAttr() - resetCount: ", m_computersystemAttr.resetCount));
        m_computersystemAttr.resetLimit = MAKEWORD(psystemReset+cResetLimit,psystemReset+cResetLimit+1);
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemResetAttr() - resetLimit: ",  m_computersystemAttr.resetLimit));

        return true;
    }
    /**
      Read the attributes of System Information structure .

      Parameters:  psmbiosTable- Point to the beginning of Smbios Table Buffer.
      Parameters:  offsetStructure- offset where the System Information structure is in SMBIOS Table.
      Parameters:  offsetStringSet- offset where the string-set is in SMBIOS Table.
      Returns:     true,successful to read it;otherwise,false.
     */
    bool ComputerSystemInstance::ReadSystemInfoAttr(const MiddleData& smbiosTable,const size_t offsetStructure,
            const size_t offsetStringSet)
    {
        const unsigned char* psystemInfo = &(smbiosTable[offsetStructure]);

        //
        //WakeUpType,System Manufacturer,Model.
        //
        unsigned short wakeupType = psystemInfo[cWakeupType];
        m_computersystemAttr.wakeUpType = wakeupType;
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemInfoAttr() - wakeupType: ", wakeupType));

        size_t stringIndex = psystemInfo[cSystemManufacturer];
        m_computersystemAttr.manufacturer = m_scxsmbios->ReadSpecifiedString(smbiosTable,offsetStringSet,stringIndex);

        stringIndex = psystemInfo[cProductName];
        m_computersystemAttr.model = m_scxsmbios->ReadSpecifiedString(smbiosTable,offsetStringSet,stringIndex);

        return true;
    }
    /**
      Read the attributes of System Enclosure or Chassis structure.

      Parameters:  smbiosTable- SMBIOS Table.
      Parameters:  offsetStructure- offset where the System Enclosure or Chassis structure is in SMBIOS Table.
      Parameters:  offsetStringSet- offset where the string-set is in SMBIOS Table.
      Returns:     true,successful to read it;otherwise,false.
     */
    bool ComputerSystemInstance::ReadSystemEnclosureOrChassisAttr(const MiddleData& smbiosTable,const size_t offsetStructure,
            const size_t /*offsetStringSet*/)
    {
        if(1 > smbiosTable.size())
        {
            throw SCXCoreLib::SCXInternalErrorException(L"The length of SMBIOS Table is invalid.", SCXSRCLOCATION);
        }
        const unsigned char* ptableBuf = &(smbiosTable[0]);
        const unsigned char* psystemChassis = ptableBuf + offsetStructure;

        //
        //ChassisBootupState,Manufacturer,PowerSupplyState,ThermalState,Type.
        //Bit 7 Chassis lock is present if 1;Bits 6:0 Enumeration value,System Enclosure or Chassis Types.
        //
        unsigned short flag = 0x7F; //!< 0111 1111,calculate to get the low six bits in Type byte data via operator "&"
        unsigned short type = psystemChassis[cType];
        unsigned short curDeviceType = FILTER(type,flag);
        SCX_LOGTRACE(m_log, StrAppend(L"ComputerSystemInstance::ReadSystemEnclosureOrChassisAttr() - curDeviceType: ", curDeviceType));
        if(cPeripheralDevice != curDeviceType) //!< We only get the Chassis Info,not Peripheral Devices here.
        {
            m_computersystemAttr.chassisBootupState = psystemChassis[cBootUpState];
            SCX_LOGTRACE(m_log, StrAppend(L"ReadSystemEnclosureOrChassisAttr() - chassisBootupState: ",  m_computersystemAttr.chassisBootupState));
            m_computersystemAttr.powerSupplyState = psystemChassis[cPowerSupplyState];
            SCX_LOGTRACE(m_log, StrAppend(L"ReadSystemEnclosureOrChassisAttr() - powerSupplyState: ", m_computersystemAttr.powerSupplyState));
            m_computersystemAttr.thermalState = psystemChassis[cThermalState];
            SCX_LOGTRACE(m_log, StrAppend(L"ReadSystemEnclosureOrChassisAttr() - thermalState: ", m_computersystemAttr.thermalState));
        }

        return true;
    }
#endif /*if FILTERLINUX*/

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/


/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       PAL representation of a ComputerSystem. 

  \date        11-04-18 14:00:00 

 */
/*----------------------------------------------------------------------------*/
#ifndef COMPUTERSYSTEMINSTANCE_H
#define COMPUTERSYSTEMINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/computersystemdependencies.h>

#if FILTERLINUX
#include <scxsystemlib/scxsmbios.h>
#endif /*linux*/

#if FILTERLINUX
/** Filter the value of some bits in a digital*/
#define FILTER(target,flag) static_cast<unsigned short>((target) & (flag))
/** Filter the value of one single bit in a digital*/
#define FILTERBIT(target,flag,offset) static_cast<unsigned short>((target) & ((flag)<<(offset)))
#endif /*linux*/


using namespace SCXCoreLib;
using std::vector;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
      Struct representing all implemented attributes for ComputerSystem,which is corresponding to the MOF class "SCXCM_ComputerSystem". 

     */
    struct ComputerSystemAttributes 
    {
        bool automaticResetCapability;                       //!< If True, the automatic reset is enabled. 
        unsigned short bootOptionOnLimit;                    //!< Boot option limit is ON. Identifies the system action when the ResetLimit value is reached.
        unsigned short bootOptionOnWatchDog;                 //!< Type of reboot action after the time on the watchdog timer is elapsed. 
        unsigned short chassisBootupState;                   //!< Boot up state of the chassis.
        bool daylightInEffect;                               //!< If True, the daylight savings mode is ON.
        std::wstring dnsHostName;                               //!< Name of local computer according to the domain name server (DNS).
        std::wstring manufacturer;                              //!< Name of a computer manufacturer.
        std::wstring model;                                     //!< Product name that a manufacturer gives to a computer. This property must have a value.
        bool networkServerModeEnabled;                       //!< If True, the network Server Mode is enabled.
        unsigned short powerSupplyState;                     //!< State of the power supply or supplies when last booted. The following list identifies the values for this property.
        vector<unsigned int> powerManagementCapabilities;    //!< Array of the specific power-related capabilities of a logical device.
        bool powerManagementSupported;                       //!< If True, device can be power-managed.
        short resetCount;                                    //!< Number of automatic resets since the last reset.
        short resetLimit;                                    //!< Number of consecutive times a system reset is attempted.
        unsigned short thermalState;                         //!< Thermal state of the system when last booted.
        unsigned short wakeUpType;                           //!< Event that causes the system to power up.
    };

    /*----------------------------------------------------------------------------*/
    /**
      Class that represents values related to ComputerSystem.
      Concrete implementation of an instance of a ComputerSystem. 
     */
    class ComputerSystemInstance : public EntityInstance
    {
        friend class ComputerSystemEnumeration;

        public:

#if FILTERLINUX
        explicit ComputerSystemInstance(SCXHandle<SCXSmbios> scxsmbios,SCXHandle<ComputerSystemDependencies> deps); 
#elif defined(sun) || defined(aix) || defined(hpux)
        explicit ComputerSystemInstance(SCXCoreLib::SCXHandle<ComputerSystemDependencies> deps = SCXCoreLib::SCXHandle<ComputerSystemDependencies>(new ComputerSystemDependencies()));
#else
        ComputerSystemInstance();
#endif
        virtual ~ComputerSystemInstance();

        virtual void Update();
        virtual void CleanUp();

        /*----------------------------------------------------------------------------*/
        /**
        Get AutomaticResetCapability.

        Parameters:  automaticResetCapability - RETURN: AutomaticResetCapability of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetAutomaticResetCapability(bool& automaticResetCapability) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get BootOptionOnLimit.

        Parameters:  bootOptionOnLimit - RETURN: BootOptionOnLimit of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetBootOptionOnLimit(unsigned short& bootOptionOnLimit) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get BootOptionOnWatchDog.

        Parameters:  bootOptionOnWatchDog - RETURN: BootOptionOnWatchDog of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetBootOptionOnWatchDog(unsigned short& bootOptionOnWatchDog) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get BootupState.

        Parameters:  bootupState - RETURN: BootupState of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetBootupState(std::wstring& bootupState) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get ChassisBootupState.

        Parameters:  chassisBootupState - RETURN: ChassisBootupState of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetChassisBootupState(unsigned short& chassisBootupState) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get DaylightInEffect.

        Parameters:  daylightInEffect - RETURN: DaylightInEffect of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetDaylightInEffect(bool& daylightInEffect) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get DNSHostName.

        Parameters:  dnsHostName - RETURN: DNSHostName of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetDNSHostName(std::wstring& dnsHostName) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get description.

        Parameters:  description - RETURN: description of CPU type of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For platform not supported
        */
        bool GetDescription(std::wstring& description) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get Manufacturer.

        Parameters:  manufacturer - RETURN: Manufacturer of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetManufacturer(std::wstring& manufacturer) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get Model.

        Parameters:  model - RETURN: Model of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetModel(std::wstring& model) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get NetworkServerModeEnabled.

        Parameters:  networkServerModeEnabled - RETURN: NetworkServerModeEnabled of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetNetworkServerModeEnabled(bool& networkServerModeEnabled) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get PowerSupplyState.

        Parameters:  powerSupplyState - RETURN: PowerSupplyState of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetPowerSupplyState(unsigned short& powerSupplyState) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get PowerManagementCapabilities.

        Parameters:  powerManagementCapabilities - RETURN: PowerManagementCapabilities of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetPowerManagementCapabilities(vector<unsigned int>& powerManagementCapabilities) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get PowerManagementSupported.

        Parameters:  powerManagementSupported - RETURN: PowerManagementSupported of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetPowerManagementSupported(bool& powerManagementSupported) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get ResetCount.

        Parameters:  resetCount - RETURN: ResetCount of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetResetCount(short& resetCount) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get ResetLimit.

        Parameters:  resetLimit - RETURN: ResetLimit of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetResetLimit(short& resetLimit) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get ThermalState.

        Parameters:  thermalState - RETURN: ThermalState of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetThermalState(unsigned short& thermalState) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get WakeUpType.

        Parameters:  wakeUpType - RETURN: WakeUpType of ComputerSystem instance

        Retval:      true if a value is supported by this implementation
        ThrowException: SCXNotSupportException - For not implemented platform.
        */
        bool GetWakeUpType(unsigned short& wakeUpType) const;

        private:
#if FILTERLINUX
        /*----------------------------------------------------------------------------*/
        /**
          Parse SMBIOS Structure Table. 

          Parameters:  curSmbiosEntry- strcuture holding part fields of SMBIOS Structure Table Entry Point to parse SMBIOS Table. 
          Returns:     whether it's successful to parse it. 
         */
        bool ParseSmbiosTable(const struct SmbiosEntry &curSmbiosEntry);
        /**
          Read the attributes of System Reset structure Information . 

          Parameters:  psmbiosTable- Point to the beginning of Smbios Table Buffer. 
          Parameters:  offsetStructure- offset where the structure is in SMBIOS Table. 
          Returns:     true,successful to read it;otherwise,false. 
         */
        bool ReadSystemResetAttr(const unsigned char* psmbiosTable,const size_t offsetStructure);
        /**
          Read the attributes of System Information structure . 

          Parameters:  psmbiosTable- Point to the beginning of Smbios Table Buffer. 
          Parameters:  offsetStructure- offset where the System Information structure is in SMBIOS Table. 
          Parameters:  offsetStringSet- offset where the string-set is in SMBIOS Table. 
          Returns:     true,successful to read it;otherwise,false. 
         */
        bool ReadSystemInfoAttr(const MiddleData& smbiosTable,const size_t offsetStructure,
                                const size_t offsetStringSet);
        /**
          Read the attributes of System Enclosure or Chassis structure. 

          Parameters:  smbiosTable- SMBIOS Table. 
          Parameters:  offsetStructure- offset where the System Enclosure or Chassis structure is in SMBIOS Table. 
          Parameters:  offsetStringSet- offset where the string-set is in SMBIOS Table. 
          Returns:     true,successful to read it;otherwise,false. 
         */
        bool ReadSystemEnclosureOrChassisAttr(const MiddleData& smbiosTable,const size_t offsetStructure,
                                              const size_t offsetStringSet);
#endif
        private:
        std::wstring m_runLevel;             //!< The content of run level.
#if FILTERLINUX
        SCXCoreLib::SCXHandle<SCXSmbios> m_scxsmbios; //!< Collects external dependencies of this class
        bool m_hasSystemReset; //!<Whether a System Reset structure is filled in SMBIOS Table(Related to the attributes AutomaticResetCapability,BootOptionOnLimit,BootOptionOnWatchDog,ResetCount,ResetLimit)
        enum BootOptionOn  //!< The values of attribute BootOptionOnLimit or BootOptionOnWatchDog.
        {
            eReserve = 1,       //!<Reserved, do not use
            eOperatingSystem,   //!<Operating system
            eSystemUtilities,   //!<System utilities
            eDotreboot          //!<Do not reboot
        };
        enum BootOptionOnSumLimit //!< The calculation sum of two bits(3rd and 4th bit) representing BootOptionOnSumLimit.
        {
            eDoubleZero = 0,       //!<00b
            eZeroOne = 8,          //!<01b
            eOneZero = 16,         //!<10b
            eDoubleOne = 24        //!<11b
        };
        enum BootOptionOnWatchDog //!< The calculation sum of two bits(1st and 2nd bit) representing BootOptionOnWatchDog.
        {
            eDoubleZeroDog = 0,       //!<00b
            eZeroOneDog = 2,          //!<01b
            eOneZeroDog = 4,          //!<10b
            eDoubleOneDog = 6         //!<11b
        };
#elif defined(sun) || defined(aix) || defined(hpux)
        bool m_isGetDayLightFlag;             //!< Fail or ok for get dayLightFlag.
        vector<std::wstring> m_powerConfAllLines;             //!< The content of power.conf file.
#endif

        private:

        SCXCoreLib::SCXHandle<ComputerSystemDependencies> m_deps; //!< Collects external dependencies of this class.
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
        struct ComputerSystemAttributes m_computersystemAttr;   //!< ComputerSystem attributes. 


    };

}

#endif /* COMPUTERSYSTEMINSTANCE_H */ 
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

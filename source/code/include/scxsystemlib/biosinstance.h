/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       PAL representation of a BIOS 

  \date        11-01-14 15:00:00 

 */
/*----------------------------------------------------------------------------*/
#ifndef BIOSINSTANCE_H
#define BIOSINSTANCE_H

#include <vector>
#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxsystemlib/osinstance.h>

#if defined(linux) || (defined(sun) && !defined(sparc))
#include <scxsystemlib/scxsmbios.h>
#elif defined(sparc)
#include <scxsystemlib/biosdepend.h>
#endif


using namespace SCXCoreLib;
using std::vector;

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
      Struct representing all attributes for Bios 

     */
    struct BIOSAttributes 
    {
        bool smbiosPresent;
        std::wstring smbiosBiosVersion;
        std::wstring systemSerialNumber;
        vector<unsigned short> biosCharacteristics;
        unsigned short installableLanguages;
        unsigned short smbiosMajorVersion;
        unsigned short smbiosMinorVersion;
        std::wstring manufacturer;
        SCXCoreLib::SCXCalendarTime installDate;
        std::wstring name;
        std::wstring version;
        enum OSTYPE targetOperatingSystem;
    };

    enum eSoftwareElementState 
    {
            eDeployable,            //!< Deployable
            eInstallable,              //!< Installable
            eExecutable,              //!< Executable
            eRunning                 //!< Running
    };

    /*----------------------------------------------------------------------------*/
    /**
      Class that represents values related to BIOS.
      Concrete implementation of an instance of a BIOS. 
     */
    class BIOSInstance : public EntityInstance
    {
        friend class BIOSEnumeration;

    public:

#if defined(linux) || (defined(sun) && !defined(sparc))
        explicit BIOSInstance(SCXCoreLib::SCXHandle<SCXSmbios> scxsmbios = SCXCoreLib::SCXHandle<SCXSmbios>(new SCXSmbios()) );
#elif defined(sun) && defined(sparc)
        explicit BIOSInstance(SCXCoreLib::SCXHandle<BiosDependencies> deps);
#else
        explicit BIOSInstance();
#endif
        virtual ~BIOSInstance();

        virtual void Update();


        /*----------------------------------------------------------------------------*/
        /**
          Check that whether the SMBIOS is available on this computer system.

          Parameters:  smbiosPresent- whether SMBIOS is available  
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetSmbiosPresent(bool &smbiosPresent) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get BIOS version as reported by SMBIOS.

          Parameters:  smbiosBiosVersion- BIOS version as reported by SMBIOS.
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetSmbiosBiosVersion(std::wstring &smbiosBiosVersion) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get array of BIOS characteristics supported by the system as defined by the System Management BIOS Reference Specification.

          Parameters:  biosCharacteristics- Array of BIOS characteristics supported by the system as defined by the System Management BIOS Reference Specification.
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetBiosCharacteristics(vector<unsigned short> &biosCharacteristics) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get number of languages available for installation on this system. 

          Parameters:  installableLanguages- Number of languages available for installation on this system. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetInstallableLanguages(unsigned short &installableLanguages) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get Major SMBIOS version number. 

          Parameters:  smbiosMajorVersion- Major SMBIOS version number. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetSMBIOSMajorVersion(unsigned short &smbiosMajorVersion) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get Minor SMBIOS version number. 

          Parameters:  smbiosMinorVersion- Minor SMBIOS version number. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetSMBIOSMinorVersion(unsigned short &smbiosMinorVersion) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get Manufacturer of this software element. 

          Parameters:  manufacturer- Manufacturer of this software element. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetManufacturer(std::wstring &manufacturer) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get Date and time the object was installed. 

          Parameters:  installDate- Date and time the object was installed.
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.

          In fact, the exact meaning of Installdate is the release time of BIOS provided by manufacture.
         */
        bool GetInstallDate(SCXCoreLib::SCXCalendarTime &installDate) const;
        /*----------------------------------------------------------------------------*/
        /**
          Get Name used to identify this software element. 

          Parameters:  name- Name used to identify this software element. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetName(std::wstring &name) const;

        /*----------------------------------------------------------------------------*/
        /**
          Get the System's Serial Number

          Parameters:  sn- System's serial number
          Returns:     whether the implementation for this platform supports the value or not.
         */
        bool GetSystemSerialNumber(std::wstring &sn) const;

        /*----------------------------------------------------------------------------*/
        /**
          Get Version of BIOS. 

          Parameters:  version- Version of BIOS. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetVersion(std::wstring &version) const;

        /*----------------------------------------------------------------------------*/
        /**
          Get TargetOperatingSystem of BIOS. 
          Parameters:  targetOperatingSystem- TargetOperatingSystem of BIOS. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetTargetOperatingSystem(unsigned short &targetOperatingSystem) const;

        /*----------------------------------------------------------------------------*/
        /**
          Get softwareElementState of BIOS. 
          Parameters:  softwareElementState- softwareElementState of BIOS. 
          Returns:     whether the implementation for this platform supports the value or not.
          ThrowException: SCXNotSupportException - For not implemented platform.
         */
        bool GetSoftwareElementState(unsigned short &softwareElementState) const;
    private:

#if defined(linux) || (defined(sun) && !defined(sparc))
        /*----------------------------------------------------------------------------*/
        /**
          Parse SMBIOS Structure Table. 

          Parameters:  curSmbiosEntry- strcut holding part fields of SMBIOS Structure Table Entry Point to parse SMBIOS Table. 
          Returns:     whether it's successful to parse it. 
         */
        bool ParseSmbiosTable(const struct SmbiosEntry &curSmbiosEntry);
        /*----------------------------------------------------------------------------*/
        /**
          Set the attribute Characteristics. 

          Parameters:  low- low word of Characteristics bytes. 
          Parameters:  high- high byte of Characteristics. 
          Returns:     true,successful to set it;otherwise,false. 
         */
        bool SetCharacteristics(const scxulong& low,const unsigned char high);
        /**
          Set the attributes of BIOS Information . 

          Parameters:  smbiosTable- SMBIOS Table. 
          Parameters:  structureStringStart- offset of the string-set start postion of current SMBIOS structure to smbiosTable. 
          Parameters:  structureStart- the start address of current SMBIOS structure. 
          Parameters:  SMBIOSStructPointLen - the length of the formatted area of the SMBIOS structure.
          Returns:     true,successful to set it;otherwise,false. 
         */
        bool SetBiosInfo(const MiddleData& smbiosTable,const size_t structureStringStart,
                         const unsigned char* structureStart, const unsigned short SMBIOSStructPointLen);

        /**
          Set the attributes of BIOS Information . 

          Parameters:  smbiosTable- SMBIOS Table. 
          Parameters:  structureStringStart- offset of the string-set start postion of current SMBIOS structure to smbiosTable. 
          Parameters:  structureStart- the start address of current SMBIOS structure. 
          Returns:     true,successful to set it;otherwise,false. 
         */
        bool SetSystemInfo(const MiddleData& smbiosTable,const size_t structureStringStart,
                         const unsigned char* structureStart);

        SCXCoreLib::SCXHandle<SCXSmbios> m_scxsmbios; //!< Collects external dependencies of this class and analyse it.
#elif defined(sun) && defined(sparc)
        /*----------------------------------------------------------------------------*/
        /**
           Parse Installed date from prom version. 
        */
        void ParseInstallDate();

        SCXCoreLib::SCXHandle<BiosDependencies> m_deps;  //general bios dependancy
#endif

        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
        struct BIOSAttributes m_biosPro;   //!< Bios properties.
        bool m_existBiosLanguage;     //!< type BIOS Language whether exist in SM table

    };

}

#endif /* BIOSINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

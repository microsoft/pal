/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file

  \brief       PAL representation of a BIOS

  \date        11-01-14 15:00:00

 */
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/biosinstance.h>
#include <scxsystemlib/biosenumeration.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/logsuppressor.h>

#if defined(aix)
#include <scxsystemlib/scxodm.h>
#elif defined(hpux)
#include <unistd.h>
#endif

#define TARGET_OS_UNKNOWN 0
#define TARGET_OS_OTHER 1
#define TARGET_OS_HPUX 8
#define TARGET_OS_AIX 9
#define TARGET_OS_WINNT 18
#define TARGET_OS_LINUX 36
#define TARGET_OS_SOLARIS 29
#define TARGET_OS_BSDUNIX 41
#define TARGET_OS_FREEBSD 43

using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{

    /**The length of Characteristics*/
    const size_t cCharacteristicsLength = 40;
    /** The type value of BIOS Information structure*/
    const unsigned short cBIOSInformation = 0;
    /** The type value of System Information structure*/
    const unsigned short cSystemInformation = 1;
    /** The type value of BIOS Language structure*/
    const unsigned short cBIOSLanguage = 13;
    /** offset where the string number of the System's Serial Number is*/
    const int cStrSystemInfoSerialNumber = 0x07;
    /** offset where the number of languages available in BIOS Language Info Structure is*/
    const int cLanguagesAavialable = 0x04;
    /** offset where the string number of the BIOS Vendor's Name in BIOS Info Structure is*/
    const int cStrNoName = 0x04;
    /** offset where the string number of the BIOS Version in BIOS Info Structure is*/
    const int cStrNoBiosVersion = 0x05;
    /** offset where the string number of the BIOS release date in BIOS Info Structure is*/
    const int cStrNoReleaseDate = 0x08;
    /** offset where the BIOS Characteristics in BIOS Info Structure is (Bits 0-31) */
    const int cCharacteristicsLow = 0x0A;
    /** offset where the BIOS Characteristics in BIOS Info Structure is (Bits 32-39) */
    const int cCharacteristicsHigh = 0x12;
    // The length of SMBIO Structure used to extract the BIOS characteristics.
    const int cSMBIOSStructLen = 0x13;

    /*----------------------------------------------------------------------------*/
    /**
      Constructor

      \param[in] deps  Dependencies for the BIOS data colletion.
    */
#if defined(linux) || (defined(sun) && !defined(sparc))
    BIOSInstance::BIOSInstance(SCXCoreLib::SCXHandle<SCXSmbios> scxsmbios) :
        m_scxsmbios(scxsmbios),
        m_existBiosLanguage(false)
    {
        m_biosPro.smbiosPresent = false;
        m_biosPro.installableLanguages = 0;
        m_biosPro.biosCharacteristics.clear();
        m_biosPro.smbiosMajorVersion = 0;
        m_biosPro.smbiosMinorVersion = 0;
        m_biosPro.installDate = SCXCalendarTime::FromPosixTime(0L);

        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.bios.biosinstance"));
    }
#elif defined(sun) && defined(sparc)
    BIOSInstance::BIOSInstance(SCXCoreLib::SCXHandle<BiosDependencies> deps) :
        m_deps(deps),
        m_existBiosLanguage(false)
    {
        m_biosPro.smbiosPresent = false;
        m_biosPro.installableLanguages = 0;
        m_biosPro.biosCharacteristics.clear();
        m_biosPro.smbiosMajorVersion = 0;
        m_biosPro.smbiosMinorVersion = 0;
        m_biosPro.installDate = SCXCalendarTime::FromPosixTime(0L);

        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.bios.biosinstance"));
    }
#else
    BIOSInstance::BIOSInstance()
    {
        m_biosPro.smbiosPresent = false;
        m_biosPro.installableLanguages = 0;
        m_biosPro.biosCharacteristics.clear();
        m_biosPro.smbiosMajorVersion = 0;
        m_biosPro.smbiosMinorVersion = 0;
        m_biosPro.installDate = SCXCalendarTime::FromPosixTime(0L);

        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.bios.biosinstance"));
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
      Default destructor
    */
    BIOSInstance::~BIOSInstance()
    {
        SCX_LOGTRACE(m_log, wstring(L"BIOSInstance default destructor: "));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Update current Bios instance to latest values
    */
    void BIOSInstance::Update()
    {
        static LogSuppressor errorSuppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        static LogSuppressor warningSuppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        static LogSuppressor infoSuppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
#if defined(linux) || (defined(sun) && !defined(sparc))
        struct SmbiosEntry smbiosEntry;
        smbiosEntry.tableAddress = 0;
        smbiosEntry.tableLength = 0;
        smbiosEntry.structureNumber = 0;
        smbiosEntry.majorVersion = 0;
        smbiosEntry.minorVersion = 0;
        smbiosEntry.smbiosPresent = false;
        smbiosEntry.name = wstring(L"");

        if(m_scxsmbios->ParseSmbiosEntryStructure(smbiosEntry))
        {
            ParseSmbiosTable(smbiosEntry);
        }
#elif defined(sun) && defined(sparc)

        wstring manufacturer;
        wstring version;
        std::vector<wstring> parts;

        //Solaris define in enum OSTYPE in osinstance.h
        m_biosPro.targetOperatingSystem=Solaris;
        m_biosPro.smbiosPresent=false;

        try
        {
            m_deps->GetPromManufacturer(manufacturer);
            m_biosPro.manufacturer = manufacturer;
        }
        catch(const SCXException& e)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"Manufacturer property not found - ", e.What()));
        }

        try
        {
            //version is like "OBP 4.30.4 2009/08/19 07:25"
            m_deps->GetPromVersion(version);
            m_biosPro.version=version;

            SCXCoreLib::StrTokenize(version, parts, L" ");
            m_biosPro.name = parts[0];
        }
        catch(const SCXException& e)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"Version property not found - ", e.What()));
        }

        //analyze installDate

        ParseInstallDate();
       
        std::istringstream processInput;
        std::ostringstream processOutput;
        std::ostringstream processErr;
        int result;
        try
        {
            result = SCXProcess::Run(L"prtfru", processInput, processOutput, processErr, 15000);
            if (processErr.str().size() > 0 || result != 0)
            {
                SCX_LOG(m_log, warningSuppressor.GetSeverity(L"prtfru error"), StrAppend(L"Error when running 'prtfru': ", processErr.str().c_str()));
            }
        }
        catch (SCXException &e)
        {
            SCX_LOG(m_log, warningSuppressor.GetSeverity(L"prtfru exception"), StrAppend(StrAppend(L"Exception thrown when attempting to run command 'prtfru'.", result), e.What()));
        }
        
        // prtfru parsing
        // We're looking for a line that contains 'System_Id', and then we're taking the last token on the right
        // 
        // Example output:
        //       /InstallationR[0]/System_Id: SERIALNUMBER
        bool serialNumberFound = false;
        string outputstr = processOutput.str();
        size_t system_id_loc = outputstr.find("System_Id:");
        if (system_id_loc != string::npos)
        {
            // get end of line
            size_t end_of_line_loc = outputstr.find_first_of("\n", system_id_loc);
            if (end_of_line_loc != string::npos)
            {
                size_t after_system_id_loc = system_id_loc + 11;
                m_biosPro.systemSerialNumber = StrTrim(StrFromUTF8(outputstr.substr(after_system_id_loc, end_of_line_loc - after_system_id_loc)));
                serialNumberFound = true;
            }
        }

        if (serialNumberFound == false)
        {
            SCX_LOG(m_log, infoSuppressor.GetSeverity(L"serial not found"), L"Unable to find serial number in prtfru.");
        }
        
#elif defined(hpux)
        char buf[BUFSIZ];
        buf[BUFSIZ-1] = '\0';
        confstr(_CS_MACHINE_SERIAL, buf, BUFSIZ-1);
        m_biosPro.systemSerialNumber = StrTrim(StrFromUTF8(buf));

#elif defined(aix)
        // odm lookup on CuAt where attribute=systemid
        try 
        {

            SCXodm odm;
            struct CuAt atData;
            memset(&atData, '\0', sizeof(atData));
            void * pResult = odm.Get(CuAt_CLASS, L"attribute=systemid", &atData);
            if (pResult != NULL)
            {
                if (atData.value[0] != '\0')
                {
                    m_biosPro.systemSerialNumber = StrFromUTF8(atData.value);
                }
            }
            else
            {
                SCX_LOG(m_log, warningSuppressor.GetSeverity(L"no odm entry"), L"Unable to find odm entry for CuAt where attribute=systemid.");
            }
        }
        catch (SCXodmException &e)
        {
            SCX_LOG(m_log, infoSuppressor.GetSeverity(L"odm exception"), StrAppend(L"When looking up odm entry for CuAt where attribute=systemid, an exception was thrown: ", e.What()));
        }

        try 
        {
            SCXodm odm;
            struct CuAt atData;
            memset(&atData, '\0', sizeof(atData));
            void * pResult = odm.Get(CuAt_CLASS, L"attribute=fwversion", &atData);
            if (pResult != NULL)
            {
                if (atData.value[0] != '\0')
                {
                    m_biosPro.version = StrFromUTF8(atData.value);
                }
            }
            else
            {
                SCX_LOG(m_log, warningSuppressor.GetSeverity(L"no odm entry"), L"Unable to find odm entry for CuAt where attribute=fwversion.");
            }
        }
        catch (SCXodmException &e)
        {
            SCX_LOG(m_log, infoSuppressor.GetSeverity(L"odm exception"), StrAppend(L"When looking up odm entry for CuAt where attribute=fwversion, an exception was thrown: ", e.What()));
        }
#else
#error No implementation for platform.
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Check that whether the SMBIOS is available on this computer system.

      \param[out]   smbiosPresent - whether SMBIOS is available
      \returns      whether the implementation for this platform supports the value.
    */
   bool BIOSInstance::GetSmbiosPresent(bool &smbiosPresent) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun)
        smbiosPresent = m_biosPro.smbiosPresent;
        // Implemented platform,and the property can be supported.
        fRet = true;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get BIOS version as reported by SMBIOS.

      \param[out]   smbiosBiosVersion - BIOS version as reported by SMBIOS.
      \returns      whether the implementation for this platform supports the value.
    */
    bool BIOSInstance::GetSmbiosBiosVersion(wstring &smbiosBiosVersion) const
    {
        bool fRet = false;
#if defined(linux) || (defined(sun) && !defined(sparc))
        smbiosBiosVersion = m_biosPro.smbiosBiosVersion;
        fRet = true;
#elif defined(sun) && defined(sparc)
        fRet = false;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get array of BIOS characteristics supported by the system as defined by the System Management BIOS Reference Specification.

      \param[out]   biosCharacteristics - Array of BIOS characteristics supported by the system as defined by the System Management BIOS Reference Specification.
      \returns      whether the implementation for this platform supports the value.
    */
    bool BIOSInstance::GetBiosCharacteristics(vector<unsigned short> &biosCharacteristics) const
    {
        bool fRet = false;
#if defined(linux) || (defined(sun) && !defined(sparc))
        biosCharacteristics = m_biosPro.biosCharacteristics;
        fRet = true;
#elif defined(sun) && defined(sparc)
        fRet = false;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get number of languages available for installation on this system.

      \param[out]   installableLanguages - Number of languages available for installation on this system.
      \returns      whether the implementation for this platform supports the value.
    */
    bool BIOSInstance::GetInstallableLanguages(unsigned short &installableLanguages) const
    {
        bool fRet = false;
#if defined(linux) || (defined(sun) && !defined(sparc))
        if(m_existBiosLanguage)
        {
            installableLanguages = m_biosPro.installableLanguages;
            fRet = true;
        }
#elif defined(sun) && defined(sparc)
        fRet = false;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get Major SMBIOS version number.

      \param[out]   smbiosMajorVersion - Major SMBIOS version number.
      \returns      whether the implementation for this platform supports the value.
    */
    bool BIOSInstance::GetSMBIOSMajorVersion(unsigned short &smbiosMajorVersion) const
    {
        bool fRet = false;
#if defined(linux) || (defined(sun) && !defined(sparc))
        smbiosMajorVersion = m_biosPro.smbiosMajorVersion;
        fRet = true;
#elif defined(sun) && defined(sparc)
        fRet = false;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get the minor SMBIOS version number.

      \param[out]   smbiosMinorVersion - Minor SMBIOS version number.
      \returns      whether the implementation for this platform supports the value or not.
    */
    bool BIOSInstance::GetSMBIOSMinorVersion(unsigned short &smbiosMinorVersion) const
    {
        bool fRet = false;
#if defined(linux) || (defined(sun) && !defined(sparc))
        smbiosMinorVersion = m_biosPro.smbiosMinorVersion;
        fRet = true;
#elif defined(sun) && defined(sparc)
        fRet = false;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get the manufacturer of this software element.

      \param[out]   manufacturer - Manufacturer of this software element.
      \returns      whether the implementation for this platform supports the value or not.
    */
    bool BIOSInstance::GetManufacturer(wstring &manufacturer) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun)
        manufacturer = m_biosPro.manufacturer;
        fRet = true;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get the date and time that the object was installed.

      \param[out]   installDate - Date and time the object was installed.
      \returns      whether the implementation for this platform supports the value or not.

      In fact, the exact meaning of Installdate is the release time of BIOS provided by manufacturer.
    */
    bool BIOSInstance::GetInstallDate(SCXCalendarTime& installDate) const
    {
        bool fRet = false;

#if defined(linux) || defined(sun)
        if (m_biosPro.installDate.IsInitialized())
        {
            installDate = m_biosPro.installDate;
        }

        fRet = true;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get the name used to identify this software element.

      \param[out]   name - Name used to identify this software element.
      \returns      whether the implementation for this platform supports the value or not.
    */
    bool BIOSInstance::GetName(wstring &name) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun)
        name = m_biosPro.name;
        fRet = true;
#endif
        return fRet;
    }

    bool BIOSInstance::GetSystemSerialNumber(wstring &sn) const
    {
        sn = m_biosPro.systemSerialNumber;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get the BIOS version.

      \param[out]   version - version of BIOS.
      \returns      whether the implementation for this platform supports the value or not.
    */
    bool BIOSInstance::GetVersion(wstring &version) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix)
        version = m_biosPro.version;
        fRet = true;
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get TargetOperatingSystem of BIOS.

      \param[out]   targetOperatingSystem- TargetOperatingSystem of BIOS.
      \returns      whether the implementation for this platform supports the value or not.

      \notes        This is a key field in the Win32_BIOS class, so it must be present.
                    In the cases where SMBIOS doesn't return this value, we return a
                    value tied to the OS that the provider was built for. In reality,
                    BIOS is mostly an MS_DOS concept, so we could also return
                    TARGET_OS_WINNT, for most of these, since the BIOSs were probably
                    originally made for Windows.
    */
    bool BIOSInstance::GetTargetOperatingSystem(unsigned short &targetOperatingSystem) const
    {
        bool fRet;
#if defined(linux)
        targetOperatingSystem = TARGET_OS_LINUX;
        fRet = true;
#elif defined(sun) && defined(sparc)
        targetOperatingSystem = m_biosPro.targetOperatingSystem;
        fRet = true;
#elif defined(sun)
        targetOperatingSystem = TARGET_OS_SOLARIS;  // Solaris x86
        fRet = true;
#elif defined(aix)
        targetOperatingSystem = TARGET_OS_AIX;
        fRet = true;
#elif defined(hpux)
        targetOperatingSystem = TARGET_OS_HPUX;
        fRet = true;
#else
# error "Unrecognized platform"
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get softwareElementState of BIOS.

      \param[out]   softwareElementState - softwareElementState of BIOS.
      \returns      whether the implementation for this platform supports the value or not.
    */
    bool BIOSInstance::GetSoftwareElementState(unsigned short &softwareElementState) const
    {
        bool fRet = false;
        //eRunning defined in eunm eSoftwareElementState, it applies to all platform
#if defined(linux) || defined(sun)
        softwareElementState = eRunning;
        fRet = true;
#endif
        return fRet;
    }

#if defined(linux) || (defined(sun) && !defined(sparc))
    /*----------------------------------------------------------------------------*/
    /**
      Parse the SMBIOS structure table.

      \param[out]   curSmbiosEntry - structure holding part fields of SMBIOS Structure Table Entry Point to parse SMBIOS Table.
      \returns      true if the table parsed successfully, false if not.
    */
    bool BIOSInstance::ParseSmbiosTable(const struct SmbiosEntry &curSmbiosEntry)
    {
        try
        {
            if(1 > curSmbiosEntry.tableLength)
            {
                throw SCXCoreLib::SCXInternalErrorException(L"The length of SMBIOS Table is invalid.", SCXSRCLOCATION);
            }

            //
            //SMBIOSMajorVersion,SMBIOSMinorVersion,smbiosPresent,Name
            //
            m_biosPro.smbiosMajorVersion = curSmbiosEntry.majorVersion;
            m_biosPro.smbiosMinorVersion = curSmbiosEntry.minorVersion;
            m_biosPro.smbiosPresent = curSmbiosEntry.smbiosPresent;
            if(curSmbiosEntry.smbiosPresent)
            {
                m_biosPro.name = L"Default System BIOS";
            }

            //
            //Get content of SMBIOS Table via Entry Point.
            //
            MiddleData smbiosTable(curSmbiosEntry.tableLength);
            m_scxsmbios->GetSmbiosTable(curSmbiosEntry,smbiosTable);

            //
            //Search SMBIOS Table to find BIOS structure.
            //
            unsigned char* ptableBuf = &(smbiosTable[0]);
            size_t curNumber = 0;
            size_t curLength = 0;
            while ((curNumber<curSmbiosEntry.structureNumber) && (curSmbiosEntry.tableLength - curLength >= cHeaderLength))
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
                    break;
                }

                //
                //Type 0 means this SMBIOS structure is BIOS Information Structure
                //
                unsigned char *pcur = ptableBuf+curLength;
                switch(type)
                {

                    case cBIOSInformation:
                        {
                            //
                            //Text strings appear closely after current structure, so read string from curLength+length.
                            //
                            size_t strStartPos = curLength + length;
                            SetBiosInfo(smbiosTable,strStartPos,pcur, length);
                            break;
                        }
                    case cSystemInformation:
                        {
                            //
                            //Text strings appear closely after current structure, so read string from curLength+length.
                            //
                            size_t strStartPos = curLength + length;
                            SetSystemInfo(smbiosTable, strStartPos, pcur);
                            break;
                        }
                    case cBIOSLanguage:
                        {
                            m_existBiosLanguage = true;
                            //
                            //InstallableLanguages,number of languages available for installation on this system.
                            //
                            unsigned short installableLanguage = pcur[cLanguagesAavialable];
                            SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosTable() - intallableLanguage: ", installableLanguage));
                            m_biosPro.installableLanguages = installableLanguage;
                            break;
                        }
                    default:
                        //
                        //We only care the structures who have BIOS information.
                        //Otherwise,nothing to do.
                        //
                        break;
                }

                //
                //otherwise,find the next SMBIOS structure for the one whose type is 0 or 13.
                //the start address of next structure is after cur structure and it's corresponding string section following.
                //the following string section terminated with two null (00h) BYTES.
                //
                curLength += length;
                curNumber++;
                while (curLength < curSmbiosEntry.tableLength)
                {
                    if((0 != (ptableBuf+curLength)[0]) ||
                            0!= (ptableBuf+curLength)[1])
                    {
                        curLength++;
                    }
                    else /*otherwise,the terminal bytes appear(two null together).*/
                    {
                        SCX_LOGTRACE(m_log, wstring(L"ParseSmbiosTable textstring terminate"));
                        curLength += 2;
                        break;
                    }
                }
            }
        }
        catch(const SCXException& e)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"failed to parse Smbios Table.", SCXSRCLOCATION);
        }

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Set the attribute characteristics bits.

      \param[out]   low - low word of characteristics bytes.
      \param[out]   high - high byte of characteristics.
      \returns      true if setting it was successful; false otherwise.
    */
    bool BIOSInstance::SetCharacteristics(const scxulong& low, const unsigned char high)
    {
        for (size_t curBit = 0; curBit < cCharacteristicsLength; ++curBit)
        {
            if (curBit<32)
            {
                if (low & (1 << curBit))
                {
                    SCX_LOGTRACE(m_log, StrAppend(L"SetCharacteristics() - biosCharacteristics: ", curBit));
                    m_biosPro.biosCharacteristics.push_back(static_cast<unsigned short>(curBit));
                }
            }
            else
            {
                size_t tmpIndex = curBit - 32;
                if (high & (1 << tmpIndex))
                {
                    SCX_LOGTRACE(m_log, StrAppend(L"SetCharacteristics() - biosCharacteristics: ", tmpIndex+32));
                    m_biosPro.biosCharacteristics.push_back(static_cast<unsigned short>(tmpIndex+32));
                }
            }
        }

        return true;
    }

    bool BIOSInstance::SetSystemInfo(const MiddleData& smbiosTable, const size_t structureStringStart,
                                   const unsigned char* structureStart)
    {
        const unsigned char *pcur = structureStart;

        size_t stringIndex = pcur[cStrSystemInfoSerialNumber];
        SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosTable() - string index is: ", stringIndex));
        m_biosPro.systemSerialNumber = m_scxsmbios->ReadSpecifiedString(smbiosTable,structureStringStart,stringIndex);

        return true;
    }

    /**
      Set the BIOS information attributes.

      \param[in]    smbiosTable - SMBIOS Table.
      \param[in]    structureStringStart - offset of the string-set start postion of current SMBIOS structure to smbiosTable.
      \param[in]    structureStart - the start address of current SMBIOS structure.
      \param[in]    SMBIOSStructPointLen - the length of the formatted area of the SMBIOS structure.
      \returns      true if setting it was successful; false otherwise.
    */
    bool BIOSInstance::SetBiosInfo(const MiddleData& smbiosTable, const size_t structureStringStart,
                                   const unsigned char* structureStart, const unsigned short SMBIOSStructPointLen)
    {
        const unsigned char *pcur = structureStart;

        //
        //BIOS Version,Vendor,Release Date,BiosCharacteristics,InstallableLanguages,Version.
        //
        size_t stringIndex = pcur[cStrNoBiosVersion];
        SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosTable() - string index is: ", stringIndex));
        //
        //Text strings appear closely after current structure, so read string from curLength+length.
        //
        m_biosPro.smbiosBiosVersion = m_scxsmbios->ReadSpecifiedString(smbiosTable,structureStringStart,stringIndex);
        stringIndex = pcur[cStrNoName];
        m_biosPro.manufacturer = m_scxsmbios->ReadSpecifiedString(smbiosTable,structureStringStart,stringIndex);
        stringIndex = pcur[cStrNoReleaseDate];
        wstring strInstallDate = m_scxsmbios->ReadSpecifiedString(smbiosTable,structureStringStart,stringIndex);
        vector<wstring> dateTokens;
        SCXCoreLib::StrTokenize(strInstallDate,dateTokens,L"/");
        //
        //strInstallDate like "08/24/2010"
        //
        if(3 == dateTokens.size())
        {
            m_biosPro.installDate.SetMonth(StrToUInt(dateTokens[0]));
            m_biosPro.installDate.SetDay(StrToUInt(dateTokens[1]));
            m_biosPro.installDate.SetYear(StrToUInt(dateTokens[2]));

            //
            //Version is composed of manufacturer and installDate,like "HPQOEM - 20090825"
            //
            m_biosPro.version = m_biosPro.manufacturer + L"-" + dateTokens[2] + dateTokens[0] + dateTokens[1];
        }

#ifdef BIGENDIAN
        scxulong characteristicsLow = MAKELONG(MAKEWORD(pcur+cCharacteristicsLow+4,pcur+cCharacteristicsLow+5),MAKEWORD(pcur+cCharacteristicsLow+6,pcur+cCharacteristicsLow+7));
#else
        scxulong characteristicsLow = MAKELONG(MAKEWORD(pcur+cCharacteristicsLow,pcur+cCharacteristicsLow+1),MAKEWORD(pcur+cCharacteristicsLow+2,pcur+cCharacteristicsLow+3));
#endif
        // Find the upper portion of the BIOS characteristics.
        unsigned char characteristicsHigh = 0x0;
        if (SMBIOSStructPointLen > cSMBIOSStructLen )        
        {
            characteristicsHigh = *(pcur + cCharacteristicsHigh);
        }
        SetCharacteristics(characteristicsLow,characteristicsHigh);

        return true;
    }

#elif defined(sun) && defined(sparc)
    void BIOSInstance::ParseInstallDate()
    {
        std::vector<wstring> parts;
        SCXCoreLib::StrTokenize(m_biosPro.version, parts, L" ");
        SCXCoreLib::SCXCalendarTime installDate(1970, 1, 1);
        bool fSuccess = false;

        //analyze installDate
        try
        {
            wstring date;
            wstring time;
            for(int i=0;i<parts.size();i++)
            {
                size_t pos = parts[i].find_first_of('/');
                if( pos != wstring::npos )
                {
                    date=parts[i];
                    time=parts[i+1];
                    break;
                }
            }
            SCXCoreLib::StrTokenize(date, parts, L"/");
            installDate.SetYear(StrToUInt(parts[0]));
            installDate.SetMonth(StrToUInt(parts[1]));
            installDate.SetDay(StrToUInt(parts[2]));
            SCXCoreLib::StrTokenize(time, parts, L":");
            installDate.SetHour(StrToUInt(parts[0]));
            installDate.SetMinute(StrToUInt(parts[1]));
            fSuccess = true;
        }
        catch (const SCXException& e)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"parse bios installDate fails:", m_biosPro.version).append(L" - ").append(e.What()));
            fSuccess = false;
        }

        // Make the assignment if all parts are validated.
        if (fSuccess)
        {
            m_biosPro.installDate = installDate;
        }
    }

#endif //defined(linux) || (defined(sun) && !defined(sparc))

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

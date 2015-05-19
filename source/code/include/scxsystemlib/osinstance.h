/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       PAL representation of Operating System
    \date        08-03-04 14:36:00

*/
/*----------------------------------------------------------------------------*/

#ifndef OSINSTANCE_H
#define OSINSTANCE_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>
#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/scxostypeinfo.h>
#include <scxcorelib/stringaid.h>

#include <string>
#include <vector>

// Definitions for uname()
#include <sys/utsname.h>

#if defined(hpux)
#include <sys/pstat.h>
#endif

#if defined(linux) || defined(sun)
#include <langinfo.h>
#endif

namespace SCXSystemLib
{
    using std::vector;
    using SCXCoreLib::SCXCalendarTime;

    /*
     Parse the standard-format LANG environment variable string to get a telephone country code, a Windows
     locale code and a Windows code page

     \param[in]     langStr - an std::wstring representation of the LANG environment variable
     \param[out]    countryCode - the telephone country code  corresponding to LANG
     \param[out]    osLanguage - the Windows locale code corresponding to LANG
     \param[out]    codeSet - the Windows code page corresponding to LANG
    */
    extern bool ParseLangVariable(const std::wstring& langStr, std::wstring& countryCode, unsigned int& osLanguage, std::wstring& codeSet);

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents the common set of OS parameters.

       This class only implements the total instance and has no
       collection threrad.

       These are the type mappings used in the property methods.
       typedef unsigned short uint16;
       typedef unsigned int uint32;
       typedef scxulong uint64;
       typedef signed short sint16;
       typedef bool boolean;
       typedef SCXCalendarTime datetime;
    */
    class OSInstance : public EntityInstance
    {
        struct OSInfo
        {
            std::wstring    BootDevice;         //!< Name of the disk drive from which the Windows operating system starts
            std::wstring    CodeSet;            //!< Code page value an operating system uses, example "1255"
            std::wstring    CountryCode;        //!< Code for the country/region that an operating system uses, example: "45"
            vector<std::wstring>    MUILanguages; //!< Multilingual User Interface Pack (MUI Pack) languages installed on the computer, example: "en-us"
            unsigned int    OSLanguage;         //!< Language version of the operating system installed.
            unsigned int    ProductType;        //!< Additional system information, reference to enum OSProductType
                };

        friend class ProcessEnumeration;

        static const wchar_t *moduleIdentifier;         //!< Shared module string

    public:
        OSInstance();
        virtual ~OSInstance();

        virtual void Update();
        virtual void CleanUp();

        virtual const std::wstring DumpString() const;

        /* Properties of SCXCM_OperatingSystem*/
        bool GetBootDevice(std::wstring& bd) const;
        bool GetCodeSet(std::wstring& cs) const;
        bool GetCountryCode(std::wstring& cc) const;
        bool GetMUILanguages(vector<std::wstring>& langs) const;
        bool GetOSLanguage(unsigned int& lang) const;
        bool GetProductType(unsigned int& pt) const;
        bool GetBuildNumber(std::wstring& buildNumber) const;
        bool GetManufacturer(std::wstring& manufacturer) const;

        /* Properties of CIM_OperatingSystem */
        bool GetOSType(unsigned short& ost) const;
        bool GetOtherTypeDescription(std::wstring& otd) const;
        bool GetVersion(std::wstring& ver) const;
        bool GetLastBootUpTime(SCXCalendarTime& lbut) const;
        bool GetLocalDateTime(SCXCalendarTime& ldt) const;
        bool GetCurrentTimeZone(signed short& ctz) const;
        bool GetNumberOfLicensedUsers(unsigned int& nolu) const;
        bool GetNumberOfUsers(unsigned int& numberOfUsers) const;
        bool GetMaxNumberOfProcesses(unsigned int& mp) const;
        bool GetMaxProcessMemorySize(scxulong& mpms) const;
        bool GetMaxProcessesPerUser(unsigned int& mppu) const;

        /* Properties of PG_OperatingSystem */
        bool GetSystemUpTime(scxulong& sut) const;

    private:
        SCXOSTypeInfo m_osInfo;                 //!< Static OS Information
        struct OSInfo m_osDetailInfo;           //!< Detail OS Information
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.

        struct utsname m_unameInfo;             //!< Saves the output from uname()
        bool m_unameIsValid;                    //!< Is m_unameInfo valid now?

        void SetBootTime(void);                 //!< Sets the system_boot variables
        SCXCalendarTime m_system_boot;          //!< Time of system boot
        bool m_system_boot_isValid;             //!< Is m_system_boot valid now?
        SCXCalendarTime m_now;                  //!< Current time on system

#if defined(linux)
        void PrecomputeMaxProcesses(void);
        unsigned int m_MaxProcesses;            //!< Maximum number of process contexts.
        std::wstring m_platform;                //!< The current platform and version
#endif

#if defined(hpux)
        struct pst_static m_psts;               //!< Holds output from pstat_getstatic
        struct pst_dynamic m_pstd;              //!< Holds output from pstat_getdynamic
        bool m_psts_isValid;                    //!< Is m_psts valid now?
        bool m_pstd_isValid;                    //!< Is m_pstd valid now?
#endif
        void SetUptime(void);                   //!< Sets the upsec variables
        scxulong m_upsec;                       //!< Uptime in seconds
        bool m_upsec_isValid;                   //!< Is m_upsec valid now?

        std::wstring m_LangSetting;               //!< System LANG environment variable setting

        /*----------------------------------------------------------------------------*/
        /**
        Get OS lang setting.

        Parameters:  OSLang - RETURN: OSLang id

        Retval:      fasle if can not get the value of "LANG" SETTING
        */
        virtual bool getOSLangSetting(std::wstring& lang) const;

    };

    /**
       A constant returned by the GetOSType call.
       This is defined by the CIM standard for Operating System type.
    */
    enum OSTYPE {
        /* Pacify doxygen */
        Unknown,
        Other,
        MACOS,
        ATTUNIX,
        DGUX,
        DECNT,
        Digital_Unix,
        OpenVMS,
        HP_UX,
        AIX,
        MVS,
        OS400,
        OS2,
        JavaVM,
        MSDOS,
        WIN3x,
        WIN95,
        WIN98,
        WINNT,
        WINCE,
        NCR3000,
        NetWare,
        OSF,
        DCOS,
        Reliant_UNIX,
        SCO_UnixWare,
        SCO_OpenServer,
        Sequent,
        IRIX,
        Solaris,
        SunOS,
        U6000,
        ASERIES,
        TandemNSK,
        TandemNT,
        BS2000,
        LINUX,
        Lynx,
        XENIX,
        VM_ESA,
        Interactive_UNIX,
        BSDUNIX,
        FreeBSD,
        NetBSD,
        GNU_Hurd,
        OS9,
        MACH_Kernel,
        Inferno,
        QNX,
        EPOC,
        IxWorks,
        VxWorks,
        MiNT,
        BeOS,
        HP_MPE,
        NextStep,
        PalmPilot,
        Rhapsody,
        Windows_2000,
        Dedicated,
        OS_390,
        VSE,
        TPF,
        Windows_Me,
        Open_UNIX,
        OpenBDS,
        NotApplicable,
        Windows_XP,
        zOS,
        Windows_2003,
        Windows_2003_64
    };

    /**
        A constant returned by the GetProductType call.
    */
    enum OSProductType
    {
                ePTUnknown = 0,
                ePTWorkStation = 1,
                ePTDomainController = 2,
                ePTServer = 3,                                  //!< Server
                ePTMax
    };
}
#endif /* OSINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

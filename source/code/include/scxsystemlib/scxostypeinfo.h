/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       This file contains the class SCXOSTypeInfo
    
    \date        2009-05-01 11:39:54

    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXOSNAME_H
#define SCXOSNAME_H

#include <string>
#include <sys/utsname.h>

#if defined(hpux)
#include <sys/pstat.h>
#endif

#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{
    /**
       Class representing all external dependencies.
    */
    class SCXOSTypeInfoDependencies
    {
    public:
        virtual ~SCXOSTypeInfoDependencies() {};
#if defined(PF_DISTRO_ULINUX)
        virtual const std::wstring getScriptPath() const;
        virtual const std::wstring getReleasePath() const;
        virtual bool isReleasePathWritable() const;
        virtual const std::wstring getConfigPath() const;
#endif // defined(PF_DISTRO_ULINUX)
    };


    /*----------------------------------------------------------------------------*/
    /**
       Class to retreive common, static information about the current OS.

       This class can be used to find out identity of the current operating system.
       Expect it to be extended with bit width and system uptime etc., and 
       OS configuration parameters that do not change with less than system boot. 
       
    */
    class SCXOSTypeInfo
    {
    public:
        explicit SCXOSTypeInfo(SCXCoreLib::SCXHandle<SCXOSTypeInfoDependencies> = SCXCoreLib::SCXHandle<SCXOSTypeInfoDependencies>( new SCXOSTypeInfoDependencies()));
        virtual ~SCXOSTypeInfo();

        std::wstring    GetDescription() const;  
        std::wstring    GetCaption() const;  
        /** Returns the OS Version, typically from uname for Unix, /etc/xxx-release info for Linux */
        std::wstring    GetOSVersion() const { return m_osVersion; }
        std::wstring    GetOSName(bool bCompatMode = false) const;
        /** Returns a short alias for the OS in question, e.g. "Solaris" or "SLED" or "RHEL" */
        std::wstring    GetOSAlias()   const { return m_osAlias;   } 

        std::wstring    GetOSFamilyString() const;
        std::wstring    GetArchitectureString() const;

        std::wstring    GetUnameArchitectureString() const;

        std::wstring    GetProcessor() const { return m_osProcessor; }
        std::wstring    GetHardwarePlatform() const { return m_osHardwarePlatform; }
        std::wstring    GetOperatingSystem() const { return m_osOperatingSystem; }
        std::wstring    GetManufacturer() const { return m_osManufacturer; }

    private:
        void Init();
        bool ExtractToken(const std::wstring& sToken, const std::vector<std::wstring>& list, std::wstring& sValue);

        SCXCoreLib::SCXHandle<SCXOSTypeInfoDependencies> m_deps;   //!< Dependency object for unit tests
        std::wstring m_osVersion;               //!< Cached value for osVersion
        std::wstring m_osName;                  //!< Cached value for osName
        std::wstring m_osCompatName;            //!< Cached value for osName (Compatible)
        std::wstring m_osAlias;                 //!< Cached value for osAlias
        bool m_unameIsValid;                    //!< Is m_unameInfo valid now?
        struct utsname m_unameInfo;             //!< Saves the output from uname()
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.

        std::wstring m_osArchitString;            //!< Cached value for system architecture

        std::wstring m_osProcessor;               //!< Cached value for Processor from uname
        std::wstring m_osHardwarePlatform;        //!< Cached value for HWPlatform from uname
        std::wstring m_osOperatingSystem;         //!< Cached value for operating sys name
        std::wstring m_osManufacturer;            //!< Cached value for operating sys manufacturer

#if defined(linux)
        std::wstring m_linuxDistroCaption;      //!< The current platform and version
#endif
    };
    
}

#endif /* SCXOSNAME_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

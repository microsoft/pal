/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file

\brief       PAL representation of a Installed Package

\date        11-01-21 12:00:00
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxsystemlib/installedsoftwareinstance.h>
#include <scxsystemlib/installedsoftwaredepend.h>

#include <string>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{
    
    /*----------------------------------------------------------------------------*/
    /**
    Constructor with Parameter id
    Parameters:  id - for rpm API, it means displayName
    - on solaris, it means the folder name which contains pkginfo file
    */
    InstalledSoftwareInstance::InstalledSoftwareInstance(wstring& id, SCXCoreLib::SCXHandle<InstalledSoftwareDependencies> deps):
        m_deps(deps),
        m_versionMajor(0),
        m_versionMinor(0)
    {
        SetId(id);
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.software.installedsoftwareinstance");
        SCX_LOGTRACE(m_log, L"InstalledSoftwareInstance constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
    Destructor
    */
    InstalledSoftwareInstance::~InstalledSoftwareInstance()
    {
        SCX_LOGTRACE(m_log, L"InstalledSoftwareInstance destructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
    Update values

    */
    void InstalledSoftwareInstance::Update()
    {
        SCX_LOGTRACE(m_log, L"InstalledSoftwareInstance Update()");
        m_evidenceSource = L"M";
#if defined(sun)
        vector<wstring> allLines;
        m_deps->GetAllLinesOfPkgInfo(GetId(), allLines);
        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end(); it++) {
            size_t pos = it->find_first_of('=');
            if( pos != wstring::npos ) {
                wstring key = it->substr(0, pos);
                wstring value = it->substr(pos+1);
                std::transform(key.begin(), key.end(), key.begin(), (int (*)(int))std::toupper);

                if(key == L"BASEDIR") {
                    m_installedLocation = value;
                }
                else if(key == L"PKG") {
                    m_productName = value;
                }
                else if(key == L"NAME") {
                    m_displayName = value;
                }
                else if(key == L"VERSION") {
                    m_productVersion = value;
                    SetDetailedVersion(value);
                }
                else if(key == L"VENDOR") {
                    m_publisher = value;
                }
                else if(key == L"INSTDATE") {
                    SetInstallDate(value);
                }
            }
        }
        //productID consists of productName and productVersion
        m_productID.append(m_productName).append(L" ").append(m_productVersion);

#elif defined(linux)
        vector<wstring> content;
        map<wstring, wstring> contentMap;
        m_deps->GetSoftwareInfoRawData(GetId(),content);
        m_displayName = GetId();
        for (std::vector<wstring>::iterator itSoftwareInfo = content.begin(); itSoftwareInfo != content.end(); itSoftwareInfo++)
        {
            size_t pos = itSoftwareInfo->find_first_of(':');
            if( pos != wstring::npos ) {
                wstring key = itSoftwareInfo->substr(0, pos);
                wstring value = itSoftwareInfo->substr(pos+1);
                contentMap.insert(make_pair(key, value));
            }
        }
        if ( contentMap.find( L"Name" ) != contentMap.end() )
        {
            m_productName = contentMap[L"Name"];
        }
        if ( contentMap.find( L"Vendor" ) != contentMap.end() )
        {
            m_publisher = contentMap[L"Vendor"];
        }
        if ( contentMap.find( L"InstallTime" ) != contentMap.end() )
        {
            m_installDate = SCXCalendarTime::FromPosixTime(StrToLong(contentMap[L"InstallTime"]));
        }
        if ( contentMap.find( L"SourceRPM" ) != contentMap.end() )
        {
            m_installSource = contentMap[L"SourceRPM"];
        }
        if ( contentMap.find( L"Version" ) != contentMap.end() )
        {
            m_productVersion = contentMap[L"Version"];
            SetDetailedVersion(m_productVersion);
        }
        m_productID=m_displayName;
#elif defined(aix)
        m_productID = GetId();
        if (m_deps->GetProperties(m_productID, m_productVersion, m_displayName, m_installDate))
        {
            SCX_LOGTRACE(m_log, wstring(L"Collected properties for ") + m_productID);
        }
#elif defined(hpux)
        InstalledSoftwareDependencies::PROPMAP mapProperties;
        wstring indexFileName = GetId() + L"pfiles/INDEX";

        m_productID = GetId();
        SCX_LOGTRACE(m_log, wstring(L"Collected properties for ") + m_productID);

        if (m_deps->GetAllPropertiesOfIndexFile(indexFileName, mapProperties))
        {
            InstalledSoftwareDependencies::PROPMAP::const_iterator cit;

            // Publisher
            cit = mapProperties.find(InstalledSoftwareDependencies::keyPublisher);
            if (cit != mapProperties.end())
            {
                // Example: "Hewlett-Packard Company"
                m_publisher = cit->second;
            }

            // DisplayName
            cit = mapProperties.find(InstalledSoftwareDependencies::keyTitle);
            if (cit != mapProperties.end())
            {
                // Example: "HPVM Guest AVIO Storage Software"
                m_displayName = cit->second;
            }

            // Version
            cit = mapProperties.find(InstalledSoftwareDependencies::keyRevision);
            if (cit != mapProperties.end())
            {
                // Example: "B.11.31.0909"
                size_t pos = cit->second.find_first_of(L"0123456789");
                if( pos != wstring::npos )
                {
                    wstring majmin(cit->second.substr(pos));
                    SetDetailedVersion(majmin);
                }

                // Convert to major/minor.
                m_productVersion = cit->second;
            }

            // InstallDate
            cit = mapProperties.find(InstalledSoftwareDependencies::keyInstallDate);
            if (cit != mapProperties.end())
            {
                 // Example: "201101021650.58"
                 SetInstallDate(cit->second);
            }

            // InstallSource
            cit = mapProperties.find(InstalledSoftwareDependencies::keyInstallSource);
            if (cit != mapProperties.end())
            {
                // Example: "hostrhpi05:/var/opt/ignite/depots/Rel_B.11.31/core_media"
                m_installSource = cit->second;
            }

            // InstalledLocation
            cit = mapProperties.find(InstalledSoftwareDependencies::keyDirectory);
            if (cit != mapProperties.end())
            {
                // Example: "/"
                m_installedLocation = cit->second;
            }

            // ProductName
            cit = mapProperties.find(InstalledSoftwareDependencies::keyTag);
            if (cit != mapProperties.end())
            {
                // Example: "AVIO-GVSD"
                m_productName = cit->second;
            }
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
    Clean up resources

    */
    void InstalledSoftwareInstance::CleanUp()
    {
        SCX_LOGTRACE(m_log, L"InstalledSoftwareInstance CleanUp()");
    }

    const wstring InstalledSoftwareInstance::DumpString() const
    {
        wstring result;
        return result;
    }

    /* Properties of InstalledSoftware */

    /*----------------------------------------------------------------------------*/
    /**
    Get Product ID

    Parameters: productID - RETURN: productID of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetProductID(wstring& productID) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix) || defined(hpux)
        productID = m_productID;
        fRet = true;
#else
        throw SCXNotSupportedException(L"Product ID", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get Display Name

    Parameters: displayName - RETURN: display Name of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetDisplayName(wstring& displayName) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix) || defined(hpux)
        // Set property here, otherwise building process would be failed in AIX & HPUX.
        displayName = m_displayName;

        // Implemented platform,and the property can be supported.
        fRet = true;
#else
        // Not implemented platform
        throw SCXNotSupportedException(L"Display Name", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software evidence source

    Parameters: evidenceSource - RETURN: evidence source of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetEvidenceSource(wstring& evidenceSource) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix)
        evidenceSource = m_evidenceSource;
        fRet = true;
#else
        throw SCXNotSupportedException(L"Evidence Source", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software InstalledLocation

    Parameters: installedLocation - RETURN: installed Location of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetInstalledLocation(wstring& installedLocation) const
    {
        bool fRet = false;
#if defined(sun) || defined(hpux)
        installedLocation = m_installedLocation;
        fRet = true;
#elif defined(linux) || defined(aix)
        installedLocation = installedLocation; // Silence warning of unused arg.
        // Implemented platform,and the property can't be supported.
        fRet = false;
#else
        throw SCXNotSupportedException(L"Installed Location", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software install Source

    Parameters: installSource - RETURN: install Source of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetInstallSource(wstring& installSource) const
    {
        bool fRet = false;
#if defined(linux) || defined(hpux)
        installSource = m_installSource;
        fRet = true;
#elif defined(sun) || defined(aix)
        fRet = false;
#else
        throw SCXNotSupportedException(L"Install Source", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software product Name

    Parameters: productName - RETURN: product Name of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetProductName(wstring& productName) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix) || defined(hpux)
        productName = m_productName;
        fRet = true;
#else
        throw SCXNotSupportedException(L"Product Name", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software product Version

    Parameters: productVersion - RETURN: product Version of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetProductVersion(wstring& productVersion) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix) || defined(hpux)
        productVersion = m_productVersion;
        fRet = true;
#else
        throw SCXNotSupportedException(L"Product Version", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software publisher

    Parameters: publisher - RETURN: publisher of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetPublisher(wstring& publisher) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(hpux)
        publisher = m_publisher;
        fRet = true;
#elif defined(aix)
        fRet = false;
#else
        throw SCXNotSupportedException(L"Publisher", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software version Major

    Parameters: versionMajor - RETURN: version Major of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetVersionMajor(unsigned int& versionMajor) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix) || defined(hpux)
        versionMajor = m_versionMajor;
        fRet = true;
#else
        throw SCXNotSupportedException(L"Major Version", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software version Minor

    Parameters: versionMinor - RETURN: version Minor of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetVersionMinor(unsigned int& versionMinor) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(hpux)
        versionMinor = m_versionMinor;
        fRet = true;
#else
        throw SCXNotSupportedException(L"Minor Version", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get software install Date

    Parameters:`installDate - RETURN: install Date of software instance
    Retval:     true if a value is supported by this platform
    Throw:      SCXNotSupportedException - For not implemented platform
    */
    bool InstalledSoftwareInstance::GetInstallDate(SCXCalendarTime& installDate) const
    {
        bool fRet = false;
#if defined(sun) || defined(linux) || defined(aix) || defined(hpux)
        if (m_installDate.IsInitialized())
        {
            installDate = m_installDate;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"Install Date", SCXSRCLOCATION);
#endif
        return fRet;
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
    Translate install date from string to SCXCalendarTime

    Parameters:  installDate - its format like Aug 04 2010 10:24

    */
    void InstalledSoftwareInstance::SetInstallDate(const wstring& installDate)
    {
        tm          installtm;
        const std::string nsInstallDate = StrToUTF8(installDate);
        const char *buf      = nsInstallDate.c_str();
        size_t      ccLength = installDate.length();
        const char *bufend   = buf + ccLength;
        const char *format   = "%b %d %Y %H:%M";
        const char *ret      = strptime(buf, format, &installtm);

        if (ret == bufend)
        {
            SCXCoreLib::SCXCalendarTime instDate((scxyear)(installtm.tm_year + 1900), (scxmonth)(installtm.tm_mon + 1), (scxday)(installtm.tm_mday));
            instDate.SetHour((scxhour)(installtm.tm_hour));
            instDate.SetMinute((scxminute)(installtm.tm_min));
            m_installDate = instDate;
        }
        else
        {
            SCX_LOGWARNING(m_log, StrAppend(L"strptime installDate fails: ", installDate));
            m_installDate = SCXCalendarTime::FromPosixTime(0);
        }
    }
#elif defined(hpux)
    /*----------------------------------------------------------------------------*/
    /**
    Translate install date from string to SCXCalendarTime

    Parameters:  installDate - 
                 INDEX calendar dates are unlike CIM DATE or other common forms.
                 INDEX calendar date format: YYYYMMDDhhmm.ss
                 Example INDEX entry: install_date 201101021654.08

    */
    void InstalledSoftwareInstance::SetInstallDate(const wstring& installDate)
    {

        try
        {
            unsigned int year = SCXCoreLib::StrToUInt(installDate.substr(0, 4));
            unsigned int month = SCXCoreLib::StrToUInt(installDate.substr(4, 2));
            unsigned int day = SCXCoreLib::StrToUInt(installDate.substr(6, 2));
            unsigned int hour = SCXCoreLib::StrToUInt(installDate.substr(8, 2));
            unsigned int minute = SCXCoreLib::StrToUInt(installDate.substr(10, 2));
            // Char at position 12 is '.'
            unsigned int second = SCXCoreLib::StrToUInt(installDate.substr(13, 2));

            SCXCalendarTime scxctInstall(year, month, day);
            scxctInstall.SetHour(hour);
            scxctInstall.SetMinute(minute);
            scxctInstall.SetSecond(second);
            m_installDate = scxctInstall;
        }
        catch(SCXException& e)
        {
            SCX_LOGWARNING(m_log, L"Bad INDEX install_date: " + installDate + L" " + e.What() + e.Where());
        }
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
    Gather the Major and Minor version from Product version

    Parameters:  version - Product version, its format like 11.23.32, REV- or 0.4b41

    */
    void InstalledSoftwareInstance::SetDetailedVersion(const wstring& version)
    {
        size_t pos = version.find_first_of('.');
        if( pos != wstring::npos )
        {
            wstring left = version.substr(0, pos);
            wstring right = version.substr(pos+1);
            try
            {
                m_versionMajor = StrToUInt(left);
                // Exclude characters
                for(pos = 0; pos< right.size(); pos++)
                {
                    if( right[pos] < '0' || right[pos] > '9' ) {
                        break;
                    }
                }
                if(pos > 0) {
                    left = right.substr(0, pos);
                    m_versionMinor = StrToUInt(left);
                }
            }
            catch (const SCXException& e)
            {
                SCX_LOGWARNING(m_log, StrAppend(L"parse InstalledSoftwareInstance version fails:", version).append(L" - ").append(e.What()));
            }
        }
    }
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

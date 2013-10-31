/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file

\brief       PAL representation of installed software
\date        2011-01-18 14:36:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SOFTWAREINSTANCE_H
#define SOFTWAREINSTANCE_H
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxtime.h>
#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/installedsoftwaredepend.h>
#include <string>

namespace SCXSystemLib
{
    using SCXCoreLib::SCXCalendarTime;
    /*----------------------------------------------------------------------------*/
    /**
    Class that represents the installed software instance.
    */
    class InstalledSoftwareInstance : public EntityInstance
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
        Constructor with Parameter id
        Parameters:  id - for rpm API, it means displayName
        - on solaris, it means the folder name which contains pkginfo file
        */
        InstalledSoftwareInstance(std::wstring& id, SCXCoreLib::SCXHandle<InstalledSoftwareDependencies> deps = SCXCoreLib::SCXHandle<InstalledSoftwareDependencies>(new InstalledSoftwareDependencies()));

        virtual ~InstalledSoftwareInstance();

        virtual void Update();
        virtual void CleanUp();
        virtual const std::wstring DumpString() const;

        /* Properties of InstalledSoftware */

        /*----------------------------------------------------------------------------*/
        /**
        Get Product ID

        Parameters: productID - RETURN: productID of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetProductID(std::wstring& productID) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get Display Name

        Parameters: displayName - RETURN: display Name of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetDisplayName(std::wstring& displayName) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software evidence source

        Parameters: evidenceSource - RETURN: evidence source of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetEvidenceSource(std::wstring& evidenceSource) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software InstalledLocation

        Parameters: installedLocation - RETURN: installed Location of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetInstalledLocation(std::wstring& installedLocation) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software install Source

        Parameters: installSource - RETURN: install Source of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetInstallSource(std::wstring& installSource) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software product Name

        Parameters: productName - RETURN: product Name of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetProductName(std::wstring& productName) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software product Version

        Parameters: productVersion - RETURN: product Version of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetProductVersion(std::wstring& productVersion) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software publisher

        Parameters: publisher - RETURN: publisher of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetPublisher(std::wstring& publisher) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software version Major

        Parameters: versionMajor - RETURN: version Major of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetVersionMajor(unsigned int& versionMajor) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software version Minor

        Parameters: versionMinor - RETURN: version Minor of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetVersionMinor(unsigned int& versionMinor) const;

        /*----------------------------------------------------------------------------*/
        /**
        Get software install Date

        Parameters: installDate - RETURN: install Date of software instance
        Retval:     true if a value is supported by this platform
        Throw:      SCXNotSupportedException - For not implemented platform
        */
        bool GetInstallDate(SCXCalendarTime& installDate) const;


    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
        SCXCoreLib::SCXHandle<InstalledSoftwareDependencies> m_deps;    //!< Dependencies to rely on

        std::wstring m_productID;                //!< The software product ID, consists of productName and productVersion
        std::wstring m_displayName;                //!< The software display name, consists of productName and productVersion
        std::wstring m_evidenceSource;             //!< Describes how this software was discovered
        SCXCalendarTime m_installDate;          //!< Date and time of when the software product was installed
        std::wstring m_installedLocation;          //!< The full path to the primary directory that is associated with the software
        std::wstring m_installSource;              //!< The full path of the directory from which the software was installed
        std::wstring m_productName;                //!< The name of the installed product that is displayed to the user
        std::wstring m_productVersion;             //!< The version of the product
        std::wstring m_publisher;                  //!< The company that publishes the software
        std::wstring m_registeredUser;             //!< The registered user for the product
        unsigned int m_versionMajor;            //!< The major product version that is derived from the ProductVersion property
        unsigned int m_versionMinor;            //!< The minor product version that is derived from the ProductVersion property

#if defined(sun)
        /*----------------------------------------------------------------------------*/
        /**
        Translate install date from string to SCXCalendarTime

        Parameters:  installDate - its format like Aug 04 2010 10:24

        */
        void SetInstallDate(const std::wstring& installDate);
#elif defined(hpux)
        /*----------------------------------------------------------------------------*/
        /**
        Translate install date from string to SCXCalendarTime

        Parameters:  installDate - its format like YYYYMMDDhhmm.ss

        */
        void SetInstallDate(const std::wstring& installDate);
#endif

        /*----------------------------------------------------------------------------*/
        /**
        Gather the Major and Minor version from Product version

        Parameters:  version - Product version, its format like 11.23.32, REV-

        */
        void SetDetailedVersion(const std::wstring& version);
    };
}
#endif /* SOFTWAREINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

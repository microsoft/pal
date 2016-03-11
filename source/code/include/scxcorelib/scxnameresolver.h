/*----------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
*/
/**
   /file
   
   /brief      Provides information about the host (name resolution services)
   
   /date       1-17-2008
   
   Provides information about a machine.
   
*/

#if ! defined(SCXNAMERESOLVER_H)
#define SCXNAMERESOLVER_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthreadlock.h>

#include <string>

#if defined(aix)
#include <sys/types.h>
#endif

#include <unistd.h>

// prototype to avoid including <sys/utsname>
struct utsname;

/*
 * DNS may or may not be used for lookup logic (depending on if we trust the DNS servers)
 *
 * We've gone back and fourth on this issue, and ultimately this will be decided by what our
 * customers want. Today, we don't know what that is.
 *
 * #define SCXNAMERESOLVER_USE_DNS_LOOKUPS 1
 *
 * will control this. If defined, DNS lookups will be used. If undefined, they will never be
 * used.
 */

#define SCXNAMERESOLVER_USE_DNS_LOOKUPS 1


class SCXNameResolverTest;

namespace SCXCoreLib 
{
    //! Data source
    enum eNameResolverSource {
        eNone = 0,              //!< Source Unknown
        eGethostname,           //!< Source gethostname()
        eEtcHosts,              //!< Source from /etc/hosts
        eEtcResolvConf,         //!< Source from /etc/resolv.conf
        eUname,                 //!< Source from uname()
        eDns,                   //!< Source from DNS: gethostbyname()
        eSourceMax              //!< Value past the end
    };

    // class NameResolverDependencies;

    /*----------------------------------------------------------------------------*/
    //! Encapsulates all external dependencies of the Name Resolution Module
    class NameResolverDependencies
    {
    public:
        //! Constructor
        NameResolverDependencies() { }

        char *getenv(const char *name) const { return ::getenv(name); }
        uid_t geteuid() const { return ::geteuid(); }
    };

    /*------------------------------------------------------------------------*/
    /**
       This class implements methods to get the domain name and hostname.

       This is an internal class, and should not be called directly.
       Instead, use the "NameResolver" class defined below.

       Note that the NameResolverInternal class can be somewhat lengthy to run
       (reading several files and performing DNS lookup operations).  As a
       result, caching is relevant for speed, and is handled automatically by
       the NameResolver public interface.
    */
    class NameResolverInternal {
        friend class NameResolver;
        friend class ::SCXNameResolverTest;

    protected:
        // Make constructor protected; this guarantees that nobody uses the internal clsss
        NameResolverInternal(SCXCoreLib::SCXHandle<NameResolverDependencies> deps = SCXCoreLib::SCXHandle<NameResolverDependencies>(new NameResolverDependencies()));

    public:
        virtual ~NameResolverInternal();
        void Update();

        // Methods to get the host and domain names.
        std::wstring GetHostname(std::string * pHostnameRaw = NULL) const;
        std::wstring GetDomainname() const;
        std::wstring GetHostDomainname() const;
        eNameResolverSource GetHostnameSource() const;
        eNameResolverSource GetDomainnameSource() const;

        // Debug Methods
        std::wstring DumpString() const;
        std::wstring DumpSourceString(eNameResolverSource e) const;

    protected:
        // Methods to set the host and domain names.
        void GetHostVia_uname();
        void GetHostVia_gethostname();
        void GetHostVia_gethostbyname(const std::wstring & name);
        void GetHostVia_EtcHosts(const std::string & name);
        void GetHostVia_ResolvConf(const std::string & name);
        void ParseFile_nsswitchConf(const std::string & name);

        // Utility methods
        void Normalize();

    private:
        SCXCoreLib::SCXHandle<NameResolverDependencies> m_deps; //!< Dependencies to rely on

        eNameResolverSource m_hostnameSource;   //!< Source location for the hostname
        std::wstring m_hostname;                //!< hostname
        std::string m_hostname_raw;             //!< Hostname before conversion to wide char string and normalization
        eNameResolverSource m_domainnameSource; //!< Source location for the domain name
        std::wstring m_domainname;              //!< domain name

        eNameResolverSource m_resolveDomain;    //!< Resolve domain name first via 'files' or 'dns'
        bool m_fResolveBoth;                    //!< Resolve using both 'files' AND 'dns'?

        // Do not allow copying
        NameResolverInternal(const NameResolverInternal &);             //!< Intentionally not implemented.
        NameResolverInternal & operator=(const NameResolverInternal &); //!< Intentionally not implemented.

        // Utility methods
        size_t GetNameBufSize() const;
        std::wstring GetHostByName(const std::wstring & name) const;
    };// end class NameResolverInternal


    /*------------------------------------------------------------------------*/
    /**
       This class implements methods to get the domain name and hostname, as
       well as other information (32-bit/64-bit, etc).

       To use this class, simply instanciate it and look up what you need.
       For example:

       NameResolver mi;
       std::wstring hostname = mi.GetHostname();
       std::wstring domainname = mi.GetDomainname();

       Note that if you want both the hostname and domain name, you should
       use the class for that:

       std::wstring fqdn_hostname = mi.GetHostDomainname();


       This information is cached.  Users should not be concerned of any
       performance implications with this class.
    */
    class NameResolver {
    public:
        NameResolver() { };
        ~NameResolver() { };

        /*
         * Function Update() isn't currently exposed for the following reasons:
         * 1) Nobody needs it today, and
         * 2) If someone needs it, it may introduce multithreading issues
         *
         * For now, we don't implement that.  We can revisit later if needed.
         * At this time, the global NameResolver instance is built when it is
         * first used.
         *
         *
         * void Update() { GetHandle()->Update(); }
         */

        // Methods to get the host and domain names.
        /** \copydoc SCXCoreLib::NameResolverInternal::GetHostname */
        std::wstring GetHostname(std::string * pHostnameRaw = NULL) const { return GetHandle()->GetHostname(pHostnameRaw); }
        /** \copydoc SCXCoreLib::NameResolverInternal::GetDomainname */
        std::wstring GetDomainname() const { return GetHandle()->GetDomainname(); }
        /** \copydoc SCXCoreLib::NameResolverInternal::GetHostDomainname */
        std::wstring GetHostDomainname() const { return GetHandle()->GetHostDomainname(); }
        /** \copydoc SCXCoreLib::NameResolverInternal::GetHostnameSource */
        eNameResolverSource GetHostnameSource() const { return GetHandle()->GetHostnameSource(); }
        /** \copydoc SCXCoreLib::NameResolverInternal::GetDomainnameSource */
        eNameResolverSource GetDomainnameSource() const { return GetHandle()->GetDomainnameSource(); }

        // Debug Methods
        /** \copydoc SCXCoreLib::NameResolverInternal::DumpString */
        std::wstring DumpString() const { return GetHandle()->DumpString(); }
        /** \copydoc SCXCoreLib::NameResolverInternal::DumpSourceString */
        std::wstring DumpSourceString(eNameResolverSource e) const
            { return GetHandle()->DumpSourceString(e); }

        /**
           For test purposes only - return if we've been initialized or not
         */
        bool IsInitialized() const { return NULL != m_pMI; }

        /**
           For test purposes only - Destroy the static pointer

           NOTE: Do not use this in production code!  It is not thread safe!
         */
        void DestructStatic() const
        {
            if (NULL != m_pMI)
            {
                delete m_pMI;
                m_pMI = NULL;
            }
        }

    private:
        static NameResolverInternal* m_pMI;              //!< Static member (initialized once per run)
        static SCXThreadLockHandle m_lockH;             //!< Thread lock handle

        /**
           Initialize the handle if needed - and return it
        */
        static NameResolverInternal* GetHandle()
        {
            if (NULL == m_pMI)
            {
                // Fetch a lock
                SCXThreadLock lock(m_lockH);

                // Check one more time if we've been initialized to avoid a race condition
                if (NULL == m_pMI)
                {
                    m_pMI = new NameResolverInternal;
                    m_pMI->Update();
                }
            }

            return m_pMI;
        }

        // Do not allow copying
        NameResolver(const NameResolver &);             //!< Intentionally left unimplemented.
        NameResolver & operator=(const NameResolver &); //!< Intentionally left unimplemented.
    };// end class NameResolver

    // Add direct accessor method to eliminate circular linkage issues
    std::wstring GetHostDomainname();

}// end namespace SCXCoreLib

#endif /* SCXNAMERESOLVER_H */

/*--------------------------E-N-D---O-F---F-I-L-E----------------------------*/

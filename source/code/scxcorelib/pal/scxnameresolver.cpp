/*----------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
*/
/**
   /file

   /brief      Provides DNS name resolution services.

   /date       1-17-2008

       Get the domain name using gethostbyname_r() under Solaris.

       According to the man page, needs lib nsl
*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxnameresolver.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>

#include <sys/utsname.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#if defined(aix)
  #include <libperfstat.h>
  #if PF_MAJOR >= 6
    #include <sys/wpar.h>
  #endif // PF_MAJOR >= 6
#endif // defined(aix)

#if defined(sun) && (PF_MAJOR > 5 || (PF_MAJOR == 5 && PF_MINOR >= 10))
#define _SUN_HAS_ZONE_SUPPORT_
#include <zone.h>
#endif

#if defined(macos)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(sun)
#include <sys/systeminfo.h>
#endif

/*
 * Static information for the NameResolver class
 *
 * The NameResolver class is defined once for eberybody to share.  It's set up
 * once (when first referenced), then shared by all users of the class.  By
 * sharing, we eliminate lengthy lookups of this information multiple times.
 */

SCXCoreLib::NameResolverInternal* SCXCoreLib::NameResolver::m_pMI = NULL;
SCXCoreLib::SCXThreadLockHandle  SCXCoreLib::NameResolver::m_lockH(L"SCXNameResolver");


namespace SCXCoreLib
{
    using std::string;
    using std::wstring;

    /**
       Constructor
    */
    NameResolverInternal::NameResolverInternal(SCXCoreLib::SCXHandle<NameResolverDependencies> deps)
        : m_deps(deps),
          m_hostnameSource(eNone),
          m_hostname(),
          m_hostname_raw(),
          m_domainnameSource(eNone),
          m_domainname(),
          m_resolveDomain(eNone),
          m_fResolveBoth(false)
    {
        /*
          NOTE:  If member variables are added, add appropriate reset
          code to Update() method!
         */
    }

    /**
       Destructor
    */
    NameResolverInternal::~NameResolverInternal()
    {
    }

    /**
       Display a text version of this class.
       \return A std::wstring containing a serialized version of this class.
    */
    std::wstring NameResolverInternal::DumpString() const
    {
        return SCXDumpStringBuilder("NameResolverInternal")
        .Text("hostnameSource",  DumpSourceString(m_hostnameSource))
        .Text("hostname", m_hostname)
        .Text("domainnameSource", DumpSourceString(m_domainnameSource))
        .Text("domainname", m_domainname)
        ;
    }

    /**
       Provides the text name of the enum value.
       \param e The enum value to translate.
       \return The text name of the enum value.
    */
    std::wstring NameResolverInternal::DumpSourceString(eNameResolverSource e) const
    {
        switch(e) {
        case eNone:          return L"eNone"; break;
        case eEtcHosts:      return L"eEtcHosts"; break;
        case eEtcResolvConf: return L"eEtcResolvConf"; break;
        case eUname:         return L"eUname"; break;
        case eDns:           return L"eDns"; break;
        case eGethostname:   return L"eGethostname"; break;
        case eSourceMax:     return L"eSourceMax"; break;
        default:             return L"unknown value"; break;
        }

        std::wstringstream ws;
        ws << L"Undefined enum value for eTagSource." << e << std::ends;
        throw SCXInternalErrorException(ws.str().c_str(), SCXSRCLOCATION);
    }

    /**
        Get the host name.

        \return The machine's hostname.
    */
    std::wstring NameResolverInternal::GetHostname(std::string * pHostnameRaw /* = NULL */) const
    {
        if(pHostnameRaw)
            *pHostnameRaw = m_hostname_raw; 

        return m_hostname;
    }

    /** 
        Get the domain name.

        \return The machine's domain name (or empty if unknown).
    */
    std::wstring NameResolverInternal::GetDomainname() const
    {
        return m_domainname;
    }

    /** 
        Get the host name and domain name.

        \return The machine's host name and domain name.
    */
    std::wstring NameResolverInternal::GetHostDomainname() const
    {
        std::wstring fqdn = m_hostname;
        if (eNone != m_domainnameSource)
        {
            fqdn.append(L".");
            fqdn.append(m_domainname);
        }

        return fqdn;
    }

    /**
       Get the hostname source.

       \returns The source of the hostname.
    */
    SCXCoreLib::eNameResolverSource NameResolverInternal::GetHostnameSource() const
    {
        return m_hostnameSource;
    }

    /**
       Get the domainname source.

       \returns The source of the domain name.
    */
    SCXCoreLib::eNameResolverSource NameResolverInternal::GetDomainnameSource() const
    {
        return m_domainnameSource;
    }


/*----------------------------------------------------------------------------*/
// Private methods
/*----------------------------------------------------------------------------*/

    /**
       Use sysconf() to get the maximum buffer size of machine names.

       \returns The system returned size if available, otherwise 257;

       sysconf(_SC_HOST_NAME_MAX) returns the max size of the host string
       minus 1 byte for the null byte.

       Docs across Linux, Solaris, and HPUX say that the maximum buffer
       size is 256 bytes + 1. 
    */
    size_t NameResolverInternal::GetNameBufSize() const
    {
        size_t size = 257;
#if defined(sun) && (PF_MAJOR==5) && (PF_MINOR<=9)
        // WI 7933: SC_HOST_NAME_MAX does not exist on Solaris 8
        // Since documentation indicates 256+1 (according to comment) I think it is 
        // OK to just skip the call to sysconf since there os no good alternative. 
#else
        long sysSize = sysconf(_SC_HOST_NAME_MAX);
        if((-1 != sysSize) && (size < (size_t)sysSize ))
        {
            size = sysSize + 1;
        }
#endif
        return size;
    }

    /**
       Call the platform specific uname() and determine hostname and domainname
       if possible.
    */
    void NameResolverInternal::GetHostVia_uname()
    {
        struct utsname buf;
        int rc = uname( & buf );
        if (-1 == rc)
        {
            return;
        }

        if('\0' != buf.nodename[0]) 
        {
            m_hostnameSource = eUname;
            m_hostname_raw = buf.nodename;
            m_hostname = StrFromUTF8(buf.nodename);
        }

#if defined(linux)
        if('\0' != buf.domainname[0]) 
        {
            m_domainnameSource = eUname;
            m_domainname = StrFromUTF8(buf.domainname);
        }
#endif

        return;
    }

    /**
       Get the hostname using gethostname(2).
    */
    void NameResolverInternal::GetHostVia_gethostname()
    {
        const size_t len = GetNameBufSize();
        std::vector<char> buf; 
        
        // length must be positive number
        SCXASSERT ( len > 0 );

        buf.resize(len+1, 0); // just to make sure string has '\0' at the end
        
        int rc = gethostname(&buf[0], len);
        if(0 == rc)
        {
            m_hostname_raw = std::string(&buf[0]);
            m_hostname = StrFromUTF8(&buf[0]);
            m_hostnameSource = eGethostname;
        }
    }


    /**
       Normalize domain name, if defined, and if necessary.

       This means:
           If host is:    sub.microsoft.com
           and domain is: microsoft.com
       then normalize the host to be "sub" and the domain to be "microsoft.com".

       On the other hand:
           If host is:    sub.microsoft.uk
           and domain is: microsoft.com
       then we'd make no change.

       \pre Assumes that object is set up, and that Update() has been called.
    */
    void NameResolverInternal::Normalize()
    {
        // Now fix up the host name/domain name
        // (If domain name is included at end of host name, eliminate it)

        if(eNone != m_domainnameSource) 
        {
            SCXASSERT( ! m_domainname.empty());

            size_t lenHost = m_hostname.length();
            size_t lenDomain = m_domainname.length();

            if (lenHost > lenDomain)
            {
                std::wstring hostFragment = m_hostname.substr(lenHost - lenDomain - 1 /* include dot */);
                if (0 == hostFragment.compare(L"." + m_domainname))
                {
                    m_hostname.erase(lenHost - lenDomain - 1, lenHost);
                }
            }
        }
        else
        {
            // Just return an empty string if there is no domainname.
            m_domainname.clear();
        }
    }


    /**
       Get the host name and domain name and save in member variable.

       \throws SCXException if the host name cannot be determined.
    */
    void NameResolverInternal::Update()
    {
        // Reset relevent members to a known state

        m_hostname = m_domainname = L"";
        m_hostname_raw = "";
        m_hostnameSource = m_domainnameSource = m_resolveDomain = eNone;
        m_fResolveBoth = false;

        // Look up the host name on the current system
        //
        // The preferred method is to call gethostname() which should always
        // return the full hostname.  If that fails, then revert back to uname()
        // which may only return the first 8 characters of the hostname
        // (depending on the platform).

        GetHostVia_gethostname();
        if(eNone == m_hostnameSource)
        {
            GetHostVia_uname();
            if(eNone == m_hostnameSource)
            {
                throw SCXInternalErrorException(L"Can not deduce hostname.", SCXSRCLOCATION);
            }
        }

        // See if we resolve the domain name first via 'files' or via 'dns' (eNone -> default)
        ParseFile_nsswitchConf("/etc/nsswitch.conf");
        SCXASSERT ( eNone == m_resolveDomain
                    || eEtcHosts == m_resolveDomain
                    || eDns == m_resolveDomain);

        // If we couldn't resolve via nsswitch.conf, then use default values
        if ( eNone == m_resolveDomain )
        {
            m_resolveDomain = eEtcHosts;
            m_fResolveBoth = true;
        }

        // Look up the domain name on the current system
        //
        // We try and get the domainname until it is found or the methods of
        // getting it are exhausted.
        //
        // When domainnameSource != eNone, then the domainname has been set.
        // Otherwise, domainname is undefined.
        //
        // We use nsswitch.conf settings if it's found (if not, we default to "files dns")

        // Do we check "files" first?
        if ( eEtcHosts == m_resolveDomain )
        {
            // First we try and fetch from /etc/resolv.conf
            GetHostVia_ResolvConf("/etc/resolv.conf");

            if (eNone == m_domainnameSource) {
                // Next stop: /etc/hosts
                GetHostVia_EtcHosts("/etc/hosts");

#if defined(SCXNAMERESOLVER_USE_DNS_LOOKUPS)
                // If we resolve both "files" and "dns" ...
                if(m_fResolveBoth && eNone == m_domainnameSource) {
                    // Try and fetch from DNS
                    GetHostVia_gethostbyname(m_hostname);
                }
#endif
            }
        }
        else {
            SCXASSERT( eDns == m_resolveDomain );

#if defined(SCXNAMERESOLVER_USE_DNS_LOOKUPS)
            // First we try and fetch from dns
            if(eNone == m_domainnameSource) {
                // Try and fetch from DNS
                GetHostVia_gethostbyname(m_hostname);
            }
#endif

            // If we resolve both "dns" and "files" ...
            if (m_fResolveBoth && eNone == m_domainnameSource) {
                // Next we try and fetch from /etc/resolv.conf
                GetHostVia_ResolvConf("/etc/resolv.conf");

                if (eNone == m_domainnameSource) {
                    // Next stop: /etc/hosts
                    GetHostVia_EtcHosts("/etc/hosts");
                }
            }
        }
        Normalize();
        m_hostname = StrToLower(m_hostname);
        m_domainname = StrToLower(m_domainname);
    }


#if defined(linux)

    /**
       Get the DNS entry for \a name.
       \param name Name to lookup.
       \returns The name found in DNS or the empty string.
    */
    std::wstring NameResolverInternal::GetHostByName(const std::wstring & name) const
    {
        const size_t bufsize = 1024;
        char  abuf[1024];
        struct hostent h;
        struct hostent * ph = 0;
        int herrno;
        int rc = gethostbyname_r(StrToUTF8(name).c_str(), &h, &abuf[0], bufsize,
                                 &ph, &herrno);
        if((0 == rc) && (0 != ph)) 
        {
            return StrFromUTF8(h.h_name);
        }
        else if(ERANGE == rc)
        {
            throw SCXErrnoERANGE_Exception(L"gethostbyname_r", L"This is often caused by a corrupted /etc/hosts file. Please check that file for formatting issues.", ERANGE, SCXSRCLOCATION);
        }
        return L"";
    }


#elif defined(sun)

    /**
       Get the DNS entry for \a name.

       \param name Name to lookup.
       \returns The name found in DNS or the empty string.
    */
    std::wstring NameResolverInternal::GetHostByName(const std::wstring & name) const
    {
        const size_t bufsize = GetNameBufSize();
        std::vector<char> abuf; 

        // length must be positive number
        SCXASSERT ( bufsize > 0 );

        abuf.resize( bufsize + 1, 0 ); // just to make sure string has '\0' at the end
        
        struct hostent h;
        int herrno;
        struct hostent *ph = gethostbyname_r(StrToUTF8(name).c_str(),
                                             &h,
                                             &abuf[0],
                                             bufsize,
                                             &herrno);
        if(0 != ph)
        {
            return StrFromUTF8(h.h_name);
        }
        return L"";
    }


#elif defined(hpux) || defined(macos)

    /**
       Get the domain name using gethostbyname() under HPUX.

       According to the man page, the non _r version provides
       thread support.
    */
    std::wstring NameResolverInternal::GetHostByName(const std::wstring & name) const
    {
        struct hostent *ph = gethostbyname(StrToUTF8(name).c_str());
        if(0 != ph)
        {
            return StrFromUTF8(ph->h_name);
        }
        return L"";
    }

#elif defined(aix)

    /**
       Get the DNS entry for \a name.

       \param name Name to lookup.
       \returns The name found in DNS or the empty string.
    */
    std::wstring NameResolverInternal::GetHostByName(const std::wstring & name) const
    {
        struct hostent h;
        struct hostent_data h_data;
        int res = gethostbyname_r(StrToUTF8(name).c_str(),
                                  &h,
                                  &h_data);
        if(0 == res)
        {
            return StrFromUTF8(h.h_name);
        }
        return L"";
    }


#else
#  error NameResolverInternal::GetHostByName() was not implemented for this platform.
#endif


    /**
       Get the domain name using gethostbyname_r() under Linux

       GetNamBufSize() was too small. I'm not sure the maximum size,
       but something large works.

       \param name Host name to be looked up.
    */
    void NameResolverInternal::GetHostVia_gethostbyname(const std::wstring & name)
    {
        m_domainname = GetHostByName(name);

        // If the domain comes back as identical to the host, just bag it
        if (m_domainname == name)
        {
            m_domainname.clear();
            return;
        }

        // The name comes back as "hostname.domainname"
        //
        // To protect against dotted hosts (i.e. "sub.bar"), check for prefix
        // rather than looking for a dot to isolate the host name.
        //
        // Note: On some systems (Redhat), if you have a default /etc/hosts
        // file (and nsswitch.conf is set to "files dns" for hosts), then we
        // may get something wierd, like "localhost.localdomain".
        //
        // To protect against this, disregard response if host name isn't part
        // of the response.

        if( ! m_domainname.empty())
        {
            if (StrIsPrefix(m_domainname, name + L".", true))
            {
                string::size_type pos = name.length();
                m_domainname.erase(0, pos + 1);

                if(0 != m_domainname.size())
                {
                    m_domainnameSource = eDns;
                }
            }
            else
            {
                m_domainname.clear();
            }
        }
    }


    /**
       Read /etc/hosts as supplied by \a name and set the domain name
       if suitable domain is found in the hosts file.

       \param name The full path name of the file to read.
    */
    void NameResolverInternal::GetHostVia_EtcHosts(const std::string & name)
    {
        std::ifstream is(name.c_str());
        if( ! is )
        {
            return;
        }

        // The /etc/hosts file has lines like:
        //    <ip-addr> <host1> <host2> <host3>
        // Examples of lines from /etc/hosts:
        //    127.0.0.2       scxcore-suse01.opsmgr.lan scxcore-suse01
        //    10.195.172.48   scxhpr2 scxhpr2.opsmgr.lan
        //
        // We look at the host entries.  If one of them matches our host name
        // exactly, then we look in that line for <hostname>.<domain>.  If
        // found, then the <domain> part is used as our domain.

        std::string line;
        while( is && getline(is, line) )
        {
            std::wstring wline = SCXCoreLib::StrFromUTF8(line.c_str());
            std::wstring::size_type pos;

            // Strip off any comment ("#" and anything after it)
            if ((pos = wline.find(L"#")) != std::wstring::npos)
            {
                wline.erase(pos, wline.length());
            }
            if (wline.empty())
            {
                // In case comment was from start of line, or line was just empty
                continue;
            }

            wline = StrToLower(wline);
            std::vector<std::wstring> parts;
            StrTokenize(wline, parts, L" \t");

            // Search through the string checking for our own host name
            string::size_type i;
            bool found = false;
            for (i = 1; i < parts.size(); i++)
            {
                if (0 == StrCompare(m_hostname, parts[i], true))
                {
                    // Ok, we found a line with our hostname - look for FQDN

                    found = true;
                    break;
                }
            }

            if ( ! found )
            {
                continue;
            }

            // Okay, we found our host name - look for FQDN with host name
            for (i = 1; i < parts.size(); i++)
            {
                if (StrIsPrefix(parts[i], m_hostname + L".", true))
                {
                    m_domainname = parts[i].substr(m_hostname.length() + 1);
                    m_domainnameSource = eEtcHosts;

                    // We found what we want, so we can just return now
                    return;
                }
            }
        }
    }


    /**
       Read resolv.conf as supplied by \a name and set the domain name
       if the 'domain' line is found.

       \param name The full path name of the file to read.
    */
    void NameResolverInternal::GetHostVia_ResolvConf(const std::string & name)
    {
        std::ifstream is(name.c_str());
        if( ! is )
        {
            return;
        }
        string line;
        while( is && getline(is, line) )
        {
            wstring wline = SCXCoreLib::StrFromUTF8(line); 

            if(0 == wline.find(L"domain"))
            {
                // Strip leading whitespace.
                wstring::size_type pos = 6;
                wstring::size_type size = 0;
                while(isspace(wline[pos+size]) && ((pos+size) < wline.size()))
                {
                    ++size;
                }
                wline.erase(0, pos+size);
                // The domainname begins at what is left.
                if(wline.size() > 0)
                {
                    // Locate trailing whitespace...
                    wstring::iterator c = wline.begin();
                    while(c != wline.end() && ! isspace(*c))
                    {
                        ++c;
                    }
                    if(wline.end() != c)
                    {
                        // ...and strip it.
                        wline.erase(c,wline.end());
                    }

                    // The domain name should be all that remains.
                    m_domainnameSource = eEtcResolvConf;
                    m_domainname = wline;
                    break;
                }
            }
        }
    }


    /**
       Read nsswitch.conf as supplied by \a name and set flags to control order
       of lookups.  If file isn't found (not supplied for all platforms), or if
       we have a parsing problem, then just default to "files dns" behavior.  That
       is, check local files first, then fall back to DNS.

       \param name The full path name of the file to read.
       \returns m_resolveDomain eSourceMax=Default, e_EtcHosts=FILES, eDns=DNS
     */
    void NameResolverInternal::ParseFile_nsswitchConf(const std::string & name)
    {
        // Set context to a known state
        m_resolveDomain = eNone;
        m_fResolveBoth = false;

        std::wifstream is(name.c_str());
        if( ! is )
        {
            return;
        }

        // The /etc/nsswitch.conf file has lines like:
        //    <service>: entry [entry]...
        //
        // We look at the 'hosts' service.  If found, then we see what's first:
        // 'files' or 'dns', and we use that first.  If unexpected entries are
        // found on the line, then we fall back to 'files' first and then 'dns'.

        std::wstring line;
        while( is && getline(is, line) )
        {
            string::size_type pos;

            // Strip off any comment ("#" and anything after it)
            if ((pos = line.find(L"#")) != string::npos)
            {
                line.erase(pos, line.length());
            }
            if (line.empty())
            {
                // In case comment was from start of line, or line was just empty
                continue;
            }

            line = StrToLower(line);
            std::vector<std::wstring> parts;
            StrTokenize(line, parts, L": \t");

            // Do we have the "hosts" entry in the file?
            if (parts.size() < 2 || 0 != StrCompare(L"hosts", parts[0])) {
                continue;
            }

            // See what's first: 'files' or 'dns'
            string::size_type i;
            for (i = 1; i < parts.size(); i++)
            {
                if (0 == StrCompare(L"files", parts[i], true))
                {
                    if (eNone == m_resolveDomain)
                    {
                        m_resolveDomain = eEtcHosts;
                    }
                    else
                    {
                        m_fResolveBoth = true;
                    }
                }
                else if (0 == StrCompare(L"dns", parts[i], true))
                {
                    if (eNone == m_resolveDomain)
                    {
                        m_resolveDomain = eDns;
                    }
                    else
                    {
                        m_fResolveBoth = true;
                    }
                }
            }

            break;
        }
    }

    /** 
        Get the host name and domain name.

        \return The machine's host name and domain name.
    */
    std::wstring GetHostDomainname()
    {
        NameResolver mi;
        return mi.GetHostDomainname();
    }
}// end namespace SCXCoreLib

/*--------------------------E-N-D---O-F---F-I-L-E----------------------------*/

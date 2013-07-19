/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**

    \file

    \brief          SCXCoreLib::NameResolverInternal CppUnit test class
    \date           2008-01-18 14:20:00


*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxnameresolver.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <wctype.h>

#if defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
// ok
#else
#error "Not implemented for this plattform"
#endif

using SCXCoreLib::NameResolver;
using SCXCoreLib::StrToUTF8;
using std::string;


/**
   Test class for SCXCoreLib::NameResolver
   
   \date       2008-01-18 14:20:00
 */
class SCXNameResolverTest : public CPPUNIT_NS::TestFixture
{
public:
    CPPUNIT_TEST_SUITE( SCXNameResolverTest );
    CPPUNIT_TEST( TestInitializedOnlyOnce );
    CPPUNIT_TEST( TestDumpString );
    CPPUNIT_TEST( TestGetHostname );
    CPPUNIT_TEST( TestGetHostnameDefault );
    CPPUNIT_TEST( TestGetHostname_by_uname );
    CPPUNIT_TEST( TestGetDomainname );
    CPPUNIT_TEST( TestNormalize );
    CPPUNIT_TEST( TestGetHostByName );
    CPPUNIT_TEST( TestReadEtcHosts );
    CPPUNIT_TEST( TestReadResolvConf );
    CPPUNIT_TEST( TestFile_nsswitch_conf );
    CPPUNIT_TEST( TestStaticClass );
    CPPUNIT_TEST_SUITE_END();

private:
    // Class specific data.

public:
    /**
       Test setup.
    */
    void setUp(void)
    {
    }

    /**
       Test cleanup.
    */
    void tearDown(void)
    {
    }

    /**
       Verify that the NameResolver class is initialized only once
     */
    void TestInitializedOnlyOnce()
    {
        SCXCoreLib::NameResolver mcache;         // Cached object
        SCXCoreLib::NameResolver mcache2;        // Second cached object

        // If already initialized, undo that ...
        mcache.DestructStatic();

        CPPUNIT_ASSERT( !mcache.IsInitialized() );
        CPPUNIT_ASSERT( !mcache2.IsInitialized() );

        // Force the first cached object to be initialized
        mcache.DumpString();

        // The second object better be initialized now ...
        CPPUNIT_ASSERT( mcache2.IsInitialized() );
    }

    /**
       Test DumpString() with known values.
    */
    void TestDumpString(void)
    {
        SCXCoreLib::NameResolverInternal nri;

        nri.m_hostnameSource = SCXCoreLib::eDns;
        nri.m_hostname = L"garbagein";
        nri.m_domainnameSource = SCXCoreLib::eDns;
        nri.m_domainname = L"garbage.out";
        std::wstring ds = nri.DumpString();
        std::wcout << L": " << ds;
        CPPUNIT_ASSERT(ds.size() > 0);
        CPPUNIT_ASSERT(string::npos != ds.find(L"hostnameSource='eDns'"));
        CPPUNIT_ASSERT(string::npos != ds.find(L"hostname='garbagein'"));
        CPPUNIT_ASSERT(string::npos != ds.find(L"domainnameSource='eDns'"));
        CPPUNIT_ASSERT(string::npos != ds.find(L"domainname='garbage.out'"));
    }

    /**
       Test getting the hostname for this machine.
    */
    void TestGetHostname(void)
    {
        try
        {
            SCXCoreLib::NameResolverInternal mi;

            mi.Update();
            std::wstring hostname = mi.GetHostname();
            std::wcout << L": \"" << hostname << L"\""
                    << L" (" << mi.DumpSourceString(mi.GetHostnameSource()) << L")";
            CPPUNIT_ASSERT( hostname.size() > 0 );
        }
        catch(SCXCoreLib::SCXException & e)
        {
            std::wstringstream ss;
            ss << e.What() << std::endl << e.Where();
            std::wcout << ss.str() << std::endl;
            CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(ss.str()));
        }
    }

    /** 
        Test getting default (unprocessed) host name 
    */
    void TestGetHostnameDefault(void)
    {
        try
        {
            SCXCoreLib::NameResolverInternal mi;

            mi.Update();
            std::string hostname_default = ""; 
            std::wstring hostname = mi.GetHostname(&hostname_default); 
            std::wstring hostname_default_w = SCXCoreLib::StrFromUTF8(hostname_default); 
            CPPUNIT_ASSERT(hostname_default_w.compare(hostname) == 0); 
        }
        catch(SCXCoreLib::SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where();
            std::wcout << ss.str() << std::endl;
            CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(ss.str()));
        }
     }

    /**
       Test getting the hostname for this machine.
    */
    void TestGetHostname_by_uname(void)
    {
        try
        {
            SCXCoreLib::NameResolverInternal nri;

            // Get the object in a rational state for testing
            nri.Update();
            nri.m_hostname.clear();
            nri.m_domainname.clear();
            nri.m_hostnameSource = nri.m_domainnameSource = SCXCoreLib::eNone;
       
            nri.GetHostVia_uname();

            std::wcout << L": " << nri.m_hostname.c_str();
            if(SCXCoreLib::eNone != nri.GetDomainnameSource()) 
            {
                std::wcout << L" " << nri.GetDomainname();
            } 
            else
            {
                std::wcout << L" unavailable";
            }
        }
        catch(SCXCoreLib::SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where();
            std::wcout << ss.str() << std::endl;
            CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(ss.str()));
        }
    }

    /*
      Test getting the domain name for this machine.
    */
    void TestGetDomainname(void)
    {
        try
        {
            SCXCoreLib::NameResolverInternal mi;
            std::wstring domainname = mi.GetDomainname();
            if(domainname.empty())
            {
                domainname = L"unavailable";
            }
            std::wcout << L": \"" << domainname << L"\""
                    << L" (" << mi.DumpSourceString(mi.GetDomainnameSource()) << L")";
        }
        catch(SCXCoreLib::SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where();
            std::wcout << ss.str() << std::endl;
            CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(ss.str()));
        }
    }

    /*
      Test normalization by passing known data and looking at the results
     */

    void TestNormalize()
    {
        // Set up the object and check a few cases

        SCXCoreLib::NameResolverInternal nri;

        nri.m_hostname = L"foo.bar.microsoft.com";
        nri.m_domainname = L"microsoft.com";
        nri.m_domainnameSource = SCXCoreLib::eSourceMax;
        nri.Normalize();
        CPPUNIT_ASSERT( nri.m_hostname == L"foo.bar" );
        CPPUNIT_ASSERT( nri.m_domainname == L"microsoft.com" );

        nri.m_hostname = L"foo.bar.microsoft.uk";
        nri.m_domainname = L"microsoft.com";
        nri.Normalize();
        CPPUNIT_ASSERT( nri.m_hostname == L"foo.bar.microsoft.uk" );
        CPPUNIT_ASSERT( nri.m_domainname == L"microsoft.com" );
    }

    /*
      Test getting domain name with a hostname that will cause the lookup to fail
      and throw an exception.
    */
    void TestGetHostByName()
    {
        // Get the object in a rational state for testing

        SCXCoreLib::NameResolverInternal nri;

        nri.Update();
        nri.m_domainname.clear();
        nri.m_domainnameSource = SCXCoreLib::eNone;
       
        std::wcout << L": " << nri.m_hostname.c_str();
            
        nri.GetHostVia_gethostbyname(nri.m_hostname);
        if(SCXCoreLib::eDns == nri.GetDomainnameSource()) 
        {
            std::wcout << L" " << nri.GetDomainname();
        } 
        else
        {
            std::wcout << L" unavailable";
        }

        nri.m_hostnameSource = SCXCoreLib::eDns;
        nri.m_hostname = L"garbagein";
        nri.m_domainnameSource = SCXCoreLib::eNone;
        nri.m_domainname = L"garbage.out";
        nri.GetHostVia_gethostbyname(nri.m_hostname);
        CPPUNIT_ASSERT( SCXCoreLib::eNone == nri.m_domainnameSource );
    }

    /**
       Not a real test, as such, but validation for hostname/domainname

       The host name and domain name should contain non-space, printable
       characters with only basic validation.  According to Wikipedia,
       names can contain a-z, 0-9 and dashes.  We allow dots and the
       underscore character as well.

       As for other restrictions (no leading dash, for example), we don't
       bother.
     */
    bool isIdentifierValid( const std::wstring& name )
    {
        for( std::wstring::const_iterator iter = name.begin(); iter != name.end(); iter++ )
        {
            const wint_t c = *iter;
            if (!(iswalnum(c) || L'-' == c || L'_' == c || L'.' == c))
            {
                return false;
            }
        }

        return true;
    }

    /**
       Test reading the domain name from /etc/hosts file
    */
    void TestReadEtcHosts()
    {
        // Get the object in a rational state for testing

        SCXCoreLib::NameResolverInternal nri;

        nri.Update();
        nri.m_domainname.clear();
        nri.m_domainnameSource = SCXCoreLib::eNone;

        // Bogus file name will fail to get the domain name.
        nri.GetHostVia_EtcHosts("/etc/hosts.does.not.exist");
        CPPUNIT_ASSERT( SCXCoreLib::eNone == nri.m_domainnameSource );

        // Domain name should come from this file (if it is there).
        nri.GetHostVia_EtcHosts("/etc/hosts");
        if (SCXCoreLib::eEtcHosts == nri.m_domainnameSource)
        {
            CPPUNIT_ASSERT( isIdentifierValid(nri.m_domainname) );
            std::wcout << ": " << nri.m_hostname << L" " << nri.m_domainname;
        }
        else
        {
            std::wcout << ": " << nri.m_hostname << L" unavailable";
        }
    }

    /**
       Test reading the domain name from /etc/resolv.conf
    */
    void TestReadResolvConf()
    {
        // Get the object in a rational state for testing

        SCXCoreLib::NameResolverInternal nri;

        nri.Update();
        nri.m_domainname.clear();
        nri.m_domainnameSource = SCXCoreLib::eNone;

        // bogus file name will fail to get the domain name.
        nri.GetHostVia_ResolvConf("/etc/resolv.blah");
        CPPUNIT_ASSERT( SCXCoreLib::eNone == nri.m_domainnameSource );

        // Domain name should come from this file (if it is there).
        nri.GetHostVia_ResolvConf("/etc/resolv.conf");
        if (SCXCoreLib::eEtcResolvConf == nri.m_domainnameSource)
        {
            CPPUNIT_ASSERT( isIdentifierValid(nri.m_domainname) );
            std::wcout << ": " << nri.m_hostname << L" " << nri.m_domainname;
        }
        else
        {
            std::wcout << ": " << nri.m_hostname << L" unavailable";
        }
    }

    /**
       Test reading the nsswitch.conf file
    */
    void TestFile_nsswitch_conf()
    {
        SCXCoreLib::NameResolverInternal nri;

        // First check for "files", then "dns"
        std::ofstream out;
        out.open( "nsswitch_test.conf" );
        CPPUNIT_ASSERT( out );
        out << "# This is a comment line" << std::endl;
        out << "hosts: files dns" << std::endl;
        out.close();
        nri.ParseFile_nsswitchConf("nsswitch_test.conf");
        CPPUNIT_ASSERT( SCXCoreLib::eEtcHosts == nri.m_resolveDomain && true == nri.m_fResolveBoth );

        // Next check for "dns", then "files"
        out.open( "nsswitch_test.conf" );
        CPPUNIT_ASSERT( out );
        out << "foo: " << std::endl;
        out << "hosts: dns files" << std::endl;
        out.close();
        nri.ParseFile_nsswitchConf("nsswitch_test.conf");
        CPPUNIT_ASSERT( SCXCoreLib::eDns == nri.m_resolveDomain && true == nri.m_fResolveBoth );

        // Next: Just "files"
        out.open( "nsswitch_test.conf" );
        CPPUNIT_ASSERT( out );
        out << "bar" << std::endl;
        out << "hosts: files" << std::endl;
        out.close();
        nri.ParseFile_nsswitchConf("nsswitch_test.conf");
        CPPUNIT_ASSERT( SCXCoreLib::eEtcHosts == nri.m_resolveDomain && false == nri.m_fResolveBoth );

        // Next: Just "dns"
        out.open( "nsswitch_test.conf" );
        CPPUNIT_ASSERT( out );
        out << "hosts: dns" << std::endl;
        out << "# This is another comment line" << std::endl;
        out.close();
        nri.ParseFile_nsswitchConf("nsswitch_test.conf");
        CPPUNIT_ASSERT( SCXCoreLib::eDns == nri.m_resolveDomain && false == nri.m_fResolveBoth );

        // Clean up the temporary file ...
        unlink("nsswitch_test.conf");

        // Finally, verify proper behavior if nothing is found
        nri.ParseFile_nsswitchConf("/this/file/should/not/ever/exist/absolutely/definitely");
        CPPUNIT_ASSERT ( SCXCoreLib::eNone == nri.m_resolveDomain && false == nri.m_fResolveBoth );
    }

    void TestStaticClass()
    {
        /*
         * While NameResolverInternal is heavily tested by this test code, class NameResolver
         * (the static class for caching purposes) is not.  There's essentially zero code
         * behind it (it simply calls NameResolverInternal), but these tests absolutely verify
         * that everything's "kosher"
         */

        SCXCoreLib::NameResolver mcache;         // Cached object
        SCXCoreLib::NameResolverInternal mint;   // Object built on demand
        mint.Update();

        CPPUNIT_ASSERT( mcache.GetHostname() == mint.GetHostname() );
        CPPUNIT_ASSERT( mcache.GetDomainname() == mint.GetDomainname() );
        CPPUNIT_ASSERT( mcache.GetHostDomainname() == mint.GetHostDomainname() );
        CPPUNIT_ASSERT( mcache.GetHostnameSource() == mint.GetHostnameSource() );
        CPPUNIT_ASSERT( mcache.GetDomainnameSource() == mint.GetDomainnameSource() );

        // Pick up the debug methods as well
        CPPUNIT_ASSERT( mcache.DumpString() == mint.DumpString() );
        CPPUNIT_ASSERT( mcache.DumpSourceString(mcache.GetHostnameSource())
                        == mint.DumpSourceString(mint.GetHostnameSource()) );
        CPPUNIT_ASSERT( mcache.DumpSourceString(mcache.GetDomainnameSource())
                        == mint.DumpSourceString(mint.GetDomainnameSource()) );
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXNameResolverTest );

//--------------------------E-N-D---O-F---F-I-L-E-------------------------------

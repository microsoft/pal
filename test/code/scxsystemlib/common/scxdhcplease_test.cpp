/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
\file

\brief       Test of DHCPLeaseInfo

\date        2008-03-04 11:03:56

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/scxdhcplease.h>

#include <testutils/scxunit.h>
#include <testutils/scxtestutils.h>

#include <stdio.h>

using namespace SCXSystemLib;
using namespace SCXCoreLib;

// Tests the network interface PAL
class DHCPLeaseTest : public CPPUNIT_NS::TestFixture
{
public:
    SCXFilePath testFile;
    std::wstring fileData;
    SCXCalendarTime expectedLeaseExpires;
    SCXCalendarTime expectedLeaseObtained;
    std::wstring expectedDHCPServer;
    std::wstring expectedDefaultGateway;
    std::wstring expectedDomainName;
    
    CPPUNIT_TEST_SUITE( DHCPLeaseTest );
    CPPUNIT_TEST( TestStrToSCXCalendarTime );
    CPPUNIT_TEST( TestConstructorAndGetters );
    CPPUNIT_TEST_SUITE_END();
    
    //---
    void TestConstructorAndGetters()
    {
#if defined(sun)
        DHCPLeaseInfo leaseInfo
        (
            std::wstring(L"lan0"),
            std::wstring
            (
                L"echo 'Interface  State         Sent  Recv  Declined  Flags\n"
                      L"lan0       BOUND            1     0         0  [PRIMARY]\n"
                      L"(Began, Expires, Renew) = (04/25/2012 19:15, 04/26/2012 07:15, 04/26/2012 01:15)'"
            )
        );
#else
        testFile = SCXFile::CreateTempFile(fileData);
        SelfDeletingFilePath leaseFile( testFile.Get() );

        DHCPLeaseInfo leaseInfo(L"lan0", testFile.Get());
#endif
#if !defined(aix)
    #if !defined(sun)
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Domain Name: "    , leaseInfo.getDomainName())),
                               leaseInfo.getDomainName().compare(expectedDomainName) == 0 );
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"DHCP Server: "    , leaseInfo.getDHCPServer())),
                               leaseInfo.getDHCPServer().compare(expectedDHCPServer) == 0 );
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Default Gateway: ", leaseInfo.getDefaultGateway())),
                               leaseInfo.getDefaultGateway().compare(expectedDefaultGateway) == 0 );
    #endif
        SCXCalendarTime leaseExpires = leaseInfo.getLeaseExpires();
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Year "  , leaseExpires.GetYear())),
                               int(leaseExpires.GetYear()) == int(expectedLeaseExpires.GetYear()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Month " , leaseExpires.GetMonth())),
                               int(leaseExpires.GetMonth() ) == int(expectedLeaseExpires.GetMonth()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Day "   , leaseExpires.GetDay())),
                               int(leaseExpires.GetDay()) == int(expectedLeaseExpires.GetDay()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Hour "  , leaseExpires.GetHour())),
                               int(leaseExpires.GetHour()) == int(expectedLeaseExpires.GetHour()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Minute ", leaseExpires.GetMinute())),
                               int(leaseExpires.GetMinute()) == int(expectedLeaseExpires.GetMinute()));

        SCXCalendarTime leaseObtained = leaseInfo.getLeaseObtained();
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Year " , leaseObtained.GetYear())),
                               int(leaseObtained.GetYear()) == int(expectedLeaseObtained.GetYear()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Month " , leaseObtained.GetMonth())),
                               int(leaseObtained.GetMonth()) == int(expectedLeaseObtained.GetMonth()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Day "   , leaseObtained.GetDay())),
                               int(leaseObtained.GetDay()) == int(expectedLeaseObtained.GetDay()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Hour "  , leaseObtained.GetHour())),
                               int(leaseObtained.GetHour()) == int(expectedLeaseObtained.GetHour()));
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"Expire Minute ", leaseObtained.GetMinute())),
                               int(leaseObtained.GetMinute()) == int(expectedLeaseObtained.GetMinute()));
#endif
    }
    
    void TestStrToSCXCalendarTime()
    {
        SCXCalendarTime calendarTime;
        calendarTime = DHCPLeaseInfo::strToSCXCalendarTime(L"3/1/2014", L"1:15");
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetYear "  , calendarTime.GetYear())),
                               (int) calendarTime.GetYear() == 2014);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetDay "   , calendarTime.GetDay())),
                               (int) calendarTime.GetDay() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMonth " , calendarTime.GetMonth())),
                               (int) calendarTime.GetMonth() == 3);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetHour "  , calendarTime.GetHour())),
                               (int) calendarTime.GetHour() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMinute ", calendarTime.GetMinute())),
                               (int) calendarTime.GetMinute() == 15);

        calendarTime = DHCPLeaseInfo::strToSCXCalendarTime(L"03/1/2014", L"1:15");                                                  
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetYear "  , calendarTime.GetYear())),
                               (int) calendarTime.GetYear() == 2014);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetDay "   , calendarTime.GetDay())),
                               (int) calendarTime.GetDay() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMonth " , calendarTime.GetMonth())),
                               (int) calendarTime.GetMonth() == 3);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetHour "  , calendarTime.GetHour())),
                               (int) calendarTime.GetHour() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMinute ", calendarTime.GetMinute())),
                               (int) calendarTime.GetMinute() == 15);

        calendarTime = DHCPLeaseInfo::strToSCXCalendarTime(L"3/01/2014", L"1:15");                                                  
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetYear "  , calendarTime.GetYear())),
                               (int) calendarTime.GetYear() == 2014);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetDay "   , calendarTime.GetDay())),
                               (int) calendarTime.GetDay() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMonth " , calendarTime.GetMonth())),
                               (int) calendarTime.GetMonth() == 3);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetHour "  , calendarTime.GetHour())),
                               (int) calendarTime.GetHour()  == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMinute ", calendarTime.GetMinute())),
                               (int) calendarTime.GetMinute() == 15);

        calendarTime = DHCPLeaseInfo::strToSCXCalendarTime(L"03/01/2014", L"1:15");                                                 
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetYear "  , calendarTime.GetYear())),
                               (int) calendarTime.GetYear() == 2014);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetDay "   , calendarTime.GetDay())),
                               (int) calendarTime.GetDay() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMonth " , calendarTime.GetMonth())),
                               (int) calendarTime.GetMonth() == 3);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetHour "  , calendarTime.GetHour())),
                               (int) calendarTime.GetHour() == 1);
        CPPUNIT_ASSERT_MESSAGE(StrToUTF8(StrAppend(L"SCXCalendarTime GetMinute ", calendarTime.GetMinute())),
                               (int) calendarTime.GetMinute() == 15);
    }
    
public:
    void setUp()
    {
#if defined(PF_DISTRO_REDHAT) || defined(PF_DISTRO_ULINUX)
        fileData =
            L"lease {\n"
            L"  interface \"eth0\";\n"
            L"  fixed-address 10.217.5.146;\n"
            L"  option subnet-mask 255.255.254.0;\n"
            L"  option routers 10.217.4.1;\n"
            L"  option dhcp-lease-time 43200;\n"
            L"  option dhcp-message-type 5;\n"
            L"  option domain-name-servers 10.217.2.6,10.195.172.6;\n"
            L"  option dhcp-server-identifier 10.217.2.6;\n"
            L"  option dhcp-renewal-time 21600;\n"
            L"  option dhcp-rebinding-time 37800;\n"
            L"  option domain-name \"SCX.com\";\n"
            L"  renew 3 2012/04/25 01:36:19;\n"
            L"  rebind 3 2012/04/25 06:55:00;\n"
            L"  expire 3 2012/04/25 08:25:00;\n"
            L"}\n"
            L"lease {\n"
            L"  interface \"lan0\";\n"
            L"  fixed-address 10.217.5.146;\n"
            L"  option subnet-mask 255.255.254.0;\n"
            L"  option routers 10.217.4.1;\n"
            L"  option dhcp-lease-time 43200;\n"
            L"  option dhcp-message-type 5;\n"
            L"  option domain-name-servers 10.217.2.6,10.195.172.6;\n"
            L"  option dhcp-server-identifier 10.217.2.6;\n"
            L"  option dhcp-renewal-time 21600;\n"
            L"  option dhcp-rebinding-time 37800;\n"
            L"  option domain-name \"SCX.com\";\n"
            L"  renew 3 2012/04/25 07:11:59;\n"
            L"  rebind 3 2012/04/25 12:06:19;\n"
            L"  expire 3 2012/04/25 13:36:19;\n"
            L"}\n";
        expectedLeaseExpires = SCXCalendarTime (2012, 4, 25, 13, 36, 19, SCXRelativeTime());
        expectedLeaseObtained = SCXCalendarTime(2012, 4, 25, 7, 11, 59, SCXRelativeTime());
        expectedDHCPServer = L"10.217.2.6";
        expectedDefaultGateway = L"10.217.4.1";
        expectedDomainName = L"SCX.com";
#elif defined(hpux)
        fileData =
            L"00 4 lan0\n"
            L"01 0 \n"
            L"02 0 \n"
            L"03 0 \n"
            L"04 0 \n"
            L"05 7 SCX.com\n"
            L"06 4 43200\n"
            L"07 4 1335344331\n"
            L"08 4 0\n"
            L"09 4 0\n"
            L"10 4 1\n"
            L"11 6 16 aa 19 ff 30 7a \n"
            L"12 4 10.217.5.127\n"
            L"13 4 255.255.254.0\n"
            L"14 4 0.0.0.0\n"
            L"15 4 10.217.4.1 \n"
            L"16 4 10.217.2.6\n"
            L"17 4 0.0.0.0\n"
            L"18 0 \n"
            L"19 8 10.217.2.6 10.195.172.6 \n"
            L"20 0 \n"
            L"21 4 0.0.0.0\n"
            L"22 0 \n"
            L"23 0 \n"
            L"24 64 63 82 53 63 35 1 5 3a 4 0 0 54 60 3b 4 0 0 93 a8 33 4 0 0 a8 c0 36 4 a d9 2 6 1 4 ff ff fe 0 6 8 a d9 2 6 a c3 ac 6 f 8 53 43 58 2e 63 6f 6d 0 3 4 a d9 4 1 ff \n";
        expectedLeaseExpires = SCXCalendarTime (2012, 4, 25, 8, 58, 51, SCXRelativeTime());
        expectedLeaseObtained = SCXCalendarTime(2012, 4, 25, 2, 58, 51, SCXRelativeTime());
        expectedDomainName = L"SCX.com";
        expectedDHCPServer = L"10.217.2.6";
        expectedDefaultGateway = L"10.217.4.1";
#elif defined(PF_DISTRO_SUSE)
        fileData =
            L"IPADDR='10.217.5.79'\n"
            L"NETMASK='255.255.254.0'\n"
            L"NETWORK='10.217.4.0'\n"
            L"BROADCAST='10.217.5.255'\n"
            L"ROUTES=''\n"
            L"GATEWAYS='10.217.5.255'\n"
            L"DNSDOMAIN='redmond.corp.microsoft.com'\n"
            L"DNSSERVERS='10.200.81.201 10.200.81.202 10.184.232.13 10.184.232.14'\n"
            L"DHCPSID='10.184.232.100'\n"
            L"LEASEDFROM='1335018597'\n"
            L"LEASETIME='619200'\n"
            L"RENEWALTIME='309600'\n"
            L"REBINDTIME='541800'\n"
            L"INTERFACE='lan0'\n"
            L"CLASSID='dhcpcd 3.2.3'\n"
            L"CLIENTID='01:00:16:3e:09:d1:95'\n"
            L"DHCPCHADDR='00:16:3e:09:d1:95'\n"
            L"NETBIOSNAMESERVER='157.54.14.163,157.59.200.249,157.54.14.154'\n";
        expectedLeaseExpires = SCXCalendarTime(2012, 4, 28, 18, 29, 57, SCXRelativeTime());
        expectedLeaseObtained = SCXCalendarTime(2012, 4, 25, 4, 29, 57, SCXRelativeTime());
        expectedDomainName = L"redmond.corp.microsoft.com";
        expectedDHCPServer = L"10.184.232.100";
        expectedDefaultGateway = L"10.217.5.79";
#elif defined(sun)
        expectedLeaseExpires  = SCXCalendarTime(2012, 4, 26, 7, 15, 0, SCXRelativeTime());
        expectedLeaseObtained = SCXCalendarTime(2012, 4, 26, 1, 15, 0, SCXRelativeTime());
#endif
    }
};
CPPUNIT_TEST_SUITE_REGISTRATION( DHCPLeaseTest );


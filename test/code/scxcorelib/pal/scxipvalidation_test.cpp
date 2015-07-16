
/**
    \file

    \brief       Test of network interface configuration

    \date        2015-06-019 10:46:56

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <scxcorelib/scxip.h>

using namespace SCXCoreLib;

using namespace std;

//! Tests IP address validation in the PAL
class SCXIsValidIPAddressTest : public CPPUNIT_NS::TestFixture /* CUSTOMIZE: use a class name with relevant name */
{

public:
    CPPUNIT_TEST_SUITE( SCXIsValidIPAddressTest ); /* CUSTOMIZE: Name must be same as classname */
#if defined(linux)
    CPPUNIT_TEST( TestIPv4addressIsValid );
    CPPUNIT_TEST( TestIPv4MalformedAddress );
    CPPUNIT_TEST( TestIPv4IncompleteAddress );
    CPPUNIT_TEST( TestZeroedIPv4Address );
    CPPUNIT_TEST( TestLengthenedIPv4Address );
    CPPUNIT_TEST( TestNonDottedDecimalIPv4Input );
    CPPUNIT_TEST( TestInvalidCharacterIPv4 );
    CPPUNIT_TEST( TestOutOfBoundsIPv4 );
    CPPUNIT_TEST( TestIPv6addressIsValid );
    CPPUNIT_TEST( TestIPv6MalformedAddress );
    CPPUNIT_TEST( TestIPv6IncompleteAddress );
    CPPUNIT_TEST( TestZeroedIPv6Address );
    CPPUNIT_TEST( TestLengthenedIPv6Address );
    CPPUNIT_TEST( TestIPv4EmbeddedIPv6 );
    CPPUNIT_TEST( TestInvalidCharacterIPv6 );
    CPPUNIT_TEST( TestOutOfBoundsIPv6 );
    CPPUNIT_TEST( TestIPStringValid );
    CPPUNIT_TEST( TestHexToIPAddressConversion);
    CPPUNIT_TEST( TestIPAddressToHexConversion);

#else
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void TestIPv4addressIsValid()
    {
        const char * testipv4MAXVALS = "255.255.255.255";
        const char * testipv4Normal = "192.168.0.1";
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv4MAXVALS));
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv4Normal));
    }

    void TestIPv4MalformedAddress()
    {
        const char * testipv4MALFORMED = "192168.0.1.";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(testipv4MALFORMED));
    }

    void TestIPv4IncompleteAddress()
    {
        const char * ipv4incomplete = "123.";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(ipv4incomplete));
    }

    void TestZeroedIPv4Address()
    {
        const char * testipv4ZERO = "0.0.0.0";
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv4ZERO));
    }

    void TestLengthenedIPv4Address()
    {
        const char * testipv4LONG = "199.199.199.199.199";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(testipv4LONG));
    }

    void TestNonDottedDecimalIPv4Input()
    {
        const char * integerIP = "2204876902";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(integerIP));
    }

    void TestInvalidCharacterIPv4()
    {
        const char * invalidChar = "192.168:0.1";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(invalidChar));
    }

    void TestOutOfBoundsIPv4()
    {
        const char * outOfBoundsIpv4 = "256.255.255.255";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(outOfBoundsIpv4));
    }

    void TestIPv6addressIsValid()
    {
        const char * testipv6Normal = "2001:4898:80e8:ee31::3";
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv6Normal));
    }
    
    void TestIPv6MalformedAddress()
    {
        const char * testipv6MALFORMED = "2001:4132.00e8::0";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(testipv6MALFORMED));
    }

    void TestIPv6IncompleteAddress()
    {
        const char * ipv6incomplete = "123:";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(ipv6incomplete));
    }

    void TestZeroedIPv6Address()
    {
        const char * testipv6ZERO = "0000:0000:0000:0000:0000:0000:0000:0000";
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv6ZERO));
    }

    void TestLengthenedIPv6Address()
    {
        const char * testipv6LONG = "0000:0000:0000:0000:0000:0000:0000:0001.4199";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(testipv6LONG));
    }

    void TestInvalidCharacterIPv6()
    {
        const char * invalidChar = "!!!0:0000:0000:0000:0000:0000:0000:0000";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(invalidChar));
    }

    void TestOutOfBoundsIPv6()
    {
        const char * outOfBoundsIpv6 = "GGG0:0000:0000:0000:0000:0000:0000:0000";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(outOfBoundsIpv6));
    }

    void TestIPv4EmbeddedIPv6()
    {
        const char * embeddedIP = "0:5.0.2.1";
        CPPUNIT_ASSERT_EQUAL(0, SCXCoreLib::IP::IsValidIPAddress(embeddedIP));
    }

    void TestIPStringValid()
    {
        const std::string testipv4Normal = "192.168.0.1";
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv4Normal));
    }


    void TestHexToIPAddressConversion()
    {
        CPPUNIT_ASSERT_EQUAL( StrToUTF8(IP::ConvertHexToIpAddress(L"0xA8EAFFFF")), "168.234.255.255");
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"00F8E70A").compare(L"0.248.231.10") == 0 );
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"A8EAFFFF").compare(L"168.234.255.255") == 0 );
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"01008000").compare(L"1.0.128.0") == 0 );
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"DFCFFFFF").compare(L"223.207.255.255") == 0 );

        // min and max private addresses
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"0A000000").compare(L"10.0.0.0") == 0 );
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"C0A8FFFF").compare(L"192.168.255.255") == 0 );

        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"7FFFFFFF").compare(L"127.255.255.255") == 0 );
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"7F000000").compare(L"127.0.0.0") == 0 );
    }

    void TestIPAddressToHexConversion()
    {
        std::wstring ipAddress(L"0.248.231.10");
        std::wstring hex = SCXCoreLib::IP::ConvertIpAddressToHex(ipAddress);
        CPPUNIT_ASSERT( (hex.compare(L"00F8E70A")==0));

        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"0.248.231.10").compare(L"00F8E70A") == 0 );
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"168.234.255.255").compare(L"A8EAFFFF") == 0 );
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"1.0.128.0").compare(L"01008000") == 0 );
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"223.207.255.255").compare(L"DFCFFFFF") == 0 );

        // min and max private addresses
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"10.0.0.0").compare(L"0A000000") == 0 );
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"192.168.255.255").compare(L"C0A8FFFF") == 0 );

        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"127.0.0.0").compare(L"7F000000") == 0 );
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"127.255.255.255").compare(L"7FFFFFFF") == 0 );
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXIsValidIPAddressTest ); /* CUSTOMIZE: Name must be same as classname */

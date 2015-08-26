/**
   \file         scxipvalidation_test.cpp

    \brief       Test of network interface configuration

    \date        2015-06-019 10:46:56

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxip.h>

using namespace SCXCoreLib;

//! Tests IP address validation in the PAL
class SCXIsValidIPAddressTest : public CPPUNIT_NS::TestFixture
{

public:
    CPPUNIT_TEST_SUITE( SCXIsValidIPAddressTest );

#if defined (SCX_UNIX)

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
    CPPUNIT_TEST( TestHexAddressMinimumValue );
    CPPUNIT_TEST( TestHextAddressMaximumValue );
    CPPUNIT_TEST( TestHexAddressWithIncorrect0xPrefix );
    CPPUNIT_TEST( TestHexAddressWithBadCharacter );
    CPPUNIT_TEST( TestHexAddressWithAllCharactersPartOne );
    CPPUNIT_TEST( TestHexAddressWithAllCharactersPartTwo );
    CPPUNIT_TEST( TextHexAddressIncorrectLengthTooShort );
    CPPUNIT_TEST( TestHexAddressIncorrectLengthTooLong );
    CPPUNIT_TEST( TestHexToIPAddressConversionMinimumValue );
    CPPUNIT_TEST( TestHexToIPAddressConversionMaximumValue );
    CPPUNIT_TEST( TestHexToIPAddressConversionWithAllCharactersPartOne );
    CPPUNIT_TEST( TestHexToIPAddressConversionWithAllCharactersPartTwo );
    CPPUNIT_TEST( TestHexToIPAddressConversionWithBadInputParameter );
    CPPUNIT_TEST( TestIPAddressToHexConversionMinimumValue );
    CPPUNIT_TEST( TestIPAddressToHexConversionMaximumValue );
    CPPUNIT_TEST( TestIPAddressToHexConversionWithAllCharactersPartOne );
    CPPUNIT_TEST( TestIPAddressToHexConversionWithAllCharactersPartTwo );
    CPPUNIT_TEST( TestIPAddressToHexConversionWithBadInputParameter );

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
        CPPUNIT_ASSERT_EQUAL(1, SCXCoreLib::IP::IsValidIPAddress(testipv4Normal.c_str()));
    }

    void TestHexAddressMinimumValue()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("00000000")==true);
    }

    void TestHextAddressMaximumValue()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("FFFFFFFF")==true);
    }

    void TestHexAddressWithIncorrect0xPrefix()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("0x00F8E70A")==false);
    }

    void TestHexAddressWithBadCharacter()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("00F8E70#")==false);
    }

    void TestHexAddressWithAllCharactersPartOne()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("ABCDEF01")==true);
    }

    void TestHexAddressWithAllCharactersPartTwo()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("23456789")==true);
    }

    void TextHexAddressIncorrectLengthTooShort()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("F8E7A39")==false);
    }

    void TestHexAddressIncorrectLengthTooLong()
    {
        CPPUNIT_ASSERT(IP::IsValidHexAddress("F8E7A3987")==false);
    }

    void TestHexToIPAddressConversionMinimumValue()
    {
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"00000000").compare(L"0.0.0.0") == 0 );
    }

    void TestHexToIPAddressConversionMaximumValue()
    {
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"FFFFFFFF").compare(L"255.255.255.255") == 0 );
    }

    void TestHexToIPAddressConversionWithAllCharactersPartOne()
    {
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"ABCDEf01").compare(L"171.205.239.1") == 0 );
    }

    void TestHexToIPAddressConversionWithAllCharactersPartTwo()
    {
        CPPUNIT_ASSERT( IP::ConvertHexToIpAddress(L"23456789").compare(L"35.69.103.137") == 0 );
    }

    void TestHexToIPAddressConversionWithBadInputParameter()
    {
        SCXUNIT_ASSERT_THROWN_EXCEPTION( IP::ConvertIpAddressToHex(L"127.255.255.25n"), SCXCoreLib::SCXInvalidArgumentException, L"not a valid ip address" );
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestIPAddressToHexConversionMinimumValue()
    {
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"0.0.0.0").compare(L"00000000") == 0 );
    }

    void TestIPAddressToHexConversionMaximumValue()
    {
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"255.255.255.255").compare(L"FFFFFFFF") == 0 );
    }

    void TestIPAddressToHexConversionWithAllCharactersPartOne()
    {
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"171.205.239.1").compare(L"ABCDEF01") == 0 );
    }

    void TestIPAddressToHexConversionWithAllCharactersPartTwo()
    {
        CPPUNIT_ASSERT( IP::ConvertIpAddressToHex(L"35.69.103.137").compare(L"23456789") == 0 );
    }

    void TestIPAddressToHexConversionWithBadInputParameter()
    {
        SCXUNIT_ASSERT_THROWN_EXCEPTION( IP::ConvertHexToIpAddress(L"7F00000n"), SCXCoreLib::SCXInvalidArgumentException, L"not a valid hex number" );
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXIsValidIPAddressTest ); /* CUSTOMIZE: Name must be same as classname */

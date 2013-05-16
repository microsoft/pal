/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       String encoding conversion tests

    \date        2011-Jan-10 17:50 GMT
*/
/*----------------------------------------------------------------------------*/

#include <algorithm>
#include <iostream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxstrencodingconv.h>
#include <testutils/scxunit.h>

static const unsigned int s_CODE_POINT_MAXIMUM_VALUE       =   0x10FFFF;
static const unsigned int s_CODE_POINT_SURROGATE_HIGH_MIN  =   0xD800;
static const unsigned int s_CODE_POINT_SURROGATE_HIGH_MAX  =   0xDBFF;
static const unsigned int s_CODE_POINT_SURROGATE_LOW_MIN   =   0xDC00;
static const unsigned int s_CODE_POINT_SURROGATE_LOW_MAX   =   0xDFFF;

static unsigned char s_utf16BytesLE[] =
{
    0xFF, 0xFE,
    0x0A, 0x00, 0x55, 0x00, 0x54, 0x00, 0x46, 0x00, 0x2D, 0x00, 0x38, 0x00, 0x20, 0x00, 0x65, 0x00,
    0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x64, 0x00, 0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x61, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x70, 0x00, 0x6C, 0x00,
    0x61, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x2D, 0x00, 0x74, 0x00, 0x65, 0x00, 0x78, 0x00, 0x74, 0x00,
    0x20, 0x00, 0x66, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x0A, 0x00, 0x3E, 0x20, 0x3E, 0x20,
    0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20,
    0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20,
    0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20,
    0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20, 0x3E, 0x20,
    0x3E, 0x20, 0x3E, 0x20, 0x0A, 0x00, 0x0A, 0x00, 0x4A, 0x00, 0x6F, 0x00, 0x61, 0x00, 0x6E, 0x00,
    0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x20, 0x00, 0x41, 0x00, 0x72, 0x00, 0x63, 0x00, 0x20, 0x00,
    0x5B, 0x00, 0xC8, 0x02, 0x6D, 0x00, 0x61, 0x00, 0xB3, 0x02, 0x6B, 0x00, 0x8A, 0x02, 0x73, 0x00,
    0x20, 0x00, 0x6B, 0x00, 0x75, 0x00, 0xD0, 0x02, 0x6E, 0x00, 0x5D, 0x00, 0x20, 0x00, 0x3C, 0x00,
    0x68, 0x00, 0x74, 0x00, 0x74, 0x00, 0x70, 0x00, 0x3A, 0x00, 0x2F, 0x00, 0x2F, 0x00, 0x77, 0x00,
    0x77, 0x00, 0x77, 0x00, 0x2E, 0x00, 0x63, 0x00, 0x6C, 0x00, 0x2E, 0x00, 0x63, 0x00, 0x61, 0x00,
    0x6D, 0x00, 0x2E, 0x00, 0x61, 0x00, 0x63, 0x00, 0x2E, 0x00, 0x75, 0x00, 0x6B, 0x00, 0x2F, 0x00,
    0x7E, 0x00, 0x6D, 0x00, 0x67, 0x00, 0x6B, 0x00, 0x32, 0x00, 0x35, 0x00, 0x2F, 0x00, 0x3E, 0x00,
    0x14, 0x20, 0x20, 0x00, 0x32, 0x00, 0x30, 0x00, 0x30, 0x00, 0x32, 0x00, 0x2D, 0x00, 0x30, 0x00,
    0x37, 0x00, 0x2D, 0x00, 0x32, 0x00, 0x35, 0x00
};

class SCXStrEncodingConvTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStrEncodingConvTest );
    CPPUNIT_TEST( TestUTF8ToUTF16le );
    CPPUNIT_TEST( TestUTF8ToUTF16leBad );
    CPPUNIT_TEST( TestUTF16leToUTF8 );
    CPPUNIT_TEST( TestUTF16leToUTF8Bad );
    CPPUNIT_TEST( TestUTF8ToUTF16 );
    CPPUNIT_TEST( TestUTF16ToUTF8 );
    CPPUNIT_TEST( TestUTF16ToUTF8Bad );
    CPPUNIT_TEST_SUITE_END();

private:
    std::vector<unsigned char> utf16LEBytes;
    std::string utf8String;

public:
    void setUp()
    {
        utf8String.assign("\x0A"
                          "UTF-8 encoded sample plain-text file\x0A"
                          "\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80"
                          "\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80"
                          "\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE"
                          "\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2"
                          "\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80"
                          "\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE"
                          "\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2\x80\xBE\xE2"
                          "\x80\xBE\x0A"
                          "\x0A"
                          "Joan of Arc [\xCB\x88ma\xCA\xB3k\xCA\x8As ku\xCB\x90n] <http://www.cl.cam.ac.uk/~mgk25/>\xE2\x80\x94 2002-07-25"
                          );
        utf16LEBytes.assign(&s_utf16BytesLE[2], &s_utf16BytesLE[sizeof s_utf16BytesLE]);
    }

    void TestUTF8ToUTF16le()
    {
        std::vector<unsigned char> test;

        CPPUNIT_ASSERT(SCXCoreLib::Utf8ToUtf16le(utf8String, test));
        CPPUNIT_ASSERT_EQUAL(test.size(), utf16LEBytes.size());
        CPPUNIT_ASSERT(std::mismatch(test.begin(), test.end(), utf16LEBytes.begin()).first == test.end());
    }

    void TestUTF8ToUTF16leBad()
    {
        // copy 8 correct bytes to test string
        std::string bad;
        bad.assign(8, '\0');
        memcpy(&bad[0], "ABcdEFgh", 8);

        // dangling prefix byte at end of string
        std::vector<unsigned char> test;
        bad[7] = (char)0xB1;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // dangling 3-byte prefix byte + 1 continuation byte 2 bytes before end of string
        bad[6] = (char)0xE1;
        bad[7] = (char)0x88;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // prefix byte without continuation byte
        memcpy(&bad[0], "0QW34r30", 8);
        bad[4] = (char)0xE1;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // continuation byte without prefix byte
        bad[4] = (char)0x83;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // prefix byte followed by another prefix byte
        bad[4] = (char)0xC1;
        bad[4] = (char)0xC3;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // 3-byte prefix byte + 1 continuation byte
        bad[4] = (char)0xB3;
        bad[5] = (char)0x80;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // an overlong form (a character that can be encoded with a shorter form)
        // form the character 0x83 with three bytes instead of two
        bad[4] = (char)0xE0;
        bad[5] = (char)0x82;
        bad[6] = (char)0x83;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));

        // correctly-formed character larger than the maximum UTF value (0x10FFFF)
        // form the character 0x110000
        bad[4] = (char)0xF4;
        bad[5] = (char)0x10;
        bad[6] = (char)0x80;
        bad[7] = (char)0x80;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf8ToUtf16le(bad, test));
    }

    void TestUTF16leToUTF8()
    {
        std::string test;

        CPPUNIT_ASSERT(SCXCoreLib::Utf16leToUtf8(utf16LEBytes, test));
        CPPUNIT_ASSERT_EQUAL(test.size(), utf8String.size());
        CPPUNIT_ASSERT(test == utf8String);
    }

    void TestUTF16leToUTF8Bad()
    {
        // copy 4 characters, 8 bytes, to test string
        std::vector<unsigned char> bad(&utf16LEBytes[0], &utf16LEBytes[8]);

        // a string with a dangling surrogate high character at the end
        utf16LEBytes[6] = (unsigned char)(s_CODE_POINT_SURROGATE_HIGH_MIN & 0x00FF);
        utf16LEBytes[7] = (unsigned char)((s_CODE_POINT_SURROGATE_HIGH_MIN >> 8) & 0x00FF);
        std::string test;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16leToUtf8(utf16LEBytes, test));

        // a string with a dangling surrogate high character
        bad.assign(&utf16LEBytes[0], &utf16LEBytes[8]);
        utf16LEBytes[2] = (unsigned char)(s_CODE_POINT_SURROGATE_HIGH_MIN & 0x00FF);
        utf16LEBytes[3] = (unsigned char)((s_CODE_POINT_SURROGATE_HIGH_MIN >> 8) & 0x00FF);
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16leToUtf8(utf16LEBytes, test));

        // a string with a dangling surrogate low character
        utf16LEBytes[2] = (unsigned char)(s_CODE_POINT_SURROGATE_LOW_MIN & 0x00FF);
        utf16LEBytes[3] = (unsigned char)((s_CODE_POINT_SURROGATE_LOW_MIN >> 8) & 0x00FF);
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16leToUtf8(utf16LEBytes, test));

        // a string with a character larger that the maximum UTF value (0x10FFFF)
        utf16LEBytes[2] = (unsigned char)(s_CODE_POINT_SURROGATE_HIGH_MAX & 0x00FF);
        utf16LEBytes[3] = (unsigned char)((s_CODE_POINT_SURROGATE_HIGH_MAX >> 8) & 0x00FF);
        utf16LEBytes[4] = (unsigned char)(s_CODE_POINT_SURROGATE_LOW_MAX & 0x00FF);
        utf16LEBytes[5] = (unsigned char)((s_CODE_POINT_SURROGATE_LOW_MAX >> 8) & 0x00FF);
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16leToUtf8(utf16LEBytes, test));
    }

    void TestUTF8ToUTF16()
    {
        std::vector<unsigned char> test;

        CPPUNIT_ASSERT(SCXCoreLib::Utf8ToUtf16(utf8String, test));
        CPPUNIT_ASSERT_EQUAL(test.size(), sizeof s_utf16BytesLE);
        CPPUNIT_ASSERT(std::mismatch(test.begin(), test.end(), s_utf16BytesLE).first == test.end());
    }

    void TestUTF16ToUTF8()
    {
        // change the low-endian test string into high-endian for this test
        std::vector<unsigned char> utf16BytesHE;
        utf16BytesHE.assign(sizeof s_utf16BytesLE, 0);
        for (size_t i = 0; i < sizeof s_utf16BytesLE; i += 2)
        {
            utf16BytesHE[i] = s_utf16BytesLE[i + 1];
            utf16BytesHE[i + 1] = s_utf16BytesLE[i];
        }

        // do the test
        std::string test;
        CPPUNIT_ASSERT(SCXCoreLib::Utf16ToUtf8(utf16BytesHE, test));
        CPPUNIT_ASSERT_EQUAL(test.size(), utf8String.size());
        CPPUNIT_ASSERT(test == utf8String);
    }

    void TestUTF16ToUTF8Bad()
    {
        std::vector<unsigned char> badLE(&s_utf16BytesLE[2], &s_utf16BytesLE[10]);

        // no BOM
        // change the low-endian test string into high-endian for this test
        std::vector<unsigned char> badHE;
        badHE.assign(10, '\0');
        for (size_t i = 0; i < 8; i += 2)
        {
            badHE[i] = badLE[i + 1];
            badHE[i + 1] = badLE[i];
        }
        std::string test;
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16ToUtf8(utf16LEBytes, test));

        // change the low-endian test string into high-endian for this test
        badLE.assign(&s_utf16BytesLE[0], &s_utf16BytesLE[10]);
        for (size_t i = 0; i < 10; i += 2)
        {
            badHE[i] = badLE[i + 1];
            badHE[i + 1] = badLE[i];
        }

        // a string with a dangling surrogate high character
        badHE[4] = (unsigned char)((s_CODE_POINT_SURROGATE_HIGH_MIN >> 8) & 0x000000FF);
        badHE[5] = (unsigned char)(s_CODE_POINT_SURROGATE_HIGH_MIN & 0x000000FF);
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16ToUtf8(badHE, test));
        // a string with a dangling surrogate low character
        badHE[4] = (unsigned char)((s_CODE_POINT_SURROGATE_LOW_MIN >> 8) & 0x000000FF);
        badHE[5] = (unsigned char)(s_CODE_POINT_SURROGATE_LOW_MIN & 0x000000FF);
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16ToUtf8(badHE, test));

        // a string with a character larger that the maximum UTF value (0x10FFFF)
        badHE[4] = (unsigned char)((s_CODE_POINT_SURROGATE_HIGH_MAX >> 8) & 0x000000FF);
        badHE[5] = (unsigned char)(s_CODE_POINT_SURROGATE_HIGH_MAX & 0x000000FF);
        badHE[6] = (unsigned char)((s_CODE_POINT_SURROGATE_LOW_MIN >> 8) & 0x000000FF);
        badHE[7] = (unsigned char)(s_CODE_POINT_SURROGATE_LOW_MIN & 0x000000FF);
        CPPUNIT_ASSERT(!SCXCoreLib::Utf16ToUtf8(utf16LEBytes, test));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStrEncodingConvTest );

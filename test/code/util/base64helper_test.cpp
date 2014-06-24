/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2014-06-13 12:20:00

    Base64Helper class unit tests.
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <util/Base64Helper.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>


std::string s_inputArray[5] = {
    "pleasure.",
    "leasure.",
    "easure.",
    "asure.",
    "sure."
};

std::string s_encodedArray[5] =
{
    "cGxlYXN1cmUu",
    "bGVhc3VyZS4=",
    "ZWFzdXJlLg==",
    "YXN1cmUu",
    "c3VyZS4="
};


using namespace SCXCoreLib;

class Base64Helper_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( Base64Helper_Test );

    CPPUNIT_TEST( TestEncode );
    CPPUNIT_TEST( TestDecode );
    CPPUNIT_TEST( TestEncodeAsString );
    CPPUNIT_TEST( TestDecodeAsString );
    CPPUNIT_TEST( TestDecodeWithSameParameter );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestEncode()
    {
        std::vector<unsigned char> input1;
        std::string output1;

        for (int i = 0 ; i < 5; ++i)
        {
            input1.clear();
            input1 = ToUnsignedCharVector(s_inputArray[i]);
            util::Base64Helper::Encode(input1, output1);

            CPPUNIT_ASSERT_EQUAL(s_encodedArray[i], output1);
        }
    }

    void TestDecode()
    {
        std::vector<unsigned char> output1;

        for (int i = 0 ; i < 5; ++i)
        {
            output1.clear();
            util::Base64Helper::Decode(s_encodedArray[i], output1);

            std::string outputStr = FromUnsignedCharVector(output1);
            
            CPPUNIT_ASSERT_EQUAL(s_inputArray[i], outputStr);
        }
    }

    void TestEncodeAsString()
    {
        std::string output1;

        for (int i = 0 ; i < 5; ++i)
        {
            util::Base64Helper::Encode(s_inputArray[i], output1);
            CPPUNIT_ASSERT_EQUAL(s_encodedArray[i], output1);
        }
    }

    void TestDecodeAsString()
    {
        for (int i = 0 ; i < 5; ++i)
        {
            std::string output;
            util::Base64Helper::Decode(s_encodedArray[i], output);
            CPPUNIT_ASSERT_EQUAL(s_inputArray[i], output);
        }
    }

    void TestDecodeWithSameParameter()
    {
        // Be sure specifying same parameter for input and output works properly
        for (int i = 0; i < 5; ++i)
        {
            std::string parameter = s_encodedArray[i];
            util::Base64Helper::Decode(parameter, parameter);
            CPPUNIT_ASSERT_EQUAL(s_inputArray[i], parameter);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( Base64Helper_Test );

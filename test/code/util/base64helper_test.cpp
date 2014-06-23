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


class Base64Helper_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( Base64Helper_Test );

    CPPUNIT_TEST( TestEncode );
    CPPUNIT_TEST( TestDecode );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
       Utility function to convert a String to a unsigned char vector

       \param [in] input Input String
       \return Corresponding unsigned char vector
    */
    static inline std::vector<unsigned char> ToUnsignedCharVector(const std::string& input)
    {
        std::basic_string<unsigned char> unsignedStr(reinterpret_cast<const unsigned char*>(input.c_str()));
        std::vector<unsigned char> output(unsignedStr.begin(), unsignedStr.end());
        return output;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Utility function to convert a unsigned char vector to string

       \param [in] input The input vector
       \return The corresponding string 
    */
    static inline std::string FromUnsignedCharVector(const std::vector<unsigned char>& input)
    {
        std::string output(reinterpret_cast<const char*>(&(input[0])), input.size());
        return output;
    }

    void TestEncode()
    {
        std::vector<unsigned char> input1;
        std::string output1, input2;

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
        std::vector<unsigned char> input1, output1;

        for (int i = 0 ; i < 5; ++i)
        {
            output1.clear();
            util::Base64Helper::Decode(s_encodedArray[i], output1);

            std::string outputStr = FromUnsignedCharVector(output1);
            
            CPPUNIT_ASSERT_EQUAL(s_inputArray[i], outputStr);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( Base64Helper_Test );

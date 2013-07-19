/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**

   \file

   \brief          SCXCoreLib::Marshal CppUnit test class
   \date           2011-04-18 14:20:00


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxmarshal.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>

#include <iostream>
#include <fstream>
#include <limits.h>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <wctype.h>


/**
   Test class for SCXCoreLib::Marshal and SCXCoreLib::UnMarshal class
   
   \date       2011-04-18 14:20:00
*/
class SCXMarshalTest : public CPPUNIT_NS::TestFixture 
{
public:
    CPPUNIT_TEST_SUITE( SCXMarshalTest );
    CPPUNIT_TEST( TestWriteInt );
    CPPUNIT_TEST( TestWriteBigInt );
    CPPUNIT_TEST( TestWriteBigNegInt );
    CPPUNIT_TEST( TestWriteWString );
    CPPUNIT_TEST( TestWriteEmptyWString );
    CPPUNIT_TEST( TestWriteVectorOfString );
    CPPUNIT_TEST( TestWriteRegexWithIndex );
    CPPUNIT_TEST( TestWriteVectorOfRegexWithIndex );
    CPPUNIT_TEST( TestMarshalException );
    CPPUNIT_TEST_SUITE_END();

public:
 
    void TestWriteInt(void)
    {
        // Marshal an integer
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        int val = 10;
        msobj.Write(val);

        // UnMarshall the integer and verify it's correct
        SCXCoreLib::UnMarshal unobj(stream);
        int vv;
        unobj.Read(vv);
        CPPUNIT_ASSERT(vv == val);
    }

    void TestWriteBigInt(void)
    {
        // Marshal a big integer
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        int val = INT_MAX;
        msobj.Write(val);

        // UnMarshall the integer and verify it's correct
        SCXCoreLib::UnMarshal unobj(stream);
        int vv;
        unobj.Read(vv);
        CPPUNIT_ASSERT(vv == val);
    }

    void TestWriteBigNegInt(void)
    {
        // Marshal a big negative number
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        int val = INT_MIN;
        msobj.Write(val);

        // UnMarshal and verify it's correct
        SCXCoreLib::UnMarshal unobj(stream);
        int vv;
        unobj.Read(vv);
        CPPUNIT_ASSERT(vv == val);
    }

    void TestWriteWString(void)
    {
        // Marshal a wstring (wide string)
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        std::wstring istr = L"abc  def";
        msobj.Write(istr);

        // UnMarshal and verify it's correct
        SCXCoreLib::UnMarshal unobj(stream);
        std::wstring ostr;
        unobj.Read(ostr);
        CPPUNIT_ASSERT(istr == ostr);
    }

    void TestWriteEmptyWString(void)
    {
        // Marshal an empty wstring (wide string)
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        std::wstring istr = L"";
        msobj.Write(istr);

        // UnMarshal and verify it's correct
        SCXCoreLib::UnMarshal unobj(stream);
        std::wstring ostr;
        unobj.Read(ostr);
        CPPUNIT_ASSERT(istr == ostr);
    }

    void TestWriteVectorOfString(void)
    {
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        vector<std::wstring> vws;
        std::wstring istr1 = L"abc";
        std::wstring istr2 = L"def";
        vws.push_back(istr1);
        vws.push_back(istr2);
        msobj.Write(vws);

        SCXCoreLib::UnMarshal unobj(stream);
        vector<std::wstring> vws2;
        unobj.Read(vws2);

        CPPUNIT_ASSERT_EQUAL(vws.size(), vws2.size());
        for(int ii = 0; ii < (int) vws.size(); ii++)
        {
            CPPUNIT_ASSERT(vws[ii] == vws2[ii]);
        }
    }

    void TestWriteRegexWithIndex(void)
    {
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        SCXCoreLib::SCXRegexWithIndex ri;
        std::wstring expression(L"abc");
        ri.index = 2;
        ri.regex = new SCXCoreLib::SCXRegex(expression);
        msobj.Write(ri);

        SCXCoreLib::UnMarshal unobj(stream);
        SCXCoreLib::SCXRegexWithIndex ri2;
        unobj.Read(ri2);

        CPPUNIT_ASSERT_EQUAL(ri.index, ri2.index);
        CPPUNIT_ASSERT(ri.regex->Get() == ri2.regex->Get());
    }
 
    void TestWriteVectorOfRegexWithIndex(void)
    {
        // Marshal a vector of SCXRegexWithIndex structures
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        vector<SCXCoreLib::SCXRegexWithIndex> vri;

        SCXCoreLib::SCXRegexWithIndex ri1;
        std::wstring expression1(L"abc");
        ri1.index = 1;
        ri1.regex = new SCXCoreLib::SCXRegex(expression1);
        SCXCoreLib::SCXRegexWithIndex ri2;
        std::wstring expression2(L"def");
        ri2.index = 2;
        ri2.regex = new SCXCoreLib::SCXRegex(expression2);

        vri.push_back(ri1);
        vri.push_back(ri2);
        msobj.Write(vri);

        // UnMarshal and verify that it's correct
        SCXCoreLib::UnMarshal unobj(stream);

        vector<SCXCoreLib::SCXRegexWithIndex> vri2;
        unobj.Read(vri2);

        CPPUNIT_ASSERT_EQUAL(vri.size(), vri2.size());
        for(int ii = 0; ii < (int) vri.size(); ii++)
        {
            CPPUNIT_ASSERT_EQUAL(vri[ii].index, vri2[ii].index);
            CPPUNIT_ASSERT(vri[ii].regex->Get() == vri2[ii].regex->Get());
        }

        // Assure that the received regular expressions are actually correct
        CPPUNIT_ASSERT_EQUAL(std::string("abc"), SCXCoreLib::StrToUTF8(vri2[0].regex->Get()));
        CPPUNIT_ASSERT_EQUAL(std::string("def"), SCXCoreLib::StrToUTF8(vri2[1].regex->Get()));
    }

    void TestMarshalException(void)
    {
        // Marshal an integer
        stringstream stream;
        SCXCoreLib::Marshal msobj(stream);
        int val = 10;
        msobj.Write(val);

        // UnMarshal as a string - should raise exception
        SCXCoreLib::UnMarshal unobj(stream);
        std::wstring ws;

        CPPUNIT_ASSERT_THROW_MESSAGE(
            "\"SCXMarshalFormatException\" exception expected",
            unobj.Read(ws),
            SCXCoreLib::SCXMarshalFormatException);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXMarshalTest );

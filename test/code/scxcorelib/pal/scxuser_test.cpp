/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Test for the user PAL

    \date        2008-03-25 11:22:33

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxuser.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>

#include <unistd.h>
#include <sys/types.h>

class SCXUserTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXUserTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( TestUserIdCorrect );
    CPPUNIT_TEST( TestIsRoot );
    CPPUNIT_TEST( TestIsNotRoot );
    CPPUNIT_TEST( TestName );
    CPPUNIT_TEST_SUITE_END();

private:


public:
    void setUp(void)
    {

    }

    void tearDown(void)
    {

    }

    void callDumpStringForCoverage()
    {
        CPPUNIT_ASSERT(SCXCoreLib::SCXUser().DumpString().find(L"SCXUser") != std::wstring::npos);
    }

    void TestUserIdCorrect(void)
    {
        SCXCoreLib::SCXUser current;
        CPPUNIT_ASSERT_EQUAL(static_cast<SCXCoreLib::SCXUserID>(geteuid()), current.GetUID());
    }

    void TestIsRoot(void)
    {
        SCXCoreLib::SCXUser user(0);
        CPPUNIT_ASSERT(user.IsRoot());
    }

    void TestIsNotRoot(void)
    {
        SCXCoreLib::SCXUser user(1);
        CPPUNIT_ASSERT( ! user.IsRoot());
    }

    void TestName(void)
    {
        SCXCoreLib::SCXUser user(0);
        SCXCoreLib::SCXUser user1(1);
        CPPUNIT_ASSERT(user.GetName() == L"root");
        CPPUNIT_ASSERT(user1.GetName() != L"root");
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXUserTest );

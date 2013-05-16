/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-07-25 15:00:13

    Log backend test
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/testlogbackend.h>

using namespace SCXCoreLib;


class SCXLogBackendTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogBackendTest );
    CPPUNIT_TEST( TestLogThisItem );
    CPPUNIT_TEST( TestGetEffectiveSeverity );
    CPPUNIT_TEST( TestThreadSafe );
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestLogThisItem()
    {
        TestLogBackend b;
        b.SetSeverityThreshold(L"scx.core", eInfo);
        SCXLogItem trace   = SCXLogItem(L"scx.core", eTrace,   L"some trace message", SCXSRCLOCATION, 0);
        SCXLogItem warning = SCXLogItem(L"scx.core", eWarning, L"some warning message", SCXSRCLOCATION, 0);

        // Make sure DoLogItem gets called when item passes severity threshold.
        b.LogThisItem(warning);
        CPPUNIT_ASSERT(L"some warning message" == b.GetLastLogItem().GetMessage());
        b.LogThisItem(trace);
        CPPUNIT_ASSERT(L"some trace message" != b.GetLastLogItem().GetMessage());
    }

    void TestGetEffectiveSeverity()
    {
        TestLogBackend b;
        CPPUNIT_ASSERT( true == b.SetSeverityThreshold(L"", eError) );
        CPPUNIT_ASSERT( eError == b.GetEffectiveSeverity(L"what") );
        CPPUNIT_ASSERT( eError == b.GetEffectiveSeverity(L"what.ever") );
        CPPUNIT_ASSERT( true == b.SetSeverityThreshold(L"what.ever", eWarning) );
        CPPUNIT_ASSERT( eError == b.GetEffectiveSeverity(L"what") );
        CPPUNIT_ASSERT( eWarning == b.GetEffectiveSeverity(L"what.ever") );
        CPPUNIT_ASSERT( eWarning == b.GetEffectiveSeverity(L"what.ever.dude") );
        CPPUNIT_ASSERT( eError == b.GetEffectiveSeverity(L"what.you.want") );
        CPPUNIT_ASSERT( false == b.ClearSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( true == b.ClearSeverityThreshold(L"what.ever") );
        CPPUNIT_ASSERT( eError == b.GetEffectiveSeverity(L"what.ever") );
        CPPUNIT_ASSERT( eError == b.GetEffectiveSeverity(L"what.ever.dude") );
    }

    void TestThreadSafe()
    {
        SCXCoreLib::SCXThreadLockHandle lockH = SCXCoreLib::ThreadLockHandleGet();
        TestLogBackend b(lockH);
        
        SCXCoreLib::SCXThreadLock lock(lockH);

        SCXCoreLib::SCXLogItem l(L"scxcore.something",
                                 SCXCoreLib::eWarning,
                                 L"something",
                                 SCXSRCLOCATION,
                                 0);

        CPPUNIT_ASSERT_THROW(b.LogThisItem(l), SCXCoreLib::SCXThreadLockHeldException);
        CPPUNIT_ASSERT_THROW(b.GetEffectiveSeverity(L"scxcore.something"), SCXCoreLib::SCXThreadLockHeldException);
        CPPUNIT_ASSERT_THROW(b.SetSeverityThreshold(L"scxcore.something", eError), SCXCoreLib::SCXThreadLockHeldException);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogBackendTest );

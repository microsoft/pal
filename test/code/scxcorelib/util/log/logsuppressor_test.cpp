/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       Test for the log suppressor.
   \date        2009-06-19 06:44:05
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/scxthreadlock.h>

struct MockLog
{
    SCXCoreLib::SCXLogSeverity m_severity;
    std::wstring                  m_message;


    void Log(SCXCoreLib::SCXLogSeverity severity, const std::wstring & message, const SCXCoreLib::SCXCodeLocation & location)
    {
        m_severity = severity;
        m_message  = message;

        (void) location;
    }

    SCXCoreLib::SCXLogSeverity GetSeverityThreshold() const
    {
        return SCXCoreLib::eNotSet;
    }

    MockLog()
        : m_severity(SCXCoreLib::eNotSet), m_message()
    {
    }
};

class LogSuppressorTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( LogSuppressorTest );
    CPPUNIT_TEST( FirstCallToGetSeverityReturnsInitialSeverity );
    CPPUNIT_TEST( SecondCallToGetSeverityReturnsDropToSeverity );
    CPPUNIT_TEST( TwoCallsToGetSeverityWithDifferentIDReturnsInitialSeverity );
    CPPUNIT_TEST( GetSeverityTakesThreadLock );
    CPPUNIT_TEST( WorksWithLogMacro );
    CPPUNIT_TEST_SUITE_END();

public:

    void FirstCallToGetSeverityReturnsInitialSeverity()
    {
        SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eWarning, suppressor.GetSeverity(L"Some Id"));
    }

    void SecondCallToGetSeverityReturnsDropToSeverity()
    {
        SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        suppressor.GetSeverity(L"Some Id");
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eTrace, suppressor.GetSeverity(L"Some Id"));
    }

    void TwoCallsToGetSeverityWithDifferentIDReturnsInitialSeverity()
    {
        SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eWarning, suppressor.GetSeverity(L"ID 1"));
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eWarning, suppressor.GetSeverity(L"ID 2"));
    }

    void GetSeverityTakesThreadLock()
    {
        SCXCoreLib::SCXThreadLockHandle lockH = SCXCoreLib::ThreadLockHandleGet();
        SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace, lockH);
        SCXCoreLib::SCXThreadLock lock(lockH);
        CPPUNIT_ASSERT_THROW(suppressor.GetSeverity(L"Some ID"), SCXCoreLib::SCXThreadLockHeldException);
    }

    // Verify that, when LogSuppressor is used with log macro, it behaves as expected
    void WorksWithLogMacro()
    {
        MockLog mockLog;
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eInfo);

        // If called twice, verify eWarning (first time) followed by eInfo
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eWarning, suppressor.GetSeverity(L"ID 1"));
        SCX_LOG(mockLog, suppressor.GetSeverity(L"ID 1"), L"info");
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eInfo, mockLog.m_severity);

        // If called once (inline with SCX_LOG), verify eWarning
        SCX_LOG(mockLog, suppressor.GetSeverity(L"ID 2"), L"warning");
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eWarning, mockLog.m_severity);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( LogSuppressorTest );

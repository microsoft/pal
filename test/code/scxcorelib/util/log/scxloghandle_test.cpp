/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-06-05 15:55:56

    Test class for scxloghandle.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <testutils/scxunit.h>
#include "scxcorelib/util/log/scxlogmediatorsimple.h"
#include <scxcorelib/scxlogitem.h>

#include <scxcorelib/testlogbackend.h>
#include <scxcorelib/testlogmediator.h>
#include <scxcorelib/testlogconfigurator.h>

using namespace SCXCoreLib;


/*----------------------------------------------------------------------------*/
/**
    Test class for SCXLogHandle.
    
    Created     07-06-07 09:46:30
    
    Purpose     Test functionality of the SCXLogHandle.

    Tests as much as possible. Main area of functionality is severity filtering.
    
*/
class SCXLogHandleTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogHandleTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( TestHysterical );
    CPPUNIT_TEST( TestTrace );
    CPPUNIT_TEST( TestInfo );
    CPPUNIT_TEST( TestWarning );
    CPPUNIT_TEST( TestError );
    CPPUNIT_TEST( TestSuppress );
    CPPUNIT_TEST( TestInternal );
    CPPUNIT_TEST( TestThreadID );
    CPPUNIT_TEST( TestClearSeverityThreshold );
    CPPUNIT_TEST_SUITE_END();

private:
    SCXHandle<TestLogMediator> m_mediator;
    SCXHandle<TestLogConfigurator> m_configurator;
    SCXLogHandle m_log;

public:
    SCXLogHandleTest() :
        m_mediator(new TestLogMediator()),
        m_configurator(new TestLogConfigurator(m_mediator)),
        m_log(L"scx.core", m_mediator, m_configurator)
    {
        m_configurator->m_testBackend->SetSeverityThreshold(L"", eWarning);
    }

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void callDumpStringForCoverage()
    {
        CPPUNIT_ASSERT(m_log.DumpString().find(L"SCXLogHandle") != std::wstring::npos);
        CPPUNIT_ASSERT(SCXLogHandleFactory::Instance().DumpString().find(L"SCXLogHandleFactory") != std::wstring::npos);
        m_log.SetSeverityThreshold(eHysterical);
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.DumpString().find(L"SCXLogItem") != std::wstring::npos);
        SCXLogMediatorSimple mediator;
        CPPUNIT_ASSERT(mediator.DumpString().find(L"SCXLogMediatorSimple") != std::wstring::npos);
    }

    void TestHysterical(void)
    {
        m_log.SetSeverityThreshold(eHysterical);
        
        // Above threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Hysterical");
        CPPUNIT_ASSERT(i.GetSeverity() == eHysterical);

        // Above threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Trace");
        CPPUNIT_ASSERT(i.GetSeverity() == eTrace);

        // Above threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Info");
        CPPUNIT_ASSERT(i.GetSeverity() == eInfo);

        // Above threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Warning");
        CPPUNIT_ASSERT(i.GetSeverity() == eWarning);

        // Above threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Error");
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
    }

    void TestTrace(void)
    {
        m_log.SetSeverityThreshold(eTrace);

        // Below threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eHysterical);

        // Above threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Trace");
        CPPUNIT_ASSERT(i.GetSeverity() == eTrace);

        // Above threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Info");
        CPPUNIT_ASSERT(i.GetSeverity() == eInfo);

        // Above threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Warning");
        CPPUNIT_ASSERT(i.GetSeverity() == eWarning);

        // Above threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Error");
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
    }

    void TestInfo(void)
    {
        m_log.SetSeverityThreshold(eInfo);

        // Below threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eHysterical);

        // Below threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eTrace);

        // Above threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Info");
        CPPUNIT_ASSERT(i.GetSeverity() == eInfo);

        // Above threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Warning");
        CPPUNIT_ASSERT(i.GetSeverity() == eWarning);

        // Above threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Error");
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
    }

    void TestWarning(void)
    {
        m_log.SetSeverityThreshold(eWarning);

        // Below threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eHysterical);

        // Below threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eTrace);

        // Below threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eInfo);

        // Above threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Warning");
        CPPUNIT_ASSERT(i.GetSeverity() == eWarning);

        // Above threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Error");
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
    }

    void TestError(void)
    {
        m_log.SetSeverityThreshold(eError);

        // Below threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eHysterical);

        // Below threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eTrace);

        // Below threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eInfo);

        // Below threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eWarning);

        // Above threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Error");
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
    }

    void TestSuppress(void)
    {
        m_log.SetSeverityThreshold(eSuppress);

        // Below threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eHysterical);

        // Below threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eTrace);

        // Below threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eInfo);

        // Below threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eWarning);

        // Below threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eError);
    }

    void TestInternal(void)
    {
        m_log.SetSeverityThreshold(eTrace);

        // Internal build should not get through unless ENABLE_INTERNAL_LOGS is defined.
        SCX_LOGINTERNAL(m_log, eError, L"Internal");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();

#if defined(ENABLE_INTERNAL_LOGS)
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
#else
        CPPUNIT_ASSERT(i.GetSeverity() != eError);
#endif
    }

    void TestThreadID()
    {
        m_log.SetSeverityThreshold(eError);

        SCX_LOGERROR(m_log, L"Error");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(L"Error" == i.GetMessage());
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::SCXThread::GetCurrentThreadID(),i.GetThreadId());
    }

    void TestClearSeverityThreshold()
    {
        m_log.SetSeverityThreshold(eError);
        m_log.ClearSeverityThreshold();
        // Should now default back to warning which is default.

        // Below threshold
        SCX_LOGHYSTERICAL(m_log, L"Hysterical");
        SCXLogItem i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eHysterical);

        // Below threshold
        SCX_LOGTRACE(m_log, L"Trace");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eTrace);

        // Below threshold
        SCX_LOGINFO(m_log, L"Info");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetSeverity() != eInfo);

        // Above threshold
        SCX_LOGWARNING(m_log, L"Warning");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Warning");
        CPPUNIT_ASSERT(i.GetSeverity() == eWarning);

        // Above threshold
        SCX_LOGERROR(m_log, L"Error");
        i = m_configurator->m_testBackend->GetLastLogItem();
        CPPUNIT_ASSERT(i.GetMessage() == L"Error");
        CPPUNIT_ASSERT(i.GetSeverity() == eError);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogHandleTest );

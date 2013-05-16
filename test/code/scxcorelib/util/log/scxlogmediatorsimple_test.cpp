/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Contains tests of the simple log mediator.

    \date        2008-07-18 11:48:53

*/
/*----------------------------------------------------------------------------*/

#include "scxcorelib/util/log/scxlogmediatorsimple.h"
#include <scxcorelib/scxlogitem.h>
#include "scxcorelib/util/log/scxlogbackend.h"
#include <testutils/scxunit.h>

#include <scxcorelib/testlogbackend.h>

class SCXLogMediatorSimpleTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogMediatorSimpleTest );
    CPPUNIT_TEST( TestNoConsumers );
    CPPUNIT_TEST( TestRegisterConsumer );
    CPPUNIT_TEST( TestUnregisterConsumer );
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

    void TestNoConsumers()
    {
        SCXCoreLib::SCXLogMediatorSimple mediator;
        CPPUNIT_ASSERT_NO_THROW(mediator.LogThisItem(SCXCoreLib::SCXLogItem(L"scxcore.something",
                                                                            SCXCoreLib::eHysterical,
                                                                            L"cowabunga",
                                                                            SCXSRCLOCATION,
                                                                            0)));
    }

    void TestRegisterConsumer(void)
    {
        SCXCoreLib::SCXLogMediatorSimple mediator;
        SCXCoreLib::SCXHandle<TestLogBackend> b1( new TestLogBackend() );
        SCXCoreLib::SCXHandle<TestLogBackend> b2( new TestLogBackend() );
        b1->SetSeverityThreshold(L"", SCXCoreLib::eWarning);
        b2->SetSeverityThreshold(L"", SCXCoreLib::eWarning);
        CPPUNIT_ASSERT( true == mediator.RegisterConsumer(b1) );
        CPPUNIT_ASSERT( true == mediator.RegisterConsumer(b2) );

        SCXCoreLib::SCXLogItem l(L"scxcore.something",
                                 SCXCoreLib::eWarning,
                                 L"TestRegisterConsumer",
                                 SCXSRCLOCATION,
                                 0);
        mediator.LogThisItem(l);
        
        CPPUNIT_ASSERT(b1->GetLastLogItem().GetMessage() == L"TestRegisterConsumer");
        CPPUNIT_ASSERT(b2->GetLastLogItem().GetMessage() == L"TestRegisterConsumer");
    }

    void TestUnregisterConsumer(void)
    {
        SCXCoreLib::SCXLogMediatorSimple mediator;
        SCXCoreLib::SCXHandle<TestLogBackend> b1( new TestLogBackend() );
        SCXCoreLib::SCXHandle<TestLogBackend> b2( new TestLogBackend() );
        b1->SetSeverityThreshold(L"", SCXCoreLib::eWarning);
        b2->SetSeverityThreshold(L"", SCXCoreLib::eWarning);
        CPPUNIT_ASSERT( true == mediator.RegisterConsumer(b1) );
        CPPUNIT_ASSERT( false == mediator.DeRegisterConsumer(b2) );
        CPPUNIT_ASSERT( true == mediator.RegisterConsumer(b2) );

        SCXCoreLib::SCXLogItem l(L"scxcore.something",
                                 SCXCoreLib::eWarning,
                                 L"TestUnregisterConsumer",
                                 SCXSRCLOCATION,
                                 0);
        mediator.LogThisItem(l);
        
        CPPUNIT_ASSERT(b1->GetLastLogItem().GetMessage() == L"TestUnregisterConsumer");
        CPPUNIT_ASSERT(b2->GetLastLogItem().GetMessage() == L"TestUnregisterConsumer");
        
        CPPUNIT_ASSERT( true == mediator.DeRegisterConsumer(b1) );
        SCXCoreLib::SCXLogItem l2(L"scxcore.something",
                                  SCXCoreLib::eWarning,
                                  L"TestUnregisterConsumer2",
                                  SCXSRCLOCATION,
                                  0);
        mediator.LogThisItem(l2);
        CPPUNIT_ASSERT(b1->GetLastLogItem().GetMessage() == L"TestUnregisterConsumer");
        CPPUNIT_ASSERT(b2->GetLastLogItem().GetMessage() == L"TestUnregisterConsumer2");
    }

    void TestGetEffectiveSeverity(void)
    {
        SCXCoreLib::SCXLogMediatorSimple mediator;
        SCXCoreLib::SCXHandle<TestLogBackend> b1( new TestLogBackend() );
        SCXCoreLib::SCXHandle<TestLogBackend> b2( new TestLogBackend() );

        b1->SetSeverityThreshold(L"", SCXCoreLib::eWarning);
        b2->SetSeverityThreshold(L"", SCXCoreLib::eTrace);
        CPPUNIT_ASSERT( true == mediator.RegisterConsumer(b1) );
        CPPUNIT_ASSERT( true == mediator.RegisterConsumer(b2) );

        CPPUNIT_ASSERT( SCXCoreLib::eTrace == mediator.GetEffectiveSeverity(L"doesnt.matter") );
    }
    
    void TestThreadSafe()
    {
        SCXCoreLib::SCXThreadLockHandle lockH = SCXCoreLib::ThreadLockHandleGet();
        SCXCoreLib::SCXLogMediatorSimple mediator(lockH);
        
        SCXCoreLib::SCXThreadLock lock(lockH);

        SCXCoreLib::SCXLogItem l(L"scxcore.something",
                                 SCXCoreLib::eWarning,
                                 L"something",
                                 SCXSRCLOCATION,
                                 0);
        SCXCoreLib::SCXHandle<TestLogBackend> b;

        CPPUNIT_ASSERT_THROW(mediator.LogThisItem(l), SCXCoreLib::SCXThreadLockHeldException);
        CPPUNIT_ASSERT_THROW(mediator.GetEffectiveSeverity(L"scxcore.something"), SCXCoreLib::SCXThreadLockHeldException);
        CPPUNIT_ASSERT_THROW(mediator.RegisterConsumer(b), SCXCoreLib::SCXThreadLockHeldException);
        CPPUNIT_ASSERT_THROW(mediator.DeRegisterConsumer(b), SCXCoreLib::SCXThreadLockHeldException);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogMediatorSimpleTest );

/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-12-18 16:55:00

    SCXCondition test class.
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxsignal.h>

#include <testutils/scxunit.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <unistd.h>


using namespace std;
using namespace SCXCoreLib;

const int MILLISECONDS_PER_SECOND = 1000;

class SCXSignalTest : public SCXCoreLib::SCXSignal
{
public:
    SCXSignalTest(u_short sentinel, int sig=SIGRTMIN)
        : SCXSignal(sentinel, sig)
    {
        sm_cHandler1Signals = sm_cHandler2Signals = 0;
    }

    int GetSignalNumber() { return m_sigNumber; }
    u_short GetMagicSentinel() { return m_magic; }
    bool isHandlerAllocated(u_short payload) { return (m_hndlrFunctions.count(payload) > 0); }

    static void Handler1(siginfo_t *si)
    {
        (void) si;
        sm_cHandler1Signals++;
    }

    static void Handler2(siginfo_t *si)
    {
        (void) si;
        sm_cHandler2Signals++;
    }

    static int sm_cHandler1Signals;
    static int sm_cHandler2Signals;
};


int SCXSignalTest::sm_cHandler1Signals = 0;
int SCXSignalTest::sm_cHandler2Signals = 0;

int cDispatchedSignals = 0;
SCXSignalTest *pDispatchSignalTest = NULL;

extern "C"
{
    static void SignalDispatch(int sig, siginfo_t *si, void *ucontext)
    {
        cDispatchedSignals++;
        pDispatchSignalTest->Dispatcher(sig, si, ucontext);
    }
}



class SCXSignalTestMethods : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXSignalTestMethods );
    CPPUNIT_TEST( TestConstructor );
    CPPUNIT_TEST( TestConstructorWithAlternateSignal );
    CPPUNIT_TEST( TestAssignHandler );
    CPPUNIT_TEST( TestAcceptSignalsWithoutSignal );
    CPPUNIT_TEST( TestSignal );
    CPPUNIT_TEST( TestSignalDelivery );

    SCXUNIT_TEST_ATTRIBUTE( TestSignal, SLOW);
    SCXUNIT_TEST_ATTRIBUTE( TestSignalDelivery, SLOW);
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
        cDispatchedSignals = 0;
        pDispatchSignalTest = NULL;
    }

    void tearDown(void)
    {
    }

    void TestConstructor(void)
    {
        SCXSignalTest s(0xCF);

        // Verify proper member variables are set
        CPPUNIT_ASSERT_EQUAL( SIGRTMIN, s.GetSignalNumber() );
        CPPUNIT_ASSERT_EQUAL( static_cast<u_short> (0xCF), s.GetMagicSentinel() );
    }

    void TestConstructorWithAlternateSignal(void)
    {
        SCXSignalTest s(0x7AFA, SIGRTMAX);

        // Verify proper member variables are set
        CPPUNIT_ASSERT_EQUAL( SIGRTMAX, s.GetSignalNumber() );
        CPPUNIT_ASSERT_EQUAL( static_cast<u_short> (0x7AFA), s.GetMagicSentinel() );
    }

    void TestAssignHandler(void)
    {
        SCXSignalTest s(0xCF);

        // Unassigned entry should not be in the map
        CPPUNIT_ASSERT_EQUAL( false, s.isHandlerAllocated(1) );

        // Assign a handler for signal SIGRTMIN and verify that it is assigned
        s.AssignHandler( 1, &SCXSignalTest::Handler1 );
        CPPUNIT_ASSERT_EQUAL( true, s.isHandlerAllocated(1) );

        // Verify that if we assign a handler again, we get exception
        CPPUNIT_ASSERT_THROW(
            s.AssignHandler( 1, &SCXSignalTest::Handler1 ),
            SCXCoreLib::SCXInvalidArgumentException );

        SCXUNIT_ASSERTIONS_FAILED(1);
    }

    void TestAcceptSignalsWithoutSignal()
    {
        SCXSignalTest s(0x1);

        s.AcceptSignals( SignalDispatch );
    }

    void TestSignal()
    {
        SCXSignalTest s(0x1);

        pDispatchSignalTest = &s;
        s.AcceptSignals( SignalDispatch );
        s.SendSignal( getpid(), 1 );
 
        // Wait for the signal to be delivered, verify that we got it
        // We can't use pause() here, as the signal is already delivered
        // Thus, we sleep for 1/10th of a second; that should be plenty

        usleep(100 * MILLISECONDS_PER_SECOND);
        CPPUNIT_ASSERT_EQUAL( 1, cDispatchedSignals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler1Signals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler2Signals );
    }

    void TestSignalDelivery()
    {
        SCXSignalTest s(0x2);

        // Tell our test dispatcher what our object is
        pDispatchSignalTest = &s;

        // Assign handlers and assign our test dispatcher
        s.AssignHandler( 1, &SCXSignalTest::Handler1 );
        s.AssignHandler( 2, &SCXSignalTest::Handler2 );
        s.AcceptSignals( SignalDispatch );

        // No signals mean no counters set
        CPPUNIT_ASSERT_EQUAL( 0, cDispatchedSignals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler1Signals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler2Signals );

        // Deliver a signal with a payload that we don't deal with
        s.SendSignal( getpid(), 3 );
        usleep(100 * MILLISECONDS_PER_SECOND);
        CPPUNIT_ASSERT_EQUAL( 1, cDispatchedSignals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler1Signals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler2Signals );

        // Deliver a signal with a payload #1
        s.SendSignal( getpid(), 1 );
        usleep(100 * MILLISECONDS_PER_SECOND);
        CPPUNIT_ASSERT_EQUAL( 2, cDispatchedSignals );
        CPPUNIT_ASSERT_EQUAL( 1, SCXSignalTest::sm_cHandler1Signals );
        CPPUNIT_ASSERT_EQUAL( 0, SCXSignalTest::sm_cHandler2Signals );
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXSignalTestMethods );

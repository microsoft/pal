/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-12-18 16:55:00

    SCXCondition test class.
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>
#include <testutils/scxunit.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <iostream>
#include <stdlib.h>

#if defined(sun) || defined(linux)
    #include <time.h>
#elif defined(macos)
    #include <sys/time.h>
#endif

#if defined(WIN32)
    #include <Windows.h>
#endif

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif

static SCXCoreLib::SCXCondition BroadcastCondition;

using namespace std;
using namespace SCXCoreLib;

class SCXWorker : public SCXCoreLib::SCXCondition
{
public:
    SCXWorker()
        : SCXCoreLib::SCXCondition(),
          m_fShutdown(false) {}
    void SetPredicate() { m_fShutdown = true; }
    bool GetPredicate() { return m_fShutdown; }

public:
    void BeginCondition() { SCXCoreLib::SCXCondition::BeginCondition(); }
    void EndCondition()   { SCXCoreLib::SCXCondition::EndCondition(); }
    enum eConditionResult Wait() { return SCXCoreLib::SCXCondition::Wait(); }
    void Signal() { SCXCoreLib::SCXCondition::Signal(); }

private:
    bool m_fShutdown;
};

class SCXWorkerThreadParam : public SCXThreadParam
{
public:
    SCXWorkerThreadParam() : m_signalCount(0), m_bSignal(false) {}
    int m_signalCount;
    bool m_bSignal;
};

class SCXConditionTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXConditionTest );
    CPPUNIT_TEST( TestUnusedCondition );
    CPPUNIT_TEST( TestSuccessState );
    CPPUNIT_TEST( TestElapsedSleep );
    CPPUNIT_TEST( TestCondition );
    CPPUNIT_TEST( TestResetSleepTime );
    CPPUNIT_TEST( TestUselessSignal );
    CPPUNIT_TEST( TestSignal );
    CPPUNIT_TEST( TestSignalNoSleep );
    CPPUNIT_TEST( TestBroadcast );

    SCXUNIT_TEST_ATTRIBUTE( TestCondition, SLOW);
    CPPUNIT_TEST_SUITE_END();

private:

#if defined(sun) || defined(linux) || defined(hpux) || defined(aix) || defined(macos)

    scxulong GetMSDiff(const struct timespec& tp1, const struct timespec& tp2)
    {
        if (m_fDebugOutput)
        {
            cerr << "GetMSDiff - tp2.tv_sec  = " << tp2.tv_sec 
                 << " - tp1.tv_sec  = " << tp1.tv_sec << endl
                 << "GetMSDiff - tp2.tv_nsec = " << tp2.tv_nsec
                 << " - tp1.tv_nsec = " << tp1.tv_nsec << endl;
        }

        return (tp2.tv_sec - tp1.tv_sec) * 1000 + tp2.tv_nsec / 1000000 - tp1.tv_nsec / 1000000;
    }

#endif

    static const int m_fDebugOutput = false;
    static const int m_SleepTime = 400;
    static const int m_LoopCount = 10;
    static const int m_MaxDiff = 100;

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

#if defined(SCX_UNIX)
    void GetUnixTime(struct timespec *ts)
    {
#if defined(sun) || defined(linux) || defined(hpux) || defined(aix)
        if (0 != clock_gettime(CLOCK_REALTIME, ts))
        {
            CPPUNIT_ASSERT( ! "clock_gettime() failed" );
        }
#elif defined(macos)
        struct timeval  tv;

        if (0 != gettimeofday(&tv, NULL))
        {
            CPPUNIT_ASSERT( ! "gettimeofday() failed" );
        }
        TIMEVAL_TO_TIMESPEC(&tv, ts);
#else
#error "Plattform not implemented"
#endif
    }
#endif /* SCX_UNIX */

    void TestUnusedCondition(void)
    {
        SCXWorker c;
        c.SetSleep(1);
    }

    void TestSuccessState(void)
    {
        SCXWorker c;
        c.SetSleep(1);

        SCXConditionHandle h(c);
        CPPUNIT_ASSERT_NO_THROW(CPPUNIT_ASSERT(SCXCondition::eCondTimeout == h.Wait()));
    }

    void TestElapsedSleep(void)
    {
        SCXWorker c;
        c.SetSleep(1);

        // Be sure we work when our sleep has passed immediately

        SCXConditionHandle h(c);
        CPPUNIT_ASSERT(SCXCondition::eCondTimeout == h.Wait());
        SCXThread::Sleep(100);
        CPPUNIT_ASSERT(SCXCondition::eCondTimeout == h.Wait());
    }

    void TestResetSleepTime(void)
    {
        SCXWorker c;

        unsigned long SleepTimes[] = {500, 750, 100, 250};
        scxulong runtime;

        for (unsigned long i = 0; i < sizeof(SleepTimes)/sizeof(unsigned long); i++)
        {
            c.SetSleep(SleepTimes[i]);

            // On one of the (shorter) tests, run several waits without calling SetSleep
            int limit = (SleepTimes[i] == 100 ? 3 : 1);

            for (int j = 0; j < limit; j++)
            {
                // Get the current time
#if defined(SCX_UNIX)
                struct timespec tp1;
                GetUnixTime(&tp1);
#elif defined(WIN32)
                scxulong t1 = GetTickCount();
#else
#error "Platform not implemented"
#endif

                // Wait for the timeout
                enum SCXCondition::eConditionResult r;
                {
                    SCXConditionHandle h(c);
                    while (SCXCondition::eCondTimeout != (r = h.Wait())) ;
                }
                CPPUNIT_ASSERT( !c.GetPredicate() );
                CPPUNIT_ASSERT( SCXCondition::eCondTimeout == r );
    
                // Calculate the elapsed time
#if defined(SCX_UNIX)
                struct timespec tp2;
                GetUnixTime(&tp2);
                runtime = GetMSDiff(tp1, tp2);
#elif defined(WIN32)
                scxulong t2 = GetTickCount();

                runtime = t2 - t1;
#else
#error "Platform not implemented"
#endif

                // Check that the time elapsed was within tolerance
                std::stringstream message;
                message << "Exceeded runtime tolerance, lowRange (" << SleepTimes[i] - m_MaxDiff
                        << ") <= runtime (" << runtime
                        << ") <= highRange (" << SleepTimes[i] + m_MaxDiff
                        << "). This is a timing based test so if it is off by a small amount it can be safely ignored.";
                SCXUNIT_ASSERT_BETWEEN_MESSAGE(message.str(), runtime, SleepTimes[i] - m_MaxDiff, SleepTimes[i] + m_MaxDiff);
            }
        }
    }

    void TestCondition(void)
    {
        // Test that, in the no "signal" case, we sleep properly as directed

        SCXWorker c;
        scxlong runtime = 65536; // A sufficiently large difference to trigger failure
        bool passed = false;
        int count = 0;
        static int worktimes[m_LoopCount] = { 
            0,
            m_SleepTime/10, 
            2*m_SleepTime/10, 
            3*m_SleepTime/10, 
            4*m_SleepTime/10, 
            5*m_SleepTime/10,
            6*m_SleepTime/10, 
            7*m_SleepTime/10, 
            8*m_SleepTime/10, 
            9*m_SleepTime/10, 
        };

#if defined(SCX_UNIX)
        struct timespec tp1;
        GetUnixTime(&tp1);
#elif defined(WIN32)
        scxulong t1 = GetTickCount();
#else
#error "Platform not implemented"
#endif

        c.SetSleep(m_SleepTime);
        while (count < 10 && !passed)
        {
            count++;

            if (m_fDebugOutput)
                cerr << "Count = " << count << endl;

            for (int i=0; i<m_LoopCount; i++)
            {
                if (m_fDebugOutput)
                    cerr << "Do work  = " << worktimes[i] << endl;

                SCXThread::Sleep(worktimes[i]);

                enum SCXCondition::eConditionResult r;
                {
                    SCXConditionHandle h(c);
                    while (SCXCondition::eCondTimeout != (r = h.Wait())) ;
                }
                CPPUNIT_ASSERT( !c.GetPredicate() );
                CPPUNIT_ASSERT( SCXCondition::eCondTimeout == r );
            }

#if defined(SCX_UNIX)
            struct timespec tp2;
            GetUnixTime(&tp2);
            runtime = GetMSDiff(tp1, tp2);
#elif defined(WIN32)
            scxulong t2 = GetTickCount();

            if (m_fDebugOutput)
                cerr << "t1 = " << t1 << ", t2 = " << t2 << endl;

            runtime = t2 - t1;
#else
#error "Platform not implemented"
#endif

            if (m_fDebugOutput)
                cerr << "Runtime = " << runtime << endl;

            // Check that the total loop time is within the accepted range
            if (runtime < (m_SleepTime * m_LoopCount) * count + m_MaxDiff &&
                runtime > (m_SleepTime * m_LoopCount) * count - m_MaxDiff)
            {
                passed = true;
            }
            else
            {
                cerr << runtime << ", "
                     << m_SleepTime * m_LoopCount * count << ", "
                     << m_MaxDiff << endl;
            }
        }

        CPPUNIT_ASSERT_MESSAGE("Total loop time not within expacted margin", passed);
    }

    void TestUselessSignal(void)
    {
        // Verify that a "useless" signal (a signal with nobody waiting) is fine

        SCXWorker a;
        a.SetSleep(m_SleepTime);
        CPPUNIT_ASSERT(false == a.GetPredicate());

        {
            SCXConditionHandle h(a);
            h.Signal();
        }

        // Signal, in and of itself, does nothing with the predicate
        CPPUNIT_ASSERT(false == a.GetPredicate());
    }

    static void TestSignalThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXWorkerThreadParam* p = dynamic_cast<SCXWorkerThreadParam*>(param.GetData());
        CPPUNIT_ASSERT(NULL != p);

        p->m_cond.SetSleep(30000);     // 30 seconds to sleep

        SCXConditionHandle h(p->m_cond);
        while ( !p->GetTerminateFlag() )
        {
            enum SCXCondition::eConditionResult r = h.Wait();

            // We should be signaled well before our thread wakes up!
            CPPUNIT_ASSERT(SCXCondition::eCondTimeout != r);
        }
    }

    void TestSignal(void)
    {
        // Object "params" is owned by thread object and is deleted with thread object
        SCXWorkerThreadParam* params = new SCXWorkerThreadParam;
        SCXThread *thread = new SCXCoreLib::SCXThread(TestSignalThreadBody, params);
        scxlong runtime = 65536; // A sufficiently large difference to trigger failure

        CPPUNIT_ASSERT( NULL != params );

        // Give us a bit of time to start up
        SCXCoreLib::SCXThread::Sleep(500);

#if defined(SCX_UNIX)
        struct timespec tp1;
        GetUnixTime(&tp1);
#elif defined(WIN32)
        scxulong t1 = GetTickCount();
#else
#error "Platform not implemented"
#endif

        // Request orderly shutdown of worker thread (terminating the sleep)

        CPPUNIT_ASSERT(false == params->GetTerminateFlag());
        thread->RequestTerminate();
        CPPUNIT_ASSERT(true == params->GetTerminateFlag());
        thread->Wait();

#if defined(SCX_UNIX)
        struct timespec tp2;
        GetUnixTime(&tp2);
        runtime = GetMSDiff(tp1, tp2);
#elif defined(WIN32)
        scxulong t2 = GetTickCount();

        if (m_fDebugOutput)
            cerr << "t1 = " << t1 << ", t2 = " << t2 << endl;

        runtime = t2 - t1;
#else
#error "Platform not implemented"
#endif

        // Verify that orderly shutdown of worker thread happened quickly
        // (Verify that thread->RequestTerminate() interrupted the Sleep())
        std::stringstream message;
        message << "Runtime too long. Runtime = " << runtime << " MaxDiff = " << m_MaxDiff
                << ". This is a timing based test so if it is off by a small amount it can be safely ignored.";
        CPPUNIT_ASSERT_MESSAGE(message.str(), runtime <= m_MaxDiff);

        // Do NOT delete params - this is handled by the thread object
        delete thread;
    }

    static void TestSignalNoSleepThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXWorkerThreadParam* p = dynamic_cast<SCXWorkerThreadParam*>(param.GetData());
        CPPUNIT_ASSERT(NULL != p);

        p->m_cond.SetSleep(0); // Indefinite sleep

        SCXConditionHandle h(p->m_cond);
        while ( !p->GetTerminateFlag() )
        {
            while ( !p->m_bSignal)
            {
                enum SCXCondition::eConditionResult r = h.Wait();

                // We should never wake up in a timeout state!
                CPPUNIT_ASSERT(SCXCondition::eCondTimeout != r);
            }

            p->m_bSignal = false;
            p->m_signalCount++;
        }
    }

    void TestSignalNoSleep(void)
    {
        // Object "params" is owned by thread object and is deleted with thread object
        SCXWorkerThreadParam* params = new SCXWorkerThreadParam;
        SCXThread *thread = new SCXCoreLib::SCXThread(TestSignalNoSleepThreadBody, params);
        CPPUNIT_ASSERT( NULL != params );

        // Give us a bit of time to start up
        SCXCoreLib::SCXThread::Sleep(500);

        // Deliver some signals and insure that the signals have been delivered
        // (and only those signals get delivered)
        for (int i = 1; i <= 10; i++)
        {
            {
                SCXConditionHandle h(params->m_cond);
                params->m_bSignal = true;
                h.Signal();
            }

            // Loop until other thread sets m_bSignal to false or until
            // we have looped 40 times (2 seconds).
            for (int j = 0; j < 40; j++)
            {
                SCXCoreLib::SCXThread::Sleep(m_MaxDiff);
                SCXConditionHandle h(params->m_cond);
                
                // Break if other thread set m_bSignal to false.
                if (false == params->m_bSignal)
                    break;
            }
            
            SCXConditionHandle h(params->m_cond);
            CPPUNIT_ASSERT(false == params->m_bSignal);
            CPPUNIT_ASSERT(i == params->m_signalCount);
        }

        // Request orderly shutdown of worker thread (terminating the sleep)

        CPPUNIT_ASSERT(false == params->GetTerminateFlag());
        {
            SCXConditionHandle h(params->m_cond);
            params->m_bSignal = true;
        }
        thread->RequestTerminate();
        CPPUNIT_ASSERT(true == params->GetTerminateFlag());
        thread->Wait();

        // Do NOT delete params - this is handled by the thread object
        delete thread;
    }

    static void TestSignalBroadcastThread(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXWorkerThreadParam* p = dynamic_cast<SCXWorkerThreadParam*>(param.GetData());
        CPPUNIT_ASSERT(NULL != p);

        BroadcastCondition.SetSleep(0); // Indefinite sleep

        SCXConditionHandle h(BroadcastCondition);

        enum SCXCondition::eConditionResult r = SCXCondition::eCondNone;
        while (r != SCXCondition::eCondTestPredicate)
        {
            r = h.Wait();

            CPPUNIT_ASSERT(SCXCondition::eCondTimeout != r);
        }

        p->m_signalCount++;
    }

    void TestBroadcast(void)
    {
        // Object "params" is owned by thread object and is deleted with thread object
        SCXWorkerThreadParam* params1 = new SCXWorkerThreadParam;
        SCXWorkerThreadParam* params2 = new SCXWorkerThreadParam;
        SCXWorkerThreadParam* params3 = new SCXWorkerThreadParam;
        SCXWorkerThreadParam* params4 = new SCXWorkerThreadParam;

        SCXThread *thread1 = new SCXCoreLib::SCXThread(TestSignalBroadcastThread, params1);
        SCXThread *thread2 = new SCXCoreLib::SCXThread(TestSignalBroadcastThread, params2);
        SCXThread *thread3 = new SCXCoreLib::SCXThread(TestSignalBroadcastThread, params3);
        SCXThread *thread4 = new SCXCoreLib::SCXThread(TestSignalBroadcastThread, params4);

        CPPUNIT_ASSERT( NULL != params1 );
        CPPUNIT_ASSERT( NULL != params2 );
        CPPUNIT_ASSERT( NULL != params3 );
        CPPUNIT_ASSERT( NULL != params4 );

        // Give us a bit of time to send the broadbast
        SCXCoreLib::SCXThread::Sleep(500);

#if defined(SCX_UNIX)
        struct timespec tp1;
        GetUnixTime(&tp1);
#elif defined(WIN32)
        scxulong t1 = GetTickCount();
#else
#error "Platform not implemented"
#endif


        // Broadcast
        {
            SCXConditionHandle h(BroadcastCondition);
            h.Broadcast();
        }


        // Give us a bit of time to start up
        SCXCoreLib::SCXThread::Sleep(1000);

        CPPUNIT_ASSERT(1 == params1->m_signalCount);
        CPPUNIT_ASSERT(1 == params2->m_signalCount);
        CPPUNIT_ASSERT(1 == params3->m_signalCount);
        CPPUNIT_ASSERT(1 == params4->m_signalCount);

        // Do NOT delete params - this is handled by the thread object
        delete thread1;
        delete thread2;
        delete thread3;
        delete thread4;
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXConditionTest );

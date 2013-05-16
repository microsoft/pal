/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2012-11-19 11:00:00

    Test class for SCXThreadPool PAL.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxatomic.h>
#include <scxcorelib/scxthreadpool.h>

#include <testutils/scxunit.h>

using namespace SCXCoreLib;

namespace
{
    scx_atomic_t s_executionCount;

    class TestParam : public SCXThreadParam
    {
    public:
        TestParam(SCXThreadPool *tp)
            : SCXCoreLib::SCXThreadParam(),
              m_pThreadPool(tp)
        {
        }

        virtual ~TestParam() { }

        SCXThreadPool* GetThreadPool() { return m_pThreadPool; }

    private:
        SCXThreadPool* m_pThreadPool;
    };

    // This exists solely to pick up protected members for test
    class TestableThreadAttr : public SCXThreadAttr
    {
    public:
        size_t GetDefaultStackSize() const { return c_defaultStackSize; }
    };

    // Dependency class to delay worker thread task execution
    // (eliminates tight timing constraints in unit test)

    class TestableThreadPool : public SCXThreadPool
    {
    public:
        TestableThreadPool(SCXHandle<SCXThreadPoolDependencies> deps)
            : SCXThreadPool(deps)
        { }

        SCXCondition* GetCondition() { return &m_cond; }
    };

    class TestThreadPoolDependencies : public SCXThreadPoolDependencies
    {
    private:
        bool m_fDelayedExecution;

    public:
        TestThreadPoolDependencies()
            : SCXThreadPoolDependencies(),
              m_fDelayedExecution(false)
        { }

        virtual bool IsWorkerTaskExecutionDelayed() { return m_fDelayedExecution; }

        void BeginWorkerTaskExecutionDelay() { m_fDelayedExecution = true; }
        void EndWorkerTaskExecutionDelay(TestableThreadPool* tp)
        {
            m_fDelayedExecution = false;

            SCXConditionHandle h( *(tp->GetCondition()) );
            h.Broadcast();
        }
    };
}

class SCXThreadPoolTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXThreadPoolTest );
    CPPUNIT_TEST( TestConstruction );
    CPPUNIT_TEST( TestThreadCountGet );
    CPPUNIT_TEST( TestThreadLimitGetSet );
    CPPUNIT_TEST( TestDumpString );
    CPPUNIT_TEST( TestWorkerThreadStart );
    CPPUNIT_TEST( TestWorkerThreadShutdown );
    CPPUNIT_TEST( TestWorkerThreadQueueItem );
    CPPUNIT_TEST( TestWorkerThreadThrottleUp );
    CPPUNIT_TEST( TestWorkerThreadThrottleDown );
    CPPUNIT_TEST( TestLockRetention );
    CPPUNIT_TEST( TestWorkerStackSize );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void VerifyPoolIsRunning(SCXThreadPool& tp, long threadCount)
    {
        // Wait for a bit for the thread pool to start up
        int count = 0;
        while ( !tp.isRunning() && ++count <= 20 )
            usleep( 50000 );

        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Worker pool is not running!", static_cast<bool> (true), tp.isRunning() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Should have exactly one worker thread running", threadCount, tp.GetThreadCount() );
    }

    void TestConstruction()
    {
        SCXThreadPool tp;
    }

    void TestThreadCountGet()
    {
        SCXThreadPool tp;
        CPPUNIT_ASSERT_EQUAL( static_cast<long>(0), tp.GetThreadCount() );
    }

    void TestThreadLimitGetSet()
    {
        SCXThreadPool tp;
        CPPUNIT_ASSERT_EQUAL( static_cast<long>(8), tp.GetThreadLimit() );         // Default thread limit

        // Override thread limit and verify that it takes
        tp.SetThreadLimit( 12 );
        CPPUNIT_ASSERT_EQUAL( static_cast<long>(12), tp.GetThreadLimit() );
    }

    void TestDumpString()
    {
        SCXThreadPool tp;
        std::wstring dumpString = tp.DumpString();

        std::wcout << dumpString;
    }

    void TestWorkerThreadStart()
    {
        SCXThreadPool tp;

        // Upon startup, we should be in a running state, and we should have
        // one running thread (Can grow as work is queued).

        CPPUNIT_ASSERT_EQUAL( static_cast<long>(0), tp.GetThreadCount() );
        tp.Start();
        VerifyPoolIsRunning( tp, 1 );
    }

    void TestWorkerThreadShutdown()
    {
        SCXThreadPool tp;
        tp.Start();
        VerifyPoolIsRunning( tp, 1 );

        // Now shut down and verify that we're really shut down
        tp.Shutdown();
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Worker pool is still running!", static_cast<bool> (false), tp.isRunning() );
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Threads are still running in pool!", static_cast<long>(0), tp.GetThreadCount() );
    }

    // Queue a work item, make sure it runs
    //   QueueItemWork: Worker thread procedure
    //   QueueItem: Actual test function that runs
    static void TestWorkerThreadQueueItemWorker(SCXCoreLib::SCXThreadParamHandle& handle)
    {
        TestParam* p = static_cast<TestParam *>(handle.GetData());
        CPPUNIT_ASSERT( NULL != p );

        scx_atomic_increment( &s_executionCount );
    }

    void TestWorkerThreadQueueItem()
    {
        SCXThreadPool tp;
        tp.Start();
        VerifyPoolIsRunning( tp, 1 );

        // Queue a work item, insure that it runs
        s_executionCount = 0;

        SCXThreadParamHandle testParamHandle (new TestParam(&tp));
        SCXThreadPoolTaskHandle task ( new SCXThreadPoolTask(&TestWorkerThreadQueueItemWorker, testParamHandle) );
        tp.QueueTask(task);

        // Wait for a bit for the worker thread to be processed
        int count = 0;
        while ( !s_executionCount && ++count <= 20 )
            usleep( 50000 );

        std::stringstream stream;
        stream << "Worker thread did not run properly, s_executionCount == " << s_executionCount;
        CPPUNIT_ASSERT_MESSAGE( stream.str(), s_executionCount == 1 );
    }

    // Queue a bunch of work items, make sure they all run, and verify threads are added
    // ThrottleUpThreads is a helper class to be called from elsewhere ...
    void ThrottleUpThreads(TestableThreadPool* tp, SCXHandle<TestThreadPoolDependencies> deps)
    {
        const long ThreadsToRun = 8;
        const long ItemsToQueue = 128;

        // Test function - delay worker task execution
        deps->BeginWorkerTaskExecutionDelay();

        tp->SetThreadLimit( ThreadsToRun );
        tp->Start();
        VerifyPoolIsRunning( *tp, 1 );

        // Initialize the counter
        s_executionCount = 0;

        // Queue a work item, insure that it runs
        for (int i = 0; i < ItemsToQueue; i++)
        {
            SCXThreadParamHandle testParamHandle (new TestParam(tp));
            SCXThreadPoolTaskHandle task ( new SCXThreadPoolTask(&TestWorkerThreadQueueItemWorker, testParamHandle) );
            tp->QueueTask(task);
        }

        // Test function - delay worker task execution
        deps->EndWorkerTaskExecutionDelay( tp );

        // Wait for a bit for the worker thread to be processed
        //
        // This is getting processed through 8 threads, and there's a race condition here:
        // . We want the thread count small enough so we can get enough items queued
        //   (they'll start running just as they are queued),
        // . The thread count should be big enough where it doesn't take too long to run
        //
        // There was a timing problem here, ultimately made awful on single-CPU systems.
        // Added dependencies class and TestableThreadPool solely to work around these
        // timing problems (sigh) ...
        //
        // See Begin/EndWorkerTaskExecutionDelay methods (primarily test methods only).

        int count = 0;
        while ( s_executionCount != ItemsToQueue && ++count <= 40 )
            usleep( 50000 );

        std::stringstream stream;
        stream << "Worker thread did not run properly, s_executionCount == " << s_executionCount;
        CPPUNIT_ASSERT_MESSAGE( stream.str(), s_executionCount == ItemsToQueue );

        // Everything ran - verify that we did, indeed, throttle up the number of worker threads
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Insufficient worker threads created!", ThreadsToRun, tp->GetThreadCount() );
    }

    void TestWorkerThreadThrottleUp()
    {
        SCXHandle<TestThreadPoolDependencies> deps(new TestThreadPoolDependencies());
        TestableThreadPool tp(deps);
        ThrottleUpThreads( &tp, deps );
    }

    // Throttle down should reduce number of threads if, when running, thread count is adjusted
    // Verify that actually happens.  Throttle up a bunch of threads, then reduce and verify that
    // the reduction actually happened.

    void TestWorkerThreadThrottleDown()
    {
        SCXHandle<TestThreadPoolDependencies> deps(new TestThreadPoolDependencies());
        TestableThreadPool tp(deps);
        ThrottleUpThreads( &tp, deps );

        long newThreadCount = tp.GetThreadLimit() / 2;
        CPPUNIT_ASSERT_MESSAGE( "(ThreadLimit / 2) is not > 0!",
                                ( newThreadCount < tp.GetThreadLimit() && newThreadCount > 0 ));

        tp.SetThreadLimit( newThreadCount );

        // Wait for a bit for the worker threads to be reduced
        int count = 0;
        while ( (tp.GetThreadCount() != newThreadCount)  && ++count <= 20 )
            usleep( 50000 );

        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Thread count does not appear to have throttled down", newThreadCount, tp.GetThreadCount() );
    }

    // Verify that lock is released when calling thread function
    // (Do this via a nested call into the thread pool)

    static void TestLockRetentionWorker(SCXCoreLib::SCXThreadParamHandle& handle)
    {
        TestParam* p = static_cast<TestParam *>(handle.GetData());
        CPPUNIT_ASSERT( NULL != p );
        SCXThreadPool *pPool = p->GetThreadPool();

        SCXThreadParamHandle testParamHandle (new TestParam(pPool));
        SCXThreadPoolTaskHandle task ( new SCXThreadPoolTask(&TestWorkerThreadQueueItemWorker, testParamHandle) );
        pPool->QueueTask(task);
    }

    void TestLockRetention()
    {
        SCXThreadPool tp;
        tp.Start();
        VerifyPoolIsRunning( tp, 1 );

        // Queue a work item, insure that it runs
        s_executionCount = 0;

        SCXThreadParamHandle testParamHandle (new TestParam(&tp));
        SCXThreadPoolTaskHandle task ( new SCXThreadPoolTask(&TestLockRetentionWorker, testParamHandle) );
        tp.QueueTask(task);

        // Wait for a bit for the worker thread to be processed
        int count = 0;
        while ( !s_executionCount && ++count <= 20 )
            usleep( 50000 );

        std::stringstream stream;
        stream << "Worker thread did not run properly, s_executionCount == " << s_executionCount;
        CPPUNIT_ASSERT_MESSAGE( stream.str(), s_executionCount == 1 );
    }

    // Verify that a special stack size for the worker threads actually works ...
    // Just queue a threadfunc that will verify that the stack size is as we expect

    static void TestWorkerStackSizeFunc(SCXCoreLib::SCXThreadParamHandle& handle)
    {
        TestParam* p = static_cast<TestParam *>(handle.GetData());
        CPPUNIT_ASSERT( NULL != p );

        // The following code block will verify that the worker thread attributes (stack size)
        // is honored appropriately.  Note that this test only works on Linux.  Since there is
        // no platform-specific code around the implementation, and since SCXThreadAttr itself
        // has unit tests to verify behavior on all platforms, this test is sufficient here.
#if defined(linux)
        TestableThreadAttr tta;
        const size_t c_stacksize = tta.GetDefaultStackSize() * 2;

        // Let's make sure the stack size is what we set it to be.
        // SCXThreadId is just a typedef of pthread_t on SCX_UNIX.
        pthread_t threadID = pthread_self();
        pthread_attr_t attr;

        CPPUNIT_ASSERT_EQUAL_MESSAGE( "pthread_getattr_np failed", 0,
            pthread_getattr_np(threadID, &attr) );

        size_t actualsize;
                CPPUNIT_ASSERT_EQUAL_MESSAGE( "pthread_attr_getstacksize failed", 0,
            pthread_attr_getstacksize(&attr, &actualsize) );

        // according to pthread_attr_setstacksize's man page, the allocated stack size should be
        // greater than or equal to the requested stack size
        std::stringstream ss;
        ss << "Actual stack size (" << actualsize << ") should be greater than or equal to requested size (" << c_stacksize <<  ")";
        CPPUNIT_ASSERT_MESSAGE(ss.str(), actualsize >= c_stacksize);
#endif

        scx_atomic_increment( &s_executionCount );
    }

    void TestWorkerStackSize()
    {
        // Determine the stack size we're going to use for worker threads
        TestableThreadAttr tta;
        const size_t c_stacksize = tta.GetDefaultStackSize() * 2;

        // Start up the worker threads with appropriate stack size
        SCXThreadPool tp;
        tp.SetWorkerStackSize( c_stacksize );
        tp.Start();
        VerifyPoolIsRunning( tp, 1 );

        // Queue a work item, insure that it runs
        s_executionCount = 0;

        SCXThreadParamHandle testParamHandle (new TestParam(&tp));
        SCXThreadPoolTaskHandle task ( new SCXThreadPoolTask(&TestWorkerStackSizeFunc, testParamHandle) );
        tp.QueueTask(task);

        // Wait for a bit for the worker thread to be processed
        int count = 0;
        while ( !s_executionCount && ++count <= 20 )
            usleep( 50000 );

        std::stringstream stream;
        stream << "Worker thread did not run properly, s_executionCount == " << s_executionCount;
        CPPUNIT_ASSERT_MESSAGE( stream.str(), s_executionCount == 1 );
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXThreadPoolTest );

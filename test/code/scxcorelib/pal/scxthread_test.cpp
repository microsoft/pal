/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2007-06-19 14:00:00

    Test class for SCXThread PAL.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxhandle.h>
#include <testutils/scxunit.h>
#include <scxcorelib/stringaid.h>

#include <scxcorelib/scxexception.h>

#include <time.h>
#include <sstream>

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif

// These constants are used in LargeStackThread
static const size_t cs_bufferSize = (1 << 23); // 2^23 = 8M

// On HP ia64 systems, the stack has approximately half of its allocation reserved for the register stack
// which is separated from the normal stack by a guard page.  Documentation on this can be found here:
// http://software.intel.com/en-us/articles/itaniumr-processor-family-performance-advantages-register-stack-architecture
#if defined(hpux) && defined(ia64)
static const size_t cs_fudgeFactor = (size_t)(cs_bufferSize * 1.1/2.0);
#elif defined(ppc)
static const size_t cs_fudgeFactor = (1 << 17); // 2^17 = 128k
#else
static const size_t cs_fudgeFactor = (1 << 13); // 2^13 = 8k
#endif

class ThreadLockParam : public SCXCoreLib::SCXThreadParam
{
public:
    ThreadLockParam(const SCXCoreLib::SCXThreadLockHandle& lock1, const SCXCoreLib::SCXThreadLockHandle& lock2)
        : m_lock1(lock1), m_lock2(lock2)
    {

    }

    const SCXCoreLib::SCXThreadLockHandle& GetLockHandle1() const { return m_lock1; }
    const SCXCoreLib::SCXThreadLockHandle& GetLockHandle2() const { return m_lock2; }
protected:
    SCXCoreLib::SCXThreadLockHandle m_lock1;
    SCXCoreLib::SCXThreadLockHandle m_lock2;
};

class SCXThreadTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXThreadTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( TestParams );
    CPPUNIT_TEST( TestSleep );
    CPPUNIT_TEST( TestCurrentThreadID );
    CPPUNIT_TEST( TestThreadBasics );
    CPPUNIT_TEST( TestThreadWait );
    CPPUNIT_TEST( TestThreadTerminate );
#if !defined (_DEBUG)
    CPPUNIT_TEST( TestThreadBodyCatchesSCXException );
    CPPUNIT_TEST( TestThreadBodyCatchesSTLException );
#endif
    CPPUNIT_TEST( TestManualStartOK );
    CPPUNIT_TEST( TestStartTwiceFails );
    CPPUNIT_TEST( TestThreadExceptionHaveCorrectThreadID );
    CPPUNIT_TEST( TestNestedDetach );
    CPPUNIT_TEST( TestSetStackSizeError );
    CPPUNIT_TEST( TestThreadStackSize );
#if defined(linux)
    CPPUNIT_TEST( TestThreadStackSize_getstacksize );
#endif
    SCXUNIT_TEST_ATTRIBUTE(TestSleep, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestThreadBasics, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestThreadWait, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestThreadTerminateForced, SLOW);
    CPPUNIT_TEST_SUITE_END();

    SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> GivenARunningThread()
    {
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> thread(new SCXCoreLib::SCXThread(SCXThreadTest::SimpleThreadBodyTerminate));
        time_t later = time(NULL) + 10;
        while ( ! thread->IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread does not seam to be started", thread->IsAlive());
        return thread;
    }
public:
    static void SimpleThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        const ThreadLockParam* pl = dynamic_cast<const ThreadLockParam*>(param.GetData());

        SCXCoreLib::SCXThreadLock l1(pl->GetLockHandle1());
        SCXCoreLib::SCXThreadLock l2(pl->GetLockHandle2());
        while ( ! param->GetTerminateFlag())
            SCXCoreLib::SCXThread::Sleep(10);
    }

    static void SimpleThreadBodyWait(SCXCoreLib::SCXThreadParamHandle& )
    {
        SCXCoreLib::SCXThread::Sleep(500);
    }

    static void SimpleThreadBodyTerminate(SCXCoreLib::SCXThreadParamHandle& param)
    {
        while ( ! param->GetTerminateFlag())
            SCXCoreLib::SCXThread::Sleep(10);
    }

    static void SimpleThreadBodyThrowsSCXException(SCXCoreLib::SCXThreadParamHandle& param)
    {
        while ( ! param->GetTerminateFlag())
            SCXCoreLib::SCXThread::Sleep(10);
        throw SCXCoreLib::SCXResourceExhaustedException(L"Testing", L"Testing", SCXSRCLOCATION);
    }

    static void SimpleThreadBodyThrowsSTDException(SCXCoreLib::SCXThreadParamHandle& param)
    {
        while ( ! param->GetTerminateFlag())
            SCXCoreLib::SCXThread::Sleep(10);
        throw std::exception();
    }

    static void SimpleNestedThread(SCXCoreLib::SCXThreadParamHandle& param)
    {
        const ThreadLockParam* pl = dynamic_cast<const ThreadLockParam*>(param.GetData());
        SCXCoreLib::SCXThreadLockHandle waitLockHandle = pl->GetLockHandle1();

        // create a new subthread with a simple body, then immediately detach it and then terminate it.
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadParam> p(new SCXCoreLib::SCXThreadParam());
        SCXCoreLib::SCXThread* nestedThread = new SCXCoreLib::SCXThread(SCXThreadTest::SimpleThreadBodyTerminate, p);
        delete nestedThread;
        p->SetTerminateFlag();

        // notify the caller of this thread that the innermost thread has been detached
        SCXCoreLib::SCXThreadLock waitLock(waitLockHandle);

        while ( !param->GetTerminateFlag() )
            SCXCoreLib::SCXThread::Sleep(10);
    }

    static void LargeStackThread(SCXCoreLib::SCXThreadParamHandle& param)
    {
        // allocate a buffer on the stack that we will write into
        char buf[cs_bufferSize - cs_fudgeFactor];
        
        // if the stack doesn't have this much space, it will segfault due to heap page protection
        memset(buf, 0, cs_bufferSize - cs_fudgeFactor);

        while ( ! param->GetTerminateFlag())
            SCXCoreLib::SCXThread::Sleep(10);
    }

    void callDumpStringForCoverage()
    {
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> thread = GivenARunningThread();
        CPPUNIT_ASSERT(thread->DumpString().find(L"SCXThread") != std::wstring::npos);
        thread->RequestTerminate();
        CPPUNIT_ASSERT(SCXCoreLib::SCXThreadParam().DumpString().find(L"SCXThreadParam") != std::wstring::npos);
    }

    void TestParams(void)
    {
        SCXCoreLib::SCXThreadParam p;
        p.SetString(L"string", L"a string");
        CPPUNIT_ASSERT(p.GetString(L"string") == L"a string");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(p.GetString(L"missing"), SCXCoreLib::SCXInvalidThreadParamValueException, L"missing");

        // test changing the value
        p.SetString(L"string", L"another string");
        CPPUNIT_ASSERT(p.GetString(L"string") == L"another string");
    }

    void TestSleep(void)
    {
        time_t now = time(NULL);
        time_t later = now + 2; // Wait for two seconds to be sure not flipping second before check.
        SCXCoreLib::SCXThread::Sleep(2000);
        now = time(NULL);
        CPPUNIT_ASSERT(later <= now);
    }

    void TestCurrentThreadID(void)
    {
#if defined(WIN32)
        SCXCoreLib::SCXThreadId native = GetCurrentThreadId();
#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
        SCXCoreLib::SCXThreadId native = pthread_self();
#else
#error Not implemented on this platform
#endif
        CPPUNIT_ASSERT(native == SCXCoreLib::SCXThread::GetCurrentThreadID());
    }

    void TestThreadBasics(void)
    {
        SCXCoreLib::SCXThreadLockHandle lh1 = SCXCoreLib::ThreadLockHandleGet();
        SCXCoreLib::SCXThreadLockHandle lh2 = SCXCoreLib::ThreadLockHandleGet();
        SCXCoreLib::SCXThreadParamHandle p(new ThreadLockParam(lh1, lh2));

        SCXCoreLib::SCXThreadLock l1(lh1, false);
        SCXCoreLib::SCXThreadLock l2(lh2);
        SCXCoreLib::SCXThread thread(SCXThreadTest::SimpleThreadBody, p);
        CPPUNIT_ASSERT(p.GetData() == thread.GetThreadParam().GetData()); // make sure thread and out param is the same.
        time_t later = time(NULL) + 10;
        while ( ! l1.IsLocked() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread does not seam to be started", l1.IsLocked());
        CPPUNIT_ASSERT(thread.IsAlive());
        CPPUNIT_ASSERT(thread.GetThreadID() != SCXCoreLib::SCXThread::GetCurrentThreadID());
        l2.Unlock();
        later = time(NULL) + 10;
        thread.RequestTerminate();
        while (l1.IsLocked() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread does not seam to be shutting down", ! l1.IsLocked());
        CPPUNIT_ASSERT_MESSAGE("Lock in thread does not seam to be released",l1.TryLock());
        CPPUNIT_ASSERT_NO_THROW(thread.Wait());
        CPPUNIT_ASSERT( ! thread.IsAlive());
    }

    void TestThreadWait(void)
    {
        SCXCoreLib::SCXThread thread(SCXThreadTest::SimpleThreadBodyWait);
        CPPUNIT_ASSERT_NO_THROW(thread.Wait());
        CPPUNIT_ASSERT( ! thread.IsAlive());
    }

    void TestThreadTerminate(void)
    {
        // Test terminating a thread
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> thread = GivenARunningThread();
        CPPUNIT_ASSERT_NO_THROW(thread->RequestTerminate());
        time_t later = time(NULL) + 10;
        while (thread->IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread did not terminate", ! thread->IsAlive());

        // terminate again
        CPPUNIT_ASSERT_NO_THROW(thread->RequestTerminate());
    }

    void TestThreadBodyCatchesSCXException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        SCXCoreLib::SCXThread thread(SCXThreadTest::SimpleThreadBodyThrowsSCXException);
        time_t later = time(NULL) + 10;
        while ( ! thread.IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread does not seem to be started", thread.IsAlive());
        thread.RequestTerminate();
        later = time(NULL) + 2;
        while (thread.IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread did not terminate", ! thread.IsAlive());
        // If an exception gets through the testrunner will have died by now.
        SCXUNIT_ASSERTIONS_FAILED(1);
    }

    void TestThreadBodyCatchesSTLException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        SCXCoreLib::SCXThread thread(SCXThreadTest::SimpleThreadBodyThrowsSTDException);
        time_t later = time(NULL) + 10;
        while ( ! thread.IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread does not seem to be started", thread.IsAlive());
        thread.RequestTerminate();
        later = time(NULL) + 2;
        while (thread.IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread did not terminate", ! thread.IsAlive());
        // If an exception gets through the testrunner will have died by now.
        SCXUNIT_ASSERTIONS_FAILED(1);
    }

    void TestUniqueThreadOwnership()
    {
        SCXCoreLib::SCXThread thread1(SCXThreadTest::SimpleThreadBodyWait);
        SCXCoreLib::SCXThread thread2(SCXThreadTest::SimpleThreadBodyWait);
        CPPUNIT_ASSERT_THROW(thread1 = thread2, SCXCoreLib::SCXInvalidStateException);
    }

    void TestManualStartOK()
    {
        SCXCoreLib::SCXThread thread;
        thread.Start(SCXThreadTest::SimpleThreadBodyTerminate);
        time_t later = time(NULL) + 10;
        while ( ! thread.IsAlive() && later > time(NULL))
            SCXCoreLib::SCXThread::Sleep(10);
        CPPUNIT_ASSERT_MESSAGE("Thread does not seam to be started", thread.IsAlive());
        thread.RequestTerminate();
        thread.Wait();
        CPPUNIT_ASSERT_MESSAGE("Thread did not terminate", ! thread.IsAlive());
    }

    void TestStartTwiceFails()
    {
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> thread = GivenARunningThread();
        SCXUNIT_ASSERT_THROWN_EXCEPTION(thread->Start(SCXThreadTest::SimpleThreadBodyTerminate), SCXCoreLib::SCXThreadStartException, L"started");
        thread->RequestTerminate();
        thread->Wait();
    }

    void TestThreadExceptionHaveCorrectThreadID()
    {
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> thread = GivenARunningThread();
        try {
            thread->Start(SCXThreadTest::SimpleThreadBodyTerminate);
            CPPUNIT_FAIL("Expected exception not thrown: SCXThreadStartException");
        } catch (SCXCoreLib::SCXThreadStartException& e) {
            CPPUNIT_ASSERT_EQUAL(SCXCoreLib::SCXThread::GetCurrentThreadID(), e.GetThreadID());
        }
    }

    void TestNestedDetach()
    {
        SCXCoreLib::SCXThreadLockHandle waitLockHandle = SCXCoreLib::ThreadLockHandleGet();
        SCXCoreLib::SCXThreadLockHandle dummy = SCXCoreLib::ThreadLockHandleGet();
        SCXCoreLib::SCXThreadParamHandle p(new ThreadLockParam(waitLockHandle, dummy));
        
        // firstThread will create a new thread, then delete that thread, thereby detaching 
        // that thread.  At that point, firstThread should still be joinable
        SCXCoreLib::SCXThread* firstThread = new SCXCoreLib::SCXThread(SCXThreadTest::SimpleNestedThread, p);
        
        // wait until waitLockHandle is locked by SimpleNestedThread.  Once locked,
        // the thread created in SimpleNestedThread must have been detached.
        while (!waitLockHandle.IsLocked())
        {
            SCXCoreLib::SCXThread::Sleep(10);
        }

        // at this point firstThread should still be joinable
        firstThread->RequestTerminate();
        try 
        {
            firstThread->Wait();
        }
        catch (SCXCoreLib::SCXInternalErrorException e)
        {
            CPPUNIT_FAIL("firstThread failed on pthread_join");
        }
        delete firstThread;
    }

    void TestSetStackSizeError()
    {
        const size_t c_goodstacksize = 256000;
        const size_t c_badstacksize = 1;

        SCXCoreLib::SCXThreadAttr threadAttr;
        CPPUNIT_ASSERT_NO_THROW(threadAttr.SetStackSize(c_goodstacksize));
        
        SCXCoreLib::SCXThreadAttr threadAttrError;
        CPPUNIT_ASSERT_THROW_MESSAGE("SCXInternalErrorException exception expected", threadAttrError.SetStackSize(c_badstacksize), SCXCoreLib::SCXInternalErrorException);
        SCXUNIT_ASSERTIONS_FAILED(1);
    }

    void TestThreadStackSize()
    {
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadParam> p(new SCXCoreLib::SCXThreadParam());
        SCXCoreLib::SCXThreadAttr threadAttr;

        // use the cs_bufferSize constant
        CPPUNIT_ASSERT_NO_THROW(threadAttr.SetStackSize(cs_bufferSize));

        SCXCoreLib::SCXThread* thread = new SCXCoreLib::SCXThread(SCXThreadTest::LargeStackThread, p, &threadAttr);

        // clean up the thread
        thread->RequestTerminate();
        thread->Wait();
        delete thread;
    }

// pthread_getattr_np is not supported on AIX, HPUX, and Sun
#if defined(linux)
    void TestThreadStackSize_getstacksize()
    {
        const size_t c_stacksize = 320000;

        // set up a thread with a stack size of 'c_stacksize'
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadParam> p(new SCXCoreLib::SCXThreadParam());
        SCXCoreLib::SCXThreadAttr threadAttr;

        CPPUNIT_ASSERT_NO_THROW(threadAttr.SetStackSize(c_stacksize));

        SCXCoreLib::SCXThread* thread = new SCXCoreLib::SCXThread(SCXThreadTest::SimpleThreadBodyTerminate, p, &threadAttr);

        // Let's make sure the stack size is what we set it to be.
        // SCXThreadId is just a typedef of pthread_t on SCX_UNIX.
        SCXCoreLib::SCXThreadId pt = thread->GetThreadID();
        pthread_attr_t attr;

        int errorval = pthread_getattr_np(pt, &attr);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("pthread_getattr_np failed", 0, errorval);

        size_t actualsize;
        errorval = pthread_attr_getstacksize(&attr, &actualsize);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("pthread_attr_getstacksize failed", 0, errorval);

        // according to pthread_attr_setstacksize's man page, the allocated stack size should be
        // greater than or equal to the requested stack size
        std::stringstream ss;
        ss << "Actual stack size (" << actualsize << ") should be greater than or equal to requested size  (" << c_stacksize <<  ")";
        CPPUNIT_ASSERT_MESSAGE(ss.str(), actualsize >= c_stacksize);

        // clean up the thread
        thread->RequestTerminate();
        thread->Wait();
        delete thread;
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXThreadTest );

/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-05-28 09:20:00

    Thread lock test class.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxthreadlock.h>
#include <testutils/scxunit.h>

#if defined(WIN32)
#include <windows.h>
#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
#include <pthread.h>
#include <signal.h>
#else
#error "Not implemented for this plattform"
#endif

extern "C"
{
    typedef void* (*pThreadFn)(void *);
}

class SCXThreadLockTest : public CPPUNIT_NS::TestFixture
{
public:
    CPPUNIT_TEST_SUITE( SCXThreadLockTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( TestInvalid );
    CPPUNIT_TEST( TestReLock );
    CPPUNIT_TEST( TestGlobalLockNamesCreated );
    CPPUNIT_TEST( TestGlobalLockNamesReused );
    CPPUNIT_TEST( TestGlobalLocksAndUnnamedLocksSeparate );
    CPPUNIT_TEST( TestRecursive );
    CPPUNIT_TEST( TestAnonymousSimple );
    CPPUNIT_TEST( TestNamedSimple );
    CPPUNIT_TEST( TestRefCount );
    CPPUNIT_TEST( TestAssign );
    CPPUNIT_TEST_SUITE_END();

private:
    struct ThreadParam
    {
        std::wstring lockName;
        SCXCoreLib::SCXThreadLockHandle* lockHandle;
        bool threadPaused;
        bool threadComplete;

        ThreadParam() : lockName(L""), lockHandle(NULL), threadPaused(false), threadComplete(false) { }
    };

#if defined(WIN32)
    typedef HANDLE thread_handle_t;
    
    static thread_handle_t StartThread(void* fn, void* param, int* id)
    {
        return CreateThread(NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(fn), param, 0, reinterpret_cast<LPDWORD>(id));
    }

    static void WaitThread(thread_handle_t h, unsigned int timeout = 0)
    {
        time_t to = 0;
        if (timeout > 0)
            to = time(NULL) + timeout;
        while ( ! DoneThread(h) && (0 == to || to > time(NULL)))
            SleepThread(0);;
    }

    static bool DoneThread(thread_handle_t h)
    {
        DWORD ec = STILL_ACTIVE;
        CPPUNIT_ASSERT(GetExitCodeThread(h, &ec));
        return ec != STILL_ACTIVE;
    }

    static void SleepThread(unsigned int milliSeconds)
    {
        Sleep(milliSeconds);
    }

#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
    typedef pthread_t thread_handle_t;

    static thread_handle_t StartThread(void*(*fn)(void*), void* param, int* /*id*/)
    {
        thread_handle_t h;
        int r = pthread_create(&h, NULL, (pThreadFn) fn, param);
        if (0 != r)
            return 0;
        return h;
    }

    static void WaitThread(thread_handle_t h, unsigned int timeout = 0)
    {
        time_t to = 0;
        if (timeout > 0)
            to = time(NULL) + timeout;
        while ( ! DoneThread(h) && (0 == to || to > time(NULL)))
            SleepThread(0);

        if (DoneThread(h))
            pthread_join(h, NULL);
    }

    static bool DoneThread(thread_handle_t h)
    {
        if (0 == pthread_kill(h, 0))
            return false;
        return true;
    }

    static void SleepThread(unsigned int milliSeconds)
    {
        usleep(milliSeconds * 1000);
    }
#else 
#error Not implemented on this platform
#endif
    static void ThreadPause(ThreadParam* p)
    {
        while ( ! p->threadPaused)
        {
            SleepThread(0);
        }
    }

    static void ThreadResume(ThreadParam* p)
    {
        p->threadComplete = true;
    }

    static void ThreadWaitResume(ThreadParam* p)
    {
        p->threadPaused = true;
        while ( ! p->threadComplete)
        {
            SleepThread(0);
        }
    }
#if defined(WIN32)
    static int SimpleLock(void* param)
#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
    static void* SimpleLock(void* param)
#endif
    {
        ThreadParam* p = static_cast<ThreadParam*>(param);
        if (NULL == p->lockHandle)
        {
            if (p->lockName.empty())
            {
                SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet());
                ThreadWaitResume(p);
            }
            else 
            {
                SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet(p->lockName));
                ThreadWaitResume(p);
            }
        }
        else 
        {
            SCXCoreLib::SCXThreadLock lock(*(p->lockHandle));
            ThreadWaitResume(p);
        }
        return 0;
    }

public:
    void setUp(void)
    {

    }

    void tearDown(void)
    {

    }

    void callDumpStringForCoverage()
    {
        SCXCoreLib::SCXThreadLock lock(L"SomeLock");
        CPPUNIT_ASSERT(lock.DumpString().find(L"SCXThreadLock") != std::wstring::npos);
        CPPUNIT_ASSERT(lock.DumpString().find(L"SomeLock") != std::wstring::npos);
        CPPUNIT_ASSERT(SCXCoreLib::SCXThreadLockHandle().DumpString().find(L"SCXThreadLockHandle") != std::wstring::npos);
        CPPUNIT_ASSERT(SCXCoreLib::SCXThreadLockFactory::GetInstance().DumpString().find(L"SCXThreadLockFactory") != std::wstring::npos);
    }

    void TestInvalid(void)
    {
        SCXCoreLib::SCXThreadLockHandle handle;
        SCXCoreLib::SCXThreadLock noImpl(handle, false);
        SCXCoreLib::SCXThreadLock unlocked(L"Test Unlocked", false);
        
        // Test (un)locking invalid objects.
        SCXUNIT_ASSERT_THROWN_EXCEPTION(noImpl.Lock(), SCXCoreLib::SCXThreadLockInvalidException, L"No implementation");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(noImpl.Unlock(), SCXCoreLib::SCXThreadLockInvalidException, L"No implementation");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(noImpl.TryLock(), SCXCoreLib::SCXThreadLockInvalidException, L"No implementation");

        // Test querying lock status on invalid locks.
        SCXUNIT_ASSERT_THROWN_EXCEPTION(noImpl.HaveLock(), SCXCoreLib::SCXThreadLockInvalidException, L"No implementation");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(noImpl.IsLocked(), SCXCoreLib::SCXThreadLockInvalidException, L"No implementation");

        // Test unlocking lock not held.
        SCXUNIT_ASSERT_THROWN_EXCEPTION(unlocked.Unlock(), SCXCoreLib::SCXThreadLockNotHeldException, L"not held");
    }

    void TestReLock(void)
    {
        // Test relocking anonymous lock
        {
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet());
            CPPUNIT_ASSERT_THROW(lock.Lock(), SCXCoreLib::SCXThreadLockHeldException);
            CPPUNIT_ASSERT_THROW(lock.TryLock(), SCXCoreLib::SCXThreadLockHeldException);
        }

        // Test relocking named lock
        {
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet(L"TestLock"));
            CPPUNIT_ASSERT_THROW(lock.Lock(), SCXCoreLib::SCXThreadLockHeldException);
            CPPUNIT_ASSERT_THROW(lock.TryLock(), SCXCoreLib::SCXThreadLockHeldException);
        }

        // Test relocking named lock with different lock instances
        {
            SCXCoreLib::SCXThreadLock lock1(SCXCoreLib::ThreadLockHandleGet(L"TestLock", false));
            SCXCoreLib::SCXThreadLock lock2(SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock(L"TestLock", false), false); 
            CPPUNIT_ASSERT_THROW(lock2.Lock(), SCXCoreLib::SCXThreadLockHeldException);
            CPPUNIT_ASSERT_THROW(lock2.TryLock(), SCXCoreLib::SCXThreadLockHeldException);
        }
    }

    void TestGlobalLockNamesCreated(void)
    {
        // Test names in the factory are created and destroyed.
        CPPUNIT_ASSERT_EQUAL(0u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
        {
            SCXCoreLib::SCXThreadLock lockA(L"LockA");
            CPPUNIT_ASSERT_EQUAL(1u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            {
                SCXCoreLib::SCXThreadLock lockB(L"LockB");
                CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
                {
                    SCXCoreLib::SCXThreadLock lockC(L"LockC");
                    CPPUNIT_ASSERT_EQUAL(3u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
                }
                CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            }
            CPPUNIT_ASSERT_EQUAL(1u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
        }
        CPPUNIT_ASSERT_EQUAL(0u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
    }
    
    void TestGlobalLockNamesReused(void)
    {
        // Test names are reused.
        {
            CPPUNIT_ASSERT_EQUAL(0u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockA = SCXCoreLib::ThreadLockHandleGet(L"LockA");
            CPPUNIT_ASSERT_EQUAL(2u, lockA.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(1u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockA1 = SCXCoreLib::ThreadLockHandleGet(L"LockA");
            CPPUNIT_ASSERT_EQUAL(3u, lockA.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(3u, lockA1.GetRefCount());
            
            CPPUNIT_ASSERT_EQUAL(1u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockB = SCXCoreLib::ThreadLockHandleGet(L"LockB");
            CPPUNIT_ASSERT_EQUAL(3u, lockA.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(3u, lockA1.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, lockB.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockB1 = SCXCoreLib::ThreadLockHandleGet(L"LockB");
            CPPUNIT_ASSERT_EQUAL(3u, lockA.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(3u, lockA1.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(3u, lockB.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(3u, lockB1.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
        }
    }

    void TestGlobalLocksAndUnnamedLocksSeparate(void)
    {
        // Test global names and unnamed locks are kept sepparate. Also test that locks with empty name are
        // treated as unnamed locks.
        {
            CPPUNIT_ASSERT_EQUAL(0u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockA = SCXCoreLib::ThreadLockHandleGet(L"LockA");
            CPPUNIT_ASSERT_EQUAL(2u, lockA.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(1u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockB = SCXCoreLib::ThreadLockHandleGet(L"LockB");
            CPPUNIT_ASSERT_EQUAL(2u, lockB.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockC = SCXCoreLib::ThreadLockHandleGet();
            CPPUNIT_ASSERT_EQUAL(1u, lockC.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
            SCXCoreLib::SCXThreadLockHandle lockD = SCXCoreLib::ThreadLockHandleGet(L"");
            CPPUNIT_ASSERT_EQUAL(2u, lockA.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, lockB.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(1u, lockC.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(1u, lockD.GetRefCount());
            CPPUNIT_ASSERT_EQUAL(2u, SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLockCnt());
        }
    }

    void TestRecursive(void)
    {
        // Test lock is not recursive.
        {
            SCXCoreLib::SCXThreadLock lock(L"TestNonRecursiveLock");
            CPPUNIT_ASSERT_EQUAL(false, lock.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock.HaveLock());
            CPPUNIT_ASSERT_THROW(lock.Lock(), SCXCoreLib::SCXThreadLockHeldException);
        }
    
        // Test lock is recursive.
        {
            SCXCoreLib::SCXThreadLock lock(L"TestRecursiveLock", true, true);
            CPPUNIT_ASSERT_EQUAL(true, lock.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock.HaveLock());
            SCXCoreLib::SCXThreadLock lock1(L"TestRecursiveLock", true, true);
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock1.HaveLock());
            lock.Lock();
            CPPUNIT_ASSERT_EQUAL(true, lock.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock.HaveLock());
            lock1.Lock();
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock1.HaveLock());
            
            lock.Unlock();
            CPPUNIT_ASSERT_EQUAL(true, lock.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock.HaveLock());
            lock1.Unlock();
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock1.HaveLock());
            // One before last unlock. Both SCXThreadLock objects should still be locked.
            lock.Unlock();
            CPPUNIT_ASSERT_EQUAL(true, lock.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock.HaveLock());
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsLocked());
            CPPUNIT_ASSERT_EQUAL(true, lock1.HaveLock());
            // Last unlock. Both SCXThreadLock should be unlocked.
            lock1.Unlock();
            CPPUNIT_ASSERT_EQUAL(true, lock.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(false, lock.IsLocked());
            CPPUNIT_ASSERT_EQUAL(false, lock.HaveLock());
            CPPUNIT_ASSERT_EQUAL(true, lock1.IsRecursive());
            CPPUNIT_ASSERT_EQUAL(false, lock1.IsLocked());
            CPPUNIT_ASSERT_EQUAL(false, lock1.HaveLock());
        }
    }

    void TestAnonymousSimple(void)
    {
        // Test two anonymous locks are not the same
        {
            ThreadParam p;
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet());
            thread_handle_t h = StartThread(SimpleLock, &p, NULL);
            ThreadPause(&p);
            ThreadResume(&p);
            WaitThread(h, 60);
            CPPUNIT_ASSERT(DoneThread(h));
        }

        // Test the same anonymous lock
        {
            ThreadParam p;
            SCXCoreLib::SCXThreadLockHandle lh = SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock();
            SCXCoreLib::SCXThreadLock lock(lh, false);
            p.lockHandle = &lh;
            thread_handle_t h = StartThread(SimpleLock, &p, NULL);
            ThreadPause(&p);
            CPPUNIT_ASSERT( ! DoneThread(h)); // Test locking thread is running
            CPPUNIT_ASSERT( ! lock.TryLock());
            ThreadResume(&p);
            lock.Lock();
            WaitThread(h, 60);
            CPPUNIT_ASSERT(DoneThread(h));
        }

        // Test anonymous with TryLock
        {
            ThreadParam p;
            SCXCoreLib::SCXThreadLockHandle lh = SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock();
            SCXCoreLib::SCXThreadLock lock(lh, false);
            p.lockHandle = &lh;
            thread_handle_t h = StartThread(SimpleLock, &p, NULL);
            ThreadPause(&p);
            CPPUNIT_ASSERT( ! DoneThread(h)); // Test locking thread is running
            CPPUNIT_ASSERT_THROW(lock.TryLock(100), SCXCoreLib::SCXNotSupportedException);
            CPPUNIT_ASSERT( ! lock.TryLock());
            ThreadResume(&p);
            WaitThread(h,60);
            CPPUNIT_ASSERT(lock.TryLock());
        }

        // Test creating two named locks with empty name are not the same.
        {
            SCXCoreLib::SCXThreadLock lock1(SCXCoreLib::ThreadLockHandleGet(L""));
            SCXCoreLib::SCXThreadLock lock2(SCXCoreLib::ThreadLockHandleGet(L""), false);
            CPPUNIT_ASSERT( ! lock2.HaveLock());
            CPPUNIT_ASSERT( ! lock2.IsLocked());
            CPPUNIT_ASSERT(lock1.IsLocked());
        }
    }

    void TestNamedSimple(void)
    {
        // Test two different named locks
        {
            ThreadParam p;
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet(L"TestLock1"), true);
            p.lockName = L"TestLock2";
            thread_handle_t h = StartThread(SimpleLock, &p, NULL);
            ThreadPause(&p);
            ThreadResume(&p);
            WaitThread(h, 60);
            CPPUNIT_ASSERT(DoneThread(h));
        }

        // Test same named lock
        {
            ThreadParam p;
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock(L"TestLock"), false);
            p.lockName = L"TestLock";
            thread_handle_t h = StartThread(SimpleLock, &p, NULL);
            ThreadPause(&p);
            CPPUNIT_ASSERT( ! DoneThread(h)); // Test locking thread is running
            CPPUNIT_ASSERT( ! lock.TryLock());
            CPPUNIT_ASSERT( ! lock.HaveLock());
            CPPUNIT_ASSERT(lock.IsLocked());
            ThreadResume(&p);
            lock.Lock();
            WaitThread(h, 60);
            CPPUNIT_ASSERT(DoneThread(h)); 
        }

        // Test named with TryLock
        {
            ThreadParam p;
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock(L"TestLock", false), false);
            p.lockName = L"TestLock";
            thread_handle_t h = StartThread(SimpleLock, &p, NULL);
            ThreadPause(&p);
            CPPUNIT_ASSERT( ! DoneThread(h)); // Test locking thread is running
            CPPUNIT_ASSERT_THROW(lock.TryLock(100), SCXCoreLib::SCXNotSupportedException);
            CPPUNIT_ASSERT( ! lock.TryLock());
            ThreadResume(&p);
            WaitThread(h,60);
            CPPUNIT_ASSERT(lock.TryLock());
        }
    }

    void TestRefCount(void)
    {
        scxulong locksUsed = SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLocksUsed();
        {
            SCXCoreLib::SCXThreadLockHandle lh1a = SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock(L"TestLock1");
            CPPUNIT_ASSERT( (locksUsed+1) == SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLocksUsed() );
            SCXCoreLib::SCXThreadLockHandle lh1b = SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock(L"TestLock1");
            CPPUNIT_ASSERT( (locksUsed+1) == SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLocksUsed() );
            SCXCoreLib::SCXThreadLockHandle lh2 = SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLock(L"TestLock2");
            CPPUNIT_ASSERT( (locksUsed+2) == SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLocksUsed() );
        }
        CPPUNIT_ASSERT( locksUsed == SCXCoreLib::SCXThreadLockFactory::GetInstance().GetLocksUsed() );
    }

    void TestAssign(void)
    {
        SCXCoreLib::SCXThreadLockHandle lh1 = SCXCoreLib::SCXThreadLockHandle(L"TestLock1");
        SCXCoreLib::SCXThreadLockHandle lh2 = SCXCoreLib::SCXThreadLockHandle(L"TestLock2");
        CPPUNIT_ASSERT(1 == lh1.GetRefCount());
        CPPUNIT_ASSERT(1 == lh2.GetRefCount());
        SCXCoreLib::SCXThreadLockHandle lh3 = SCXCoreLib::SCXThreadLockHandle(lh2);
        CPPUNIT_ASSERT(L"TestLock2" == lh3.GetName());
        CPPUNIT_ASSERT(1 == lh1.GetRefCount());
        CPPUNIT_ASSERT(2 == lh2.GetRefCount());
        lh1 = lh1;
        CPPUNIT_ASSERT(1 == lh1.GetRefCount());
        CPPUNIT_ASSERT(2 == lh2.GetRefCount());
        lh2 = lh3;
        CPPUNIT_ASSERT(1 == lh1.GetRefCount());
        CPPUNIT_ASSERT(2 == lh2.GetRefCount());
        lh3 = lh1;
        CPPUNIT_ASSERT(L"TestLock1" == lh3.GetName());
        CPPUNIT_ASSERT(2 == lh1.GetRefCount());
        CPPUNIT_ASSERT(1 == lh2.GetRefCount());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXThreadLockTest );

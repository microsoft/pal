/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        scxcondition.cpp

    \brief       Condition support for SCX.
                 Also helps keep work+sleep time constant in a loop
    
    \date        12-18-2008 15:54:00

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
//#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>

#if defined(sun) || defined(linux) || defined(hpux) || defined(aix)
    #include <errno.h>
    #include <time.h>
#elif defined(macos)
    #include <errno.h>
    #include <sys/time.h>    // for gettimeofday()
#elif defined(WIN32)
    #include <Windows.h>
#else
    #error Platform not implemented
#endif

using namespace SCXCoreLib;

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    Constructor

    \throws      SCXInternalErrorException if too large sleep time specified
    \throws      SCXErrnoException if error occurred calling system function
*/
    SCXCondition::SCXCondition()
        : m_bSetCalled(false)
          , m_sleepTime(0)
#if defined(WIN32)
          , m_timeStamp(0)
#endif
    {
#if defined(SCX_UNIX)
        // Initialize the time specification (this is ASSERTed later in code!)
        memset(&m_ts, 0, sizeof(m_ts));

        int err;
        if (0 != (err = pthread_mutex_init(&m_lock, NULL)))
        {
            throw SCXErrnoException(L"pthread_mutex_init() function call failed", err, SCXSRCLOCATION);
        }

        if (0 != (err = pthread_cond_init(&m_cond, NULL)))
        {
            throw SCXErrnoException(L"pthread_cond_init() function call failed", err, SCXSRCLOCATION);
        }
#elif defined(WIN32)
        InitializeCriticalSection(&m_lock);

        if (NULL == (m_cond = CreateEvent(NULL, true, false, NULL)))
        {
            throw SCXErrnoException(L"CreateEvent() function failed", GetLastError(), SCXSRCLOCATION);
        }
#else
        #error Platform not supported
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Destructor

    \throws      SCXInternalErrorException if we are inside a condition test
    \throws      SCXErrnoException if error occurred calling system function
*/
    SCXCondition::~SCXCondition()
    {
#if defined(SCX_UNIX)
        int err;
        if (0 != (err = pthread_cond_destroy(&m_cond)))
        {
            throw SCXErrnoException(L"pthread_cond_destroy() function call failed", err, SCXSRCLOCATION);
        }

        if (0 != (err = pthread_mutex_destroy(&m_lock)))
        {
            throw SCXErrnoException(L"pthread_mutex_destroy() function call failed", err, SCXSRCLOCATION);
        }
#elif defined(WIN32)
                DeleteCriticalSection(&m_lock);

                if (0 == CloseHandle(m_cond))
                {
                        throw SCXErrnoException(L"CloseHandle() function failed", GetLastError(), SCXSRCLOCATION);
                }
#else
                #error Platform not supported
#endif
    }


/*----------------------------------------------------------------------------*/
/**
    Call before entering condition to set the starting timestamp and sleep time

    \param       milliseconds    Number of milliseconds for work+sleep

    \throws      SCXInternalErrorException if called multiple times
*/
    void SCXCondition::SetSleep(scxulong milliseconds)
    {
        m_sleepTime = milliseconds;

#if defined(SCX_UNIX)
        memset(&m_ts, 0, sizeof(m_ts));
        GetMillisecondTimeStamp();
#elif defined(WIN32)
        m_timeStamp = GetMillisecondTimeStamp();
#endif

        m_bSetCalled = true;
    }

/*----------------------------------------------------------------------------*/
/**
    Begin a condition test

    \throws      SCXInternalErrorException if we are already in a condition
*/
    void SCXCondition::BeginCondition()
    {
        // Lock our mutex
        Lock();
    }

/*----------------------------------------------------------------------------*/
/**
    Waits for a condition

    Sleeps as directed (sleep time passed in constructor).  If enough time has
    passed since last sleep where we should be running again, then sleep is not
    performed (we wake up immediately).

    This is "automatic" on UNIX/Linux because there, wakeup time is absolute.
    In the case of Windows, we compute the next sleep period and only sleep if
    needed.  However, due to thread scheduling issues, we still may be "late"
    (but will correct over time).

    \throws      SCXInternalErrorException in case of internal error
*/
    enum SCXCondition::eConditionResult SCXCondition::Wait()
    {
        enum SCXCondition::eConditionResult returnCode = SCXCondition::eCondTestPredicate;

        // Be certain that SetSleep() has already been called
        if (!m_bSetCalled)
        {
            throw SCXInternalErrorException(L"SetSleep() method has not yet been called", SCXSRCLOCATION);
        }

#if defined(SCX_UNIX)
        // We do this check because pthread_cond_timedwait may wakeup prematurely
        int err;

        if (m_sleepTime)
        {
            // Adjust wakeup time for the next interval of sleep period
            m_ts.tv_nsec += static_cast<long>((m_sleepTime % 1000) * 1000000);  // += [0 .. 1) sec
            m_ts.tv_sec += static_cast<time_t>((m_sleepTime / 1000) + (m_ts.tv_nsec / 1000000000L));  // += x sec
            m_ts.tv_nsec %= 1000000000L;  // make it [0..1) sec

            // If there's a sleepTime, wait until that time passes
            err = pthread_cond_timedwait(&m_cond, &m_lock, &m_ts);
        }
        else
        {
            // No sleep at all?  That's a sleep until signal ...
            err = pthread_cond_wait(&m_cond, &m_lock);
        }
        if (err != 0)
        {
            if (ETIMEDOUT == err)
            {
                returnCode = SCXCondition::eCondTimeout;
            }
            else
            {
                throw SCXErrnoException(L"pthread_cond_timedwait() function call failed", err, SCXSRCLOCATION);
            }
        }
#elif defined(WIN32)
        if (m_sleepTime == 0)
        {
            // Unlock our mutex
            Unlock();

            // Wait for the object
            DWORD err = WaitForSingleObject(m_cond, INFINITE);
            DWORD exErr = GetLastError(); // Save extended error information

            // Lock our mutex
            Lock();

            if (err != WAIT_OBJECT_0)
            {
                // We presumably got WAIT_FAILED (not possible to get WAIT_TIMEOUT in this case)
                throw SCXErrnoException(L"WaitForSingleObject() function call failed", exErr, SCXSRCLOCATION);
            }
        }
        else {
            scxulong timestamp = GetMillisecondTimeStamp();

            // Check that the new target time is in the future
            if (m_timeStamp + m_sleepTime > timestamp)
            {
                // If in the future, calculate the time to sleep to reach it
                scxulong sleeptime = m_timeStamp + m_sleepTime - timestamp;

                // Unlock our mutex
                Unlock();

                // Wait for the object
                DWORD err = WaitForSingleObject(m_cond, static_cast<DWORD> (sleeptime));
                DWORD exErr = GetLastError(); // Save extended error information

                // Lock the mutex
                Lock();

                if (err == WAIT_OBJECT_0)
                {
                    // Fall through - we were signaled normally
                }
                else if (err == WAIT_TIMEOUT)
                {
                    returnCode = SCXCondition::eCondTimeout;
                }
                else
                {
                    // We presumably got WAIT_FAILED?
                    throw SCXErrnoException(L"WaitForSingleObject() function call failed", exErr, SCXSRCLOCATION);
                }
            }
            else
            {
                // If not in the future, we should not sleep at all
                // Set caculated after sleep time to use as basis for next sleep

                returnCode = SCXCondition::eCondTimeout;
            }

            m_timeStamp = m_timeStamp + m_sleepTime;
        }
#else
#error Platform not defined
#endif

        return returnCode;
    }

/*----------------------------------------------------------------------------*/
/**
    End a condition test

    \throws      SCXInternalErrorException if we are not in a condition
*/
    void SCXCondition::EndCondition()
    {
        // Unlock our mutex
        Unlock();
    }

/*----------------------------------------------------------------------------*/
/**
    Signal a condition (causes code waiting on a condition to wake up).

    Prior to signaling a condition, the signaler is responsible to set the
    predicate (the shared data value between the signaler and the waiter).

    \throws      SCXErrnoException if error occurred calling system function

*/
    void SCXCondition::Signal()
    {
#if defined(SCX_UNIX)
        int err;
        if (0 != (err = pthread_cond_signal(&m_cond)))
        {
            throw SCXErrnoException(L"pthread_cond_signal() function call failed", err, SCXSRCLOCATION);
        }
#elif defined(WIN32)
        if (0 == PulseEvent(m_cond))
        {
            throw SCXErrnoException(L"PulseEvent() function call failed", GetLastError(), SCXSRCLOCATION);
        }
#else
#error Platform not implemented
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Signal a condition to multiple recipients (causes code waiting on a condition to wake up).

    Prior to signaling a condition, the signaler is responsible to set the
    predicate (the shared data value between the signaler and the waiter).

    \throws      SCXErrnoException if error occurred calling system function

*/
    void SCXCondition::Broadcast()
    {
#if defined(SCX_UNIX)
        int err;
        if (0 != (err = pthread_cond_broadcast(&m_cond)))
        {
            throw SCXErrnoException(L"pthread_cond_broadcast() function call failed", err, SCXSRCLOCATION);
        }
#elif defined(WIN32)
#error Windows condition not implemented
#else
#error Platform not implemented
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Get current time in milliseconds

    \returns     A scxulong representing current time in milliseconds
    \throws      SCXInternalErrorException if system functions fail
*/
    scxulong SCXCondition::GetMillisecondTimeStamp()
    {
#if defined(sun) || defined(linux) || defined(hpux) || defined(aix)
        // On UNIX, we should only be called once (m_ts is used in Wait())
        SCXASSERT( 0 == m_ts.tv_sec );
        SCXASSERT( 0 == m_ts.tv_nsec );

        if (0 != clock_gettime(CLOCK_REALTIME, &m_ts))
        {
            throw SCXInternalErrorException(L"clock_gettime() failed", SCXSRCLOCATION);
        }

        return static_cast<scxulong>(m_ts.tv_sec) * 1000 + m_ts.tv_nsec / 1000000;
#elif defined(macos)
        // On Mac OS, we should only be called once (m_ts is used in Wait())
        SCXASSERT( 0 == m_ts.tv_sec );
        SCXASSERT( 0 == m_ts.tv_nsec );

        struct timeval tv;

        if (0 != gettimeofday(&tv, NULL))
        {
            throw SCXInternalErrorException(L"gettimeofday() failed", SCXSRCLOCATION);
        }

        TIMEVAL_TO_TIMESPEC(&tv, &m_ts);
        return static_cast<scxulong>(m_ts.tv_sec) * 1000 + m_ts.tv_nsec / 1000000;
#elif defined(WIN32)
        return GetTickCount();
#else
#error "Plattform not implemented"
#endif
    }

/**
    Locks a condition.

    This is handled automatically by either the SCXCondition class itself or
    by the SCXConditionHandle helper class.

    \throws      SCXErrnoException if error occurred calling system function
 */
    void SCXCondition::Lock()
    {
#if defined(SCX_UNIX)
        int err;
        if (0 != (err = pthread_mutex_lock(&m_lock)))
        {
            throw SCXErrnoException(L"pthread_mutex_lock() function call failed", err, SCXSRCLOCATION);
        }
#elif defined(WIN32)
        EnterCriticalSection(&m_lock);
#else
#error Platform not supported
#endif
    }

/**
    Unlocks a condition.

    This is handled automatically by either the SCXCondition class itself or
    by the SCXConditionHandle helper class.

    \throws      SCXErrnoException if error occurred calling system function
 */
    void SCXCondition::Unlock()
    {
#if defined(SCX_UNIX)
        int err;
        if (0 != (err = pthread_mutex_unlock(&m_lock)))
        {
            throw SCXErrnoException(L"pthread_mutex_unlock() function call failed", err, SCXSRCLOCATION);
        }
#elif defined(WIN32)
        LeaveCriticalSection(&m_lock);
#else
#error Platform not supported
#endif
        }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

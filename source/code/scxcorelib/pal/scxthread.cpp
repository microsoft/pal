/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implements the thread handling PAL.

    \date        2007-06-19 14:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include <sstream>
#if defined(WIN32)
#elif defined(SCX_UNIX)
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#else
#error "Platform not implemented"
#endif


namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        A thread param structure used to start threads.

        This is the parameter sent to the starting thread. This structure
        is only used in this file.

    */
    struct ThreadStartParams
    {
        ThreadStartParams() : body(0), param(0) {}

        SCXThreadProc body; //!< Pointer to thread body function.
        SCXThreadParamHandle param; //!< Pointer to thread parameters.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Thread entry point.

        \param     param A ThreadStartParams structure with the actual thread procedure and parameters.
        \returns   Zero

        All threads started using this PAL are started using this method.

    */
    static
#if defined(WIN32)
    DWORD WINAPI _InternalThreadStartRoutine(void* param)
#elif defined(SCX_UNIX)
    void* _InternalThreadStartRoutine(void* param)
#endif
    {
        ThreadStartParams* p = static_cast<ThreadStartParams*>(param);
        SCXASSERT(p != 0);

        if (p->body != 0)
        {
#if !defined(_DEBUG)
             try
             {
                p->body( p->param );
             }
             catch (const SCXException& e1)
             {
                 SCXASSERTFAIL(std::wstring(L"ThreadStartRoutine() Thread threw unhandled exception - ").
                              append(e1.What()).append(L" - ").append(e1.Where()).c_str());
             }
             catch (const std::exception& e2)
             {
                 SCXASSERTFAIL(std::wstring(L"ThreadStartRoutine() Thread threw unhandled exception - ").
                              append(StrFromUTF8(e2.what())).c_str());
             }
#else
             p->body( p->param );
#endif
            /* We would like to catch (...) as well but it seemes we can't since there is a bug
               in gcc. http://gcc.gnu.org/bugzilla/show_bug.cgi?id=28145 */
        }
        delete p;
        return 0;
    }
}

extern "C"
{
    static void* ThreadStartRoutine(void* param)
    {
        return SCXCoreLib::_InternalThreadStartRoutine(param);
    }
}

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.

    */
    SCXThreadParam::SCXThreadParam()
    {
        m_terminateFlag = false;
        m_lock = ThreadLockHandleGet();
        m_stringValues.clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.

    */
    SCXThreadParam::~SCXThreadParam()
    {
        m_stringValues.clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns    Object represented as string for logging.

    */
    const std::wstring SCXThreadParam::DumpString() const
    {
        return L"SCXThreadParam: " + SCXCoreLib::StrFrom(static_cast<unsigned int>(m_stringValues.size()));
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve a thread parameter string value.

        \param[in]  key name of the parameter string value to retrieve
        \returns    A parameter string.
        \throws     SCXInvalidThreadParamValueException if the given key does not exist.

    */
    const std::wstring& SCXThreadParam::GetString(const std::wstring& key) const
    {
        SCXThreadLock lock(m_lock);
        std::map<std::wstring, std::wstring>::const_iterator cv = m_stringValues.find(key);
        if (cv != m_stringValues.end())
        {
            return cv->second;
        }
        lock.Unlock();
        // If no value found, raise an exception.
        throw SCXInvalidThreadParamValueException(key, SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set a string value.

        \param[in]  key name of the parameter value
        \param[in]  value value to set.

        Sets a string parameter value.

    */
    void SCXThreadParam::SetString(const std::wstring& key, const std::wstring& value)
    {
        SCXThreadLock lock(m_lock);
        m_stringValues[key] = value;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.

    */
    SCXThread::SCXThread()
        : m_threadID(0)
        , m_paramHandle(0)
#if defined(WIN32)
        , m_threadHandle(new HANDLE(INVALID_HANDLE_VALUE))
#endif
        , m_threadMaySurviveDestruction(true)
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
        Start a new thread constructor.

        \param  proc thread entry point
        \param  param thread parameters (default: 0)
        \param  attr thread creation parameters (default: 0)

        Uses SCXThread::Start to actually start the thread.

        \note This constructor takes ownership of the given param argument and will
        delete it once the thread exits. The reason for this is that this is a convenience
        method to be used instead of the constructor taking a SCXThreadParamHandle.
    */
    SCXThread::SCXThread(SCXThreadProc proc, SCXThreadParam* param /* = 0 */, const SCXThreadAttr* attr /*= 0*/)
        : m_threadID(0)
        , m_paramHandle(0)
#if defined(WIN32)
        , m_threadHandle(new HANDLE(INVALID_HANDLE_VALUE))
#endif
        , m_threadMaySurviveDestruction(true)

    {
        Start(proc, param, attr);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Start a new thread constructor.

        \param  proc thread entry point
        \param  param thread parameters handle (reference counted)
        \param  attr thread creation parameters (default: 0)

        Uses SCXThread::Start to actually start the thread.

    */
    SCXThread::SCXThread(SCXThreadProc proc, const SCXThreadParamHandle& param, const SCXThreadAttr* attr /*= 0*/)
        : m_threadID(0)
        , m_paramHandle(new SCXThreadParam())
#if defined(WIN32)
        , m_threadHandle(new HANDLE(INVALID_HANDLE_VALUE))
#endif
        , m_threadMaySurviveDestruction(true)

    {
        Start(proc, param, attr);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Helper to start new thread.

        \param  proc thread entry point
        \param  attr thread creation parameters

        \throws  SCXThreadStartException if unable to start a new thread.

    */
    void SCXThread::SCXThreadStartHelper(SCXThreadProc proc, const SCXThreadAttr* attr)
    {
        SCXASSERT(0 != m_paramHandle.GetData());

        ThreadStartParams* p = new ThreadStartParams;
        SCXASSERT( NULL != p );
        p->body = proc;
        p->param = m_paramHandle;
#if defined(WIN32)
        *m_threadHandle = CreateThread(NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(ThreadStartRoutine), p, 0, &m_threadID);
        if (NULL == *m_threadHandle)
        {
            *m_threadHandle = INVALID_HANDLE_VALUE;
            throw SCXThreadStartException(L"CreateThread failed", SCXSRCLOCATION);
        }
#elif defined(SCX_UNIX)
        int pth_errno;
        if (0 != (pth_errno = pthread_create(&m_threadID, *attr, ThreadStartRoutine, p)))
        {
            throw SCXThreadStartException(SCXCoreLib::StrAppend(L"pthread_create failed, errno=", pth_errno), SCXSRCLOCATION);
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.

    */
    SCXThread::~SCXThread()
    {
#if defined(SCX_UNIX)
        if (m_threadMaySurviveDestruction)
        {
            if (m_threadID != 0)
            {
                // Detach the thread to reclaim thread resources automatically.
                (void)pthread_detach(m_threadID);
            }
        }
        else
        {
            // Could only have happened if "Terminate" or "Wait" was called,
            // therefore no need to terminate the thread here, also
            // the resources have been reclaimed by pthread_join
        }
#endif
#if defined(WIN32)
        if (INVALID_HANDLE_VALUE != *m_threadHandle)
        {
            CloseHandle(*m_threadHandle);
            *m_threadHandle = INVALID_HANDLE_VALUE;
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns   Object represented as string for logging.

    */
    const std::wstring SCXThread::DumpString() const
    {
#if defined(macos)
        // http://www.webservertalk.com/archive107-2006-7-1577906.html
        // A pthread_t is only guaranteed to be an arithmetic type - not an
        // unsigned long int. (it's not uncommon for pthread_t to be a pointer type.)
        return SCXCoreLib::StrAppend(L"SCXThread: ", reinterpret_cast<scxulong>(m_threadID));
#else
        return SCXCoreLib::StrAppend(L"SCXThread: ", static_cast<scxulong>(m_threadID));
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Thread starter.

       \param  proc thread entry point
       \param  param thread parameters (default: none.  Must be specified.  May be NULL.)
       \param  attr thread creation parameters (default: 0)
       \throws SCXThreadStartException if thread could not be started.

       Uses SCXThread::SCXThreadStartHelper to actually start the thread.

       \note This method takes ownership of the given param argument and will
       delete it once the thread exits. The reason for this is that this is a convenience
       method to be used instead of the method taking a SCXThreadParamHandle.
    */
    void SCXThread::Start(SCXThreadProc proc, SCXThreadParam* param /*= 0*/, const SCXThreadAttr* attr /*= 0*/)
    {
        SCXThreadParam* p = param;
        if (0 == p)
        {
            p = new SCXThreadParam();
        }
        SCXThreadParamHandle h(p);

        Start(proc, h, attr);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Thread starter

       \param  proc thread entry point
       \param  param thread parameters handle (reference counted)
       \param  attr thread creation parameters (default: 0)
       \throws SCXThreadStartException if thread could not be started.

       Uses SCXThread::SCXThreadStartHelper to actually start the thread.

    */
    void SCXThread::Start(SCXThreadProc proc, const SCXThreadParamHandle& param, const SCXThreadAttr* attr /*= 0*/)
    {
        if (0 != m_threadID)
        {
            throw SCXThreadStartException(L"Thread already started", SCXSRCLOCATION);
        }
        m_paramHandle = param;

        if (attr == 0)
        {
            SCXThreadAttr attr_temp;
            SCXThreadStartHelper(proc, &attr_temp);
        }
        else
        {
            SCXThreadStartHelper(proc, attr);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrive the thread ID.

        \returns     SCXThreadId of the thread.

    */
    SCXThreadId SCXThread::GetThreadID() const
    {
        return m_threadID;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrive the thread params.

        \returns     SCXThreadParamHandle the thread is using.

    */
    SCXThreadParamHandle& SCXThread::GetThreadParam()
    {
        return m_paramHandle;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Determine if a thread is alive or not.

        \returns   true if the thread is still active, otherwise false.

        \note If thread IDs are reused by the underlaying platform and a new
        thread is started with the same ID, this function may return true because the
        new thread is active while the original thread is dead.

    */
    bool SCXThread::IsAlive() const
    {
        bool r = false;
#if defined(WIN32)
        if (INVALID_HANDLE_VALUE != *m_threadHandle)
        {
            DWORD ec = STILL_ACTIVE;
            if (GetExitCodeThread(*m_threadHandle, &ec))
            {
                if (STILL_ACTIVE == ec)
                {
                    r = true;
                }
            }
        }
#elif defined(SCX_UNIX)
        if (m_threadID != 0 && 0 == pthread_kill(m_threadID, 0))
        {
            r = true;
        }
#endif
        return r;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Request a thread to terminate.

        \note This is NOT a forced termination. The thread must support this
        in the thread procedure by polling the parameter terminate flag.
    */
    void SCXThread::RequestTerminate()
    {
        SCXASSERT(0 != m_paramHandle.GetData());

        // Signal the thread to shut down
        SCXConditionHandle h(m_paramHandle->m_cond);
        m_paramHandle->SetTerminateFlag();
        SCXASSERT( true == m_paramHandle->GetTerminateFlag() );
        h.Signal();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Wait for the thread to complete execution.

        \throws     SCXInternalErrorException if unable to wait for thread termination.

        \note There is no need to "Wait" for all threads to remove resources
        allocated by the thread. This is merely a convenience function to make sure
        a certain thread has completed.

    */
    void SCXThread::Wait()
    {
        m_threadMaySurviveDestruction = false;
#if defined(WIN32)
        if (INVALID_HANDLE_VALUE != *m_threadHandle)
        {
            DWORD r = WaitForSingleObject(*m_threadHandle, INFINITE);
            SCXASSERT(WAIT_OBJECT_0 == r);
            if (WAIT_OBJECT_0 != r)
            {
                throw SCXCoreLib::SCXInternalErrorException(L"WaitForSingleObject did not return WAIT_OBJECT_0", SCXSRCLOCATION);
            }
        }
#elif defined(SCX_UNIX)
        if (0 != m_threadID)
        {
            int retval = pthread_join(m_threadID, 0);
            if (retval)
            {
                throw SCXCoreLib::SCXErrnoException(L"pthread_join did not succeed", retval, SCXSRCLOCATION);
            }
            m_threadID = 0;
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Pause (sleep) the calling thread for at least given number of milliseconds.

        \param[in]  milliseconds number of milliseconds to sleep.

    */
    void SCXThread::Sleep(scxulong milliseconds)
    {
#if defined(WIN32)
        ::Sleep(static_cast<DWORD>(milliseconds));
#elif defined(SCX_UNIX)
        struct timespec rqtp;

        rqtp.tv_sec = static_cast<long>(milliseconds/1000);
        rqtp.tv_nsec = static_cast<long>((milliseconds%1000)*1000*1000);

        nanosleep(&rqtp, NULL);
#else
#error "Platform not implemented"
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the calling threads, thread id.

        \returns      SCXThreadId of the calling thread.

    */
    SCXThreadId SCXThread::GetCurrentThreadID()
    {
#if defined(WIN32)
        return GetCurrentThreadId();
#elif defined(SCX_UNIX)
        return pthread_self();
#else
#error "Platform not implemented"
#endif
    }

} /* namespace SCXCoreLib */


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

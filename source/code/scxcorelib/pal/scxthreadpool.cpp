/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implements the thread pool handling PAL.

    \date        2012-11-06 11:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxthreadpool.h>
#include <scxcorelib/stringaid.h>

#include <unistd.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    */
    SCXThreadPool::SCXThreadPool(SCXHandle<SCXThreadPoolDependencies> deps)
        : m_deps(deps),
          m_logHandle(SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.threadpool")),
          m_threadCount(0),
          m_threadLimit(8),
          m_threadBusyCount(0),
          m_isRunning(false),
          m_isTerminating(false)
    {
        m_cond.SetSleep( 0 );
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.
    */
    SCXThreadPool::~SCXThreadPool()
    {
        if ( m_isRunning )
            Shutdown();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns   Object represented as string for logging.
    */
    const std::wstring SCXThreadPool::DumpString() const
    {
        return SCXDumpStringBuilder("SCXThreadPool")
            .Scalar("ThreadCount", m_threadCount)
            .Scalar("ThreadLimit", m_threadLimit)
            .Scalar("IsRunning", m_isRunning)
            .Scalar("IsTerminating", m_isTerminating);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Starts a new worker thread to run in the thread pool

        \throws  SCXInvalidStateException if trying to start a thread beyond our limit
    */
    void SCXThreadPool::StartWorkerThread()
    {
        if ( m_threadCount >= m_threadLimit )
            throw SCXInvalidStateException( L"Unable to start another thread due to thread limit", SCXSRCLOCATION );

        // Increment the number of worker threads running - and start it
        // (Can't do increment in worker thread - delay in thread execution can result in incorrect count)
        scx_atomic_increment( &m_threadCount );

        SCXThreadPoolThreadParam* params = new SCXThreadPoolThreadParam(this);
        params->m_cond.SetSleep( 0 );

        SCXThreadHandle thread(new SCXCoreLib::SCXThread(StartWorkerThreadStub, params, &m_threadAttr));

        m_hThreads.push_back(thread);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Helper class to launch a worker thread.  This static class is run directly by
        the new thread (via pthreads), and exists solely to make the jump from a
        "C" interface function to a class method.
    */
    void SCXThreadPool::StartWorkerThreadStub(SCXCoreLib::SCXThreadParamHandle& handle)
    {
        SCXThreadPoolThreadParam* params = static_cast<SCXThreadPoolThreadParam*>( handle.GetData() );
        SCXThreadPool* pool = params->GetThreadPool();

        pool->DoWorkerThread( params );
    }

    /*-----------------------------------------------------------------------*/
    /**
       Worker Thread Execution

       This method is execution of an actual worker thread.  As such, it will
       wait for an entry from the queue, pull it off, and execute it.

       Many instances of this method may run at any one time.  Code accordingly!
    */
    void SCXThreadPool::DoWorkerThread(SCXThreadPoolThreadParam* /* params */)
    {
        // Wait for the condition (shutdown, or something added to the queue)
        SCXConditionHandle h( m_cond );
        while ( !m_isTerminating )
        {
            // If we need to throttle down (reduce number of threads), then do so
            // Test prior to the condition wait in case we missed broadcast due to queue execution
            if ( m_threadCount > m_threadLimit )
                break;

            // If the queue isn't empty, don't wait - keep on going
            if ( m_tasks.empty() || m_deps->IsWorkerTaskExecutionDelayed() )
            {
                enum SCXCondition::eConditionResult r = h.Wait();

                // If we're shutting down, bye bye
                if ( m_isTerminating )
                    break;

                SCX_LOGTRACE(m_logHandle, StrAppend(L"DoWorkerThread(): Awake from condition with result: ", r));
            }

            // If we need to throttle down (reduce number of threads), then do so
            if ( m_threadCount > m_threadLimit )
                break;

            // Test hook - delay task execution if desired
            if ( m_deps->IsWorkerTaskExecutionDelayed() )
                continue;

            // Get to work!  Handle an entry from the queue ...
            if ( !m_tasks.empty() )
            {
                // Pull the first entry off the queue and dispatch
                SCXThreadPoolTaskHandle task = m_tasks.front();
                m_tasks.erase( m_tasks.begin() );

                // Launch the Worker Thread task (a bit of copied code from SCXThread.cpp)
                if (task->m_proc != 0)
                {
                    // Unlock the condition while we call the ThreadProc
                    // NOTE: We can miss broadcasts while ThreadProc is executing ...
                    m_threadBusyCount++;
                    h.Unlock();

#if !defined(_DEBUG)
                    try
                    {
                        task->m_proc( task->m_param );
                    }
                    catch (const SCXException& e1)
                    {
                        SCXASSERTFAIL(std::wstring(L"WorkerThreadStartRoutine() Thread threw unhandled exception - ").
                                      append(e1.What()).append(L" - ").append(e1.Where()).c_str());
                    }
                    catch (const std::exception& e2)
                    {
                        SCXASSERTFAIL(std::wstring(L"WorkerThreadStartRoutine() Thread threw unhandled exception - ").
                                      append(StrFromUTF8(e2.what())).c_str());
                    }
#else
                    task->m_proc( task->m_param );
#endif
                    /* We would like to catch (...) as well but it seemes we can't since there is a bug
                       in gcc. http://gcc.gnu.org/bugzilla/show_bug.cgi?id=28145 */

                    // Lock the condition now that ThreadProc is finished
                    h.Lock();
                    if ( m_threadBusyCount >= 1 )
                    {
                        m_threadBusyCount--;
                    }
                }
            }
        }

        // Must be shutting down, so terminate the worker thread
        scx_atomic_decrement_test( &m_threadCount );
    }

    /*-----------------------------------------------------------------------*/
    /**
       Sets the maximum number of threads allowed to run

       \param[in]  size  Size of the stack for worker threads
       \throws     SCXInvalidStateException if worker threads are already started
    */
    void SCXThreadPool::SetWorkerStackSize(const size_t size)
    {
        if ( m_isRunning )
            throw SCXInvalidStateException(L"Worker thread stack size can't be set after thread poolis started", SCXSRCLOCATION );

        m_threadAttr.SetStackSize(size);
    }

    /*-----------------------------------------------------------------------*/
    /**
       Sets the maximum number of threads allowed to run

       \param[in]   number of threads allowed to run
       \throws      SCXInvalidStateException if worker thread limit is out of range
    */
    void SCXThreadPool::SetThreadLimit(long limit)
    {
        // Limit of 256 worker threads is arbitrary; raise if needed
        if ( limit < 1 || limit > 256 )
            throw SCXInvalidArgumentException( L"limit", L"Thread limit out of range", SCXSRCLOCATION );

         m_threadLimit = limit;

         // If we're started, ping the thread pool to get them to exit
         SCXConditionHandle h(m_cond);
         h.Broadcast();
    }

    /*-----------------------------------------------------------------------*/
    /**
       Queues a new task to run in a worker thread

       \throws      SCXInvalidStateException if worker threads are not started yet
    */
    void SCXThreadPool::QueueTask(SCXThreadPoolTaskHandle task)
    {
        if ( !m_isRunning )
            throw SCXInvalidStateException(L"Worker Thread Pool is not yet started", SCXSRCLOCATION );

        // Add an element to the queue, and wake a worker to handle it
        {
            SCXConditionHandle h(m_cond);
            m_tasks.push_back(task);
            h.Signal();

            // If we have insufficient worker threads to handle this, add a new one (throttle up)
            if ( (m_threadBusyCount + static_cast<long>(m_tasks.size())) > m_threadCount
                 && m_threadCount < m_threadLimit)
            {
                StartWorkerThread();
            }
        }
    }

    /*-----------------------------------------------------------------------*/
    /**
       Starts threads in the thread pool

       \throws      SCXInvalidStateException if worker threads are already started
    */
    void SCXThreadPool::Start()
    {
        if ( m_isRunning )
            throw SCXInvalidStateException(L"Method Start() has already been called", SCXSRCLOCATION );

        SCXASSERT( m_threadCount == 0 );
        SCXASSERT( m_threadLimit > 0 );
        StartWorkerThread();
        m_isRunning = true;
    }

    /*-----------------------------------------------------------------------*/
    /**
       Stops threads in the thread pool
    */
    void SCXThreadPool::Shutdown()
    {
        // If we're not running, nothing to do
        if ( ! m_isRunning )
            return;

        // Trigger the workers to exit
        {
            SCXConditionHandle h( m_cond );
            m_isTerminating  = true;
            h.Broadcast();
        }

        // Wait for actively running threads to stop
        for (std::vector<SCXThreadHandle>::iterator it = m_hThreads.begin(); it != m_hThreads.end(); ++it)
        {
            SCXThreadHandle thread = *it;
            thread->RequestTerminate();
            thread->Wait();
        }

        m_isRunning = false;

        // All threads should be stopped at this point; no need for locking any longer

        m_hThreads.clear();
        m_tasks.clear();
        m_isTerminating = false;
    }

} /* namespace SCXCoreLib */


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

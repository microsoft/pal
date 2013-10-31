/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Defines the public interface for the thread pool PAL.
    \date        2012-11-06 11:00:00
*/
/*----------------------------------------------------------------------------*/

#ifndef SCXTHREADPOOL_H
#define SCXTHREADPOOL_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxatomic.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>

#include <vector>


namespace SCXCoreLib
{
    // Forward declaration for our thread pool
    class SCXThreadPool;

    /*----------------------------------------------------------------------------*/
    /**
       This implements the Thread Parameters for the ThreadPool worker threads

       \NOTE  This is for internal use for SCXThreadPool only!
    */
    class SCXThreadPoolThreadParam : public SCXCoreLib::SCXThreadParam
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor

           \param[in] spTask Safe Pointer to a Task Object to be passed to the thread
        */
        SCXThreadPoolThreadParam(SCXThreadPool* p)
            : SCXCoreLib::SCXThreadParam(),
              m_pThreadPool(p)
        {
        }

        /*----------------------------------------------------------------------------*/
        /**
           virtual destructor
        */
        virtual ~SCXThreadPoolThreadParam()
        {
        }

        SCXThreadPool* GetThreadPool() { return m_pThreadPool; }

    private:
        class SCXThreadPool* m_pThreadPool;     //!< Pointer to our thread pool object
    };

    /*----------------------------------------------------------------------------*/
    /**
      Container for the tasks that are scheduled to run in worker threads
    */
    class SCXThreadPoolTask {
    public:
        SCXThreadPoolTask(SCXThreadProc proc, SCXThreadParamHandle& param)
            : m_proc(proc),
              m_param(param)
        {
        }
    private:
        SCXThreadProc m_proc;
        SCXThreadParamHandle m_param;

        friend class SCXThreadPool;
    };

    /*----------------------------------------------------------------------------*/
    /**
      Dependency class for SCXThreadPool

      These methods generally slight modify SCXThreadPoolDependencies for
      unit test purposes.
    */
    class SCXThreadPoolDependencies
    {
    public:
        virtual ~SCXThreadPoolDependencies() {}
        virtual bool IsWorkerTaskExecutionDelayed() { return false; }
    };

    /** Reference counted thread task. */
    typedef SCXHandle<SCXThreadPoolTask> SCXThreadPoolTaskHandle;

    /** Reference counted thread handle. */
    typedef SCXHandle<SCXThread> SCXThreadHandle;

    /*----------------------------------------------------------------------------*/
    /**
        Represents a thread pool.
    */
    class SCXThreadPool
    {
    private:
        // Do not allow copying
        SCXThreadPool(const SCXThreadPool &);           //!< Intentionally not implemented
        SCXThreadPool & operator=(const SCXThreadPool &); //!< Intentionally not implemented

        void StartWorkerThread();

    protected:
        SCXHandle<SCXThreadPoolDependencies> m_deps;    //!< Dependency class object
        std::vector<SCXThreadHandle> m_hThreads;        //!< Handles of threads in the pool
        std::vector<SCXThreadPoolTaskHandle> m_tasks;   //!< Queue of tasks that we need to run

        SCXCondition m_cond;                            //!< Queue / worker thread management
        SCXLogHandle m_logHandle;                       //!< SCX log handle
        SCXThreadAttr m_threadAttr;                     //!< Thread attributes for worker threads
        scx_atomic_t m_threadCount;                     //!< Number of threads currently running
        long m_threadLimit;                             //!< Limit to number of threads allowed
        long m_threadBusyCount;                         //!< Number of worker threads currently busy
        bool m_isRunning;                               //!< Is thread pool running (Start() called)?
        bool m_isTerminating;                           //!< Workers triggered to shut down?

    protected:
        static void StartWorkerThreadStub(SCXCoreLib::SCXThreadParamHandle& handle);
        void DoWorkerThread(SCXThreadPoolThreadParam* params);

    public:
        explicit SCXThreadPool( SCXHandle<SCXThreadPoolDependencies> = SCXHandle<SCXThreadPoolDependencies>(new SCXThreadPoolDependencies()) );
        virtual ~SCXThreadPool();
        const std::wstring DumpString() const;

        // Begin: Copy of methods from SCXThreadAttr (scxthread.h)

        void SetWorkerStackSize(const size_t size);

        // End: Copy of methods from SCXThreadAttr (scxthread.h)

        /*-----------------------------------------------------------------------*/
        /**
           Get the number of currently running threads

           \returns     the current number of threads in the thread pool
        */
        long GetThreadCount() { return m_threadCount; }

        /*-----------------------------------------------------------------------*/
        /**
           Get the number of threads allowed to run

           \returns     the maximum number of threads allowed in the thread pool
        */
        long GetThreadLimit() { return m_threadLimit; }

        /*-----------------------------------------------------------------------*/
        /**
           Checks if the worker pool is up and running

           \returns     true if the worker pool is up and running
        */
        bool isRunning() { return m_isRunning && (m_threadCount >= 1); }

        void SetThreadLimit(long limit);
        void QueueTask(SCXThreadPoolTaskHandle task);

        void Start();
        void Shutdown();
    };
} /* namespace SCXCoreLib */

#endif /* SCXTHREADPOOL_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

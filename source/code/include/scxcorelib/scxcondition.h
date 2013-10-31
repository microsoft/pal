/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        scxcondition.h

    \brief       Condition support for SCX.
                 Also helps keep work+sleep time constant in a loop
    
    \date        12-18-2008 15:54:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXCONDITION_H
#define SCXCONDITION_H

#include <scxcorelib/scxexception.h>

#if defined(SCX_UNIX)
#include <pthread.h>
#elif defined(WIN32)
#include <windows.h>
#endif // defined(SCX_UNIX)
#include <string.h>

#include <set>


namespace SCXCoreLib
{
    /**
       Condition support for SCX project.

       In general, on UNIX, a condition should be something like this:

       \ex
       \code
           Lock Mutex
           bool fSleepNeeded=true
           while (! shutdown)
           {
               if (fSleepNeeded)
               {
                   // Compute next sleep time
                   fSleepNeeded = false
               }
               pthread_cond_timedwait
               if timeout
               {
                   fSleepNeeded = true
                   // Do work
               }
           }
           Unlock Mutex
       \endcode

       To signal shutdown, you'd need to do something like:
           Lock mutex
           Set shutdown flag
           Signal Condition
           Unlock mutex

       Basic Elements Needed:
           Lock
           Condition
           Shutdown flag


       In general, on Windows, the equivalent should be something like this:

       \code
           while (! shutdown)
           {
               // Compute next sleep time
               waitforsingleobject
               if timeout
               {
                   // Do work
               }
           }
       \endcode

       To signal shutdown:
           Set shutdown flag
           setevent

       Basic Elements Needed:
           Shutdown flag
           Handle (set for syncronization)



       Using this class, we encapsulate these differences into a contruct like:

       \code
           {
               SCXConditionHandle h(condition);

               while (! predicate)
               {
                   h.Wait();
                   if ( timeout )
                   {
                       // do work
                   }
               }
           }
       \endcode

       The 'predicate' is some condition to check for (i.e. shutdown of the
       thread, for example).  The 'predicate' must be defined by the user of
       this class, and the 'predicate' should be set you wish to signal the
       thread.

       To signal the condition, you should use code like:

       \code
           {
               SCXConditionHandle h(condition);
               // Set predicate to true
               h.Signal();
           }
       \endcode

       This class also helps to keep work+sleep time constant in a loop such
       that the total time == the sleep time.  That is, if we must 'do work'
       once every 5.0 seconds, then this class will help to guarentee that
       'do work' is done once every 5.0 seconds (even in case of delays).
       Note that this works somewhat better on UNIX rather than Windows due
       to low level O/S support but, over time, both platforms should work
       fine (more or less).
        
       The 'Wait()' function will either ensure that we sleep the given number
       of milliseconds, or that we immediately return if signaled.
    */

    class SCXCondition
    {
    public:
        /**
           eConditionResult records the possible return values from SCXCondition.Wait():

           eCondTimeout        Timeout has occurred; do timeout processing code
           eCondTestPredicate  The condition may have been signaled.  Test predicate.

           Note that, if the return value is eCondTestPredicate, you should not assume
           that the predicate is true.  The predicate must always be tested again to
           be certain.  This can be done with code similar to the following (assuming
           that your predicate is m_bShutdown):

           \ex
           \code
           {
               SCXConditionHandle h(condition);
               while (! m_bShutdown)
               {
                   SCXCondition::eConditionResult r = h.Wait();
                   if (SCXCondition::eCondTimeout == r)
                       // Do timeout processing code
               }
           }
           \endcode

           In this way, the predicate is always retested and, if not true, we wait again.
        */
        enum eConditionResult {
            eCondNone = 0,      //!< Initialized value
            eCondTimeout,       //!< Timeout passed - do work and continue
            eCondTestPredicate  //!< Signal? (predicate should be tested)
        };

        SCXCondition();
        virtual ~SCXCondition();

        void SetSleep(scxulong milliseconds);

    protected:
        void BeginCondition();
        void EndCondition();
        enum eConditionResult Wait();
        void Signal();
        void Broadcast();

    private:
        scxulong GetMillisecondTimeStamp();
        void Lock();
        void Unlock();

        // Do not allow copying
        SCXCondition(const SCXCondition &);             //!< Intentionally not implemented
        SCXCondition & operator=(const SCXCondition &); //!< Intentionally not implemented

    private:
#if defined(WIN32)
        typedef HANDLE NativeSynchObject;          //!< Native synchronization object implementation
        typedef CRITICAL_SECTION NativeThreadLock; //!< Native thread lock implementation
#elif defined(SCX_UNIX)
        typedef pthread_cond_t NativeSynchObject;  //!< Native synchronization object implementation
        typedef pthread_mutex_t NativeThreadLock;  //!< Native thread lock implementation
#else
#error Platform not supported
#endif // defined(_WIN32)

        bool m_bSetCalled;        //!< Marks that set has been called.
        scxulong m_sleepTime;     //!< Time to sleep.

        NativeThreadLock m_lock;  //!< Lock to control syncronization
        NativeSynchObject m_cond; //!< Synch object (condition or handle) to wait on
#if defined(SCX_UNIX)
        struct timespec m_ts;     //!< Last time spec fetched from clock_gettime()
#elif defined(WIN32)
        scxulong m_timeStamp;     //!< Timestamp marking the start of the sleep.
#else
        #error Platform not supported
#endif

        friend class SCXConditionHandle;
    };

    /**
       Get a condition handle (and a lock on a condition) to signal or wait for it.

       To manipulate conditions (specifically to wait for them or to signal them),
       you must lock the condition.  SCXConditionHandle will lock a condition and
       allow you to signal or wait for a condition.

       Prior to signaling a condition, you must set the predicate.  This should be
       done using code similar to:

       \ex
       \code
       {
           SCXConditionHandle h(condition);
           condition_predicate = true;
           h.signal()
       }
       \endcode
     */

    class SCXConditionHandle
    {
    public:
/*----------------------------------------------------------------------------*/
/**
    Constructor

    \param       cond   Condition to set up a handler for
*/
        SCXConditionHandle(SCXCondition& cond)
            : m_cond(cond),
              m_isLocked(false)
          { m_cond.BeginCondition(); m_isLocked = true; }

/*----------------------------------------------------------------------------*/
/**
    Destructor
*/
        ~SCXConditionHandle()
        {
            if (m_isLocked)
                m_cond.EndCondition();
        }

/*----------------------------------------------------------------------------*/
/**
    Helper function - wait for a condition (abstracted to insure proper
    calling convention)

    \returns    Result of m_cond.Wait() function (enumerati9on of wait results)
    \throws     SCXInvalidStateException if handle is not currently locked
*/
        enum SCXCondition::eConditionResult Wait()
        {
            if ( !m_isLocked )
                throw SCXInvalidStateException(L"Handle is not currently locked!", SCXSRCLOCATION);

            return m_cond.Wait();
        }

/*----------------------------------------------------------------------------*/
/**
    Helper function - signal a condition (abstracted to insure proper
    calling convention)

    \throws     SCXInvalidStateException if handle is not currently locked
*/
        void Signal()
        {
            if ( !m_isLocked )
                throw SCXInvalidStateException(L"Handle is not currently locked!", SCXSRCLOCATION);

            m_cond.Signal();
        }

/*----------------------------------------------------------------------------*/
/**
    Helper function - broadcast a condition (abstracted to insure proper
    calling convention)

    \throws     SCXInvalidStateException if handle is not currently locked
*/
        void Broadcast()
        {
            if ( !m_isLocked )
                throw SCXInvalidStateException(L"Handle is not currently locked!", SCXSRCLOCATION);

            m_cond.Broadcast();
        }

/*----------------------------------------------------------------------------*/
/**
    Unlock the mutex

    \throws     SCXInvalidStateException if handle is not currently locked
*/
        void Unlock()
        {
            if ( !m_isLocked )
                throw SCXInvalidStateException(L"Handle is not currently locked!", SCXSRCLOCATION);

            m_isLocked = false;
            m_cond.Unlock();
        }

/*----------------------------------------------------------------------------*/
/**
   Lock the mutex

    \throws     SCXInvalidStateException if handle is currently locked
*/
        void Lock()
        {
            if ( m_isLocked )
                throw SCXInvalidStateException(L"Handle is currently locked!", SCXSRCLOCATION);

            m_cond.Lock();
            m_isLocked = true;
        }

    private:
        // Do not allow copying
        SCXConditionHandle(const SCXConditionHandle &); //!< Intentionally not implemented
        SCXConditionHandle & operator=(const SCXConditionHandle &); //!< Intentionally not implemented

        SCXCondition & m_cond;  //!< Reference to condition itself (from constructor)

        bool m_isLocked;        //!< Is the condition currently locked?
    };
}

#endif /* SCXCONDITION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

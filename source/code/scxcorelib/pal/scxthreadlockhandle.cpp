/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief       Implements the thread lock handle PAL.
    
    \date        2007-05-28 10:46:00

*/
/*----------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxhandle.h>
#include <iostream>

#if defined(SCX_UNIX)

#include <pthread.h>
#include <errno.h>

#elif defined(WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400 //!< Needed for TryEnterCriticalSection.
#endif
#include <windows.h>

#else
#error "Not implemented for this plattform"
#endif

namespace
{
#if defined(WIN32)
    typedef CRITICAL_SECTION NativeThreadLock; //!< Native thread lock implementation
    typedef DWORD NativeThreadId;              //!< Native thread id implementation
#elif defined(SCX_UNIX)
    typedef pthread_mutex_t NativeThreadLock; //!< Native thread lock implementation
    typedef pthread_t NativeThreadId;         //!< Native thread id implementation
#endif 
    
    
    /*----------------------------------------------------------------------------*/
    /**
        Allocate and initialize a native thread lock
        \returns    Pointer to native thread lock that eventually needs to be disposed
    */
    SCXCoreLib::SCXHandle<NativeThreadLock> CreateNativeThreadLock(bool allowRecursion)
    {
#if defined(WIN32)
        SCXCoreLib::SCXHandle<NativeThreadLock> lock( new CRITICAL_SECTION );
        InitializeCriticalSection(lock.GetData());
#elif defined(SCX_UNIX)
        SCXCoreLib::SCXHandle<NativeThreadLock> lock( new pthread_mutex_t );

        int r = -1;
        if (allowRecursion)
        {
            pthread_mutexattr_t attr;

            r = pthread_mutexattr_init(&attr);
            SCXASSERT(0 == r);
            if (0 != r)
            {
                throw SCXCoreLib::SCXErrnoException(L"pthread_mutexattr_init", r, SCXSRCLOCATION);
            }
    
            r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            SCXASSERT(0 == r);
            if (0 != r)
            {
                throw SCXCoreLib::SCXErrnoException(L"pthread_mutexattr_settype", r, SCXSRCLOCATION);
            }
    
            r = pthread_mutex_init(lock.GetData(), &attr);
            SCXASSERT(0 == r);
            if (0 != r)
            {
                throw SCXCoreLib::SCXErrnoException(L"pthread_mutex_init (set recursive)", r, SCXSRCLOCATION);
            }
        }
        else
        {
            r = pthread_mutex_init(lock.GetData(), NULL);
        }
        SCXASSERT(0 == r);
        if (0 != r)
        {
            throw SCXCoreLib::SCXErrnoException(L"pthread_mutex_init", r, SCXSRCLOCATION);
        }
#endif
        return lock;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieves id of current thread
        \returns    Native thread id
    */
    NativeThreadId GetCurrentNativeThreadId() 
    {
#if defined(WIN32)
        return GetCurrentThreadId();
#elif defined(SCX_UNIX)
        return pthread_self();
#endif
        
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Free resouces held by a native thread lock
        Since SCXHandle is not thread-safe (add/dec ref are not atomic)
        bare  pointers are used
        \param[in]  lock    To be disposed
    */
    void DisposeNative(SCXCoreLib::SCXHandle<NativeThreadLock>& lock)
    {
        if (NULL != lock)
        {
#if defined(WIN32)
            DeleteCriticalSection(lock.GetData());
#elif defined(SCX_UNIX)
            pthread_mutex_destroy(lock.GetData());
#else
#error "Not implemented for this platform"
#endif
            lock = 0;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Aquire a native thread lock
        \param[in]  lock    To be aquired
    */
    void AquireNative(NativeThreadLock *lock)
    {
#if defined(WIN32)
        EnterCriticalSection(lock);
#elif defined(SCX_UNIX)
        int r = pthread_mutex_lock(lock);
        SCXASSERT(0 == r);
        if (0 != r)
        {
            throw SCXCoreLib::SCXErrnoException(L"pthread_mutex_lock", r, SCXSRCLOCATION);
        }
#else
#error "Not implemented for this platform"
#endif        
    }

    /*----------------------------------------------------------------------------*/
    /**
        Release a native thread lock
        \param[in]  lock    To be released
    */
    void ReleaseNative(NativeThreadLock *lock)
    {
#if defined(WIN32)
        LeaveCriticalSection(lock);
#elif defined(SCX_UNIX)
        int r = pthread_mutex_unlock(lock);
        SCXASSERT(0 == r);
        if (0 != r)
        {
            throw SCXCoreLib::SCXErrnoException(L"pthread_mutex_unlock", r, SCXSRCLOCATION);
        }
#else
#error "Not implemented for this platform"
#endif        
    }
}

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Reference counted implementation of the thread lock handle

    */
    struct SCXThreadLockHandleImpl
    {
        scxulong m_ref; //!< Reference counter for the implementation object.
        SCXCoreLib::SCXHandle<NativeThreadLock> m_refCountLock; //!< Platform representation of lock to prevent
                                                                //!< add(sub)Ref collisions.
        std::wstring m_name; //!< Name of the lock. Must not be changed once it is assingned because then there will
                             //!< be inconsistency between this name and name in the factory.
        int m_lockCount;     //!< Used to detect when a lock is taken more than once (typically relocked)
        SCXCoreLib::SCXHandle<NativeThreadLock> m_lock; //!< Platform representation of the lock
        bool m_lockIsRecursive; //!< Flag indicating if lock is recursive.
        NativeThreadId m_threadID;         //!< Platform representation of the holding thread ID

        friend class SCXThreadLockHandle;


        /*----------------------------------------------------------------------------*/
        /**
            Default constructor.

        */
        SCXThreadLockHandleImpl(bool allowRecursion)
            : m_ref(0)
            , m_refCountLock(NULL)
            , m_name(L"")
            , m_lockCount(0)
            , m_lock(NULL)
            , m_lockIsRecursive(false)
            , m_threadID(0)
        {
            m_lock = CreateNativeThreadLock(allowRecursion);
            m_refCountLock = CreateNativeThreadLock(allowRecursion);
            m_lockIsRecursive = allowRecursion;
            AddRef();
        }

    private:
        /*----------------------------------------------------------------------------*/
        /**
            Private copy constructor.

        */
        SCXThreadLockHandleImpl(SCXThreadLockHandleImpl&);
    public:

        /*----------------------------------------------------------------------------*/
        /**
            Destructor
        */
        ~SCXThreadLockHandleImpl()
        {
            SCXASSERT(0 == m_ref);
            DisposeNative(m_lock);
            DisposeNative(m_refCountLock);
        }

        /*----------------------------------------------------------------------------*/
        /**
            Increase reference counter.
        */
        void AddRef(void) 
        { 
            AquireNative(m_refCountLock.GetData());
            ++m_ref;
            ReleaseNative(m_refCountLock.GetData());
        }

        /*----------------------------------------------------------------------------*/
        /**
            Decrease reference counter and delete if last reference.
        */
        void Release(void) 
        {
            AquireNative(m_refCountLock.GetData());
            bool noReferencesLeft = (--m_ref == 0);
            ReleaseNative(m_refCountLock.GetData());
            // Assumes AddRef will not be called to invalidate "m_ref == 0"
            // AddRef is only called when an instance having a reference is copied.
            // If m_ref == 0 then there is no instance to be copied and 
            // therefore "AddRef" cannot be called
            if (noReferencesLeft)       
            {                
                delete this; 
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
            Indicates if lock is recursive.

            \returns      true if lock is recursive, otherwise false.
        */
        bool IsRecursive(void) const
        {
            return m_lockIsRecursive;
        }
    };

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.

        Will create an invalid SCXThreadLockHandle without implementation.
        Currently this is mainly used in unit test, but may be used in future
        to provide implementations created outside this class.

    */
    SCXThreadLockHandle::SCXThreadLockHandle(void)
        : m_pImpl(NULL), m_residesInFactory(false)
    {
    }

    /*-------------------------------------------------------------------*/
    /**
        Create a lock handle with name.

        \param[in]  lockName Name to create.
        \param[in]  allowRecursion if true sets lock to be recursive.

        Creates a lock implementation and assigns the given name. Typically anonymous
        locks will have empty name

    */
    SCXThreadLockHandle::SCXThreadLockHandle(const std::wstring& lockName, bool allowRecursion)
        : m_pImpl(NULL)
        , m_residesInFactory(false)
    {
        m_pImpl = new SCXThreadLockHandleImpl(allowRecursion);
        m_pImpl->m_name = lockName;
    }

    /*-------------------------------------------------------------------*/
    /**
        Copy constructor.

        \param[in]  other SCXThreale to copy.

        Will create copy using the same implementation object as the argument.

    */
    SCXThreadLockHandle::SCXThreadLockHandle(const SCXThreadLockHandle& other)
        : m_pImpl(other.m_pImpl)
        , m_residesInFactory(other.m_residesInFactory)
    {
        if (NULL != m_pImpl)
        {
            m_pImpl->AddRef();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.

        Will release the allocated resources (using reference counting).

    */
    SCXThreadLockHandle::~SCXThreadLockHandle(void)
    {
        if (NULL != m_pImpl) 
        {
            if (m_residesInFactory)
            {
                // Factory is being destroyed, reset or the last named lock is with a particular name is beeing
                // destroyed so the lock handle of same name held by the factory is also beeing destroyed.
                m_pImpl->Release();
            }
            else
            {
                // Handle does not reside in the factory. The actual lock is beeing destroyed. First remove the name
                // from the factory if this is the last lock using it. Locks in the factory can not have empty name.
                if(m_pImpl->m_name.empty() == false)
                {
                    // It is possible that a lock handle with a particular name was created directly and that
                    // global named lock of a same name also exists in the factory. That's why it is not enough to
                    // search the factory by the name only but we must also verify the m_pImpl pointer.
                    SCXThreadLockFactory::GetInstance().RemoveIfLastOne(m_pImpl->m_name, m_pImpl);
                }

                // Finally release the lock.
                m_pImpl->Release();
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns      Object represented as string for logging.

    */
    const std::wstring SCXThreadLockHandle::DumpString() const
    {
        std::wstring str;
        if (NULL == m_pImpl)
        {
            str = L"SCXThreadLockHandle invalid";
        }
        else
        {
            str = L"SCXThreadLockHandle(" + m_pImpl->m_name + L") is " + ((m_pImpl->m_lockCount>0)?L"LOCKED":L"unlocked");
            str += L" m_lockCount=" + SCXCoreLib::StrFrom(m_pImpl->m_lockCount);
            str += L" m_ref=" + SCXCoreLib::StrFrom(m_pImpl->m_ref);
            str += L" m_lockIsRecursive=" + SCXCoreLib::StrFrom(m_pImpl->IsRecursive());
        }
        return str;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Assignment operator.

        \param[in]  other SCXThreadLockHandle object to copy
        \returns    Reference to it self (SCXThreadLockHandle).

        Will the replace the current implementation with that of the given argument.

    */
    SCXThreadLockHandle& SCXThreadLockHandle::operator= (const SCXThreadLockHandle& other)
    {
        if (this != &other)
        {
            m_residesInFactory = other.m_residesInFactory;
            if (m_pImpl != other.m_pImpl)
            {
                if (NULL != m_pImpl)
                {
                    m_pImpl->Release();
                }
                m_pImpl = other.m_pImpl;
                if (NULL != m_pImpl)
                {
                    m_pImpl->AddRef();
                }
            }
        }
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Aquire lock.

        \throws   SCXThreadLockInvalidException If there is no implementation object.
        \throws   SCXThreadLockHeldException If the lock is already held by the calling thread.

        Will block indefinitly until lock can be aquired by calling thread.

    */
    void SCXThreadLockHandle::Lock(void)
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        if (NULL == m_pImpl->m_lock)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"Invalid lock handle", SCXSRCLOCATION);
        }
        if (!IsRecursive() && HaveLock())
        {
            throw SCXThreadLockHeldException(m_pImpl->m_name, SCXSRCLOCATION);
        }
        AquireNative(m_pImpl->m_lock.GetData());
        ++m_pImpl->m_lockCount;
        m_pImpl->m_threadID = GetCurrentNativeThreadId();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Release lock.

        \throws  SCXThreadLockInvalidException If there is no implementation object.
        \throws  SCXThreadLockNotHeldException If the lock is not held by the calling thread.

        Will release the lock immediatly if held by calling thread.

    */
    void SCXThreadLockHandle::Unlock(void)
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        if (NULL == m_pImpl->m_lock)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"Invalid lock handle", SCXSRCLOCATION);
        }

        if (!HaveLock())
        {
            throw SCXThreadLockNotHeldException(m_pImpl->m_name, SCXSRCLOCATION);
        }

        --m_pImpl->m_lockCount;
        if (m_pImpl->m_lockCount == 0)
        {
            m_pImpl->m_threadID = 0;
        }
        ReleaseNative(m_pImpl->m_lock.GetData());
    }

    /*----------------------------------------------------------------------------*/
    /**
        Try to aquire a lock.

        \param[in] timeout Will try to aquire lock no longer that given number of miliseconds.
        \returns   true if the lock could be aquired, otherwise false.
        \throws    SCXNotSupportedException If called with timeout other than zero.
        \throws    SCXThreadLockInvalidException If there is no implementation object.
        \throws    SCXThreadLockHeldException If the lock is already held by the calling thread.

        Only zero timeouts supported due to WI#570 not implemented.

    */
    bool SCXThreadLockHandle::TryLock(const unsigned int timeout /*= 0*/)
    {
        if (0 != timeout)
        {
            throw SCXCoreLib::SCXNotSupportedException(L"Non-zero timeout value:" + SCXCoreLib::StrFrom(timeout), SCXSRCLOCATION);
        }
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        if (NULL == m_pImpl->m_lock)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"Invalid lock handle", SCXSRCLOCATION);
        }
        if (!IsRecursive() && HaveLock())
        {
            throw SCXThreadLockHeldException(m_pImpl->m_name, SCXSRCLOCATION);
        }
#if defined(WIN32)
        BOOL gotLock = TryEnterCriticalSection(m_pImpl->m_lock.GetData());
        if ( ! gotLock)
        {
            return false;
        }
#elif defined(SCX_UNIX)
        int r = pthread_mutex_trylock(m_pImpl->m_lock.GetData());
        if (r == EBUSY)
        {
            return false;
        }
        SCXASSERT(0 == r);
#else
#error "Not implemented for this plattform"
#endif
        ++m_pImpl->m_lockCount;
        m_pImpl->m_threadID = GetCurrentNativeThreadId();
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if a lock is held by the calling thread.

        \returns  true if the lock is held by the calling thread, otherwise false.
        \throws   SCXThreadLockInvalidException If there is no implementation object.

        May be used to check if a lock is held or not before trying to aquire it.

        \note Even though m_lockCount and m_threadID are used without locking any resource
        this is not a problem. If HaveLock is called by the holding thread, there cannot
        be a race condition regarding these properties. And if called by a non-holding 
        thread it will return false under any circumstances which is correct.

    */
    bool SCXThreadLockHandle::HaveLock(void) const
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        if (NULL == m_pImpl->m_lock)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"Invalid lock handle", SCXSRCLOCATION);
        }
        if (m_pImpl->m_lockCount > 0)
        {
            if (m_pImpl->m_threadID == GetCurrentNativeThreadId())
            {
                return true;
            }
        }
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if a lock is set to be recursive.

        \returns  true if the lock is set to be recursive, otherwise false.
        \throws   SCXThreadLockInvalidException If there is no implementation object.

    */
    bool SCXThreadLockHandle::IsRecursive(void) const
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        if (NULL == m_pImpl->m_lock)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"Invalid lock handle", SCXSRCLOCATION);
        }
        return m_pImpl->IsRecursive();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if a lock is held by any thread.

        \returns true if the lock is held by any thread, otherwise false.
        \throws  SCXThreadLockInvalidException If there is no implementation object.

        May be used to check if a lock is held or not before trying to aquire it.

        \note Even though m_lockCount is used without locking any resource
        this is not a problem. Since this method does not try to aquire the lock
        there will always be a race condition and there is no way to make a difference
        between when the race condition is between lock handling and lock count update
        or if the difference is due to other race conditions.
        
    */
    bool SCXThreadLockHandle::IsLocked(void) const
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        if (NULL == m_pImpl->m_lock)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"Invalid lock handle", SCXSRCLOCATION);
        }
        if (m_pImpl->m_lockCount > 0)
        {
            return true;
        }
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the lock name.

        \returns Name of the lock (empty for anonymous locks).
        \throws  SCXThreadLockInvalidException If there is no implementation object.

        Typically used for logging purposes.

    */
    const std::wstring& SCXThreadLockHandle::GetName(void) const
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        return m_pImpl->m_name;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the implementation reference counter.

        \returns Current reference counter of the implementation object.
        \throws SCXThreadLockInvalidException If there is no implementation object.

        This method is typically only used by the thread lock factory GetUsedLocks method.

    */
    scxulong SCXThreadLockHandle::GetRefCount(void) const
    {
        if (NULL == m_pImpl)
        {
            throw SCXThreadLockInvalidException(L"N/A", L"No implementation set", SCXSRCLOCATION);
        }
        return m_pImpl->m_ref;
    }

} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

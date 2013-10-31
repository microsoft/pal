/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief       Defines the public interface for the thread lock PAL.

    \date        2007-05-28 08:24:00

  
    \note that implementation does not support abandoned locks (TI#569),
       trylocks with time outs (TI#570). Nor may a locked be locked
       again by the holding thread (generates exception) unless lock is set to
       be recursive. A lock may not be released if not held by the calling thread.

    Thread locks are created using a threadlock factory which is a singleton
    class which returns a thread lock handle. The thread lock handle contains
    the plattform specific implementation for the lock. Lock handles may
    be named (the factory keeps a list of all used named locks) or 
    anonymous (in which case the caller must keep track of the lock handle
    himself. The thread lock handle should be used in a lock class (which
    uses RAII pattern).

    \ex
    \code
    SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockFactory(L"TestLock"));
    \endcode

    \note Using empty (L"") lock names is the same thing as using an anonymous lock.
    \ex
    \code
    SCXCoreLib::SCXThreadLock lock1(SCXCoreLib::ThreadLockFactory(L""));
    SCXCoreLib::SCXThreadLock lock3(SCXCoreLib::ThreadLockFactory(L""));
    \endcode
    lock1 and lock2 are not using the same lock handles i.e. not the same lock.
*/
/*----------------------------------------------------------------------*/
#ifndef SCXTHREADLOCK_H
#define SCXTHREADLOCK_H

#include <string>
#include <map>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockHandleImpl contains plattform specific details for a thread lock.

        \date          2007-05-28 08:24:00

        This struct is just declared here to avoid having platform specific
        details in this header file.

    */
    struct SCXThreadLockHandleImpl;

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockHandle implements a platform independant thread lock handle.

        \date          2007-05-28 08:24:00

        This class is used to abstract thread lock instances.

    */
    class SCXThreadLockHandle
    {
        friend class SCXThreadLockFactory;

    private:
        SCXThreadLockHandleImpl* m_pImpl; //!< Contains plattform specific details for a thread lock.
        // ATTENTION, m_residesInFactory must be set to true only for those objects that are actually members of the
        // factory collection. When copying SCXThreadLockHandle objects through constructors or = operators
        // always make sure this flag is copied over unmodified. Keep in mind that STL collections, like one in the
        // factory, may copy over this object internaly so if flag is not copied properly the code may malfunction.
        bool m_residesInFactory; //!< Flag indicating if this object resides in the global factory of named locks.
    public:
        SCXThreadLockHandle(void);
        SCXThreadLockHandle(const std::wstring& lockName, bool allowRecursion = false);
        SCXThreadLockHandle(const SCXThreadLockHandle& other);
        virtual ~SCXThreadLockHandle(void);
        const std::wstring DumpString() const;

        SCXThreadLockHandle& operator= (const SCXThreadLockHandle& other);

        void Lock(void);
        void Unlock(void);
        bool TryLock(const unsigned int timeout = 0);
        bool HaveLock(void) const;
        bool IsLocked(void) const;
        bool IsRecursive(void) const;

        const std::wstring& GetName(void) const;
        scxulong GetRefCount(void) const;
    }; /* class SCXThreadLockHandle */

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLock implements a RAII pattern locking mechanism.

        \date          2007-05-28 08:24:00

        The RAII pattern is used to simplify lock handling.

    */
    class SCXThreadLock
    {
    protected:
        SCXThreadLockHandle m_lock; //!< The actual lock.

    public:
        SCXThreadLock(const SCXThreadLockHandle& handle, bool aquire = true);
        SCXThreadLock(const std::wstring& nameOfLock, bool aquire = true, bool allowRecursion = false);
        virtual ~SCXThreadLock(void);
        const std::wstring DumpString() const;

        void Lock(void);
        void Unlock(void);
        bool TryLock(const unsigned int timeout = 0);
        bool HaveLock(void) const;
        bool IsLocked(void) const;
        bool IsRecursive(void) const;

    private:
        /** Private default constructor */
        SCXThreadLock(void);
        /** Private copy constructor */
        SCXThreadLock(SCXThreadLock&);
        /** Private assignment operator */
        SCXThreadLock& operator= (SCXThreadLock&);
    }; /* class SCXThreadLock */

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockFactory implements a SCXThreadLockHandle factory.

        \date          2007-05-28 08:24:00

        This singelton class is used to create named thread locks.

    */
    class SCXThreadLockFactory
    {
        friend class SCXThreadLockHandle;
    protected:
        static SCXThreadLockFactory *s_instance; //!< Singleton instance.
        std::map<std::wstring,SCXThreadLockHandle> m_locks; //!< Contains all named locks.
        SCXThreadLockHandle  m_lockHandle; //!< lock used internally in the factory.

        SCXThreadLockFactory(void);

        void RemoveIfLastOne(const std::wstring& nameOfLock, SCXThreadLockHandleImpl* pImpl);
    public:
        virtual ~SCXThreadLockFactory(void);
        const std::wstring DumpString() const;

        static SCXThreadLockFactory& GetInstance(void);

        // Note: We can't take a bool because that can be interpreted as an L"LockName"
        SCXThreadLockHandle GetLock(const int allowRecursion);
        inline SCXThreadLockHandle GetLock(void) { return GetLock(0 /* false (as int) */); };
        SCXThreadLockHandle GetLock(const std::wstring&, const bool allowRecursion);
        inline SCXThreadLockHandle GetLock(const std::wstring& nameOfLock) { return GetLock(nameOfLock, false); }

        unsigned int GetLocksUsed(void) const;
        size_t GetLockCnt(void) const;
    protected:
        void Reset(void);

    private:
        /** Private copy constructor. */
        SCXThreadLockFactory(SCXThreadLockFactory&);
        /** Private assignment operator. */
        SCXThreadLockFactory& operator= (SCXThreadLockFactory&);
    }; /* class SCXThreadLockFactory */

    // Note: We can't take a bool because that can be interpreted as an L"LockName"
    SCXThreadLockHandle ThreadLockHandleGet(int allowRecursion);
    inline SCXThreadLockHandle ThreadLockHandleGet(void) { return ThreadLockHandleGet(0 /* false (as int) */); }
    SCXThreadLockHandle ThreadLockHandleGet(const std::wstring& nameOfLock, bool allowRecursion);
    inline SCXThreadLockHandle ThreadLockHandleGet(const std::wstring& nameOfLock) { return ThreadLockHandleGet(nameOfLock, false); }

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockException is a base exception for all thread lock exceptions.

        \date          2007-05-28 15:40:00

        Provide a common base for all thread lock exceptions.
        
    */
    class SCXThreadLockException : public SCXException
    {
    public: 
        /*----------------------------------------------------------------------*/
        /**
            Constructor
            
            \param[in] lockName Name of lock.
            \param[in] reason Reason of exception.
            \param[in] l Source code location.

        */
        SCXThreadLockException(const std::wstring& lockName, 
                               const std::wstring& reason,
                               const SCXCodeLocation& l) 
           : SCXException(l), m_LockName(lockName), m_Reason(reason) {};
                                                         
        virtual std::wstring What() const { return L"Lock '" + m_LockName + L"': " + m_Reason; }

    protected:
        std::wstring   m_LockName; //!< Name of lock.
        std::wstring   m_Reason;   //!< Reason of exception.
    }; /* class SCXThreadLockException */

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockHeldException is thrown when a thread tries to aquire a lock
        already held by the thread.

        \date          2007-05-28 15:40:00

        A thread should not be able to lock the same lock twice without
        first releasing it. This exception is thrown when this occours.

    */
    class SCXThreadLockHeldException : public SCXThreadLockException
    {
    public: 
        /*----------------------------------------------------------------------*/
        /**
            Constructor
            
            \param[in] lockName Name of lock.
            \param[in] l Source code location.

        */
        SCXThreadLockHeldException(const std::wstring& lockName, 
                                   const SCXCodeLocation& l) 
           : SCXThreadLockException(lockName, L"already held by thread", l) {};
                                                         
    }; /* class SCXThreadLockHeldException */

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockNotHeldException is thrown when a thread tries to release 
        a lock not held by the thread.

        \date          2007-06-04 08:37:00

        A thread should not be able to unlock a lock not held by itself.

    */
    class SCXThreadLockNotHeldException : public SCXThreadLockException
    {
    public: 
        /*----------------------------------------------------------------------*/
        /**
            Constructor
            
            \param[in] lockName Name of lock.
            \param[in] l Source code location.

        */
        SCXThreadLockNotHeldException(const std::wstring& lockName, 
                                      const SCXCodeLocation& l) 
           : SCXThreadLockException(lockName, L"not held by thread", l) {};
                                                         
    }; /* class SCXThreadLockNotHeldException */

    /*----------------------------------------------------------------------*/
    /**
        SCXThreadLockInvalidException is thrown when a lock operation cannot
        be completed since the object is invalid for some reason.

        \date          2007-06-04 08:17:00

        Used to signal object invalidity if trying to perform lock
        operations on an object not properly initialized.

    */
    class SCXThreadLockInvalidException : public SCXThreadLockException
    {
    public: 
        /*----------------------------------------------------------------------*/
        /**
            Constructor
            
            \param[in] lockName Name of lock.
            \param[in] reason Reason of exception.
            \param[in] l Source code location.

        */
        SCXThreadLockInvalidException(const std::wstring& lockName,
                                      const std::wstring& reason,
                                      const SCXCodeLocation& l) 
           : SCXThreadLockException(lockName, reason, l) {};
                                                         
    }; /* class SCXThreadLockInvalidException */

} /* namespace SCXCoreLib */

#endif /* SCXTHREADLOCK_H */

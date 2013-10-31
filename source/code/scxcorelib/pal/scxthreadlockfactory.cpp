/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief         Implements the thread lock factory PAL.

    \date          2007-05-28 10:46:00
  
*/
/*----------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib
{
    SCXThreadLockFactory *SCXThreadLockFactory::s_instance = NULL;

/*----------------------------------------------------------------------------*/
/**
    Convenience function to access the thread lock factory and get an anonymous
    lock handle.
    
    Parameters:  allowRecursion - true or false (if recursive lock is allowed).
    Retval:      An anonymous SCXThreadLockHandle.
    
*/
    SCXThreadLockHandle ThreadLockHandleGet(int allowRecursion)
    {
        return SCXThreadLockFactory::GetInstance().GetLock(allowRecursion);
    }

/*----------------------------------------------------------------------------*/
/**
    Convenience function to access the thread lock factory and get a named
    lock handle.
    
    Parameters:  nameOfLock - name of lock handle to get.
                 allowRecursion - true or false (if recursive lock is allowed).
    Retval:      A SCXThreadLockHandle associated with the given name.
        
    Note that calling this method with an empty string generates the same result
    as calling ThreadLockFactory(void). Use IsRecursive() method to verify that
    lock was successfully set to the recursive mode.
    
*/
    SCXThreadLockHandle ThreadLockHandleGet(const std::wstring& nameOfLock, bool allowRecursion)
    {
        return SCXThreadLockFactory::GetInstance().GetLock(nameOfLock, allowRecursion);
    }

/*----------------------------------------------------------------------------*/
/**
    Default constructor.
    
    Parameters:  None
    Retval:      N/A
        
    Creates a single anonymous lock handle to be used as thread lock to make the
    factory class thread safe.
    
*/
    SCXThreadLockFactory::SCXThreadLockFactory(void):
        m_lockHandle(L"")
    {
        Reset();
    }

/*----------------------------------------------------------------------------*/
/**
    Virtual destructor.
    
    Parameters:  None
    Retval:      N/A
        
    Releases all allocated resources.
    
*/
    SCXThreadLockFactory::~SCXThreadLockFactory(void)
    {
        delete s_instance;
        s_instance = NULL;
    }

/*----------------------------------------------------------------------------*/
/**
    Dump object as string (for logging).
    
    Parameters:  None
    Retval:      N/A
        
*/
    const std::wstring SCXThreadLockFactory::DumpString() const
    {
        SCXThreadLock lock(m_lockHandle);

        std::wstring str = L"SCXThreadLockFactory locks=" +
                SCXCoreLib::StrFrom(static_cast<scxlong>(m_locks.size())) + L'\n';
        std::map<std::wstring,SCXThreadLockHandle>::const_iterator it;
        for (it = m_locks.begin(); it != m_locks.end(); it++)
        {
            str += L"  " + it->first + L" " + it->second.DumpString() + L'\n';
        }
        return str;
    }

/*----------------------------------------------------------------------------*/
/**
    Static method to retrieve singleton instance.
    
    Parameters:  None
    Retval:      Singleton instance of factory class.
        
*/
    SCXThreadLockFactory& SCXThreadLockFactory::GetInstance(void)
    {
        if (!s_instance)
        {
            s_instance = new SCXThreadLockFactory();
        }
        return *s_instance;
    }

/*----------------------------------------------------------------------------*/
/**
    Create an anonymous thread lock handle.
    
    Parameters:  allowRecursion - true or false (if recursive lock is allowed).
    Retval:      An anonymous SCXThreadLockHandle object.

*/
    SCXThreadLockHandle SCXThreadLockFactory::GetLock(const int allowRecursion)
    {
        SCXThreadLockHandle l(L"", allowRecursion);
        return l;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve a named thread lock handle.
    
    Parameters:  nameOfLock - Name of lock to retrieve.
                 allowRecursion - true or false (if recursive lock is allowed).
    Retval:      A SCXThreadLockHandle associated with the given name.
        
    Using this method with an empty lock name will create an anonymous lock handles.
    If name is non-empty the internal list is searched for the given name. The
    found lock handle is returned if one is found. Otherwise a new lock handle
    is created and added to the list. Use IsRecursive() method to verify that
    lock was successfully set to the recursive mode.
    
*/
    SCXThreadLockHandle SCXThreadLockFactory::GetLock(const std::wstring& nameOfLock, const bool allowRecursion)
    {
        // If name is L"" it can not be global named lock. Do not modify this behaviour, other code depends on this.
        if (nameOfLock.empty())
        {
            return GetLock(allowRecursion);
        }

        // Name is valid, lock the factory and find or generate new global named lock.
        SCXThreadLock lock(m_lockHandle);

        const std::map<std::wstring,SCXThreadLockHandle>::iterator item = m_locks.find(nameOfLock);

        if (item != m_locks.end())
        {
            // Make copy of the lock to be returned from the factory.
            SCXThreadLockHandle l = item->second;
            // Before returning the lock, mark the lock as not residing in the factory. Locks not residing in the
            // factory notify the factory when they are destroyed so factory can free the global name if necessary.
            l.m_residesInFactory = false;
            // Unlock the factory just before returning. Temporary lock handle that is beeing returned will be
            // copied over and then destroyed. Every destruction of a handle triggers a call to a factory to
            // update itself if necessary, and that requires factory to be locked. That is why the factory must be
            // unlocked before the temporary lock handle is returned, or otherwise the calling thread will be deadlocked.
            lock.Unlock();
            return l;
        }
        // Named lock was not found, create a new one.
        SCXThreadLockHandle l(nameOfLock, allowRecursion);
        // Mark the lock as residing in the factory and add it to the collection of global named locks held by the
        // factory.
        l.m_residesInFactory = true;
        m_locks[nameOfLock] = l;
        // Before returning the lock mark it as not residing in the factory, as explained above.
        l.m_residesInFactory = false;
        // Unlock the factory just before returning, as explained above.
        lock.Unlock();
        return l;
    }

/*----------------------------------------------------------------------------*/
/**
    Updates the factory by removing global named locks not in use any more.
    
    Parameters:  nameOfLock - name of lock handle to be removed if it's not in use.
                 pImpl - lock handle implementation pointer.
    Retval:      None
        
    Called by every named lock handle not residing in the factory while beeing destroyed. Function checks if
    this is the last lock handle using the name, and if so removes the name from the factory.
    
*/
    void SCXThreadLockFactory::RemoveIfLastOne(const std::wstring& nameOfLock, SCXThreadLockHandleImpl* pImpl)
    {
        SCXThreadLock lock(m_lockHandle);

        // Check if name is in the factory collection. There can be named locks that are not in the factory if
        // lock handles were created directly. Also, it is possible that a named lock of a particular name created
        // directly and a global named lock of a same name exists in a factory. In this case additional parameter
        // pImpl must be used to determine if name should be removed from the factory.
        const std::map<std::wstring,SCXThreadLockHandle>::iterator item = m_locks.find(nameOfLock);
        if (item != m_locks.end() && item->second.m_pImpl == pImpl && item->second.GetRefCount() == 2)
        {
            // Name matches, pImpl matches and reference count is 2 which means only two instances of the lock
            // handle remain. One beeing destroyed and one kept by the factory. Remove the named lock from the factory.
            m_locks.erase(item);
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Reset the factory.
    
    Parameters:  None
    Retval:      None
        
    Will remove all references to any previously created locks. In practice this
    method is probably only usable to ease memory leak detection.
    
*/
    void SCXThreadLockFactory::Reset(void)
    {
        SCXThreadLock lock(m_lockHandle);

        m_locks.clear();
    }

/*----------------------------------------------------------------------------*/
/**
    Get the number of used locks.
    
    Parameters:  None
    Retval:      Number of lock handles in used.
        
    Will traverse the lock handle list in order to count number of used locks.
    A lock is considered to be in use if its reference counter is not one since
    one reference is that in the factory lock handle list.
    
*/
    unsigned int SCXThreadLockFactory::GetLocksUsed(void) const
    {
        SCXThreadLock lock(m_lockHandle);

        unsigned int r = 0;
        std::map<std::wstring,SCXThreadLockHandle>::const_iterator cur = m_locks.begin();
        std::map<std::wstring,SCXThreadLockHandle>::const_iterator end = m_locks.end();

        for ( ; cur != end; ++cur)
        {
            if (cur->second.GetRefCount() > 1)
            { /* Ref count should be one for unused locks since they are in the list (=one reference) */
                ++r;
            }
        }
        return r;
    }

/*----------------------------------------------------------------------------*/
/**
    Get number of global named locks.

    Parameters:  None
    Retval:      Number of global named locks.
*/
    size_t SCXThreadLockFactory::GetLockCnt(void) const
    {
        SCXThreadLock lock(m_lockHandle);
        return m_locks.size();
    }

} /* namespace SCXCoreLib */

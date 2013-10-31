/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief       Implements the thread lock PAL.

    \date        2007-05-28 10:46:00

*/
/*----------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxthreadlock.h>

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    Constructor to create lock using given lock handle.
    
    Parameters:  handle - SCXThreadLockHandle reference to use as lock.
                 aquire - If true (default) the lock will be aquired in constructor.
    Retval:      N/A
        
    Default behaviour is to aquire the lock, but it may be controled using parameter.
    
*/
    SCXThreadLock::SCXThreadLock(const SCXThreadLockHandle& handle, bool aquire /*= true*/)
        : m_lock(handle)
    {
        if (aquire)
            Lock();
    }

/*----------------------------------------------------------------------------*/
/**
    Create a lock using name.
    
    Parameters:  nameOfLock - Name of lock to use.
                 aquire - If true (default) the lock will be aquired in constructor.
                 allowRecursion - true or false (if recursive lock is allowed).
    Retval:      N/A
        
    Default behaviour is to aquire the lock, but it may be controled using parameter.
    Will use name to retrieve lock handle in thread lock factory. An empty name is not
    a named lock but rather an anonymous lock. Use IsRecursive() method to verify that
    lock was successfully set to the recursive mode.
    
*/
    SCXThreadLock::SCXThreadLock(const std::wstring& nameOfLock, bool aquire /*= true*/, bool allowRecursion /* = false */)
    {
        m_lock = SCXThreadLockFactory::GetInstance().GetLock(nameOfLock, allowRecursion);
        if (aquire)
            Lock();
    }
    
/*----------------------------------------------------------------------------*/
/**
    Virtual destructor.
    
    Parameters:  None
    Retval:      N/A
        
    If the lock is held it will be unlocked. If the lock handle is invalid,
    this method ignores it.
    
*/
    SCXThreadLock::~SCXThreadLock(void)
    {
        try
        {
            if (HaveLock())
            {
                Unlock();
            }
        }
        catch (SCXThreadLockInvalidException&)
        {
            /* HaveLock may throw exception when deleting invalid locks. */
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Dump object as string (for logging).
    
    Parameters:  None
    Retval:      N/A
        
*/
    const std::wstring SCXThreadLock::DumpString() const
    {
        return L"SCXThreadLock=" + m_lock.DumpString();
    }

/*----------------------------------------------------------------------------*/
/**
    Explicitly aquire lock.
    
    Parameters:  None
    Retval:      None
        
    Will block until lock can be aquired. Should not be called if lock is 
    already aquired.
    
*/
    void SCXThreadLock::Lock(void)
    {
        m_lock.Lock();
    }

/*----------------------------------------------------------------------------*/
/**
    Explicitly release lock.
    
    Parameters:  None
    Retval:      None
        
    Will release the lock immediatly. Should not be called if lock is not aquired.
    
*/
    void SCXThreadLock::Unlock(void)
    {
        m_lock.Unlock();
    }

/*----------------------------------------------------------------------------*/
/**
    Try to aquire lock (using a timeout).
    
    Parameters:  timeout - Number of miliseconds (default zero) the method may attempt
                           to aquire the lock before it must return.
    Retval:      true if the lock could be aquired within the given timeout,
                 otherwise false.
        
    This method can be used to try to aquire a lock without blocking.
    
*/
    bool SCXThreadLock::TryLock(const unsigned int timeout /*= 0*/)
    {
        return m_lock.TryLock(timeout);
    }

/*----------------------------------------------------------------------------*/
/**
    Used to check if a lock is aquired already by the calling thread.
    
    Parameters:  None
    Retval:      true if the calling thread is the owner of the lock, otherwise false.
        
    May be used to check if lock is held or not without aquiring it or risking
    a "LockHeld" exception.
    
*/
    bool SCXThreadLock::HaveLock(void) const   
    {
        return m_lock.HaveLock();
    }

/*----------------------------------------------------------------------------*/
/**
    Check if a lock is set to be recursive.

    Parameters:  None
    Retval:  true if the lock is set to be recursive, otherwise false.

*/
    bool SCXThreadLock::IsRecursive(void) const   
    {
        return m_lock.IsRecursive();
    }

/*----------------------------------------------------------------------------*/
/**
    Used to check if any thread has the lock aquired.
    
    Parameters:  None
    Retval:      true if any thread is the owner of the lock, otherwise false.
        
    Created:     07-07-02 14:30:00
    
    May be used to check if a lock is held at all without aquiring it or risking
    a "LockHeld excpetion.
    
*/
    bool SCXThreadLock::IsLocked(void) const
    {
        return m_lock.IsLocked();
    }

} /* namespace SCXCoreLib */

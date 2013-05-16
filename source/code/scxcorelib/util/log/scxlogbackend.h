/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Contains the definition of the log mediator pure virtual class.

    \date        2008-07-18 11:33:43

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGBACKEND_H
#define SCXLOGBACKEND_H

#include <scxcorelib/scxlog.h>
#include "scxlogseverityfilter.h"
namespace SCXCoreLib
{
    class SCXLogItem;

    /*----------------------------------------------------------------------------*/
    /**
        An SCXLogBackend is a special kind log item consumer that is used to
        output log items to different kinds of media.
        
        The backend can be configured using key - value pairs. It is up to
        implementing class what to do with these pairs.

        The backend also has an associated severity filter which can be modified
        using the methods in this class.
    */
    class SCXLogBackend : public SCXLogItemConsumerIf
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Defaul constructor.
        */
        SCXLogBackend() :
            m_lock(ThreadLockHandleGet())
        {
        }

        /*----------------------------------------------------------------------------*/
        /**
            Constructor that injects a thread lock handle.

            \param[in] lock Used to inject a thread lock handle.
        */
        SCXLogBackend(const SCXThreadLockHandle& lock) :
            m_lock(lock)
        {
        }

        /**
            The backend can be configured using key - value pairs. It is up to
            implementing class what to do with these pairs.
            
            \param[in] key Name of property to set.
            \param[in] value Value of property to set.
         */
        virtual void SetProperty(const std::wstring& key, const std::wstring& value) = 0;
        
        /**
            Once an implementation of the backend has recieved all configuration
            it needs it should return true when this method is called.

            \returns true if backend is fully configured and ready to operate.
         */
        virtual bool IsInitialized() const = 0;

        /**
            This implementation checks with the filter if this item
            should be logged. If it should, then it is sent to the (pure virtual)
            DoLogItem method.
            
            \param[in] item Item to send to log.
         */
        virtual void LogThisItem(const SCXLogItem& item)
        {
            SCXThreadLock lock(m_lock);
            if (m_SeverityFilter.IsLogable(item))
            {
                DoLogItem(item);
            }
        }

        /**
            Get the effective severity for a particular log module.
            
            This method enables us to make logging more efficient by giving us the
            possibility to do upstream filtering.

            \param[in] module Log module to retrieve severity for.
            \returns Effective severity for log module.

        */
        virtual SCXLogSeverity GetEffectiveSeverity(const std::wstring& module) const
        {
            SCXThreadLock lock(m_lock);
            return m_SeverityFilter.GetSeverityThreshold(module);
        }

        /**
            Set severity threshold for a module.

            \param[in] module Module to set severity threshold for.
            \param[in] severity The severity threshold to set.
            \returns true if the severity filter was actually changed as a result of
            this method call.
         */
        virtual bool SetSeverityThreshold(const std::wstring& module, SCXLogSeverity severity)
        {
            SCXThreadLock lock(m_lock);
            return m_SeverityFilter.SetSeverityThreshold(module, severity);
        }

        /**
            Unset severity threshold for a module.

            \param[in] module Module to clear severity threshold for.
            \returns true if the severity filter was actually changed as a result of
            this method call.
         */
        virtual bool ClearSeverityThreshold(const std::wstring& module)
        {
            SCXThreadLock lock(m_lock);
            return m_SeverityFilter.ClearSeverityThreshold(module);
        }

        /**
            Get the minimum log severity threshold used for any module in this backend.
            \returns Minimum severity threshold used for this backend.
         */
        virtual SCXLogSeverity GetMinActiveSeverityThreshold() const
        {
            SCXThreadLock lock(m_lock);
            return m_SeverityFilter.GetMinActiveSeverityThreshold();
        }

    protected:
        /**
            Log this item.

            \param[in] item Item to send to log.
         */
        virtual void DoLogItem(const SCXLogItem& item) = 0;

    private:
        SCXThreadLockHandle m_lock; //!< Thread lock synchronizing access to internal data.
        SCXLogSeverityFilter m_SeverityFilter; //!< Severity filter for this backend.
    };
} /* namespace SCXCoreLib */
#endif /* SCXLOGBACKEND_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

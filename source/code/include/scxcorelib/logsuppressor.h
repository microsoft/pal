/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Contains the definition of the Log suppressor.

    \date        2009-06-11 04:04:24

*/
/*----------------------------------------------------------------------------*/
#ifndef LOGSUPPRESSOR_H
#define LOGSUPPRESSOR_H

#include <scxcorelib/scxlog.h>
#include <set>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        The log suppressor is to limit the number of times a certain log is
        logged. It does this by simply returning a different severity depending
        on how many times it has been called.
    */
    class LogSuppressor
    {
    public:
        
        /**
            Constructor that sets the suppressor up with two different severities.
            \param[in] initialSeverity Severity to return the first time.
            \param[in] dropToSeverity Severity to return on consecutive calls.
         */
        LogSuppressor(SCXLogSeverity initialSeverity, SCXLogSeverity dropToSeverity) :
            m_initialSeverity(initialSeverity),
            m_dropToSeverity(dropToSeverity),
            m_usedIDs(),
            m_lockhandle(ThreadLockHandleGet())
        {}
        
        /**
            Constructor that sets the suppressor up with two different severities and
            injects a thread lock handle.
            \param[in] initialSeverity Severity to return the first time.
            \param[in] dropToSeverity Severity to return on consecutive calls.
            \param[in] lockhandle Injected thread lock handle.
         */
        LogSuppressor(SCXLogSeverity initialSeverity, SCXLogSeverity dropToSeverity, const SCXThreadLockHandle& lockhandle) :
            m_initialSeverity(initialSeverity),
            m_dropToSeverity(dropToSeverity),
            m_usedIDs(),
            m_lockhandle(lockhandle)
        {}
        
        /**
            Returns the current severity.
            \param[in] id The ID to return the severity for.
            \returns current severity.
         */
        SCXLogSeverity GetSeverity(const std::wstring& id)
        {
            SCXCoreLib::SCXThreadLock lock(m_lockhandle);
            if (m_usedIDs.find(id) == m_usedIDs.end())
            {
                m_usedIDs.insert(id);
                return m_initialSeverity;
            }
            return m_dropToSeverity;
        }

    private:
        const SCXLogSeverity m_initialSeverity; //!< Initial severity.
        const SCXLogSeverity m_dropToSeverity;  //!< Severity for consecutive calls.
        std::set<std::wstring> m_usedIDs;       //!< IDs for which GetSeverity have been called.
        SCXThreadLockHandle m_lockhandle;       //!< Thread lock synchronizing access to internal data.
    };
}

#endif

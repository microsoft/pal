/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation for the simple log mediator class.

    \date        2008-07-18 13:01:31

*/
/*----------------------------------------------------------------------------*/

#include "scxlogmediatorsimple.h"
#include <scxcorelib/scxlogitem.h>
#include <scxcorelib/scxdumpstring.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.

    */
    SCXLogMediatorSimple::SCXLogMediatorSimple() :
        m_lock(ThreadLockHandleGet())
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Constructor with injected thread lock handle.

        \param[in] lock Used to inject a thread lock handle.
    */
    SCXLogMediatorSimple::SCXLogMediatorSimple(const SCXThreadLockHandle& lock) :
        m_lock(lock)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Log a message. The supplied item is distributed to those backends that
        have registered themselves. This entry point is thread safe.

        \param[in] item Log item to add to the log mediator.

        \note This simple implementation is blocking. The calling thread must wait
        for the logging to complete.
    */
    void SCXLogMediatorSimple::LogThisItem(const SCXLogItem& item)
    {
        SCXThreadLock lock(m_lock);
        for (ConsumerSet::iterator i = m_Consumers.begin();
             i != m_Consumers.end();
             ++i)
        {
            (*i)->LogThisItem(item);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Register an SCXLogItemConsumerIf as a new receiver of log messages. It will
        recieve all items that were logged through the LogThisItem interface.

        \param[in] consumer SCXLogItemConsumerIf to register as consumer.
        \returns False if consumer can't be added.
    */
    bool SCXLogMediatorSimple::RegisterConsumer(SCXHandle<SCXLogItemConsumerIf> consumer)
    {
        SCXThreadLock lock(m_lock);
        m_Consumers.insert(consumer);
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        A registered consumer that is no longer interested in receiving SCXLogItems
        can de-register itself through this interface.

        \param[in] consumer SCXLogItemConsumerIf to de-register.
        \returns False if consumer was not previously registered.
    */
    bool SCXLogMediatorSimple::DeRegisterConsumer(SCXHandle<SCXLogItemConsumerIf> consumer)
    {
        SCXThreadLock lock(m_lock);
        return m_Consumers.erase(consumer) > 0;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the effective severity for a particular log module.

        \param[in] module Log module to retrieve severity for.
        \returns Effective severity for log module.

    */
    SCXLogSeverity SCXLogMediatorSimple::GetEffectiveSeverity(const std::wstring& module) const
    {
        SCXLogSeverity effectiveSeverity = eSuppress;
        SCXThreadLock lock(m_lock);
        for (ConsumerSet::const_iterator i = m_Consumers.begin();
             i != m_Consumers.end();
             ++i)
        {
            SCXLogSeverity backendSeverity = (*i)->GetEffectiveSeverity(module);
            if (backendSeverity < effectiveSeverity)
            {
                effectiveSeverity = backendSeverity;
            }
            if (effectiveSeverity == eHysterical)
            {
                return eHysterical;
            }
        }
        return effectiveSeverity;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Handle log rotations that have occurred
     */
    void SCXLogMediatorSimple::HandleLogRotate()
    {
        SCXThreadLock lock(m_lock);
        for (ConsumerSet::iterator i = m_Consumers.begin();
             i != m_Consumers.end();
             ++i)
        {
            (*i)->HandleLogRotate();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns The object represented as a string suitable for logging.

    */
    const std::wstring SCXLogMediatorSimple::DumpString() const
    {
        SCXDumpStringBuilder dsb("SCXLogMediatorSimple");
        return dsb;
    }
} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

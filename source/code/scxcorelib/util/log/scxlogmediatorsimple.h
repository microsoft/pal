/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Contains the definition of a simple log mediator class.

    \date        2008-07-18 11:44:22

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGMEDIATORSIMPLE_H
#define SCXLOGMEDIATORSIMPLE_H

#include "scxlogmediator.h"
#include "scxlogbackend.h"
#include <scxcorelib/scxhandle.h>
#include <set>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Simple implementation of the log mediator interface.
        This class implements a synchronous solution causing the calling thread
        to wait for logging to complete.
    */
    class SCXLogMediatorSimple : public SCXLogMediator
    {
    private:
        /*----------------------------------------------------------------------------*/
        /**
            A strict ordering is needed by the set template.
        */
        struct HandleCompare
        {
            /*----------------------------------------------------------------------------*/
            /**
                Compares two scxhandles.
                It actually compares the addresses of the data that is pointed to by the
                handles. This is of course random, but the exact order is not important in
                this implementation. Only that there _is_ a strict order.
                \param[in] h1 First handle to compare.
                \param[in] h2 Second handle to compare.
                \returns true if h1 is less than h2.
            */
            bool operator()(const SCXHandle<SCXLogItemConsumerIf> h1, const SCXHandle<SCXLogItemConsumerIf> h2) const
            {
                return h1.GetData() < h2.GetData();
            }
        };

        typedef std::set<SCXHandle<SCXLogItemConsumerIf>, HandleCompare> ConsumerSet; //!< Defines a set of consumers.

    public:
        SCXLogMediatorSimple();
        explicit SCXLogMediatorSimple(const SCXThreadLockHandle& lock);

        virtual void LogThisItem(const SCXLogItem& item);
        virtual SCXLogSeverity GetEffectiveSeverity(const std::wstring& module) const;
        virtual bool RegisterConsumer(SCXHandle<SCXLogItemConsumerIf> consumer);
        virtual bool DeRegisterConsumer(SCXHandle<SCXLogItemConsumerIf> consumer);
        virtual void HandleLogRotate();

        const std::wstring DumpString() const;
    private:
        SCXThreadLockHandle m_lock; //!< Thread lock synchronizing access to internal data.
        ConsumerSet m_Consumers; //!< Set of currently subscribed consumers.
    };
}

#endif /* SCXLOGMEDIATORSIMPLE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

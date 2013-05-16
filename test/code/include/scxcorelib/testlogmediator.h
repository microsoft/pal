/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the TestLogMediator class

    \date        2009-06-10 01:22:34

*/
/*----------------------------------------------------------------------------*/

#ifndef TESTLOGMEDIATOR_H
#define TESTLOGMEDIATOR_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxlogitem.h>
#include "scxcorelib/util/log/scxlogmediatorsimple.h"

/**
    This class is a utility mediator for testing purposes.
 */
class TestLogMediator : public SCXCoreLib::SCXLogMediator
{
public:
    virtual bool RegisterConsumer(SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogItemConsumerIf> consumer)
    {
        m_Consumer = consumer;
        return true;
    }

    virtual bool DeRegisterConsumer(SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogItemConsumerIf> /*consumer*/)
    {
        return true;
    }

    SCXCoreLib::SCXLogSeverity GetEffectiveSeverity(const std::wstring& module) const
    {
        return m_Consumer->GetEffectiveSeverity(module);
    }

    virtual void LogThisItem(const SCXCoreLib::SCXLogItem& item)
    {
        m_Consumer->LogThisItem(item);
    }
private:
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogItemConsumerIf> m_Consumer;
};

#endif

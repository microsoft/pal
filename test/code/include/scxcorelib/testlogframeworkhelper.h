/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the TestLogFrameworkHelper class

    \date        2008-07-18 11:44:22

*/
/*----------------------------------------------------------------------------*/

#ifndef TESTLOGFRAMEWORKHELPER_H
#define TESTLOGFRAMEWORKHELPER_H

#include <scxcorelib/testlogmediator.h>
#include <scxcorelib/testlogconfigurator.h>

/**
    This class is a utility for simplifying testing of logs.
    It can be used to inspect (sense) what has been logged.
 */
class TestLogFrameworkHelper
{
public:
    TestLogFrameworkHelper() :
        m_mediator(new TestLogMediator()),
        m_configurator(new TestLogConfigurator(m_mediator)),
        m_handle(L"scx.test", m_mediator, m_configurator)
    {
        m_configurator->m_testBackend->SetSeverityThreshold(L"scx.test", SCXCoreLib::eHysterical);
    }

    SCXCoreLib::SCXLogHandle& GetHandle()
    {
        return m_handle;
    }

    SCXCoreLib::SCXLogItem GetLastLogItem()
    {
        return m_configurator->m_testBackend->GetLastLogItem();
    }

private:
    SCXCoreLib::SCXHandle<TestLogMediator> m_mediator;
    SCXCoreLib::SCXHandle<TestLogConfigurator> m_configurator;
    SCXCoreLib::SCXLogHandle m_handle;
};

#endif

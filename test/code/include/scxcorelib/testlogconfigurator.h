/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the TestLogConfigurator class

    \date        2009-06-10 01:22:34

*/
/*----------------------------------------------------------------------------*/

#ifndef TESTLOGCONFIGURATOR_H
#define TESTLOGCONFIGURATOR_H

#include <scxcorelib/scxlog.h>
#include "testlogbackend.h"

/**
    This class is a utility configurator for testing purposes.
 */
class TestLogConfigurator : public SCXCoreLib::SCXLogConfiguratorIf
{
public:
    SCXCoreLib::SCXHandle<TestLogBackend> m_testBackend;
    unsigned int m_ConfigVersion;

    TestLogConfigurator(SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogMediator> mediator) : 
        m_testBackend(new TestLogBackend()),
        m_ConfigVersion(1),
        m_Mediator(mediator)
    {
        m_Mediator->RegisterConsumer(m_testBackend);
    }
    
    virtual void SetSeverityThreshold(const std::wstring& module, SCXCoreLib::SCXLogSeverity newThreshold)
    {
        if (m_testBackend->SetSeverityThreshold(module, newThreshold))
        {
            ++m_ConfigVersion;
        }
    }

    virtual void ClearSeverityThreshold(const std::wstring& module)
    {
        if (m_testBackend->ClearSeverityThreshold(module))
        {
            ++m_ConfigVersion;
        }
    }

    virtual unsigned int GetConfigVersion() const
    {
        return m_ConfigVersion;
    }

    virtual void RestoreConfiguration()
    {
    }
    
    virtual std::wstring GetMinActiveSeverityThreshold() const
    {
        return L"NOTSET";
    }
private:
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogMediator> m_Mediator; //!< Mediator to configure.
};

#endif

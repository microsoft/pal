/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation of the SCXLogHandle class.

    \date        07-06-07 10:37:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxlogitem.h>

namespace SCXCoreLib
{
    
    /*----------------------------------------------------------------------------*/
    /**
        Send a message to the log mediator if it passes the severity threshold.

        \param[in]  sev The severity of the message.
        \param[in]  message The actual log message.
        \param[in]  location Source code location.

        Create a new LogItem and push it to the LogMediator.

    */
    void SCXLogHandle::Log(SCXLogSeverity sev, const std::wstring& message, const SCXCodeLocation& location) const
    {
        /**
            \internal
            \note We do not check the severity level here.
            1: It should be checked in the log macro.
            2: It is filtered a second time on the back end side.

        */
        m_mediator->LogThisItem(SCXLogItem(m_module, sev, message, location, SCXThread::GetCurrentThreadID()));
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the severity threshold.
        \returns Current severity threshold for this log handle.

    */
    SCXLogSeverity SCXLogHandle::GetSeverityThreshold() const
    {
        if (m_configurator == 0)
        {
            // Not initialized so suppress all output through this handle.
            return eSuppress;
        }
        if (m_configVersion != m_configurator->GetConfigVersion())
        {
            m_severityThreshold = static_cast<unsigned char> (m_mediator->GetEffectiveSeverity(m_module));
            m_configVersion = m_configurator->GetConfigVersion();
        }
        return static_cast<SCXLogSeverity> (m_severityThreshold);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Sets the severity threshold for used log mediator.

        \param[in]   newSeverity New threshold to be set.

        \date        07-08-22 09:25:30

    */
    void SCXLogHandle::SetSeverityThreshold(SCXLogSeverity newSeverity)
    {
        m_configurator->SetSeverityThreshold(m_module, newSeverity);

        m_severityThreshold = newSeverity;
        m_configVersion = m_configurator->GetConfigVersion();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Unsets the severity threshold for used log mediator.

        \date        2008-08-07 12:28:06

    */
    void SCXLogHandle::ClearSeverityThreshold()
    {
        m_configurator->ClearSeverityThreshold(m_module);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.

        Simply initializes the member variables with null.

    */
    SCXLogHandle::SCXLogHandle() :
        m_module(L""),
        m_severityThreshold(eSuppress),
        m_configVersion(0),
        m_mediator(0),
        m_configurator(0)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Constructor with module name, log mediator and log configurator.

        \param  module The string representation of the module the log item belongs to.
        \param  mediator The log mediator to send items to.
        \param  configurator The log configurator used to know when to check for new configuration.

        Simply initializes the member variables.

    */
    SCXLogHandle::SCXLogHandle(const std::wstring& module, SCXHandle<SCXLogItemConsumerIf> mediator, SCXHandle<SCXLogConfiguratorIf> configurator) :
        m_module(module),
        m_mediator(mediator),
        m_configurator(configurator)
    {
        m_severityThreshold = static_cast<unsigned char> (m_mediator->GetEffectiveSeverity(m_module));
        m_configVersion = m_configurator->GetConfigVersion();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Copy constructor.

        \param[in]  o The SCXLogHandle to copy.

        Simply initializes the member variables.

    */
    SCXLogHandle::SCXLogHandle(const SCXLogHandle& o) :
        m_module(o.m_module),
        m_severityThreshold(o.m_severityThreshold),
        m_configVersion(o.m_configVersion),
        m_mediator(o.m_mediator),
        m_configurator(o.m_configurator)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Assignment operator.

        \param[in]  o The SCXLogHandle to copy.

        Simply initializes the member variables.

    */
    SCXLogHandle& SCXLogHandle::operator=(const SCXLogHandle& o)
    {
        m_module = o.m_module;
        m_severityThreshold = o.m_severityThreshold;
        m_configVersion = o.m_configVersion;
        m_mediator = o.m_mediator;
        m_configurator = o.m_configurator;
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor.

    */
    SCXLogHandle::~SCXLogHandle()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns      String representation of object.

    */
    const std::wstring SCXLogHandle::DumpString() const
    {
        return SCXDumpStringBuilder("SCXLogHandle")
            .Text("module", m_module)
            .Scalar("SeverityThreshold", m_severityThreshold);
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

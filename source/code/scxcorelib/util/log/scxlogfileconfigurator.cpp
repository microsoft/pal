/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the implementation of the log file configurator.

    \date        2008-07-21 15:10:29

*/
/*----------------------------------------------------------------------------*/

#include "scxlogfileconfigurator.h"
#include "scxlogmediator.h"
#include "scxlogstdoutbackend.h"

#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxlogpolicy.h>
#include <scxcorelib/stringaid.h>
#include <algorithm>
#include <vector>

// We use std::min, not the min() macro
#if defined(min)
#undef min
#endif

namespace SCXCoreLib
{
    namespace
    {
        /*----------------------------------------------------------------------------*/
        /**
            Parameters for the config file reader thread.
        */
        class LogFileConfiguratorParam : public SCXCoreLib::SCXThreadParam
        {
        public:
/*----------------------------------------------------------------------------*/
/**
    Constructor
*/
            // When setting up SCXThreadParam, don't allow SCXCondition to use log framework
            LogFileConfiguratorParam()
                : m_configurator(NULL)
            {}

            SCXLogFileConfigurator* m_configurator;  //!< Pointer to the log file configurator associated with the thread.
        };
    }

    /*----------------------------------------------------------------------------*/
    /**
        Constructor that takes a handle to the log mediator.
             
        \param[in] mediator Pointer to log mediator to configure.
        \param[in] configFilePath Path to configuration file to use.
        \param[in] lock Used to inject a thread lock handle. The default is to
        get a new thread lock handle from the factory.
        \param[in] configRefreshRate Used to inject a configuration refresh rate
        in milliseconds.
    */
    SCXLogFileConfigurator::SCXLogFileConfigurator(SCXHandle<SCXLogMediator> mediator,
                                                   const SCXFilePath& configFilePath,
                                                   const SCXThreadLockHandle& lock /*= ThreadLockHandleGet()*/,
                                                   scxulong configRefreshRate /* = 10000*/) :
        m_Mediator(mediator),
        m_ConfigFilePath(configFilePath),
        m_ConfigVersion(0),
        m_lock(lock),
        m_ConfigRefreshRate(configRefreshRate),
        m_ConfigUpdateThread(0),
        m_ConfFile(m_ConfigFilePath),
        m_MinActiveSeverityThreshold(eSeverityMax)
    {
        ParseConfigFile();

        SCXHandle<LogFileConfiguratorParam> p( new LogFileConfiguratorParam() );
        p->m_configurator = this;
        m_ConfigUpdateThread = new SCXCoreLib::SCXThread(ConfigUpdateThreadBody, p);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set the effective severity per module
            
        \param[in] module Log module to set severity for.
        \param[in] newThreshold The new severity threshold to set.

    */
    void SCXLogFileConfigurator::SetSeverityThreshold(const std::wstring& module, SCXLogSeverity newThreshold)
    {
        SCXThreadLock lock(m_lock);
        bool changed = false;
        for (BackendList::iterator iter = m_Backends.begin();
             iter != m_Backends.end();
             ++iter)
        {
            if (SetSeverityThreshold(*iter, module, newThreshold))
            {
                changed = true;
            }
        }
        if (changed)
        {
            ++m_ConfigVersion;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Unset the effective severity per module
            
        \param[in] module Log module to clear severity for.

    */
    void SCXLogFileConfigurator::ClearSeverityThreshold(const std::wstring& module)
    {
        SCXThreadLock lock(m_lock);
        bool changed = false;
        for (BackendList::iterator iter = m_Backends.begin();
             iter != m_Backends.end();
             ++iter)
        {
            if ((*iter)->ClearSeverityThreshold(module))
            {
                changed = true;
            }
        }
        if (changed)
        {
            ++m_ConfigVersion;

            m_MinActiveSeverityThreshold = eSeverityMax;
            for (BackendList::iterator iter = m_Backends.begin();
                 iter != m_Backends.end();
                 ++iter)
            {
                SCXLogSeverity s = (*iter)->GetMinActiveSeverityThreshold();
                if (s < m_MinActiveSeverityThreshold)
                {
                    m_MinActiveSeverityThreshold = s;
                }

            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get current config version
       
        \returns Current config version.
        
    */
    unsigned int SCXLogFileConfigurator::GetConfigVersion() const
    {
        SCXThreadLock lock(m_lock);
        return m_ConfigVersion;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Restore configuration by rereading the configuration file.
        
    */
    void SCXLogFileConfigurator::RestoreConfiguration()
    {
        SCXThreadLock lock(m_lock);
        for (BackendList::iterator iter = m_Backends.begin();
             iter != m_Backends.end();
             ++iter)
        {
            m_Mediator->DeRegisterConsumer(*iter);
        }

        m_MinActiveSeverityThreshold = eSeverityMax;

        m_Backends.clear();
        
        ParseConfigFile();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the minimum log severity threshold active anywhere in the 
        framework.
        \returns Severity threshold as string.
    */
    std::wstring SCXLogFileConfigurator::GetMinActiveSeverityThreshold() const
    {
        SCXThreadLock lock(m_lock);
        return SCXLogConfigReader_SeverityToString(m_MinActiveSeverityThreshold);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor. Kills the thread.
       
    */
    SCXLogFileConfigurator::~SCXLogFileConfigurator()
    {
        if (m_ConfigUpdateThread != 0)
        {
            // If the thread is still alive (it should be), ask it to go away
            if ( m_ConfigUpdateThread->IsAlive() )
            {
                m_ConfigUpdateThread->RequestTerminate();
                m_ConfigUpdateThread->Wait();
            }

            SCXASSERT( ! m_ConfigUpdateThread->IsAlive() );
            m_ConfigUpdateThread = 0;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Parses the configuration file.
        \returns false if configuration could not be parsed.

        This is an example of what a file may look like:
        FILE (
        PATH: /var/log/scx.log
        MODULE: WARNING
        MODULE: scx.some.module TRACE
        )
    */
    bool SCXLogFileConfigurator::ParseConfigFile()
    {
        m_ConfFile.Refresh();

        SCXLogConfigReader<SCXLogBackend, SCXLogFileConfigurator> oConfigReader;
        
        bool validConfig = oConfigReader.ParseConfigFile(m_ConfigFilePath,this);
        
        if ( ! validConfig)
        {
            SCXHandle<SCXLogBackend> defaultBackend(
                new SCXLogFileBackend(CustomLogPolicyFactory()->GetDefaultLogFileName()) );

            SetSeverityThreshold(defaultBackend, L"", CustomLogPolicyFactory()->GetDefaultSeverityThreshold());
            m_Backends.push_back(defaultBackend);
            m_Mediator->RegisterConsumer(defaultBackend);
            m_MinActiveSeverityThreshold = CustomLogPolicyFactory()->GetDefaultSeverityThreshold();
        }

        ++m_ConfigVersion;

        return validConfig;
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Checks if the configuration file has changed so that the configuration
        needs to be updated.
        \returns true if configuration should be updated.
    */
    bool SCXLogFileConfigurator::IsConfigurationChanged() const
    {
        SCXThreadLock lock(m_lock);

        SCXFileInfo file(m_ConfigFilePath);
        if (file.Exists() != m_ConfFile.Exists() || 
            (file.Exists() &&
             file.GetLastModificationTimeUTC() != m_ConfFile.GetLastModificationTimeUTC()))
        {
            return true;
        }
        return false;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Thread body that scans for updated configuration file and updates the
        configuration if it has changed.
        \param[in] param Thread parameters.
    */
    void SCXLogFileConfigurator::ConfigUpdateThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        LogFileConfiguratorParam* p = static_cast<LogFileConfiguratorParam*>(param.GetData());
        SCXASSERT(0 != p);

        SCXLogFileConfigurator* configurator = p->m_configurator;
        SCXASSERT(0 != configurator);

        p->m_cond.SetSleep(configurator->m_ConfigRefreshRate);
        {
            SCXConditionHandle h(p->m_cond);
            while ( ! param->GetTerminateFlag() )
            {
                enum SCXCondition::eConditionResult r = h.Wait();
                if (!param->GetTerminateFlag() && SCXCondition::eCondTimeout == r
                    && configurator->IsConfigurationChanged())
                {
                    configurator->RestoreConfiguration();
                }
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set the effective severity for a certain backend. Also updates
        m_MinActiveSeverityThreshold as needed.
            
        \param[in] backend Backend to update severity for.
        \param[in] module Log module to set severity for.
        \param[in] newThreshold The new severity threshold to set.
        \returns True if this sould cause a change in config version.

    */
    bool SCXLogFileConfigurator::SetSeverityThreshold(SCXHandle<SCXLogBackend> backend,
                                                      const std::wstring& module,
                                                      SCXLogSeverity newThreshold)
    {
        if (backend->SetSeverityThreshold(module, newThreshold))
        {
            if (newThreshold < m_MinActiveSeverityThreshold)
            {
                m_MinActiveSeverityThreshold = newThreshold;
            }
            return true;
        }
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Creates a new backend based on configuration line
            
        
        \param[in] name - config file entry
        \returns backend handler

    */
    SCXHandle<SCXLogBackend> SCXLogFileConfigurator::Create(const std::wstring& name)
    {
        SCXHandle<SCXLogBackend> backend(0);
        if (L"FILE (" == name)
        {
            backend = new SCXLogFileBackend();
            SetSeverityThreshold(backend, L"", CustomLogPolicyFactory()->GetDefaultSeverityThreshold());
        }
        if (L"STDOUT (" == name)
        {
            backend = new SCXLogStdoutBackend();
            SetSeverityThreshold(backend, L"", CustomLogPolicyFactory()->GetDefaultSeverityThreshold());
        }
        return backend;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Adds new backend to the list
        
        \param[in] backend - new, initialized backend entry

    */
    void SCXLogFileConfigurator::Add( SCXHandle<SCXLogBackend> backend )
    {
        m_Backends.push_back(backend);
        m_Mediator->RegisterConsumer(backend);
    
    }
    

} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

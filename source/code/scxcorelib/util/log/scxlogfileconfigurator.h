/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the definition of log file configurator class.

    \date        2008-07-22 08:55:15

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGFILECONFIGURATOR_H
#define SCXLOGFILECONFIGURATOR_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxthread.h>
#include "scxlogfilebackend.h"
#include "scxlogmediator.h"
#include "scxlogconfigreader.h"

#include <list>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Implementation of the log configurator interface.
    */
    class SCXLogFileConfigurator : public SCXLogConfiguratorIf
    {
        typedef std::list<SCXHandle<SCXLogBackend> > BackendList; //!< List of backends
    public:
        SCXLogFileConfigurator(SCXHandle<SCXLogMediator> mediator,
                               const SCXFilePath& configFilePath,
                               const SCXThreadLockHandle& lock = ThreadLockHandleGet(),
                               scxulong configRefreshRate = 10000);

        virtual void SetSeverityThreshold(const std::wstring& module, SCXLogSeverity newThreshold);
        virtual void ClearSeverityThreshold(const std::wstring& module);
        virtual unsigned int GetConfigVersion() const;
        virtual void RestoreConfiguration();
        virtual std::wstring GetMinActiveSeverityThreshold() const;
        virtual ~SCXLogFileConfigurator();

        // interface to the config-reader
        bool SetSeverityThreshold(SCXHandle<SCXLogBackend> backend, const std::wstring& module, SCXLogSeverity newThreshold);
        SCXHandle<SCXLogBackend>  Create(const std::wstring& name);
        void    Add( SCXHandle<SCXLogBackend> backend );
        
    private:
        /**
            private assignment operator.
        */
        SCXLogFileConfigurator& operator=(const SCXLogFileConfigurator&) { return *this; }
        bool ParseConfigFile();
        bool IsConfigurationChanged() const;

        SCXHandle<SCXLogMediator> m_Mediator; //!< Mediator to configure.
        BackendList m_Backends; //!< Configured backends.
        const SCXFilePath m_ConfigFilePath; //!< File path of config file.
        unsigned int m_ConfigVersion; //!< Current config version.
        SCXThreadLockHandle m_lock; //!< Thread lock synchronizing access to internal data.
        scxulong m_ConfigRefreshRate; //!< The interval the configuration thread checks for new configuration.
        SCXHandle<SCXThread> m_ConfigUpdateThread; //!< Pointer to thread.
        SCXFileInfo m_ConfFile; //!< Contains cached information about the configuration file.
        SCXLogSeverity m_MinActiveSeverityThreshold; //!< Minimum severity threshold set for any module in the framework.
        static void ConfigUpdateThreadBody(SCXCoreLib::SCXThreadParamHandle& param);
    };
}

#endif /* SCXLOGMEDIATORSIMPLE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

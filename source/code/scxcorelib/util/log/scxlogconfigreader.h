/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the definition of log file configurator class.

    \date        2008-07-22 08:55:15

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGCONFIGREADER_H
#define SCXLOGCONFIGREADER_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/stringaid.h>
#include "scxlogfilebackend.h"
#include "scxlogmediator.h"

#include <list>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Implementation of the log configurator reader.
        to make reader generic enough for both config-tool and scx-core, template is used;
        template type ("BaseBackendType") must support the following operations:
        bool IsInitialized()
        bool SetProperty(const std::wstring& key, const std::wstring& value)
        
        // interface to the caller ("ConfigConsumerInterface"):
        SCXHandle< BaseBackendType > Create( const std::wstring& name ) = 0;
        void Add( SCXHandle< BaseBackendType > backend ) = 0;
        bool SetSeverityThreshold( SCXHandle< BaseBackendType > backend, const std::wstring& module, SCXLogSeverity newThreshold) = 0;
            
        };
    */
    template <class BaseBackendType, class ConfigConsumerInterface>
    class SCXLogConfigReader
    {
    public:
        SCXLogConfigReader(){}
        bool ParseConfigFile( const SCXFilePath& configFilePath, ConfigConsumerInterface* pInterface );

    private:
    };

    // helper funcitons
    SCXLogSeverity SCXLogConfigReader_TranslateSeverityString(const std::wstring& severityString);
    std::wstring SCXLogConfigReader_SeverityToString(SCXLogSeverity severity);


    // implementation
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
    template <class BaseBackendType, class ConfigConsumerInterface>
    bool SCXLogConfigReader<BaseBackendType, ConfigConsumerInterface>::ParseConfigFile( const SCXFilePath& configFilePath, ConfigConsumerInterface* pInterface )
    {
        std::vector<std::wstring> configLines;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLinesAsUTF8(configFilePath,
                                    configLines,
                                    nlfs);
        bool validConfig = false;
        for (std::vector<std::wstring>::const_iterator i = configLines.begin();
             i != configLines.end();
             ++i)
        {
            SCXHandle< BaseBackendType > backend = pInterface->Create( *i );
            if (backend != 0)
            {
                for (++i ; i != configLines.end(); ++i)
                {
                    if (L")" == *i)
                    {
                        if (backend->IsInitialized())
                        {
                            pInterface->Add( backend );
                            validConfig = true;
                            break;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        std::vector<std::wstring> lineTokens;
                        StrTokenize(*i, lineTokens, L":");
                        if (lineTokens.size() != 2)
                        {
                            continue;
                        }
                        if (L"MODULE" == lineTokens[0])
                        {
                            // This key is for the severity filter.
                            std::vector<std::wstring> severityTokens;
                            StrTokenize(lineTokens[1], severityTokens, L" ");
                            if (severityTokens.size() == 1)
                            {
                                // This should be severity for root module.
                                SCXLogSeverity severity = SCXLogConfigReader_TranslateSeverityString(severityTokens[0]);
                                pInterface->SetSeverityThreshold(backend, L"", severity);
                            }
                            else if (severityTokens.size() == 2)
                            {
                                // should be <module> <severity>
                                SCXLogSeverity severity = SCXLogConfigReader_TranslateSeverityString(severityTokens[1]);
                                pInterface->SetSeverityThreshold(backend, severityTokens[0], severity);
                            }
                        } 
                        else
                        {
                            backend->SetProperty(lineTokens[0], lineTokens[1]);
                        }
                    }
                }

                if ( i == configLines.end() )
                    break;
                
            }
        }

        return validConfig;
    }
    
    
}

#endif /* SCXLOGCONFIGREADER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the public interface of the Log policy.

    \date        2008-08-05 14:08:54

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGPOLICY_H
#define SCXLOGPOLICY_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxfilepath.h>
#if defined(SCX_UNIX)
#include <scxcorelib/scxuser.h>
#endif

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Defines the interface for the log policy.
        This is also the default implementation of the log policy. If another
        policy is needed, then inherit from this policy overriding any methods
        that need to be changed.
    */
    class SCXLogPolicy
    {
    public:
        /**
            Virtual destructor.
        */
        virtual ~SCXLogPolicy() {}

        /**
            Get the path of the log config file.
            \returns the path of the log config file.
        */
        virtual SCXFilePath GetConfigFileName() const
        {
#if defined(WIN32)
            return SCXFilePath(L"C:\\scxlog.conf");
#elif defined(SCX_UNIX)
            return SCXFilePath(L"/etc/opt/microsoft/scx/conf/scxlog.conf");
#endif
        }

        /**
            If no config is specified, then log output will be written
            to the file specified by this method.
            \returns Path to the default log file.
        */
        virtual SCXFilePath GetDefaultLogFileName() const
        {
#if defined(WIN32)
            return SCXFilePath(L"C:\\scx.log");
#elif defined(SCX_UNIX)
            SCXUser user;
            SCXFilePath filepath(L"/var/opt/microsoft/scx/log/scx.log");
            if (!user.IsRoot())
            {
                filepath.AppendDirectory(user.GetName());
            }
            return filepath;
#endif
        }

        /**
            Get the default severity threshold.
            \returns Default severity threshold.
        */
        virtual SCXLogSeverity GetDefaultSeverityThreshold() const
        {
            return eInfo;
        }
    };
}

/**
    This is the method that the core library logging framework recieves its
    implementation of the log policy.

    If you are happy with the default log policy, then a default implementation
    of the log policy factory is provided in the header file
    scxdefaultlogpolicyfactory.h. You need to include that file in one of your
    source files. (As is done in logpolicy.cpp in the metaprovider directory
    for the SCXCoreLib dynamic library for example)
    
    If, on the other hand, you would like to use another log policy than the
    default one, override the policy object above and provide your own
    implementation of the CustomLogPolicyFactory function.
    An example of this can be found in the testrunner.
*/
SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogPolicy> CustomLogPolicyFactory();

#endif /* SCXLOGPOLICY_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

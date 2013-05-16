/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains log policy of the testrunner.

    \date        2008-08-06 16:30:27

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxlogpolicy.h>

/*----------------------------------------------------------------------------*/
/**
    Defines the testrunner log policy.
*/
class TestrunnerLogPolicy : public SCXCoreLib::SCXLogPolicy
{
public:
    /**
        Virtual destructor.
    */
    virtual ~TestrunnerLogPolicy() {}

    /**
        Get the path of the log config file.
        \returns the path of the log config file.
    */
    virtual SCXCoreLib::SCXFilePath GetConfigFileName() const
    {
        return SCXCoreLib::SCXFilePath(L"./scxlog.conf");
    }

    /**
        If no config is specified, then log output will be written
        to the file specified by this method.
        \returns Path to the default log file.
    */
    virtual SCXCoreLib::SCXFilePath GetDefaultLogFileName() const
    {
        return SCXCoreLib::SCXFilePath(L"./scxtestrunner.log");
    }
};

/*----------------------------------------------------------------------------*/
/**
    This is the testrunner log policy factory. It will return the testrunner
    log policy.

    \returns The default log policy object.
*/
SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogPolicy> CustomLogPolicyFactory()
{
    return SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogPolicy>( new TestrunnerLogPolicy() );
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

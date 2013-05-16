/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the default implementation of the log policy factory.

    \date        2008-08-06 16:21:16

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXDEFAULTLOGPOLICYFACTORY_H
#define SCXDEFAULTLOGPOLICYFACTORY_H

#include <scxcorelib/scxlogpolicy.h>

/*----------------------------------------------------------------------------*/
/**
    This is the default log policy factory. It will return a default log
    policy. Include this file somewhere in your project in order to use the
    default log policy. This function is declared in scxlogpolicy.h

    If, on the other hand, you would like to use another log policy than the
    default one, you need to implement this function in your project.

    \returns The default log policy object.
*/
SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogPolicy> CustomLogPolicyFactory()
{
    return SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogPolicy>(new SCXCoreLib::SCXLogPolicy());
}

#endif /* SCXDEFAULTLOGPOLICYFACTORY_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

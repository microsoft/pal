/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief        Operating system error handling         
    
    \date        07-09-27 17:56:00

    Operating system calls normally return error codes instead of throwing 
    exceptions. Here are some utilities to map the C-style of error handling 
    into the C++ style, that is, exceptions.
     
    
*/
/*----------------------------------------------------------------------------*/
#ifndef OSERROR_H
#define OSERROR_H

#include <string>
 
namespace SCXCoreLib {
    std::wstring TextOfWindowsLastError(unsigned int lastError);
    std::wstring UnexpectedWindowsError(const std::wstring &problem, 
            unsigned int lastError);
     
    std::wstring UnexpectedErrno(const std::wstring &problem, int errnoValue);
}

#endif /* OSERROR_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

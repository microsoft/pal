/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief     Portable interface for strerror() function

    \date      01-27-16 16:30:00

    strerror() helper functions

    \note      These functions are implemented without other PAL dependencies.
               This aids compatibility with programs like OMI's pre-exec
               program, which does not link against the PAL.
*/
/*----------------------------------------------------------------------------*/

#ifndef SCXSTRERROR_H
#define SCXSTRERROR_H

#include <string>

namespace SCXCoreLib
{
    std::string strerror(int errnum);
}

#endif /* SCXSTRERROR_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

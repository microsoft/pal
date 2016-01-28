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

#include <scxcorelib/strerror.h>

#include <string.h>
#include <string>

namespace SCXCoreLib
{
    std::string strerror(int errnum)
    {
        std::string result;

#ifdef WIN32
        char buf[128];
        strerror_s(buf, sizzeof(buf), errnum);
        result = buf;
#elif (defined(hpux) && (PF_MINOR==23)) || (defined(sun) && (PF_MAJOR==5) && (PF_MINOR<=9))
        // WI7938
        result = ::strerror(errnum);
#elif (defined(hpux) && (PF_MINOR>23)) || (defined(sun) && (PF_MAJOR==5) && (PF_MINOR>9)) || (defined(aix)) || (defined(macos))
        char buf[128];
        int r = strerror_r(errnum, buf, sizeof(buf));
        if (0 != r) {
            snprintf(buf, sizeof(buf), "Unknown error %d", errnum);
        }
        result = buf;
#else
        char buf[128];
        // Do not assign return value directly to avoid compiler error when
        // strerror_r is declared to return an int.
        char* r = strerror_r(errnum, buf, sizeof(buf));
        result = r;
#endif

        return result;
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

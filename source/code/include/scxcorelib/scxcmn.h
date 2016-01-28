/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief       Common include for all SCX files.

    \date        2007-05-16 16:26:00

    This file should always be included before
    any other includes in implementation files.

*/
/*----------------------------------------------------------------------*/
#ifndef SCXCMN_H
#define SCXCMN_H

// Include build related definitions file.
#include "defines.h"

// Define convenience macro for all UNIX/Linux platforms
// The intent here is to use this for things common across all Linux platforms
// (i.e. <unistd.h> exists on all UNIX/Linux platforms, \n line endings, etc)
#if defined(WIN32)
    // WIN32, most definitely, is not UNIX or Linux-like
#elif defined(aix) || defined(hpux) || defined(linux) || defined(macos) || defined(sun)
#define SCX_UNIX 1
/* Including stdlib.h for convenience since it is used in a lot of places */
#include <stdlib.h>
#else
#error "Platform not supported"
#endif


// Pick up definitions for int64_t and u_int_64_t (or uint64_t)
#if defined(linux) || defined(sun) || defined(hpux) || defined(aix)
#include <sys/types.h>    // int64_t & u_int64_t
#elif defined(macos)
#include <stdint.h>       // int64_t & uint64_t
#endif

#if defined(WIN32)
 
/** scxlong and scxulong types to handle long integers consistent. */
typedef __int64 scxlong;
/** scxlong and scxulong should be 64 bit on all platforms. */
typedef unsigned __int64 scxulong;

#if defined(_DEBUG)
// memory debugging stuff
#include <crtdbg.h>
// all these files have to be included before we play trick with "new" operator below 
// since it may not work with the way they use "new" internaly;
// in case you added new include and get strange compliation error please add this include to this list
#include <string>
#include <fstream>
#include <vector>
#include <xtree>
#include <xlocmon>

#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
 
#endif

#elif defined(linux)

/** scxlong and scxulong types to handle long integers consistent. */
typedef int64_t scxlong;
/** scxlong and scxulong should be 64 bit on all platforms. */
typedef u_int64_t scxulong;

#elif defined(sun) || defined(hpux) || defined(aix) || defined(macos)

/** scxlong and scxulong types to handle long integers consistent. */
typedef int64_t scxlong;
/** scxlong and scxulong should be 64 bit on all platforms. */
typedef uint64_t scxulong;

#else

#error "scxlong and scxulong not defined for this platform."

#endif

/**
    \page Real numbers in Aurora

    For real numbers use:
    
    float  - for 32 bit real numebers
    double - for 64 bit real numbers

    If we encounter a platform where this is not true, we need to define our own 32 & 64 bit real types

*/

/* Include scxassert.h for convenience since it is used everywhere. */
#include <scxcorelib/scxassert.h>

/* Include scxcompat.h so it can be used without special requirements */
#include <scxcorelib/scxcompat.h>

/* Include strerror.h so consumers must be clear on what version to use */
/* (Can't imagine a case where they's want the system one, but do give a choice) */
#include <scxcorelib/strerror.h>

// Based on g++ usage for Red Hat and SuSE, it appears that GCC v4.4 and later don't need to disable dynamic_cast. So the last condition was added at the end.

// At this point, we THINK dynamic_cast is okay ... but keep this in code "just in case"

#if defined(DYNAMIC_CAST_STILL_BROKEN_ON_AIX)
#if !defined(sun) && !defined(aix) && !(defined(PF_DISTRO_SUSE) && PF_MAJOR==11) && !(defined(PF_DISTRO_REDHAT) && PF_MAJOR>=6) && \
    !(defined(PF_DISTRO_ULINUX) && (__GNUC__>4 || (__GNUC__==4 && __GNUC_MINOR__>=3)))

    // this macro disables dynamic_cast, since t causes problems on aix 5.3 - see wi 11149, 11220
    #define dynamic_cast sorry_but_dynamic_cast_is_not_allowed_in_core_project_see_wi_11220

#endif
#endif // defined(DYNAMIC_CAST_STILL_BROKEN_ON_AIX)

#endif /* SCXCMN_H */

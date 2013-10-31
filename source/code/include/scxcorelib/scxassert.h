/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief       Assert implementation to allow handling asserts in other ways than default.

    \date        2007-05-16 16:26:00

*/
/*----------------------------------------------------------------------*/
#ifndef SCXASSERT_H
#define SCXASSERT_H

#include <scxcorelib/scxcmn.h>

#ifdef NDEBUG
/** Default assertion macro */
#define SCXASSERT(cond) 
/** Assertion macro with message parameter */
#define SCXASSERTFAIL(message)
#else
/** Default assertion macro */
#define SCXASSERT(cond) (cond)?static_cast<void>(0):SCXCoreLib::scx_assert_failed(#cond,__FILE__,__LINE__)
/** Assertion macro with message parameter */
#define SCXASSERTFAIL(message) SCXCoreLib::scx_assert_failed("",__FILE__,__LINE__,message)
#endif


namespace SCXCoreLib
{
    extern "C" void scx_assert_failed(const char* c, const char* f, const unsigned int l, const wchar_t* m = 0);
} /* namespace SCXCoreLib */

#endif /* SCXASSERT_H */

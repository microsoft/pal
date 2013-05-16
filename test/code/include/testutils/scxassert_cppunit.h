/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file        

    \brief       Definitions related to asserts for unit testing.

    \date        2007-10-17 10:47:59

*/
/*----------------------------------------------------------------------*/
#ifndef SCXASSERT_CPPUNIT_H
#define SCXASSERT_CPPUNIT_H

#include <string>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Saves information about failed asserts to be used by unit tests.
    
        \date        2007-10-17 10:51:13
    
    */
    class SCXAssertCounter
    {
    private:
        static unsigned int nrAssertsFailed;
        static std::wstring lastMessage;
    public:
        static void Reset();
        static unsigned int GetFailedAsserts();
        static const std::wstring& GetLastMessage();
        static void AssertFailed(const std::wstring& m);
    };
} /* namespace SCXCoreLib */

#endif /* SCXASSERT_CPPUNIT_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

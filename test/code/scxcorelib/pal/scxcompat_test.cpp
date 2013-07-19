/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       C and C++ compaibility library tests

    \author      Jeff Coffler <jeffcof@microsoft.com> 
    \date        2008-08-18 14:59:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>

using SCXCoreLib::StrToUTF8;
using namespace std;

class SCXCompatTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXCompatTest );
//    CPPUNIT_TEST( testSomething );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp()
    {
    }

//    void testSomething() {}

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXCompatTest ); 

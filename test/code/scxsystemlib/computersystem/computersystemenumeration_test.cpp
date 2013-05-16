/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2011-04-26 15:15:06

    ComputerSystem colletion test class.
    
    Only tests the functionality of the enumeration class.
    The actual data gathering is tested by a separate class.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/computersystemenumeration.h>
#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;

class ComputerSystemEnumeration_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( ComputerSystemEnumeration_Test );
    CPPUNIT_TEST( testCreation );

    SCXUNIT_TEST_ATTRIBUTE(testCreation, SLOW);
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }


    void testCreation()
    {
#if FILTERLINUX
        ComputerSystemEnumeration computerSystemEnum;
        computerSystemEnum.Init();
        CPPUNIT_ASSERT(0 != computerSystemEnum.GetTotalInstance());
        CPPUNIT_ASSERT(computerSystemEnum.Begin() == computerSystemEnum.End());

        SCXHandle<SCXSystemLib::ComputerSystemInstance> inst = computerSystemEnum.GetTotalInstance();
        CPPUNIT_ASSERT(0 != inst);

        computerSystemEnum.CleanUp();
#endif
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( ComputerSystemEnumeration_Test );

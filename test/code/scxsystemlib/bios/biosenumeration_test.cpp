/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2011-03-03 16:21:06

    BIOS colletion test class.
    
    Only tests the functionality of the enumeration class.
    The actual data gathering is tested by a separate class.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>


#include <scxsystemlib/biosenumeration.h>
#include <scxsystemlib/biosinstance.h>

#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;

class BiosEnumeration_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( BiosEnumeration_Test );
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
#if defined(linux) || (defined(sun) && !defined(sparc)) || (defined(sun) && defined(sparc) && PF_MINOR==10)
        BIOSEnumeration biosEnum;
        biosEnum.Init();
        CPPUNIT_ASSERT(0 != biosEnum.GetTotalInstance());
        CPPUNIT_ASSERT(biosEnum.Begin() == biosEnum.End());

        SCXHandle<SCXSystemLib::BIOSInstance> biosInst = biosEnum.GetTotalInstance();
        CPPUNIT_ASSERT(0 != biosInst);

        biosEnum.CleanUp();
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( BiosEnumeration_Test );

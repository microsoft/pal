/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2011-04-02 09:41:06

    Processor colletion test class.

    Only tests the functionality of the enumeration class.
    The actual data gathering is tested by a separate class.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>

#include <scxsystemlib/cpupropertiesenumeration.h>

#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;

class CpuPropertiesEnumeration_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( CpuPropertiesEnumeration_Test );
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
        CpuPropertiesEnumeration cpuPropertiesEnum;
        cpuPropertiesEnum.Init();
        CPPUNIT_ASSERT(0 != cpuPropertiesEnum.Size());

        SCXHandle<SCXSystemLib::CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);
        CPPUNIT_ASSERT(0 != inst);

        cpuPropertiesEnum.CleanUp();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CpuPropertiesEnumeration_Test );

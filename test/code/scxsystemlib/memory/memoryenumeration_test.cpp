/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-07-03 09:33:09

    Memory data colletion test class.
    
    Only tests the functionality of the enumeration class.
    The actual data gathering is tested by a separate class.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>

#include <scxsystemlib/memoryenumeration.h>
#include <scxsystemlib/memoryinstance.h>

#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;

class MemoryEnumeration_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( MemoryEnumeration_Test  );
    CPPUNIT_TEST( callDumpStringForCoverage );
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

    void callDumpStringForCoverage()
    {
        MemoryEnumeration memEnum;
        CPPUNIT_ASSERT(memEnum.DumpString().find(L"MemoryEnumeration") != std::wstring::npos);
    }

    void testCreation()
    {
        MemoryEnumeration memEnum;
        memEnum.Init();
        CPPUNIT_ASSERT(0 != memEnum.GetTotalInstance());
        CPPUNIT_ASSERT(memEnum.Begin() == memEnum.End());

        SCXCoreLib::SCXHandle<MemoryInstance> instance = memEnum.GetTotalInstance();
        CPPUNIT_ASSERT(0 != instance);

        memEnum.CleanUp();
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( MemoryEnumeration_Test );

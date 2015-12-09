/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-07-10 15:50:55

    Data sampler test class.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>

#include <scxsystemlib/datasampler.h>
#include <testutils/scxunit.h>

#include <cppunit/extensions/HelperMacros.h>

#include <iomanip>

using namespace SCXCoreLib;
using namespace SCXSystemLib;

class DataSampler_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( DataSampler_Test  );
    CPPUNIT_TEST( testAddSample );
    CPPUNIT_TEST( testHasWrapped );
    CPPUNIT_TEST( testGetAverage );
    CPPUNIT_TEST( testGetAverageDelta );
    CPPUNIT_TEST( testGetAverageDeltaFactored );
    CPPUNIT_TEST( testGetDelta );
    CPPUNIT_TEST( testGetAt );
    CPPUNIT_TEST( testClear );
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void testAddSample()
    {
        DataSampler<int> test(5);
        CPPUNIT_ASSERT(0 == test.GetNumberOfSamples());
        test.AddSample(1);
        CPPUNIT_ASSERT(1 == test.GetNumberOfSamples());
        test.AddSample(2);
        CPPUNIT_ASSERT(2 == test.GetNumberOfSamples());
        test.AddSample(3);
        CPPUNIT_ASSERT(3 == test.GetNumberOfSamples());
        test.AddSample(4);
        CPPUNIT_ASSERT(4 == test.GetNumberOfSamples());
        test.AddSample(5);
        CPPUNIT_ASSERT(5 == test.GetNumberOfSamples());
        test.AddSample(6);
        CPPUNIT_ASSERT(5 == test.GetNumberOfSamples());
        test.AddSample(7);
        CPPUNIT_ASSERT(5 == test.GetNumberOfSamples());
    }

    void testHasWrapped()
    {
        DataSampler<int> test(5);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
        test.AddSample(10);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
        test.AddSample(20);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
        test.AddSample(30);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
        test.AddSample(40);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
        test.AddSample(50);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
        test.AddSample(10);
        CPPUNIT_ASSERT(test.HasWrapped(5));
        test.AddSample(20);
        CPPUNIT_ASSERT(test.HasWrapped(5));
        test.AddSample(30);
        CPPUNIT_ASSERT(test.HasWrapped(5));
        test.AddSample(40);
        CPPUNIT_ASSERT(test.HasWrapped(5));
        test.AddSample(50);
        CPPUNIT_ASSERT(!test.HasWrapped(5));
    }

    void testGetAverage()
    {
        DataSampler<int> test(5);
        
        CPPUNIT_ASSERT(0 == test.GetAverage<int>());

        test.AddSample(1);

        CPPUNIT_ASSERT(1 == test.GetAverage<int>());

        test.AddSample(2);
        test.AddSample(3);
        test.AddSample(4);

        CPPUNIT_ASSERT(2 == test.GetAverage<int>());
        double avg = test.GetAverage<double>();
        CPPUNIT_ASSERT(2.49 < avg && 2.51 > avg);
    }

    void testGetAverageDelta()
    {
        DataSampler<int> test(5);
        
        // With 0 samples, the average delta should be 0.
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(0));
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(2));
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(5));
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(10));

        // With 1 sample, the average delta should be 0.
        test.AddSample(1);
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(0));
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(2));
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(5));
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(10));

        // With 2 samples.
        test.AddSample(2);
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(0));
        CPPUNIT_ASSERT(1 == test.GetAverageDelta(2));
        CPPUNIT_ASSERT(1 == test.GetAverageDelta(5));
        CPPUNIT_ASSERT(1 == test.GetAverageDelta(10));
        
        // With 10 samples (where 5 are scoodged out).
        test.AddSample(3); // Should be removed
        test.AddSample(3); // Should be removed
        test.AddSample(3); // Should be removed
        test.AddSample(100);
        test.AddSample(100);
        test.AddSample(108);
        test.AddSample(109);
        test.AddSample(110);
        CPPUNIT_ASSERT(0 == test.GetAverageDelta(0));
        CPPUNIT_ASSERT(1 == test.GetAverageDelta(2));
        CPPUNIT_ASSERT(2 == test.GetAverageDelta(5));
        CPPUNIT_ASSERT(2 == test.GetAverageDelta(10));
    }

    void testGetAverageDeltaFactored ()
    {
        DataSampler<int> test(5);
        test.AddSample(10);
        test.AddSample(20);
        CPPUNIT_ASSERT_EQUAL(420, test.GetAverageDeltaFactored(2, 42));
    }
    
    void testGetDelta ()
    {
        DataSampler<int> test(5);
        test.AddSample(2);
        test.AddSample(2);
        test.AddSample(4);
        CPPUNIT_ASSERT_EQUAL(2, test.GetDelta(3));
    }

    void testGetAt ()
    {
        DataSampler<int> test(5);
        test.AddSample(1);
        test.AddSample(2);
        test.AddSample(3);
        CPPUNIT_ASSERT_EQUAL(3, test[0]);
        CPPUNIT_ASSERT_EQUAL(2, test[1]);
        CPPUNIT_ASSERT_EQUAL(1, test[2]);
        CPPUNIT_ASSERT_THROW(test[3], SCXCoreLib::SCXIllegalIndexException<size_t>);  
        CPPUNIT_ASSERT_THROW(test[42], SCXCoreLib::SCXIllegalIndexException<size_t>); 
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void testClear ()
    {
        DataSampler<int> test(5);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), test.GetNumberOfSamples());
        test.Clear();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), test.GetNumberOfSamples());
        test.AddSample(1);
        test.AddSample(2);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), test.GetNumberOfSamples());
        test.Clear();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), test.GetNumberOfSamples());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( DataSampler_Test );

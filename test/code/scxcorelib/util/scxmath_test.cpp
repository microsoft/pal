/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-05-28 08:10:00

    math method test class.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxexception.h>
#include <testutils/scxunit.h>

#include <cppunit/extensions/HelperMacros.h>

using namespace SCXCoreLib;

class SCXMath_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXMath_Test  );

    CPPUNIT_TEST( TestPercent );
    CPPUNIT_TEST( TestBytesToMegaBytes );
    CPPUNIT_TEST( TestKiloBytesToMegaBytes );
    CPPUNIT_TEST( TestMIN );
    CPPUNIT_TEST( TestMAX );
    CPPUNIT_TEST( TestRound );
    CPPUNIT_TEST( TestRoundToLong );
    CPPUNIT_TEST( TestRoundToInt );
    CPPUNIT_TEST( TestRoundToUnsignedInt );
    CPPUNIT_TEST( TestPow );
    CPPUNIT_TEST( TestEqual );
    CPPUNIT_TEST_SUITE_END();
 
protected:
    

    void TestPercent()
    {
        // Check that percent calculation is OK in a normal case
        CPPUNIT_ASSERT(GetPercentage(10, 20, 100, 200) == 10);
        CPPUNIT_ASSERT(GetPercentage(0, 20, 0, 200) == 10);
        CPPUNIT_ASSERT(GetPercentage(42, 42, 142, 142) == 0);
        CPPUNIT_ASSERT_THROW(GetPercentage(20, 10, 200, 100), SCXInvalidArgumentException);
        CPPUNIT_ASSERT_THROW(GetPercentage(10, 20, 200, 100), SCXInvalidArgumentException);
        CPPUNIT_ASSERT_THROW(GetPercentage(20, 10, 100, 200), SCXInvalidArgumentException);

        // Check that percent calculation is OK in a normal inverse case
        CPPUNIT_ASSERT(GetPercentage(0, 0, 0, 0, true) == 0);
        CPPUNIT_ASSERT(GetPercentage(10, 20, 100, 200, true) == 90);
        CPPUNIT_ASSERT(GetPercentage(0, 20, 0, 200, true) == 90);
        CPPUNIT_ASSERT(GetPercentage(42, 42, 142, 142, true) == 100);
        CPPUNIT_ASSERT_THROW(GetPercentage(20, 10, 200, 100, true), SCXInvalidArgumentException);
        CPPUNIT_ASSERT_THROW(GetPercentage(10, 20, 200, 100, true), SCXInvalidArgumentException);
        CPPUNIT_ASSERT_THROW(GetPercentage(20, 10, 100, 200, true), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestBytesToMegaBytes()
    {
        scxulong mbInt = 2*1024*1024+512*1024;
        double mbDouble = 2*1024*1024+512*1024;
        CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(2), BytesToMegaBytes(mbInt));
        CPPUNIT_ASSERT_EQUAL(2.5, BytesToMegaBytes(mbDouble));
    }

    void TestKiloBytesToMegaBytes()
    {
        scxulong mbInt = 2*1024+864;
        double mbDouble = 2*1024+256;
        CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(2), KiloBytesToMegaBytes(mbInt));
        CPPUNIT_ASSERT_EQUAL(2.25, KiloBytesToMegaBytes(mbDouble));
    }

    void TestMIN()
    {
        CPPUNIT_ASSERT_EQUAL(42, MIN(42,42));
        CPPUNIT_ASSERT_EQUAL(0, MIN(0,1));
        CPPUNIT_ASSERT_EQUAL(1, MIN(2,1));
        CPPUNIT_ASSERT_EQUAL(-2, MIN(-2,1));
    }

    void TestMAX()
    {
        CPPUNIT_ASSERT_EQUAL(42, MAX(42,42));
        CPPUNIT_ASSERT_EQUAL(1, MAX(0,1));
        CPPUNIT_ASSERT_EQUAL(2, MAX(2,1));
        CPPUNIT_ASSERT_EQUAL(0, MAX(-2,0));
    }

    void TestRound() {
        CPPUNIT_ASSERT(Equal(Round(4.4), 4, 0));
        CPPUNIT_ASSERT(Equal(Round(3.6), 4, 0));
        CPPUNIT_ASSERT(Equal(Round(-4.4), -4, 0));
        CPPUNIT_ASSERT(Equal(Round(-3.6), -4, 0));
    }

    void TestRoundToLong() {
        CPPUNIT_ASSERT(RoundToScxLong(4.4) == 4);
        CPPUNIT_ASSERT(RoundToScxLong(3.6) == 4);
        CPPUNIT_ASSERT(RoundToScxLong(-4.4) == -4);
        CPPUNIT_ASSERT(RoundToScxLong(-3.6) == -4);
        CPPUNIT_ASSERT_THROW(RoundToScxLong(1E20), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestRoundToInt() {
        CPPUNIT_ASSERT(RoundToInt(4.4) == 4);
        CPPUNIT_ASSERT(RoundToInt(3.6) == 4);
        CPPUNIT_ASSERT(RoundToInt(-4.4) == -4);
        CPPUNIT_ASSERT(RoundToInt(-3.6) == -4);
        CPPUNIT_ASSERT_THROW(RoundToInt(3E9), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestRoundToUnsignedInt() {
        CPPUNIT_ASSERT(RoundToUnsignedInt(4.4) == 4);
        CPPUNIT_ASSERT(RoundToUnsignedInt(3.6) == 4);
        CPPUNIT_ASSERT_THROW(RoundToUnsignedInt(-4.4), SCXInvalidArgumentException);
        CPPUNIT_ASSERT_THROW(RoundToUnsignedInt(-3.6), SCXInvalidArgumentException);
        CPPUNIT_ASSERT_THROW(RoundToUnsignedInt(5E9), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }
    
    void TestPow() {
    	CPPUNIT_ASSERT(Pow(-1, 0) == 1);
    	CPPUNIT_ASSERT(Pow(0, 0) == 1);
    	CPPUNIT_ASSERT(Pow(2, 1) == 2);
    	CPPUNIT_ASSERT(Pow(2, 5) == 32);
    	CPPUNIT_ASSERT(Pow(2, 6) == 64);
    	CPPUNIT_ASSERT(Pow(2, 7) == 128);
    	CPPUNIT_ASSERT(Pow(3, 7) == 2187);
    	CPPUNIT_ASSERT(Pow(2, 8) == 256);
    } 
    
    void TestEqual() {
        CPPUNIT_ASSERT(Equal(4, 4, 0));
        CPPUNIT_ASSERT(Equal(4, 5, 1));
        CPPUNIT_ASSERT(Equal(5, 4, 1));
        CPPUNIT_ASSERT(!Equal(5.000001, 4, 1));
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXMath_Test );

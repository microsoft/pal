/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Test limits.

    \date        2007-10-11 15:33:44

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlimit.h>
#include <testutils/scxunit.h>

#if defined(hpux) && (PF_MAJOR==11) && (PF_MINOR==23)
// WI7946
#define llabs(val) ((scxlong) (val) > (scxlong) 0 ? (scxulong) (val) : (scxulong) -(val))
#endif

class SCXLimitTest : public CPPUNIT_NS::TestFixture 
{
    CPPUNIT_TEST_SUITE( SCXLimitTest );
    CPPUNIT_TEST( TestMaxLong );
    CPPUNIT_TEST( TestMinLong );
    CPPUNIT_TEST_SUITE_END();

public:
    void TestMaxLong(void)
    {
        CPPUNIT_ASSERT(0 < cMaxScxLong);
        // The representation if positive values implies all bits except the leftmost bit is set,
        scxlong v = cMaxScxLong;
        for (size_t i=0; i<7; ++i)
        {
            CPPUNIT_ASSERT_EQUAL(static_cast<scxlong>(0xFF),v&0xFF);
            v >>= 8;
        }
        CPPUNIT_ASSERT_EQUAL(static_cast<scxlong>(0x7F),v&0xFF);
    }

    void TestMinLong(void)
    {
        CPPUNIT_ASSERT(cMinScxLong < 0);
        // The representation of negative values implies that the absolute value of smallest negative value is
        // the same as the largest positive value +1.
#if defined(hpux) || defined(linux) || defined(sun) || defined(aix) || defined(macos)
        CPPUNIT_ASSERT_EQUAL(cMaxScxLong,static_cast<scxlong>(llabs(cMinScxLong+1)));
#elif defined(WIN32)
        CPPUNIT_ASSERT_EQUAL(cMaxScxLong,static_cast<scxlong>(_abs64(cMinScxLong+1)));
#else
#error "Platform not supported"
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLimitTest );

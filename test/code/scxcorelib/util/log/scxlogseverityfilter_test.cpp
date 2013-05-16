/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-07-25 14:00:46

    Log severity filter test
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include "scxcorelib/util/log/scxlogseverityfilter.h"
#include <scxcorelib/scxlogitem.h>

using namespace SCXCoreLib;


class SCXLogSeverityFilterTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogSeverityFilterTest );
    CPPUNIT_TEST( TestEmptyFilter );
    CPPUNIT_TEST( TestSetSeverityThreshold );
    CPPUNIT_TEST( TestSeverityInheritance );
    CPPUNIT_TEST( TestHysterical );
    CPPUNIT_TEST( TestIsLogable );
    CPPUNIT_TEST( TestClearSeverityThreshold );
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestEmptyFilter()
    {
        SCXLogSeverityFilter f;
        CPPUNIT_ASSERT(eNotSet == f.GetSeverityThreshold(L"what.ever"));
        CPPUNIT_ASSERT(eNotSet == f.GetSeverityThreshold(L""));
    }

    void TestSetSeverityThreshold()
    {
        SCXLogSeverityFilter f;
        CPPUNIT_ASSERT( true == f.SetSeverityThreshold(L"", eError) );
        CPPUNIT_ASSERT( false == f.SetSeverityThreshold(L"", eError) );
        CPPUNIT_ASSERT( true == f.SetSeverityThreshold(L"scx.core", eWarning) );
        CPPUNIT_ASSERT( false == f.SetSeverityThreshold(L"scx.core", eWarning) );
        CPPUNIT_ASSERT( true == f.SetSeverityThreshold(L"scx.core", eInfo) );
        CPPUNIT_ASSERT( true == f.SetSeverityThreshold(L"foo.bar", eError) );
    }

    void TestSeverityInheritance()
    {
        SCXLogSeverityFilter f;
        f.SetSeverityThreshold(L"", eError);
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.ever") );
        f.SetSeverityThreshold(L"what.ever", eWarning);
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eWarning == f.GetSeverityThreshold(L"what.ever") );
        CPPUNIT_ASSERT( eWarning == f.GetSeverityThreshold(L"what.ever.dude") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.you.want") );
    }

    void TestHysterical()
    {
        // You can not set hysterical on the root module
        SCXLogSeverityFilter f;
        CPPUNIT_ASSERT( false == f.SetSeverityThreshold(L"", eHysterical) );
        CPPUNIT_ASSERT( true == f.SetSeverityThreshold(L"scx.core", eHysterical) );
        CPPUNIT_ASSERT( eHysterical == f.GetSeverityThreshold(L"scx.core") );
        CPPUNIT_ASSERT( eNotSet == f.GetSeverityThreshold(L"scx.core.whar.ever") );
    }
    
    void TestIsLogable()
    {
        SCXLogSeverityFilter f;

        SCXLogItem notSet     = SCXLogItem(L"scx.core", eNotSet,     L"something", SCXSRCLOCATION, 0);
        SCXLogItem hysterical = SCXLogItem(L"scx.core", eHysterical, L"something", SCXSRCLOCATION, 0);
        SCXLogItem trace      = SCXLogItem(L"scx.core", eTrace,      L"something", SCXSRCLOCATION, 0);
        SCXLogItem info       = SCXLogItem(L"scx.core", eInfo,       L"something", SCXSRCLOCATION, 0);
        SCXLogItem warning    = SCXLogItem(L"scx.core", eWarning,    L"something", SCXSRCLOCATION, 0);
        SCXLogItem error      = SCXLogItem(L"scx.core", eError,      L"something", SCXSRCLOCATION, 0);

        f.SetSeverityThreshold(L"scx.core", eNotSet);
        CPPUNIT_ASSERT(false  == f.IsLogable(notSet));
        CPPUNIT_ASSERT(false  == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(false  == f.IsLogable(trace));
        CPPUNIT_ASSERT(false  == f.IsLogable(info));
        CPPUNIT_ASSERT(false  == f.IsLogable(warning));
        CPPUNIT_ASSERT(false  == f.IsLogable(error));

        f.SetSeverityThreshold(L"scx.core", eHysterical);
        CPPUNIT_ASSERT(false == f.IsLogable(notSet));
        CPPUNIT_ASSERT(true  == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(true  == f.IsLogable(trace));
        CPPUNIT_ASSERT(true  == f.IsLogable(info));
        CPPUNIT_ASSERT(true  == f.IsLogable(warning));
        CPPUNIT_ASSERT(true  == f.IsLogable(error));

        f.SetSeverityThreshold(L"scx.core", eTrace);
        CPPUNIT_ASSERT(false == f.IsLogable(notSet));
        CPPUNIT_ASSERT(false == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(true  == f.IsLogable(trace));
        CPPUNIT_ASSERT(true  == f.IsLogable(info));
        CPPUNIT_ASSERT(true  == f.IsLogable(warning));
        CPPUNIT_ASSERT(true  == f.IsLogable(error));

        f.SetSeverityThreshold(L"scx.core", eInfo);
        CPPUNIT_ASSERT(false == f.IsLogable(notSet));
        CPPUNIT_ASSERT(false == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(false == f.IsLogable(trace));
        CPPUNIT_ASSERT(true  == f.IsLogable(info));
        CPPUNIT_ASSERT(true  == f.IsLogable(warning));
        CPPUNIT_ASSERT(true  == f.IsLogable(error));

        f.SetSeverityThreshold(L"scx.core", eWarning);
        CPPUNIT_ASSERT(false == f.IsLogable(notSet));
        CPPUNIT_ASSERT(false == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(false == f.IsLogable(trace));
        CPPUNIT_ASSERT(false == f.IsLogable(info));
        CPPUNIT_ASSERT(true  == f.IsLogable(warning));
        CPPUNIT_ASSERT(true  == f.IsLogable(error));

        f.SetSeverityThreshold(L"scx.core", eError);
        CPPUNIT_ASSERT(false == f.IsLogable(notSet));
        CPPUNIT_ASSERT(false == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(false == f.IsLogable(trace));
        CPPUNIT_ASSERT(false == f.IsLogable(info));
        CPPUNIT_ASSERT(false == f.IsLogable(warning));
        CPPUNIT_ASSERT(true  == f.IsLogable(error));

        f.SetSeverityThreshold(L"scx.core", eSuppress);
        CPPUNIT_ASSERT(false == f.IsLogable(notSet));
        CPPUNIT_ASSERT(false == f.IsLogable(hysterical));
        CPPUNIT_ASSERT(false == f.IsLogable(trace));
        CPPUNIT_ASSERT(false == f.IsLogable(info));
        CPPUNIT_ASSERT(false == f.IsLogable(warning));
        CPPUNIT_ASSERT(false == f.IsLogable(error));
    }

    void TestClearSeverityThreshold()
    {
        SCXLogSeverityFilter f;
        f.SetSeverityThreshold(L"", eError);
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.ever") );
        f.SetSeverityThreshold(L"what.ever", eWarning);
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eWarning == f.GetSeverityThreshold(L"what.ever") );
        CPPUNIT_ASSERT( eWarning == f.GetSeverityThreshold(L"what.ever.dude") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.you.want") );

        // Clearing a severity threshold that is not speciffically set does nothing (and returns false).
        CPPUNIT_ASSERT( false == f.ClearSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eWarning == f.GetSeverityThreshold(L"what.ever") );
        CPPUNIT_ASSERT( eWarning == f.GetSeverityThreshold(L"what.ever.dude") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.you.want") );

        // Clearing a severity threshold will affect the ones below through inheritance.
        CPPUNIT_ASSERT( true == f.ClearSeverityThreshold(L"what.ever") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.ever") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.ever.dude") );
        CPPUNIT_ASSERT( eError == f.GetSeverityThreshold(L"what.you.want") );
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogSeverityFilterTest );

/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Tests for the simple pattern finder.

    \date        2008-01-28 16:35:56

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxpatternfinder.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h> 
#include <testutils/scxunit.h> 

class SCXPatternFindTest : public CPPUNIT_NS::TestFixture 
{
    CPPUNIT_TEST_SUITE( SCXPatternFindTest );
    CPPUNIT_TEST( RegisterInvalidPatternFails );
    CPPUNIT_TEST( ReplacePatternFails );
    CPPUNIT_TEST( NoParametersMatchFound );
    CPPUNIT_TEST( NoParametersNoMatch );
    CPPUNIT_TEST( OneParametersMatchFound );
    CPPUNIT_TEST( OneParametersNoMatch );
    CPPUNIT_TEST( FiveParametersMatchFound );
    CPPUNIT_TEST( FiveParametersNoMatch );
    CPPUNIT_TEST( TestTypicalCQL );
    CPPUNIT_TEST( TestEmptyMatches );
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXPatternFinder> m_pf;

public:

    void setUp(void)
    {
        m_pf = new SCXCoreLib::SCXPatternFinder();
        m_pf->RegisterPattern(static_cast<SCXCoreLib::SCXPatternFinder::SCXPatternCookie>(0),
                              L"A random pattern that should never match");
    }

    void tearDown(void)
    {
        m_pf = 0;
    }


    void RegisterInvalidPatternFails()
    {
        std::wstring pattern(L"pattern '1");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 42;
        CPPUNIT_ASSERT_THROW(m_pf->RegisterPattern(cookie, pattern), SCXCoreLib::SCXInternalErrorException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void ReplacePatternFails()
    {
        std::wstring pattern(L"pattern1");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 42;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT_THROW(m_pf->RegisterPattern(cookie, L"pattern2"), SCXCoreLib::SCXInternalErrorException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void NoParametersMatchFound()
    {
        std::wstring pattern(L"This is a pattern with \"no parameters\"");
        std::wstring test(SCXCoreLib::StrToUpper(pattern));
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 4711;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT(m_pf->Match(test, found, matches));
        CPPUNIT_ASSERT_EQUAL(cookie, found);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0),matches.size());
    }

    void NoParametersNoMatch()
    {
        std::wstring pattern(L"Some pattern");
        std::wstring test1(L"Some other pattern");
        std::wstring test2(L"Another pattern");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 4711;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT( ! m_pf->Match(test1, found, matches));
        CPPUNIT_ASSERT( ! m_pf->Match(test2, found, matches));
    }

    void OneParametersMatchFound()
    {
        std::wstring pattern(L"This is a pattern with %p parameter");
        std::wstring test(L"This is a pattern with ONE parameter");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 4711;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT(m_pf->Match(test, found, matches));
        CPPUNIT_ASSERT_EQUAL(cookie, found);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1),matches.size());
        CPPUNIT_ASSERT(matches.end() != matches.find(L"p"));
        CPPUNIT_ASSERT(L"ONE" == matches.find(L"p")->second);
    }

    void OneParametersNoMatch()
    {
        std::wstring pattern(L"This is a pattern with %p parameter");
        std::wstring test(L"This is a pattern with ONE parameter(s)");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 4711;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT( ! m_pf->Match(test, found, matches));
    }

    void FiveParametersMatchFound()
    {
        std::wstring pattern(L"%This %is %a '%pattern' '%with parameters'");
        std::wstring test(L"This is a pattern \"with more parameters\"");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 4711;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT(m_pf->Match(test, found, matches));
        CPPUNIT_ASSERT_EQUAL(cookie, found);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(5),matches.size());
        CPPUNIT_ASSERT(matches.end() != matches.find(L"This"));
        CPPUNIT_ASSERT(matches.end() != matches.find(L"is"));
        CPPUNIT_ASSERT(matches.end() != matches.find(L"a"));
        CPPUNIT_ASSERT(matches.end() != matches.find(L"pattern"));
        CPPUNIT_ASSERT(matches.end() != matches.find(L"with parameters"));
        CPPUNIT_ASSERT(L"This" == matches.find(L"This")->second);
        CPPUNIT_ASSERT(L"is" == matches.find(L"is")->second);
        CPPUNIT_ASSERT(L"a" == matches.find(L"a")->second);
        CPPUNIT_ASSERT(L"pattern" == matches.find(L"pattern")->second);
        CPPUNIT_ASSERT(L"with more parameters" == matches.find(L"with parameters")->second);
        
    }

    void FiveParametersNoMatch()
    {
        std::wstring pattern(L"%This %is a %pattern \"%with parameters\"");
        std::wstring test(L"This is an pattern \"with parameters\"");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 4711;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT( ! m_pf->Match(test, found, matches));
    }

    void TestTypicalCQL()
    {
        std::wstring pattern(L"select * from scx_logfilerecord where filename=%PATH");
        std::wstring test(L"select * from scx_logfilerecord where filename=\"/some/path\"");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 668;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT(m_pf->Match(test, found, matches));
        CPPUNIT_ASSERT_EQUAL(cookie, found);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1),matches.size());
        CPPUNIT_ASSERT(matches.end() != matches.find(L"PATH"));
        CPPUNIT_ASSERT(L"/some/path" == matches.find(L"PATH")->second);
    }

    void TestEmptyMatches()
    {
        std::wstring pattern(L"Find a=%a b=%b");
        std::wstring test(L"Find a= b=''");
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie cookie = 17;
        SCXCoreLib::SCXPatternFinder::SCXPatternCookie found;
        SCXCoreLib::SCXPatternFinder::SCXPatternMatch matches;
        CPPUNIT_ASSERT_NO_THROW(m_pf->RegisterPattern(cookie, pattern));
        CPPUNIT_ASSERT(m_pf->Match(test, found, matches));
        CPPUNIT_ASSERT_EQUAL(cookie, found);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2),matches.size());
        CPPUNIT_ASSERT(matches.end() != matches.find(L"a"));
        CPPUNIT_ASSERT(matches.end() != matches.find(L"b"));
        CPPUNIT_ASSERT(L"" == matches.find(L"a")->second);
        CPPUNIT_ASSERT(L"" == matches.find(L"a")->second);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXPatternFindTest );

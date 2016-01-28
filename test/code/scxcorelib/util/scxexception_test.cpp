/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date 07-06-04 22:31:53
    
    CPPUnit tests for the SCXException base class
 
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxwql.h>
#include <scxcorelib/scxexception.h> 

#include <scxcorelib/scxfilepath.h>
#ifndef WIN32
#include <scxcorelib/scxregex.h>
#endif
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scx_widen_string.h>

#include <testutils/scxunit.h>
#include <sstream>
#include <errno.h>

using namespace std;
using namespace SCXCoreLib;


namespace {

    const wstring thisFile(L"scxexception_test.cpp");

    // Dummy test exception that calls SCXException default constructor.
    class TestExceptionDummy : public SCXException {
    public:
        TestExceptionDummy() : SCXException() {}

        wstring What() const { return L"This is TestExceptionDummy"; }
    };

    // Intermediate level test exception
    class TestExceptionBase : public SCXException {
    public: 
        TestExceptionBase(const SCXCodeLocation& location) : SCXException(location) {};
    protected:
    private:
    };
    
    // Derived test exception 1
    class TestExceptionD1 : public TestExceptionBase {
    public: 
    TestExceptionD1(const SCXCodeLocation& location) : TestExceptionBase(location) {};
        
        wstring What() const { return L"This is TestExceptionD1"; }
        
    protected:
    private:
    };
    
    // Derived  test exception 2
    class TestExceptionD2 : public TestExceptionBase {
    public: 
        TestExceptionD2(const SCXCodeLocation& location) : TestExceptionBase(location) {};

        wstring What() const { return L"This is TestExceptionD2"; }
        
    protected:
    private:
        
        
    };
    
    
    
    void f1() {
        throw TestExceptionD1(SCXSRCLOCATION); 
    }

    void f2() {
        try 
        {
            f1();
        }
        catch (SCXException& e) 
        {
            e.AddStackContext(L"Extra String and extra location");
            e.AddStackContext(SCXSRCLOCATION);
            SCXRETHROW(e, L"f2()");
        }
    }
    
    void f3() 
    {
        f2();
    }
 
    void f4() 
    {
        try {
            f3();
        }
        catch (SCXException& e) 
        {
            SCXRETHROW(e, L"f4()");
        }
    }
    
    void f5() 
    {
        f4();
    }


    // Build a fake SCXCodeLocation string
    wstring ComposeSCXCodeLocation(wstring /*file*/, unsigned int line)
    {
        wstringstream location; 

        // Compose string like
        // [../...../scxexception_test.cpp:50] 
        location << wstring(L"[") << __SCXWFILE__ << L":" << StrFrom(line) << L"]";

        return location.str();
    }


}
    
    
    
/******************************************************************************
 *  
 *   This is where the tests start
 *  
 ******************************************************************************/

class SCXCodeLocationTest :public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXCodeLocationTest );
    CPPUNIT_TEST( TestSCXCodeLocationEmpty );
    CPPUNIT_TEST( TestSCXCodeLocation );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void)
    {
    }
    
    void tearDown(void)
    {
    }
    
    void TestSCXCodeLocationEmpty(void)
    {
        SCXCodeLocation emptyLocation;
        CPPUNIT_ASSERT(emptyLocation.GotInfo() == false);
    };

    void TestSCXCodeLocation(void)
    {
        SCXCodeLocation thisLocation(SCXSRCLOCATION);

        CPPUNIT_ASSERT(thisLocation.GotInfo() == true);
        
        // Verify the origin string is OK
        unsigned int originatingLine = __LINE__ - 5; // 5 lines above this one
        wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);

        CPPUNIT_ASSERT(origin == thisLocation.Where());
    }
};

    
class SCXExceptionTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXExceptionTest );
    CPPUNIT_TEST( TestRethrow );
    CPPUNIT_TEST( TestInvalidArgumentException );
    CPPUNIT_TEST( TestInvalidStateException );
    CPPUNIT_TEST( TestNULLPointerException );
    CPPUNIT_TEST( TestSCXIllegalIndexException );
    CPPUNIT_TEST( TestSCXNotSupportedException );
    CPPUNIT_TEST( TestSCXInternalErrorException );
    CPPUNIT_TEST( TestSCXResourceExhaustedException );
    CPPUNIT_TEST( TestErrnoException );
    CPPUNIT_TEST( TestErrnoFileException );
    CPPUNIT_TEST( TestInvalidRegexException );
    CPPUNIT_TEST_SUITE_END();
    
private:
    
public:
    void setUp(void)
    {
    }
    
    void tearDown(void)
    {
    }
    
    void TestRethrow(void)
    {
        // Test that this is still an SCXException
        // We cannot use CPPUNIT_ASSERT_THROW(f5(), SCXException);
        // Sine it is caught in the modified CPPUNIT_ASSERT_THROW macro.
        try {
            f5();
            CPPUNIT_FAIL("No exception thrown");
        }
        catch (const SCXException& )
        {
            // OK
        }
        catch (...)
        {
            CPPUNIT_FAIL("Excpetion thrown is not SCXException");
        }
        
        // More specifically a TestExceptionD1
        CPPUNIT_ASSERT_THROW(f5(), TestExceptionD1);
        
        // Check that it does not match wrong thing
        try {
            f5();
        }
        catch (TestExceptionD2& /*e*/) {
            CPPUNIT_ASSERT( ! "Exception of wrong type caught" );
        }
        catch (TestExceptionD1& e) {
            
            // So far so good. Start checking data in exception
            
            // Check that What() works, in general
            CPPUNIT_ASSERT(e.What() == L"This is TestExceptionD1");
            
            // Check that the stack contents are in place 
            {
                wstring where_string = e.Where();

                // The exception shall know it carries location info
                CPPUNIT_ASSERT(e.GotLocationInfo());

                // where string is now expected to be something like 
                // f4()[../../test/code/common_lib/util/scxexception_test.cpp:91]->f2()[../../test/code/common_lib/util/scxexception_test.cpp:75], thrown from [../../test/code/common_lib/util/scxexception_test.cpp:65]

                wstring::size_type loc_f4 = where_string.find(L"f4()");
                CPPUNIT_ASSERT(loc_f4 != wstring::npos);
                
                wstring::size_type loc_f2 = where_string.find(L"f2()", loc_f4);
                CPPUNIT_ASSERT(loc_f2 != wstring::npos);
                
                // Just check that the string finding test is OK, f3() does not rethrow
                wstring::size_type loc_notfound = where_string.find(L"f3()");
                CPPUNIT_ASSERT(loc_notfound == wstring::npos);
                // The exception is thrown once, then rethrown two times. Thus
                // the file name of this file should occur three times:

                wstring::size_type loc_filename1 = where_string.find(__SCXWFILE__);
                CPPUNIT_ASSERT(loc_filename1 != wstring::npos);

                wstring::size_type loc_filename2 = where_string.find(__SCXWFILE__, 
                                                                     loc_filename1 + wstring(__SCXWFILE__).length());
                CPPUNIT_ASSERT(loc_filename2 != wstring::npos);
                CPPUNIT_ASSERT(loc_filename2 > loc_filename1);

                wstring::size_type loc_filename3 = where_string.find(thisFile, 
                                                                     loc_filename2 + wstring(__SCXWFILE__).length());
                CPPUNIT_ASSERT(loc_filename3 != wstring::npos);
                CPPUNIT_ASSERT(loc_filename3 > loc_filename2);

                wstring::size_type loc_filename4 = where_string.find(thisFile, 
                                                                     loc_filename3 + wstring(__SCXWFILE__).length());
                CPPUNIT_ASSERT(loc_filename4 != wstring::npos);
                CPPUNIT_ASSERT(loc_filename4 > loc_filename3);

                wstring::size_type loc_filename5 = where_string.find(thisFile,
                                                                     loc_filename4 + wstring(__SCXWFILE__).length());
                CPPUNIT_ASSERT(loc_filename5 == wstring::npos);

            }
        }
    }


    void TestInvalidArgumentException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXInvalidArgumentException(L"myArgument", 
                                              L"Pretending it is mandatory", 
                                              SCXSRCLOCATION);
        }
        catch (SCXException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == L"Formal argument 'myArgument' is invalid: Pretending it is mandatory");
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myArgument"));
#endif
        }
    };

    void TestInvalidStateException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXInvalidStateException(L"myReason", SCXSRCLOCATION);
        }
        catch (SCXException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == L"Invalid state: myReason");
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myReason"));
#endif
        }
    };

    void TestNULLPointerException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXNULLPointerException(L"myPointer", 
                                          SCXSRCLOCATION);
        }
        catch (SCXException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == L"A NULL pointer was supplied in argument 'myPointer'");
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myPointer"));
#endif
        }
    };
    
    void TestSCXIllegalIndexException(void)
    {
        //--------------------------------------------------------------------------------
        // First no boundaries of unsigned integer type
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXIllegalIndexException<unsigned int>(L"myIndex", 
                                                         999,
                                                         SCXSRCLOCATION);
        }
        catch (SCXException& e) 
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Index 'myIndex' has illegal value 999"));
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myIndex"));
#endif
        };

        //--------------------------------------------------------------------------------
        //  No boundaries of signed integer type
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXIllegalIndexException<int>(L"myIndex", 
                                                -1,
                                                SCXSRCLOCATION);
        }
        catch (SCXException& e) 
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Index 'myIndex' has illegal value -1"));
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myIndex"));
#endif
        }

        //--------------------------------------------------------------------------------
        // Double boundaries, unsigned integer type using typedef
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXIllegalIndexExceptionUInt(L"myIndex", 
                                               12,
                                               4, true,
                                               10, true,
                                               SCXSRCLOCATION);
        }
        catch (SCXException& e) 
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Index 'myIndex' has illegal value 12 - boundaries are 4 and 10"));
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myIndex"));
#endif
        };

        //--------------------------------------------------------------------------------
        //  Lower boundary, signed integer type
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXIllegalIndexException<int>(L"myIndex", 
                                                -1,
                                                0, true,
                                                0, false,
                                                SCXSRCLOCATION);
        }
        catch (SCXException& e) 
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Index 'myIndex' has illegal value -1 - lower boundary is 0"));
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myIndex"));
#endif
        }

        //--------------------------------------------------------------------------------
        //  Upper boundary, unsigned integer type
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXIllegalIndexException<unsigned int>(L"myIndex", 
                                                         10,
                                                         0, false,
                                                         5, true,
                                                         SCXSRCLOCATION);
        }
        catch (SCXException& e) 
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Index 'myIndex' has illegal value 10 - upper boundary is 5"));
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"myIndex"));
#endif
        }

        //--------------------------------------------------------------------------------
        // Double boundaries, initiated with wstring 
//         try {
//             throw SCXIllegalIndexException<wstring, L"">(L"myIndex", 
//                                                          L"middle",
//                                                          L"Lower", true,
//                                                          L"Upper", true,
//                                                          SCXSRCLOCATION);
//         }
//         catch (SCXException& e) 
//         {
//             unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

//             // Verify the origin string is OK in Where() 
//             wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
//             wstring::size_type loc = e.Where().find(origin);
//             CPPUNIT_ASSERT(loc != wstring::npos);

//             // Verify What() returns proper string
//             CPPUNIT_ASSERT(e.What() == wstring(L"Index 'myIndex' has illegal value middle - boundaries are Lower and Upper"));
//        };

    };

    void TestSCXNotSupportedException(void)
    {
        try {
            throw SCXNotSupportedException(L"Indexing sausages", 
                                           SCXSRCLOCATION);
        }
        catch (SCXException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Indexing sausages not supported"));
        }
    };

    void TestSCXInternalErrorException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXInternalErrorException(L"Item not found", 
                                            SCXSRCLOCATION);
        }
        catch (SCXException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin, 0);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Internal Error: Item not found"));
            
            // Verify that throwing this exception also asserts.
            SCXUNIT_ASSERTIONS_FAILED(1);
#ifndef NDEBUG
            CPPUNIT_ASSERT(wstring::npos != SCXAssertCounter::GetLastMessage().find(L"Item not found"));
#endif
        }
    };

    void TestSCXResourceExhaustedException(void)
    {
        try {
            throw SCXResourceExhaustedException(L"Shared memory", 
                                                L"Address already mapped",
                                                SCXSRCLOCATION);
        }
        catch (SCXException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            CPPUNIT_ASSERT(e.What() == wstring(L"Failed to allocate resource of type Shared memory: "
                                               L"Address already mapped"));
        }
    };

    void TestErrnoException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXErrnoException(L"myFunction",
                                    2,
                                    SCXSRCLOCATION);
        }
        catch (SCXErrnoException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            wstring whatText = e.What();
            wstring::size_type whatLoc = whatText.find(L"Calling myFunction() returned an error with errno = 2 (");
            CPPUNIT_ASSERT(whatLoc != wstring::npos);

            // Verify ErrorNumber() returns the proper number
            CPPUNIT_ASSERT(e.ErrorNumber() == 2);
        }
    };

    void TestErrnoFileException(void)
    {
        SCXUNIT_RESET_ASSERTION();
        try {
            throw SCXErrnoFileException(L"myFunction",
                                    L"/my/file/path",
                                    2,
                                    SCXSRCLOCATION);
        }
        catch (SCXErrnoFileException& e)
        {
            unsigned int originatingLine = __LINE__ - 4; // 4 lines above this one

            // Verify the origin string is OK in Where() 
            wstring origin = ComposeSCXCodeLocation(__SCXWFILE__, originatingLine);
            wstring::size_type loc = e.Where().find(origin);
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string
            wstring whatText = e.What();
            wstring::size_type whatLoc = whatText.find(L"Calling myFunction() with file \"/my/file/path\" returned an error with errno = 2 (");
            CPPUNIT_ASSERT(whatLoc != wstring::npos);

            // Verify ErrorNumber() returns the proper number
            CPPUNIT_ASSERT(e.ErrorNumber() == 2);

            // Verify GetFnkcall() returns proper string
            CPPUNIT_ASSERT(e.GetFnkcall() == L"myFunction");

            // Finally, verify that GetPath() returns proper string
            CPPUNIT_ASSERT(e.GetPath() == L"/my/file/path");
        }
    }

    void TestInvalidRegexException(void)
    {
#ifndef WIN32
        SCXUNIT_RESET_ASSERTION();
        try {
            // The SCXInvalidRegexException needs a preq (regex_t); just build an invalid expression
            SCXCoreLib::SCXRegex r1(L"*");
        }
        catch (SCXInvalidRegexException& e)
        {
            // Verify the origin string is OK in Where() - it's actually over in scxregex.cpp
            wstring::size_type loc = e.Where().find(L"scxregex.cpp");
            CPPUNIT_ASSERT(loc != wstring::npos);

            // Verify What() returns proper string (exact error text can vary by system)
            loc = e.What().find(L"Compiling * returned an error code =");
            CPPUNIT_ASSERT(loc != wstring::npos);
        }
#endif
    }
};

class SCXExceptionCoverageCalls :public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXExceptionCoverageCalls );
    CPPUNIT_TEST( CoverageForSCXException );
    CPPUNIT_TEST( CoverageForAccessViolation );
    CPPUNIT_TEST( CoverageForAnalyzeException );
    CPPUNIT_TEST( CoverageForErrnoException );
    CPPUNIT_TEST( CoverageForErrnoFileException );
    CPPUNIT_TEST( CoverageForErrnoOpenException );
    CPPUNIT_TEST_SUITE_END();

    SCXHandle<SCXException> GivenException(SCXException* e)
    {
        SCXHandle<SCXException> h(e);
        return h;
    }

    void VerifyException(const SCXHandle<SCXException>& e, const wstring& what)
    {
        wstring msg = L"\"" + what + L"\" not found in \"" + e->What() + L"\"";
        if ( ! what.empty()) {
            CPPUNIT_ASSERT_MESSAGE(StrToUTF8(msg), e->What().find(what) != wstring::npos);
        }
    }

public:
    void CoverageForSCXException()
    {
        TestExceptionDummy e;
        CPPUNIT_ASSERT(L"This is TestExceptionDummy" == e.What());
    }

    void CoverageForAccessViolation()
    {
        SCXHandle<SCXException> e = GivenException(new SCXAccessViolationException(L"REASON", SCXSRCLOCATION));
        VerifyException(e, L"REASON");
        VerifyException(e, L"Access violation");
    }

    void CoverageForAnalyzeException()
    {
        SCXHandle<SCXException> e = GivenException(new SCXAnalyzeException(L"REASON", SCXSRCLOCATION));
        VerifyException(e, L"REASON");
        VerifyException(e, L"analysis");
    }

    void CoverageForErrnoException()
    {
        SCXHandle<SCXErrnoException> e(new SCXErrnoException(L"FUNCTION", EINVAL, SCXSRCLOCATION));
        VerifyException(e, L"FUNCTION");
        VerifyException(e, StrFrom(EINVAL));
        VerifyException(e, StrFromUTF8(SCXCoreLib::strerror(EINVAL)));
        CPPUNIT_ASSERT_EQUAL(EINVAL, e->ErrorNumber());
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::strerror(EINVAL), e->ErrorText());
    }

    void CoverageForErrnoFileException()
    {
        SCXHandle<SCXErrnoFileException> e(new SCXErrnoFileException(L"FUNCTION", L"PATH", EINVAL, SCXSRCLOCATION));
        VerifyException(e, L"FUNCTION");
        VerifyException(e, L"PATH");
        VerifyException(e, StrFrom(EINVAL));
        VerifyException(e, StrFromUTF8(SCXCoreLib::strerror(EINVAL)));
        VerifyException(e, e->GetFnkcall());
        VerifyException(e, e->GetPath());
    }

    void CoverageForErrnoOpenException()
    {
        SCXHandle<SCXException> e = GivenException(new SCXErrnoOpenException(L"PATH", EINVAL, SCXSRCLOCATION));
        VerifyException(e, L"open");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXCodeLocationTest );
CPPUNIT_TEST_SUITE_REGISTRATION( SCXExceptionTest );
CPPUNIT_TEST_SUITE_REGISTRATION( SCXExceptionCoverageCalls );
    
    
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
    


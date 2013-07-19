/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file        

    \brief       Unit test help macros for SCX project.

    \date        2007-06-18 11:13:00

    These macros can be used in unit tests to verify number of assertions
    passed/ignored during test. Typical usage:

    \ex
    \code
    void MyTest()
    {
        SCXUNIT_RESET_ASSERTION();       // make sure the assertion counter is reset
        CPPUNIT_ASSERT(42 == foo());     // run method and check functionality
        SCXUNIT_ASSERTIONS_FAILED(3);    // Check that the number of assertions ignored since last reset is the expected
        CPPUNIT_ASSERT(4711 != bar());   // Run new test (note that SCXUNIT_ASSERTIONS_FAILED resets the counter
        SCXUNIT_ASSERTIONS_FAILED_ANY(); // Check if at least one assertion was ignored.
    }
    \endcode

*/
/*----------------------------------------------------------------------*/
#ifndef SCXUNIT_H
#define SCXUNIT_H

// WI 13185
#if defined(macos) && (PF_MAJOR<=10) && (PF_MINOR<=4)
extern "C++" {
#include <fstream>
#include <deque>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <ostream>
}
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <scxcorelib/scxcmn.h>
#include <testutils/scxassert_cppunit.h>
#include <testutils/scxunitwarning.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunittestcaller.h>

#ifdef NDEBUG
/** Reset assertion counter. */
#define SCXUNIT_RESET_ASSERTION() 
/** Returns true if any assertions failed. */
#define SCXUNIT_ASSERTIONS_FAILED_ANY() 
/** Get number of failed assertions. */
#define SCXUNIT_ASSERTIONS_FAILED(n) 
#else
/** Reset assertion counter. */
#define SCXUNIT_RESET_ASSERTION() SCXCoreLib::SCXAssertCounter::Reset()
/** Returns true if any assertions failed. */
#define SCXUNIT_ASSERTIONS_FAILED_ANY() CPPUNIT_ASSERT(0 != SCXCoreLib::SCXAssertCounter::GetFailedAsserts()); SCXCoreLib::SCXAssertCounter::Reset()
/** Get number of failed assertions. */
#define SCXUNIT_ASSERTIONS_FAILED(n) CPPUNIT_ASSERT((n) == SCXCoreLib::SCXAssertCounter::GetFailedAsserts()); SCXCoreLib::SCXAssertCounter::Reset()
#endif /* NDEBUG */

/** Verify that value is between lower and higher. */
#define SCXUNIT_ASSERT_BETWEEN(value, lower, higher) CPPUNIT_ASSERT(((lower) <= (value)) && ((value) <= (higher)))

/** Verify that value is between lower and higher (with message). */
#define SCXUNIT_ASSERT_BETWEEN_MESSAGE(message, value, lower, higher) CPPUNIT_ASSERT_MESSAGE(message, ((lower) <= (value)) && ((value) <= (higher)))

/** Verify that value1 and value2 are either both equal to ref or both larger than ref. */
#define SCXUNIT_ASSERT_BOTH_AT_OR_BOTH_ABOVE(value1, value2, ref) CPPUNIT_ASSERT((value1 == ref && value2 == ref) || (value1 > ref && value2 > ref))

/** Regular assert with a wide string message. */
#define SCXUNIT_ASSERT_MESSAGEW(message, expression) CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::StrToUTF8(message), (expression))

/** Extend CPPUNIT_ASSERT_THROW_MESSAGE macro with SCXException support */
#ifdef CPPUNIT_ASSERT_THROW_MESSAGE
#undef CPPUNIT_ASSERT_THROW_MESSAGE
# define CPPUNIT_ASSERT_THROW_MESSAGE( message, expression, ExceptionType )   \
   do {                                                                       \
      bool cpputCorrectExceptionThrown_ = false;                              \
      CPPUNIT_NS::Message cpputMsg_( "expected exception not thrown" );       \
      cpputMsg_.addDetail( message );                                         \
      cpputMsg_.addDetail( "Expected: "                                       \
                           CPPUNIT_GET_PARAMETER_STRING( ExceptionType ) );   \
                                                                              \
      try {                                                                   \
         expression;                                                          \
      } catch ( const ExceptionType & ) {                                     \
         cpputCorrectExceptionThrown_ = true;                                 \
      } catch ( const SCXCoreLib::SCXException& scxe) {                       \
         cpputMsg_.addDetail( "Actual  : SCXCoreLib::SCXException or derived"); \
         cpputMsg_.addDetail( "What()  : " + SCXCoreLib::StrToUTF8(scxe.What())); \
         cpputMsg_.addDetail( "Where() : " + SCXCoreLib::StrToUTF8(scxe.Where())); \
      } catch ( const std::exception &e) {                                    \
         cpputMsg_.addDetail( "Actual  : " +                                  \
                              CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e,             \
                                          "std::exception or derived") );     \
         cpputMsg_.addDetail( std::string("What()  : ") + e.what() );         \
      } catch ( ... ) {                                                       \
         cpputMsg_.addDetail( "Actual  : unknown.");                          \
      }                                                                       \
                                                                              \
      if ( cpputCorrectExceptionThrown_ )                                     \
         break;                                                               \
                                                                              \
      CPPUNIT_NS::Asserter::fail( cpputMsg_,                                  \
                                  CPPUNIT_SOURCELINE() );                     \
   } while ( false )
#endif /* CPPUNIT_ASSERT_THROW_MESSAGE */

/** Extend CPPUNIT_ASSERT_NO_THROW_MESSAGE macro with SCXExeption support */
#ifdef CPPUNIT_ASSERT_NO_THROW_MESSAGE
#undef CPPUNIT_ASSERT_NO_THROW_MESSAGE
# define CPPUNIT_ASSERT_NO_THROW_MESSAGE( message, expression )               \
   do {                                                                       \
      CPPUNIT_NS::Message cpputMsg_( "unexpected exception caught" );         \
      cpputMsg_.addDetail( message );                                         \
                                                                              \
      try {                                                                   \
         expression;                                                          \
      } catch ( const SCXCoreLib::SCXException& scxe) {                       \
         cpputMsg_.addDetail( "Caught  : SCXCoreLib::SCXException or derived"); \
         cpputMsg_.addDetail( "What()  : " + SCXCoreLib::StrToUTF8(scxe.What())); \
         cpputMsg_.addDetail( "Where() : " + SCXCoreLib::StrToUTF8(scxe.Where())); \
         CPPUNIT_NS::Asserter::fail( cpputMsg_,                               \
                                     CPPUNIT_SOURCELINE() );                  \
      } catch ( const std::exception &e ) {                                   \
         cpputMsg_.addDetail( "Caught: " +                                    \
                              CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e,             \
                                          "std::exception or derived" ) );    \
         cpputMsg_.addDetail( std::string("What(): ") + e.what() );           \
         CPPUNIT_NS::Asserter::fail( cpputMsg_,                               \
                                     CPPUNIT_SOURCELINE() );                  \
      } catch ( ... ) {                                                       \
         cpputMsg_.addDetail( "Caught: unknown." );                           \
         CPPUNIT_NS::Asserter::fail( cpputMsg_,                               \
                                     CPPUNIT_SOURCELINE() );                  \
      }                                                                       \
   } while ( false )
#endif /* CPPUNIT_ASSERT_NO_THROW_MESSAGE */

/** Verify exception is thrown and description contains specified string */
# define SCXUNIT_ASSERT_THROWN_EXCEPTION( expression, ExceptionType, whatSubset )   \
    do {                                                                       \
        bool cpputCorrectExceptionThrown_ = false;                              \
        CPPUNIT_NS::Message cpputMsg_( "expected exception not thrown" );       \
        cpputMsg_.addDetail( "Expected: "                                       \
                             CPPUNIT_GET_PARAMETER_STRING( ExceptionType ) );   \
        cpputMsg_.addDetail( "  What() containing: "                         \
                             CPPUNIT_GET_PARAMETER_STRING( whatSubset ) );     \
                                                                              \
        try {                                                                   \
            expression;                                                          \
        } catch ( const ExceptionType & correct) {                              \
            if (correct.What().find(whatSubset) != std::wstring::npos)          \
                cpputCorrectExceptionThrown_ = true;                            \
            else {                                                              \
                cpputMsg_.addDetail( "Actual  : " CPPUNIT_GET_PARAMETER_STRING( ExceptionType ) " or derived" ); \
                cpputMsg_.addDetail( "What()  : " + SCXCoreLib::StrToUTF8(correct.What())); \
                cpputMsg_.addDetail( "Where() : " + SCXCoreLib::StrToUTF8(correct.Where())); \
            }                                                                  \
        } catch ( const SCXCoreLib::SCXException& scxe) {                       \
            cpputMsg_.addDetail( "Actual  : SCXCoreLib::SCXException or derived"); \
            cpputMsg_.addDetail( "What()  : " + SCXCoreLib::StrToUTF8(scxe.What())); \
            cpputMsg_.addDetail( "Where() : " + SCXCoreLib::StrToUTF8(scxe.Where())); \
        } catch ( const std::exception &e) {                                    \
            cpputMsg_.addDetail( "Actual  : " +                                  \
                                 CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e,             \
                                                                  "std::exception or derived") );     \
            cpputMsg_.addDetail( std::string("What()  : ") + e.what() );         \
        } catch ( ... ) {                                                       \
            cpputMsg_.addDetail( "Actual  : unknown.");                          \
        }                                                                       \
                                                                              \
        if ( cpputCorrectExceptionThrown_ )                                     \
            break;                                                               \
                                                                              \
        CPPUNIT_NS::Asserter::fail( cpputMsg_,                                  \
                                    CPPUNIT_SOURCELINE() );                     \
    } while ( false )

/** Log a string using the SCX logging framework. */
#define SCXUNIT_LOG(msg) do { \
    SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::Instance().GetLogHandle(L"scx.unittest"); \
    SCX_LOGINFO(log, msg); \
    } while (0)

/** Log the content of a string stream using the SCX logging framework. */
#define SCXUNIT_LOG_STREAM(str) SCXUNIT_LOG(str.str())

#endif /* SCXUNIT_H */

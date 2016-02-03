/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Unit test runner for SCX project.

    \date        2008-08-25 12:34:56

    This file implements a test caller which enforces a timeout on the tests run.
    This prevents the test runner from running indefinitly.

*/
/*----------------------------------------------------------------------*/
#ifndef SCXUNITTESTCALLER_H
#define SCXUNITTESTCALLER_H


#if defined(WIN32)
#include <windows.h>
#elif defined(SCX_UNIX)
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#else
#error "Not implemented for this plattform"
#endif
#include <scxcorelib/scxcmn.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>

#include <cppunit/ui/text/TestRunner.h>


extern "C" {
    typedef void* (*startThreadHndlrFn)(void *);
}

/*----------------------------------------------------------------------------*/
/**
    Implements a test runner for the SCX project.

    Adds possibility to run only subset of tests based on name or attribute.

*/
class SCXTestRunner : public CPPUNIT_NS::TextUi::TestRunner
{
private:
    std::vector<std::wstring> m_testNameFilters; //!< Vector of test name filters in effect
    std::string m_testAttributeFilter; //!< test attribute in effect.
    std::map<std::string, std::vector<std::string> > m_testAttributes; //!< Test attribute map.


    /*----------------------------------------------------------------------------*/
    /** Check if a test have a certain attribute.
        \param testName Name of test.
        \param attr Name of attribute.
        \returns true if the testName have given attribute, otherwise false.
    */
    bool HaveAttribute(const std::string& testName, const std::string& attr)
    {
        std::map<std::string, std::vector<std::string> >::iterator it = m_testAttributes.find(testName);
        if (it != m_testAttributes.end()) {
            for (std::vector<std::string>::iterator it2 = it->second.begin();
                 it2 != it->second.end(); it2++) {
                if (*it2 == attr) {
                    return true;
                }
            }
        }
        return false;
    }

public:
    /*----------------------------------------------------------------------------*/
    /** Default constructor
    */
    SCXTestRunner()
        : CPPUNIT_NS::TextUi::TestRunner()
        , m_testAttributeFilter("")
    { }

    /*----------------------------------------------------------------------------*/
    /** Set test name filter.
        \param s New test name filter.

        Multiple filters can be passed (treated as OR) by separating with commas.
        If you want the filter to ignore a certain name prepend "-" to the filter.
    */
    void SetTestNameFilter(const std::string& s)
    {
        std::vector<std::wstring> vec;
        SCXCoreLib::StrTokenize(SCXCoreLib::StrFromUTF8(s), vec, L",");

        for (std::vector<std::wstring>::iterator it = vec.begin(); it != vec.end(); it++)
        {
            m_testNameFilters.push_back(*it);
        }
    }

    /*----------------------------------------------------------------------------*/
    /** Set test attribute filter.
        \param attribute Name of attribute.

        If you want the filter to ignore a certain attribute prepend "!" to the attribute name.
    */
    void SetTestAttributeFilter(const std::string& attribute)
    {
        m_testAttributeFilter = attribute;
    }

    static SCXTestRunner s_Instance; //!< Static instance used by the SCXUNIT_TEST_ATTRIBUTE macro.

    /*----------------------------------------------------------------------------*/
    /** Check if a certain test should be ignored.
        \param testName name of test.
        \returns true If the given test should be ignored applying both test name
                      and attribute filters. Otherwise false.

        \note Both name and attribute filters are checked. If one of them determins
              the test should be ignored true is returned.
    */
    bool ShouldIgnore(const std::string& testName)
    {
        if (m_testAttributeFilter.length() > 0) {
            if (m_testAttributeFilter[0] == '-') {
                if (HaveAttribute(testName, m_testAttributeFilter.substr(1))) {
                    return true;
                }
            }
            else if (! HaveAttribute(testName, m_testAttributeFilter)) {
                return true;
            }
        }

        if (m_testNameFilters.size() > 0) {
            // All comparsisons are done case-insensitive ...
            const std::wstring wTestName = SCXCoreLib::StrToLower(SCXCoreLib::StrFromUTF8(testName));

            for (std::vector<std::wstring>::iterator it = m_testNameFilters.begin();
                 it != m_testNameFilters.end(); it++)
            {
                std::wstring spec = SCXCoreLib::StrToLower(*it);

                if (spec[0] == '-') {
                    if (wTestName.find(spec.substr(1)) == std::string::npos) {
                        return false;
                    }
                }
                else if (wTestName.find(spec) != std::string::npos) {
                    return false;
                }
            }

            // We didn't hit any test matches at all, so we ignore this test
            return true;
        }

        // No name filters existed, and attributes didn't match, so don't ignore
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /** Add an attribute to a test.
        \param testName name of test.
        \param a Attribute.
    */
    void AddTestAttribute(const std::string& testName, const char* a)
    {
        std::string attribute(a);
        std::map<std::string, std::vector<std::string> >::iterator it = m_testAttributes.find(testName);
        if (it == m_testAttributes.end()) {
            m_testAttributes[testName] = std::vector<std::string>();
            it = m_testAttributes.find(testName);
        }
        it->second.push_back(attribute);
    }

    /*----------------------------------------------------------------------------*/
    /** Overriden method to add tests to test runner.
        \param test Test to add.

        The parent implementation adds tests and test composites transparently.
        This method actually traverses the test composits making sure any tests
        that should be ignored are not added to the test run.
    */
    virtual void addTest(CPPUNIT_NS::Test* test)
    {
        int children = test->getChildTestCount();
        if (0 == children) {
            if ( ! ShouldIgnore(test->getName())) {
                CPPUNIT_NS::TextUi::TestRunner::addTest(test);
            }
        }
        else {
            for (int i=0; i<children; i++) {
                addTest(test->getChildTestAt(i));
            }
        }
    }
};

#if defined(IMPLEMENT_SCXTestRunner)
SCXTestRunner SCXTestRunner::s_Instance;
#endif

/*----------------------------------------------------------------------------*/
/**
    Implements a test caller for the SCX project.

    Will specify any unexpected exceptions and supports test timeouts.

*/
template <class Fixture>
class SCXTestCaller : public CPPUNIT_NS::TestCaller<Fixture>
{
    typedef void (Fixture::*TestMethod)(); //!< Test method type.

#if defined(WIN32)
    typedef HANDLE thread_handle_t; //!< Thread handle representation.
#elif defined(SCX_UNIX)
    struct THREAD_HANDLE_T {
        pthread_t thread_handle;
        pthread_attr_t thread_attr;
    };
    typedef THREAD_HANDLE_T* thread_handle_t; //!< Thread handle representation.
#endif

public:
/*----------------------------------------------------------------------------*/
/**
    Constructor

    \param       name Name of test.
    \param       test Test method.
    \param       timeout Test timeout in seconds (default: 300).
*/
    SCXTestCaller( std::string name, TestMethod test, unsigned int timeout = 300 )
        : CPPUNIT_NS::TestCaller<Fixture>(name, test)
        , m_timeout(timeout), m_pMessageFromThread(0), m_pSourceLineFromThread(0)
    { }

/*----------------------------------------------------------------------------*/
/**
    Constructor

    \param       name Name of test.
    \param       test Test method.
    \param       fixture Reference to test fixture.
    \param       timeout Test timeout in seconds (default: 300).
*/
    SCXTestCaller( std::string name, TestMethod test, Fixture& fixture, unsigned int timeout = 300 )
        :  CPPUNIT_NS::TestCaller<Fixture>(name, test, fixture)
        , m_timeout(timeout), m_pMessageFromThread(0), m_pSourceLineFromThread(0)
    { }

/*----------------------------------------------------------------------------*/
/**
    Constructor

    \param       name Name of test.
    \param       test Test method.
    \param       fixture Pointer to test fixture.
    \param       timeout Test timeout in seconds (default: 300).
*/
    SCXTestCaller( std::string name, TestMethod test, Fixture* fixture, unsigned int timeout = 300 )
        : CPPUNIT_NS::TestCaller<Fixture>(name, test, fixture)
        , m_timeout(timeout), m_pMessageFromThread(0), m_pSourceLineFromThread(0)
    { }

/*----------------------------------------------------------------------------*/
/**
    Thread body used for running tests threaded.

    \param       p Thread parameter. Should be of type SCXTestCaller.
    \returns     Zero.
*/
    static void* runTestThread(void* p)
    {
        SCXTestCaller<Fixture>* pTestCaller = (SCXTestCaller<Fixture>*)p;
        pTestCaller->DoRunTest();
        return 0;
    }

/*----------------------------------------------------------------------------*/
/**
    A helper method used to actually run the test.

    Will call the base class runTest.
*/
    void DoRunTest()
    {
        try {
            CPPUNIT_NS::TestCaller<Fixture>::runTest();
        }
        catch (const CPPUNIT_NS::Exception& cue)
        {
            m_pMessageFromThread = new CPPUNIT_NS::Message(cue.message());
            m_pSourceLineFromThread = new CPPUNIT_NS::SourceLine(cue.sourceLine());
        }
        catch (const SCXCoreLib::SCXException& scxe)
        {
            m_pMessageFromThread = new CPPUNIT_NS::Message( "unexpected exception caught" );

            m_pMessageFromThread->addDetail( "Caught  : SCXCoreLib::SCXException or derived");
            m_pMessageFromThread->addDetail( "What()  : " + SCXCoreLib::StrToUTF8(scxe.What()));
            m_pMessageFromThread->addDetail( "Where() : " + SCXCoreLib::StrToUTF8(scxe.Where()));
        }
        catch (const std::exception& e)
        {
            m_pMessageFromThread = new CPPUNIT_NS::Message( "unexpected exception caught" );

            m_pMessageFromThread->addDetail( "Caught  : std::exception or derived");
            m_pMessageFromThread->addDetail( "What()  : " + std::string(e.what()));
        }
        catch (...)
        {
            m_pMessageFromThread = new CPPUNIT_NS::Message( "unexpected exception caught" );
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Run the test.

    If timeout is non-zero a thread will be used to run the test. If the test
    runs for longer than the timeout, the test will fail with a timeout message.
*/
    void runTest()
    {
        thread_handle_t h = 0;
        try {
            if (0 == m_timeout) {
                CPPUNIT_NS::TestCaller<Fixture>::runTest();
            }
            else {
                int id;
                h = StartThread((startThreadHndlrFn) runTestThread, this, &id);
                WaitThread(h, m_timeout + 1); // Since time_t is used - allow one second margin.
                if ( ! DoneThread(h)) {
                    StopThread(h);
                    CPPUNIT_NS::Message cpputMsg_( "Test timeout" );
                    cpputMsg_.addDetail( " Timeout : " + SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(m_timeout)));
                    FreeThread(h);
                    CPPUNIT_NS::Asserter::fail( cpputMsg_ );
                }
#if defined(SCX_UNIX)
                else
                    pthread_join(h->thread_handle, NULL);
#endif
            }
        } catch ( const SCXCoreLib::SCXException& scxe) {
            CPPUNIT_NS::Message cpputMsg_( "unexpected exception caught" );

            cpputMsg_.addDetail( "Caught  : SCXCoreLib::SCXException or derived");
            cpputMsg_.addDetail( "What()  : " + SCXCoreLib::StrToUTF8(scxe.What()));
            cpputMsg_.addDetail( "Where() : " + SCXCoreLib::StrToUTF8(scxe.Where()));
            FreeThread(h);
            CPPUNIT_NS::Asserter::fail( cpputMsg_ );
        }
        FreeThread(h);
        if (0 != m_pMessageFromThread) {
            if (0 != m_pSourceLineFromThread) {
                CPPUNIT_NS::Asserter::fail( *m_pMessageFromThread, *m_pSourceLineFromThread );
            }
            else {
                CPPUNIT_NS::Asserter::fail( *m_pMessageFromThread );
            }
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Test setup handler.
*/
    void setUp()
    {
        Log("setUp");
        CPPUNIT_NS::TestCaller<Fixture>::setUp ();
    }

/*----------------------------------------------------------------------------*/
/**
    Test teardown handler.
*/
    void tearDown()
    {
        CPPUNIT_NS::TestCaller<Fixture>::tearDown ();

        if (0 != m_pSourceLineFromThread) {
            delete m_pSourceLineFromThread;
            m_pSourceLineFromThread = 0;
        }
        if (0 != m_pMessageFromThread) {
            delete m_pMessageFromThread;
            m_pMessageFromThread = 0;
        }
        Log("tearDown");
    }

/*----------------------------------------------------------------------------*/
/**
    String representation of object.
*/
    std::string toString() const
    {
        return "SCXTestCaller " + CPPUNIT_NS::TestCaller<Fixture>::getName();
    }

/*----------------------------------------------------------------------------*/
/**
    Logging helper.
    \param phase Phase that is logged.

    uses SCX logging to log which test is being in what phase.
*/
    void Log(std::string phase) const
    {
        SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::Instance().GetLogHandle(L"scx.unittestcaller");
        std::string msg = "[" + phase + "] " + toString();
        SCX_LOGINFO(log, SCXCoreLib::StrFromUTF8(msg));
    }

private:
    unsigned int m_timeout; //!< Test timeout in seconds.
    CPPUNIT_NS::Message* m_pMessageFromThread; //!< Pointer to message created in run thread.
    CPPUNIT_NS::SourceLine* m_pSourceLineFromThread; //!< Pointer to source line created in thread.

    SCXTestCaller( const SCXTestCaller &other ); //!< copy constructor intentionally private.
    SCXTestCaller &operator =( const SCXTestCaller &other ); //!< Assignement operator intentionally private.

/*----------------------------------------------------------------------------*/
/**
    Helper method to start a thread.

    \param       fn Thread body function.
    \param       param Thread parameters that will be passed to the thread body.
    \param       id Pointer to where thread id will be stored.
    \returns     A thread handle.
*/
#if defined(WIN32)
    static thread_handle_t StartThread(startThreadHndlrFn fn, void* param, int* id = 0)
#elif defined(SCX_UNIX)
    static thread_handle_t StartThread(startThreadHndlrFn fn, void* param, int* id = 0)
#endif
    {
#if defined(WIN32)
        return CreateThread(NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(fn), param, 0, reinterpret_cast<LPDWORD>(id));
#elif defined(SCX_UNIX)
        thread_handle_t h = new THREAD_HANDLE_T;
        pthread_attr_init(&(h->thread_attr));
        pthread_attr_setstacksize(&(h->thread_attr), 8*1024*1024);
        int r = pthread_create(&(h->thread_handle), &(h->thread_attr), fn, param);
        if (0 != id)
            *id = 0;
        if (0 != r)
            return 0;
        return h;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Wait for a thread.

    \param       h Thread handle.
    \param       timeout Number of seconds to wait or zero for no timeout (default: 0).
*/
    static void WaitThread(thread_handle_t h, unsigned int timeout = 0)
    {
        time_t start = time(NULL);
        time_t to = 0;
        if (timeout > 0)
            to = start + timeout;
        while ( ! DoneThread(h) && (0 == to || to > time(NULL))) {
            // strange, but true - next call to 'time' can return smaller number
            // on virtual machines, so timeout calclated as 'time()-start' would be '-1'
            SleepThread(50);
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Used to force a thread to stop.

    \param       h Thread handle to stop.
*/
    static void StopThread(thread_handle_t h)
    {
#if defined(WIN32)
        TerminateThread(h, 0);
#elif defined(SCX_UNIX)
        pthread_cancel(h->thread_handle);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Check if a thread is running or not.

    \param       h Thread handle to check.
    \returns     True if the thread is no longer running. Otherwise false.
*/
    static bool DoneThread(thread_handle_t h)
    {
#if defined(WIN32)
        DWORD ec = STILL_ACTIVE;
        GetExitCodeThread(h, &ec);
        return ec != STILL_ACTIVE;
#elif defined(SCX_UNIX)
        if (0 == pthread_kill(h->thread_handle, 0))
            return false;
        return true;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Suspend the current thread for atleast the given time.

    \param       milliSeconds Number of milliseconds to suspend thread.

    Using zero milliSeconds should yield the thread letting some other thread execute.
*/
    static void SleepThread(unsigned int milliSeconds)
    {
#if defined(WIN32)
        Sleep(milliSeconds);
#elif defined(SCX_UNIX)
        if (0 == milliSeconds)
            sched_yield();
        else
            usleep(milliSeconds * 1000);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Release resources associated with thread structure.

    \param       h A thread handle.
*/
    static void FreeThread(thread_handle_t h)
    {
        if (0 == h)
            return;
#if defined(WIN32)
        CloseHandle(h);
#elif defined(SCX_UNIX)
        delete h;
#endif
    }
};

/** Redefine macro to use SCXTestCaller for timeout support */
#ifdef CPPUNIT_TEST
#undef CPPUNIT_TEST
#define CPPUNIT_TEST( testMethod )                        \
    CPPUNIT_TEST_SUITE_ADD_TEST(                          \
        ( new SCXTestCaller<TestFixtureType>(             \
                  context.getTestNameFor( #testMethod),   \
                  &TestFixtureType::testMethod,           \
                  context.makeFixture()) ) )
#endif /* CPPUNIT_TEST */

/** Run test with timeout (in seconds). A timeout of zero means no timeout. */
#define SCXUNIT_TEST( testMethod, timeout )               \
    CPPUNIT_TEST_SUITE_ADD_TEST(                          \
        ( new SCXTestCaller<TestFixtureType>(             \
                  context.getTestNameFor( #testMethod),   \
                  &TestFixtureType::testMethod,           \
                  context.makeFixture(), (timeout)) ) )

/** Add attribute to test. */
#define SCXUNIT_TEST_ATTRIBUTE( testMethod, attr ) \
    SCXTestRunner::s_Instance.AddTestAttribute( context.getTestNameFor( #testMethod), #attr )

#endif /* SCXUNITTESTCALLER_H */

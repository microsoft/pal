/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

 */
#define IMPLEMENT_SCXUNIT_WARNING
#define IMPLEMENT_SCXTestRunner
#include <testutils/scxunit.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/Exception.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/Test.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/TextTestProgressListener.h>
#include <scxcorelib/scxcmn.h> 
#include <testutils/scxassert_cppunit.h>
#include <scxcorelib/stringaid.h>
#include <fstream>

#if !defined(WIN32)
#include <signal.h>
#include <sys/time.h>
#endif

//#include "../common_lib/util/log/scxlogbackendsimple.h"

using namespace std;

#ifdef WIN32
const char cPathDelim = '\\';
#else
const char cPathDelim = '/';
#endif

//! There was unexpected assertion failure in a test case
class SCXUnexpectedAssertionFailureException : public CPPUNIT_NS::Exception
{
public:
    SCXUnexpectedAssertionFailureException(const std::string &testName)
        : Exception(CPPUNIT_NS::Message("Unexpected assertion failure", testName)) { }

};

//! Monitors test case run times.
class TimerListener : public CPPUNIT_NS::BriefTestProgressListener
{
private:
#if defined(WIN32)
    typedef _SYSTEMTIME TimerListnerTimestamp;
#else
    typedef timeval TimerListnerTimestamp;
#endif

    void GetTimestamp(TimerListnerTimestamp* ts)
    {
#if defined(WIN32)
        GetSystemTime(ts);
#else
        gettimeofday(ts, NULL);
#endif
    }

    long GetTimeDiff(TimerListnerTimestamp* start, TimerListnerTimestamp* stop)
    {
#if defined(WIN32)
        // real time difference not really intesresting so using approximate:
        return (stop->wYear - start->wYear) * 365 * 24 * 60 * 60 * 1000
            + (stop->wMonth - start->wMonth) * 30 * 24 * 60 * 60 * 1000
            + (stop->wDay - start->wDay) * 24 * 60 * 60 * 1000
            + (stop->wHour - start->wHour) * 60 * 60 * 1000
            + (stop->wMinute - start->wMinute) * 60 * 1000
            + (stop->wSecond - start->wSecond) * 1000
            + (stop->wMilliseconds - start->wMilliseconds);
#else
        return (stop->tv_sec - start->tv_sec) * 1000
            + (stop->tv_usec - start->tv_usec) / 1000;
#endif
    }
public:
    TimerListener()
        : CPPUNIT_NS::BriefTestProgressListener()
        , m_slowLimit(-1)
    { }

    void SetSlowLimit(long v) { m_slowLimit = v; }

    std::string ReportSlow()
    {
        std::string s("");
        for (std::vector<std::string>::iterator it = m_report.begin(); it != m_report.end(); it++) {
            s += *it;
            s += "\n";
        }
        return s;
    }

    size_t GetSlowCount()
    {
        return m_report.size();
    }

    virtual void startTest( CPPUNIT_NS::Test*)
    {
        GetTimestamp(&m_startTime);
    }

    virtual void endTest( CPPUNIT_NS::Test *test)
    {
        TimerListnerTimestamp stopTime;
        GetTimestamp(&stopTime);
        long diff = GetTimeDiff(&m_startTime, &stopTime);
        if (m_slowLimit >= 0 && diff >= m_slowLimit) {
            char buf[1024*10];
            sprintf(buf, "%8.3fs %s", static_cast<float>(diff) / 1000.0F, test->getName().c_str());
            m_report.push_back(buf);
        }
    }

private:
    TimerListnerTimestamp m_startTime; //!< Tracks starting time for tests.
    long m_slowLimit; //!< The slow limit.
    std::vector<std::string> m_report; //!< Holds report for slow tests.
};
    


//! Monitors test cases for unexpected assertions.
class MyTestListener : public CPPUNIT_NS::BriefTestProgressListener
{
public:
    virtual void startTest( CPPUNIT_NS::Test *test)
    {
        SCXCoreLib::SCXAssertCounter::Reset();   
        BriefTestProgressListener::startTest(test);
    }

    virtual void endTest( CPPUNIT_NS::Test *test)
    {
        if (SCXCoreLib::SCXAssertCounter::GetFailedAsserts() > 0)
        {        
            m_eventManager->addFailure(test, new SCXUnexpectedAssertionFailureException(
                test->getName() + "; " + SCXCoreLib::StrToUTF8(SCXCoreLib::SCXAssertCounter::GetLastMessage()) ));
        }
        BriefTestProgressListener::endTest(test);
    }

    virtual void startTestRun(CPPUNIT_NS::Test *, CPPUNIT_NS::TestResult *eventManager) 
    {
        m_eventManager = eventManager;
    }

    virtual void endTestRun(CPPUNIT_NS::Test *, CPPUNIT_NS::TestResult *) 
    {
        m_eventManager = 0;
    }

private:
    CPPUNIT_NS::TestResult * m_eventManager; //!< Collects the result of running each test case
};

int main (int argc, char** argv)
{
#if defined(_DEBUG) && defined(WIN32)
    // enable memory leak reporting in run-time library
    int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpDbgFlag);
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
#endif


    string outDir;
    long slowLimit = -1;
    bool bNamesSpecified = false;
    bool bAttrsSpecified = false;

    for ( int i = 1; i < argc; i++ ){
        
        if (!strncmp(argv[i], "-logdir=", strlen("-logdir=")))
        {
            outDir = argv[i]+ strlen("-logdir=");
            if (outDir[outDir.length()] != cPathDelim)
            {
                outDir += cPathDelim;
            }
        }
        else if (!strncmp(argv[i], "-scxlogdir=", strlen("-scxlogdir=")))
        {
            string scxoutDir = argv[i]+ strlen("-scxlogdir=");
            if (scxoutDir[scxoutDir.length()] != cPathDelim)
            {
                scxoutDir += cPathDelim;
            }
/*            SCXCoreLib::SCXLogBackendSimple::s_configFileName = scxoutDir + "scxlog.conf";
              SCXCoreLib::SCXLogBackendSimple::s_logFileName = SCXCoreLib::StrFromUTF8(scxoutDir + "scx.log");*/
        }
        else if (0 == strncmp(argv[i], "-test=", strlen("-test=")))
        {
            string testName = argv[i]+ strlen("-test=");
            SCXTestRunner::s_Instance.SetTestNameFilter(testName);

            // Qualifier -test overrides environment variable
            bNamesSpecified = true;
        }
        else if (0 == strncmp(argv[i], "-attr=", strlen("-attr=")))
        {
            string attr = argv[i]+ strlen("-attr=");
            SCXTestRunner::s_Instance.SetTestAttributeFilter(attr);

            // Qualifier -attr overrides environment variable
            bAttrsSpecified = true;
        }
        else if (0 == strncmp(argv[i], "-slow=", strlen("-slow=")))
        {
            slowLimit = atol(argv[i]+ strlen("-slow="));
        }
        else
        {
            cout << "Illegal switch. Possible switches:" << endl;
            cout << "  -logdir=<dir>" << endl;
            cout << "  -scxoutDir=<dir>" << endl;
            cout << "  -test=<name>             Only run tests containg the string <name>." << endl;
            cout << "  -attr=<attribute>        Only run tests with given attribute." << endl;
            cout << "  -slow=<limit>            Report tests slower than limit in microseconds." << endl;
            cout << endl;
            cout << "Test/attribute names may be prepended with '-' to exclude them (-attr=-slow)" << endl;
            cout << endl;
            cout << "Environment variables that may be used:" << endl;
            cout << "  SCX_TESTRUN_ATTRS: Test attribte filter if -attr= qualifier is not specified" << endl;
            cout << "  SCX_TESTRUN_NAMES: Test name filter if -test= qualifier is not specified" << endl;
            cout << endl;
            return 1;
        }
    }
    
    if (! bNamesSpecified) {
        const char *cTestNames = getenv("SCX_TESTRUN_NAMES");

        if (NULL != cTestNames) {
            string sTestNames = cTestNames;
            cout << "Limiting tests to: " << sTestNames << endl;

            SCXTestRunner::s_Instance.SetTestNameFilter(sTestNames);
        }
    }

    if (! bAttrsSpecified) {
        const char *cTestAttrs = getenv("SCX_TESTRUN_ATTRS");

        if (NULL != cTestAttrs) {
            string sTestAttrs = cTestAttrs;
            cout << "Limiting attributes to: " << sTestAttrs << endl;

            SCXTestRunner::s_Instance.SetTestAttributeFilter(sTestAttrs);
        }
    }

#if !defined(WIN32)
    // We need to raise the SIGUSR1 to ALL testrunner processes in one test 
    // (see processpal_test.cpp) and the default behaviour, which is to 
    // terminate the recipient, was not satisfactory.
    signal(SIGUSR1, SIG_IGN);
#endif

    // Create the event manager and test controller
    CPPUNIT_NS::TestResult controller;

    // register listener for collecting the test-results
    CPPUNIT_NS::TestResultCollector collectedresults;
    controller.addListener(&collectedresults);

    // Add a listener that print what tests are run to the output interactively
    MyTestListener progress;
    controller.addListener( &progress );

    TimerListener timer;
    timer.SetSlowLimit(slowLimit);
    controller.addListener( &timer );

    // insert test-suite at test-runner by registry
    // NOTE: you should NOT delete this pointer.
    SCXTestRunner* testrunnerInstance = &SCXTestRunner::s_Instance;

    // Use the factory mechanism to register all tests
    testrunnerInstance->addTest (CPPUNIT_NS :: TestFactoryRegistry :: getRegistry ().makeTest ());
    // Run! 
    testrunnerInstance->run(controller); 

    std::cout << std::endl << "---- All tests run ----" << std::endl << std::endl;

    // output slow tests:
    if (timer.GetSlowCount() > 0) {
        std::cout << "SLOW TESTS (" << timer.GetSlowCount() << "):" << std::endl << timer.ReportSlow() << std::endl;
    }

    // output warnings to stdout
    for (wstring warning = SCXCoreLib::SCXUnitWarning::PopWarning(); 
         L"" != warning; 
         warning = SCXCoreLib::SCXUnitWarning::PopWarning())
    {
        wcout << L"WARNING: " << warning << std::endl;
    }

    // output results in compiler-format
//    CPPUNIT_NS :: CompilerOutputter compilerOutputter (&collectedresults, std::cerr);
//    compilerOutputter.write();

    // Format in plain text to stdout
    CPPUNIT_NS::TextOutputter stdoutTextOutputter( &collectedresults, std::cout ); 
    stdoutTextOutputter.write();

    // Format in plain text to file
    string logFileName;
    logFileName = outDir+"cppunit_result.log";
    ofstream myTextStream(logFileName.c_str());
    CPPUNIT_NS::TextOutputter textOutputter( &collectedresults, myTextStream ); 
    textOutputter.write();
    myTextStream.close();

    // Format as XML
    string xmlFileName;
    xmlFileName = outDir + "cppunit_result.xml";
    ofstream myStream(xmlFileName.c_str());
    CPPUNIT_NS::XmlOutputter xmlOutputter( &collectedresults, myStream );
    xmlOutputter.setStyleSheet("report.xsl");
    xmlOutputter.write();
    myStream.close();

    // return 0 if tests were successful
    return collectedresults.wasSuccessful () ? 0 : 1;
}

